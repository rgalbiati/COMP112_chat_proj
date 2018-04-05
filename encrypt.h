#ifndef ENCRYPT_H
#define ENCRYPT_H

#include <openssl/aes.h>
#include <openssl/evp.h>

// struct __attribute__((__packed__)) packet {
//     unsigned short type;
//     char src[20];
//     char dst[20];
//     unsigned int len;
//     unsigned int msg_id;
//     char data[400];
// };

// static struct __attribute__((__packed__)) header {
//     unsigned short type;
//     char src[20];
//     char dst[20];
//     unsigned int len;
//     unsigned int msg_id;
// };

int aes_init(unsigned char *key_data, int key_data_len, unsigned char *salt, EVP_CIPHER_CTX *e_ctx, 
             EVP_CIPHER_CTX *d_ctx);
bool encrypted_read(EVP_CIPHER_CTX *d_ctx, int fd, struct packet *p);
void encrypted_write(EVP_CIPHER_CTX *e_ctx, int fd, int type, char *src, char *dst, int len, int msg_id, 
                 char *data);
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *aad,
	int aad_len, unsigned char *key, unsigned char *iv, int iv_len,
	unsigned char *ciphertext, unsigned char *tag);
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *aad,
	int aad_len, unsigned char *tag, unsigned char *key, unsigned char *iv,
	int iv_len, unsigned char *plaintext);

#endif