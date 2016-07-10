#ifndef CHECKSUM_H
#define CHECKSUM_H

int verify_checksum(char *file, char *expected);
char *get_hash(const char* pkgname);

#endif