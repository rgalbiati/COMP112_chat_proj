// encrypt and decrypt functions from the OpenSSL Wiki & StackOverflow:
// * https://wiki.openssl.org/index.php/EVP_Authenticated_Encryption_and_Decryption  
// * https://stackoverflow.com/questions/9889492/how-to-do-encryption-using-aes-in-openssl

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <stdbool.h>
#include "clientList.h"
#include "encrypt.h"

static const int CLIENT_LIST = 8;      
static const int CHAT_LIST = 9;  
static const int CREATE_CHAT = 11; 
// static unsigned char key[] = "01234567890123456789012345678901";
// static unsigned char iv[] = "0123456789012345";
// static unsigned char aad[] = "Some AAD data";

struct __attribute__((__packed__)) header {
    unsigned short type;
    char src[20];
    char dst[20];
    unsigned int len;
    unsigned int msg_id;
};


bool encrypted_read(AES_KEY dec_key, int fd, struct packet *p){
	char header_buf[sizeof(struct header)];
    memset(header_buf, 0, sizeof(struct header));
    int numBytes = read(fd, header_buf, sizeof(struct header));
    if (numBytes < 0) {
        printf("Error reading from client\n");
        return false;
    }

    else {
        /* Data read. */
        struct header *h = (struct header *)header_buf;
        p->type = ntohs(h->type);
        memcpy(p->src, h->src, strlen(h->src) + 1);
        memcpy(p->dst, h->dst, strlen(h->dst) + 1);
        p->len = ntohl(h->len);
        p->msg_id = ntohl(h->msg_id);

        if (p->len == 0){
            char *data = "";
            memcpy(p->data, data, strlen(data) + 1);
        } 

        // not encrypted
        else if (p->type == CLIENT_LIST || p->type == CHAT_LIST || p->type == CREATE_CHAT){
            int n = 0;
            char data[400];
            n = recv(fd, data, 400, 0);

            if (n < 0) {
                printf("Error reading packet data from client %s\n", p->src);
                return false;
            }
            memcpy(p->data, data, n);
        } 

        // encrypted
        else {
            int n = 0;
            char encrypted_data[p->len];
            n = recv(fd, encrypted_data, p->len, 0);

            // printf("in read len is %d and data is %s\n", p->len, p->data);

            if (n < 0) {
                printf("Error reading packet data from client %s\n", p->src);
                return false;
            } 

            // unsigned char dec_out[400];

            unsigned char *dec_out = malloc(600);
            memset(dec_out, 0, 600);
            
            // AES dec_key;
            // AES_set_decrypt_key(key, 128, &dec_key);
            AES_decrypt(encrypted_data, dec_out, &dec_key);
            
            // printf("len %d\n", p->len);
            int dec_len = 0;
            for(int i=0;*(dec_out+i)!=0x00;i++) {
                dec_len += 1;
            }

            printf("Decrypted %d bytes of data: %s\n", dec_len, dec_out);

            p->len = dec_len;

            memset(p->data, 0, 400);
            memcpy(p->data, dec_out, p->len);
            free(dec_out);

            
        }
        return true;
    }
}

void encrypted_write(AES_KEY enc_key, int fd, int type, char *src, char *dst, int len, int msg_id, 
                 char *data)
{
    struct header p;
    p.type = htons(type);
    memset(p.src, 0, 20);
    memset(p.dst, 0, 20);
    memcpy(p.src, src, strlen(src) + 1);
    memcpy(p.dst, dst, strlen(dst) + 1);
    p.len = htonl(len);
    p.msg_id = htonl(msg_id);

    // data needs to be encrypted
    if (len > 0){
        unsigned char *enc_out = malloc(600);
        memset(enc_out, 0, 600);
    	
        AES_encrypt(data, enc_out, &enc_key);      

        int enc_len = 0;
        for(int i=0; *(enc_out+i)!=0x00; i++){
        	enc_len += 1;
        }
    
        p.len = htonl(enc_len);

        write(fd, (char *) &p, sizeof(p));

        // Write data
        if (len > 0) {
            write(fd, enc_out, enc_len);
            free(enc_out);
        }
    } 

    else {
        write(fd, (char *) &p, sizeof(p));
    }
}

