#ifndef UTILS_H
#define UTILS_H

char *pkg_path(char *pkgname);
int pkg_exists(char *fullPath);
char *remove_prefix(char *command);

#endif