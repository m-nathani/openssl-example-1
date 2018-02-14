#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <openssl/evp.h>
#include <openssl/err.h>

void initialize_fips(int mode) {
    if (FIPS_mode_set(mode)) {
        fprintf(stdout, "FUNCTION: %s, LOG: FIPS MODE SET TO %d\n", __func__, mode);
    }
    else {
        fprintf(stderr, "FUNCTION: %s, LOG: FIPS MODE NOT SET %d", __func__, mode);
        ERR_load_crypto_strings();
        fprintf(stderr, ", ERROR: ");
        ERR_print_errors_fp(stderr);
    }
}

void print_hex(FILE *out, const char *s) {
    while (*s)
        fprintf(out, "%02x", (unsigned char)*s++);
  fprintf(out, "\n");
}

int str2hex(const char hexstring[], unsigned char *val, int *len) {
    const char *pos = hexstring;
    size_t count = 0;

    for (count = 0; count < strlen(hexstring) / 2; count++) {
        sscanf(pos, "%2hhx", &val[count]);
        pos += 2;
    }
    *len = strlen(hexstring) / 2;

    return 1;
}

void main(int argc, char *argv[]) {
    initialize_fips(1);

    unsigned char *key_str = (unsigned char *)"AFB123456FADE12349870BFED78103FEDAB12345678FADEC";
    unsigned char *iv_str = (unsigned char *)"AFD123457F81235910";
    unsigned char *plaintext = (unsigned char *)"I m software dev";
    unsigned char *mode = (unsigned char *)"cbc";
    unsigned char key[1024];
    unsigned char iv[1024];
    int key_len = 0, iv_len = 0;
    unsigned char ciphertext[1024];
    unsigned char decryptedtext[1024];
    int decryptedtext_len = 0, ciphertext_len = 0;

    if(argc < 5) {
        printf("\nUsing default values, to give custom values, run: \n");
        printf("./des-endec Key(in hex) IV(in hex) plaintText mode(ofb/cbc/ecb/cfb1/cfb8/cfb64)\n");
    }

    if (argc >= 2) {
        key_str = argv[1];
    }
    if (argc >= 3) {
        iv_str = argv[2];
    }
    if (argc >= 4) {
        plaintext = argv[3];
    }
    if (argc >= 5) {
        mode = argv[4];
    }

    // Convert key from string to hex
    if (!str2hex(key_str, key, &key_len)) {
        printf("ERROR");
        return;
    }
    // Convert iv from string to hex
    if (!str2hex(iv_str, iv, &iv_len)) {
        printf("ERROR");
        return;
    }

    fprintf(stdout, "\nPlaintext: %s\n", plaintext);
    fprintf(stdout, "Key: ");
    print_hex(stdout, key);
    fprintf(stdout, "IV: ");
    print_hex(stdout, iv);
    fprintf(stdout, "Mode: %s\n", mode);

    if (strcasecmp(mode, "cbc") != 0 && strcasecmp(mode, "ecb") != 0 && strcasecmp(mode, "ofb") != 0 && strcasecmp(mode, "cfb1") != 0 && strcasecmp(mode, "cfb8") != 0 && strcasecmp(mode, "cfb64") != 0) {
        fprintf(stderr, "\nIncorrect mode!");
        fprintf(stderr, "\n3DES is only supported in CBC, ECB, OFB, CFB1, CFB8 and CFB64 modes\n");
        return;
    }

    fprintf(stdout, "\nEncryption:\n");
    printf("-----------\n");

    // Encrypt the plaintext
    ciphertext_len = encdec(plaintext, strlen(plaintext), key, key_len, iv, ciphertext, mode, 1);
    
    fprintf(stdout, "Ciphertext : ");
    print_hex(stdout, ciphertext);

    fprintf(stdout, "\nDecryption:\n");
    printf("-----------\n");

    // Decrypt the ciphertext
    decryptedtext_len = encdec(ciphertext, ciphertext_len, key, key_len, iv, decryptedtext, mode, 0);

    if (decryptedtext_len < 0) {
        // Verify error
        printf("Decrypted text failed to verify\n");
    }
    else {
        // Add a NULL terminator. We are expecting printable text
        decryptedtext[decryptedtext_len] = '\0';

        // Show the decrypted text
        printf("Decrypted text is: ");
        printf("%s\n", decryptedtext);
    }

    printf("\n");
    // Remove error strings
    ERR_free_strings();
}

int encdec(unsigned char *plaintext, int plaintext_len, unsigned char *key, int key_len,
            unsigned char *iv, unsigned char *ciphertext, unsigned char *mode, int enc) {

    EVP_CIPHER_CTX *ctx = NULL;
    int len = 0, ciphertext_len = 0;

    // Create and initialise the context
    ctx = malloc(sizeof(EVP_CIPHER_CTX));
    FIPS_cipher_ctx_init(ctx);

    const EVP_CIPHER *evpCipher;

    // Initialise the encryption operation
    if (strcasecmp(mode, "cbc") == 0) {
        evpCipher = FIPS_evp_des_ede3_cbc();
    }
    else if (strcasecmp(mode, "ofb") == 0) {
        evpCipher = FIPS_evp_des_ede3_ofb();
    }
    else if (strcasecmp(mode, "cfb1") == 0) {
        evpCipher = FIPS_evp_des_ede3_cfb1();
    }
    else if (strcasecmp(mode, "cfb8") == 0) {
        evpCipher = FIPS_evp_des_ede3_cfb8();
    }
    else if (strcasecmp(mode, "cfb64") == 0) {
        evpCipher = FIPS_evp_des_ede3_cfb64();
    }
    else if (strcasecmp(mode, "ecb") == 0) {
        evpCipher = FIPS_evp_des_ede3_ecb();
    }

    if (FIPS_cipherinit(ctx, evpCipher, NULL, NULL, enc) <= 0) {
        fprintf(stderr, "FIPS_cipherinit failed (1)\n");
        return 0;
    }

    // Initialise key and IV
    if (FIPS_cipherinit(ctx, NULL, key, iv, enc) <= 0) {
        fprintf(stderr, "FIPS_cipherinit failed (2)\n");
        return 0;
    }

    EVP_CIPHER_CTX_set_padding(ctx, 1);

    // Provide the message to be crypted, and obtain the crypted output
    if (plaintext) {
        if (1 != EVP_CipherUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
            fprintf(stderr, "EVP_CipherUpdate failed \n");
        }

        ciphertext_len = len;
    }

    // Finalise the cryption. Normally ciphertext bytes may be written at this stage
    if (1 != EVP_CipherFinal_ex(ctx, ciphertext + len, &len)) {
        fprintf(stderr, "EVP_CipherFinal_ex failed \n");
        ERR_load_crypto_strings();
        fprintf(stderr, ", ERROR: ");
        ERR_print_errors_fp(stderr);
    }
    ciphertext_len += len;

    // Clean up
    FIPS_cipher_ctx_free(ctx);

    return ciphertext_len;
}
