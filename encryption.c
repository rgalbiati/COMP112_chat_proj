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


static unsigned char key[] = "01234567890123456789012345678901";
static unsigned char iv[] = "0123456789012345";
static unsigned char aad[] = "Some AAD data";

struct __attribute__((__packed__)) header {
    unsigned short type;
    char src[20];
    char dst[20];
    unsigned int len;
    unsigned int msg_id;
};


bool encrypted_read(int fd, struct packet *p){
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
        } else {
            int n = 0;
            char *encrypted_data = malloc(p->len);
            n = recv(fd, encrypted_data, p->len, 0);

            if (n < 0) {
                printf("Error reading packet data from client %s\n", p->src);
                return false;
            } 

            // decrypt data
            char *data = malloc(400);
            char *tag = malloc(16);
            memset(tag, 0, 16);
            memset(data, 0, 400);
            int bytes = decrypt(encrypted_data, n, aad, strlen(aad), tag, key, iv, strlen(iv), data);

            if (bytes < 0) {
                printf("Error: Failure to authenticate\n");
                // return false;
            }
            printf("bytes: %d\n", bytes);
            printf("decrypted: %s\n", data);

            memcpy(p->data, data, bytes);
            p->len = bytes;
            free(encrypted_data);
            free(tag);
            free(data);
        }
        return true;
    }
}

void encrypted_write(int fd, int type, char *src, char *dst, int len, int msg_id, 
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
        unsigned char *encrypted_data = malloc(500);
        unsigned char *tag = malloc(16);
        memset(encrypted_data, 0, 500);
        memset(tag, 0, 16);

        printf("%s\n", data);
        printf("%s\n", aad);
        printf("%s\n", iv);
        // printf("%s\n", data);
        // printf("%s\n", data);


        int n = encrypt(data, strlen(data), aad, strlen(aad), key, iv, strlen(iv), encrypted_data, tag);
        p.len = n;
        write(fd, (char *) &p, sizeof(p));

        // Write data
        if (len > 0) {
            write(fd, encrypted_data, n);
        }
    } 

    else {
        write(fd, (char *) &p, sizeof(p));
    }
}

void handleErrors(void)
{
    unsigned long errCode;

    printf("An error occurred\n");
    while(errCode = ERR_get_error())
    {
        char *err = ERR_error_string(errCode, NULL);
        printf("%s\n", err);
    }
    abort();
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *aad,
	int aad_len, unsigned char *key, unsigned char *iv, int iv_len,
	unsigned char *ciphertext, unsigned char *tag)
{
	EVP_CIPHER_CTX *ctx;

	int len;

	int ciphertext_len;


	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

	/* Initialise the encryption operation. */
	if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
		handleErrors();

	/* Set IV length if default 12 bytes (96 bits) is not appropriate */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
		handleErrors();

	/* Initialise key and IV */
	if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv)) handleErrors();

	/* Provide any AAD data. This can be called zero or more times as
	 * required
	 */
	if(1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len))
		handleErrors();

	/* Provide the message to be encrypted, and obtain the encrypted output.
	 * EVP_EncryptUpdate can be called multiple times if necessary
	 */
	if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
		handleErrors();
	ciphertext_len = len;

	/* Finalise the encryption. Normally ciphertext bytes may be written at
	 * this stage, but this does not occur in GCM mode
	 */
	if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) handleErrors();
	ciphertext_len += len;

	/* Get the tag */
	if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
		handleErrors();

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	return ciphertext_len;
}

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *aad,
	int aad_len, unsigned char *tag, unsigned char *key, unsigned char *iv,
	int iv_len, unsigned char *plaintext)
{
	EVP_CIPHER_CTX *ctx;
	int len;
	int plaintext_len;
	int ret;

	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new())) handleErrors();

	/* Initialise the decryption operation. */
	if(!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
		handleErrors();

	/* Set IV length. Not necessary if this is 12 bytes (96 bits) */
	if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
		handleErrors();

	/* Initialise key and IV */
	if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv)) handleErrors();

	/* Provide any AAD data. This can be called zero or more times as
	 * required
	 */
	if(!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len))
		handleErrors();

	/* Provide the message to be decrypted, and obtain the plaintext output.
	 * EVP_DecryptUpdate can be called multiple times if necessary
	 */
	if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
		handleErrors();
	plaintext_len = len;

	/* Set expected tag value. Works in OpenSSL 1.0.1d and later */
	if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
		handleErrors();

	/* Finalise the decryption. A positive return value indicates success,
	 * anything else is a failure - the plaintext is not trustworthy.
	 */
	ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	if(ret > 0)
	{
		/* Success */
		plaintext_len += len;
		return plaintext_len;
	}
	else
	{
		/* Verify failed */
		return -1;
	}
}