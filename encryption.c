/**
  Some Code from AES encryption/decryption demo program using OpenSSL EVP apis
  https://github.com/saju/misc/blob/master/misc/openssl_aes.c
**/

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

struct __attribute__((__packed__)) header {
    unsigned short type;
    char src[20];
    char dst[20];
    unsigned int len;
    unsigned int msg_id;
};

/*
 * Encrypt *len bytes of data
 * All data going in & out is considered binary (unsigned char[])
 */
unsigned char *aes_encrypt(EVP_CIPHER_CTX *e, unsigned char *plaintext, int *len)
{
  /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
  int c_len = *len + AES_BLOCK_SIZE, f_len = 0;
  unsigned char *ciphertext = malloc(c_len);
  memset(ciphertext, 0, c_len);

  /* allows reusing of 'e' for multiple encryption cycles */
  EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL);

  /* update ciphertext, c_len is filled with the length of ciphertext generated,
    *len is the size of plaintext in bytes */
  EVP_EncryptUpdate(e, ciphertext, &c_len, plaintext, *len);

  /* update ciphertext with the final remaining bytes */
  EVP_EncryptFinal_ex(e, ciphertext+c_len, &f_len);

  *len = c_len + f_len;
  return ciphertext;
}

/*
 * Decrypt *len bytes of ciphertext
 */
unsigned char *aes_decrypt(EVP_CIPHER_CTX *e, unsigned char *ciphertext, int *len)
{
  /* plaintext will always be equal to or lesser than length of ciphertext*/
  int p_len = *len, f_len = 0;
  unsigned char *plaintext = malloc(p_len);
  memset(plaintext, 0, p_len);
  
  EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL);
  EVP_DecryptUpdate(e, plaintext, &p_len, ciphertext, *len);
  EVP_DecryptFinal_ex(e, plaintext+p_len, &f_len);

  *len = p_len + f_len;
  return plaintext;
}


bool encrypted_read(EVP_CIPHER_CTX *d_ctx, int fd, struct packet *p){
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

            if (n < 0) {
                printf("Error reading packet data from client %s\n", p->src);
                return false;
            } 


            char *data = aes_decrypt(d_ctx, encrypted_data, &p->len);

            memset(p->data, 0, 400);
            memcpy(p->data, data, p->len);;


            
        }
        return true;
    }
}

void encrypted_write(EVP_CIPHER_CTX *e_ctx, int fd, int type, char *src, char *dst, int len, int msg_id, 
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
        int new_len = len;
        char *encrypted_data = aes_encrypt(e_ctx, data, &new_len);

    
        p.len = htonl(new_len);

        write(fd, (char *) &p, sizeof(p));

        // // Write data
        if (len > 0) {
            write(fd, encrypted_data, new_len);
            free(encrypted_data);
            // free(enc_out);
        }
    } 

    else {
        write(fd, (char *) &p, sizeof(p));
    }
}




/**
 * Create a 256 bit key and IV using the supplied key_data. salt can be added for taste.
 * Fills in the encryption and decryption ctx objects and returns 0 on success
 **/
int aes_init(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx, 
             EVP_CIPHER_CTX *d_ctx)
{
  int i, nrounds = 5;
  unsigned char key[32], iv[32];
  
  /*
   * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
   * nrounds is the number of times the we hash the material. More rounds are more secure but
   * slower.
   */
  i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);
  if (i != 32) {
    printf("Key size is %d bits - should be 256 bits\n", i);
    return -1;
  }

  EVP_CIPHER_CTX_init(e_ctx);
  EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
  EVP_CIPHER_CTX_init(d_ctx);
  EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);

  return 0;
}

