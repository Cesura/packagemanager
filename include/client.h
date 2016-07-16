#ifndef CLIENT_H
#define CLIENT_H

void download_pkg(const char* address, const char* pkgname);
char *get_remote_address(char *pkgname);
char *get_remote_checksum(char *pkgname);
int is_installed(char *pkgname, int version_required);

#endif