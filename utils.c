#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <openssl/md5.h>

#include "utils.h"
#include "config.h"


/* Check if package exists */
int pkg_exists(char *fullPath) {
    
    
    if (access(fullPath, F_OK) != -1) {
        // file exists
        return 1;
    }
    else {
        // file doesn't exist
        return 0;
    }
}

/* Get full path of package */
char *pkg_path(char *pkgname) {
    char *path = get_config_opt("pkgdb_dir");
    char *fullPath = (char *)malloc(strlen(path)  + strlen(pkgname) + 8);
    strcpy(fullPath, path);
    strcat(fullPath, pkgname);
    strcat(fullPath, ".tar.xz");

    return fullPath;
}

/* Strip prefix from socket command */
char *remove_prefix(char *command) {
    return &command[4];
}