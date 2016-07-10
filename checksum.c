#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <openssl/md5.h>

#include "checksum.h"

int verify_checksum(char *file, char *expected) {
	unsigned char c[MD5_DIGEST_LENGTH];
	int i;
   	MD5_CTX mdContext;
	int bytes;
	unsigned char data[1024];

    FILE *fp = fopen(file, "rb");
	if (fp == NULL) {
        printf ("%s can't be opened.\n", file);
        return 0;
	}
	
	MD5_Init(&mdContext);
	while ((bytes = fread(data, 1, 1024, fp)) != 0)
        	MD5_Update(&mdContext, data, bytes);
	MD5_Final(c, &mdContext);

	char *hash = malloc(sizeof(char) * 33);
	int length = 0;
	for (i = 0; i < MD5_DIGEST_LENGTH; i++)
		length += sprintf(hash + length, "%02x", c[i]);		

	fclose(fp);
	
	if (strcmp(hash, expected) == 0) {
		free(hash);
		return 1;
	}

	free(hash);
	return 0;	
}