#ifndef ENCRYPT_H
#define ENCRYPT_H

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

bool encrypted_read(int fd, struct packet *p);
void encrypted_write(int fd, int type, char *src, char *dst, int len, int msg_id, 
                 char *data);
int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *aad,
	int aad_len, unsigned char *key, unsigned char *iv, int iv_len,
	unsigned char *ciphertext, unsigned char *tag);
int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *aad,
	int aad_len, unsigned char *tag, unsigned char *key, unsigned char *iv,
	int iv_len, unsigned char *plaintext);

#endif