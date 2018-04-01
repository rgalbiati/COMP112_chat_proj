//  To compile: 	gcc -g rsa_test.c -o rsa_test -lcrypto

#include <stdlib.h>
#include <stdio.h> 
#include <openssl/rsa.h>

int main(int argc, char *argv[]){
	if (argc != 2){
		printf("Use: ./rsa_test < message >\n");
		exit(1);
	}

	char *message = argv[1];

	BIGNUM *bn = NULL;
	RSA *r_pub = NULL;
	int bits = 2048;

	r_pub = RSA_new();
	// BIO *keybio = 
	RSA_generate_key_ex(r_pub, bits, NULL, NULL);

	printf("Message: %s\n", message);
 

}



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