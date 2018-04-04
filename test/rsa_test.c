//  To compile: 	gcc -g rsa_test.c -o rsa_test -lcrypto

#include <stdlib.h>
#include <stdio.h> 
#include <openssl/aes.h>

static const unsigned char key[] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};


int main()
{
    unsigned char text[]="hello world!";
    unsigned char enc_out[16];
    unsigned char dec_out[16];

    AES_KEY enc_key, dec_key;

    AES_set_encrypt_key(key, 128, &enc_key);
    AES_encrypt(text, enc_out, &enc_key);      

    AES_set_decrypt_key(key,128,&dec_key);
    AES_decrypt(enc_out, dec_out, &dec_key);

    int i;

    printf("original:\t");
    for(i=0;*(text+i)!=0x00;i++)
        printf("%X ",*(text+i));
    printf("\nencrypted:\t");
    for(i=0;*(enc_out+i)!=0x00;i++)
        printf("%X ",*(enc_out+i));
    printf("\ndecrypted:\t");
    for(i=0;*(dec_out+i)!=0x00;i++)
        printf("%X ",*(dec_out+i));

    printf("\n");

    printf("End message:\n");
    printf("%s\n", dec_out);


    return 0;
} 


// int main(int argc, char *argv[]){
// 	if (argc != 2){
// 		printf("Use: ./rsa_test < message >\n");
// 		exit(1);
// 	}

// 	char *message = argv[1];

// 	BIGNUM *bn = NULL;
// 	RSA *r_pub = NULL;
// 	int bits = 2048;

// 	r_pub = RSA_new();
// 	// BIO *keybio = 
// 	RSA_generate_key_ex(r_pub, bits, NULL, NULL);

// 	printf("Message: %s\n", message);
 

// }


// bool generate_key()
// {
//     int             ret = 0;
//     RSA             *r = NULL;
//     BIGNUM          *bne = NULL;
//     BIO             *bp_public = NULL, *bp_private = NULL;
//  
//     int             bits = 2048;
//     unsigned long   e = RSA_F4;
//  
//     // 1. generate rsa key
//     bne = BN_new();
//     ret = BN_set_word(bne,e);
//     if(ret != 1){
//         goto free_all;
//     }
//  
//     r = RSA_new();
//     ret = RSA_generate_key_ex(r, bits, bne, NULL);
//     if(ret != 1){
//         goto free_all;
//     }
//  
//     // 2. save public key
//     bp_public = BIO_new_file("public.pem", "w+");
//     ret = PEM_write_bio_RSAPublicKey(bp_public, r);
//     if(ret != 1){
//         goto free_all;
//     }
//  
//     // 3. save private key
//     bp_private = BIO_new_file("private.pem", "w+");
//     ret = PEM_write_bio_RSAPrivateKey(bp_private, r, NULL, NULL, 0, NULL, NULL);
//  
//     // 4. free
// free_all:
//  
//     BIO_free_all(bp_public);
//     BIO_free_all(bp_private);
//     RSA_free(r);
//     BN_free(bne);
//  
//     return (ret == 1);
// }
//  
// int main(int argc, char* argv[]) 
// {
//     generate_key();
//         return 0;
// }