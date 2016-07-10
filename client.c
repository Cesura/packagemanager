#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <math.h>

#include "client.h"
#include "globals.h"
#include "config.h"
#include "checksum.h"
#include "utils.h"

int pkg_count;
int pkg_num;

void download_pkg(const char* address, const char* filename) {
 
    // Create a socket first
    int sockfd = 0;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error: could not create socket \n");
        exit(1);
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5004); // port
    serv_addr.sin_addr.s_addr = inet_addr(address);

    // Attempt a connection to peer
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error: connection to '%s' failed \n", address);
        exit(1);
    }

    char pkgname[100];
    char response[10];

    strcpy(pkgname, "PKG:");
    strcat(pkgname, filename);
    send(sockfd, &pkgname, strlen(filename)+5, 0);
    recv(sockfd, &response, 10, 0);

    if (strcmp(response, "EXISTS") == 0) {
        char *pkgdb_dir = get_config_opt("pkgdb_dir");
        char *outputFile = malloc(sizeof(char) * (strlen(pkgdb_dir) + 8));

        strcpy(outputFile, pkgdb_dir);
        strcat(outputFile, filename);
        strcat(outputFile, ".tar.xz");

        // Create file where data will be stored
        FILE *fp = fopen(outputFile, "wb");
        if (NULL == fp) {
            printf("Error opening output file\n");
            exit(1);
        }

        // Destination file size
        int fileSize;
        recv(sockfd, &fileSize, 4, 0);
        fileSize = ntohl(fileSize);
     
        // Receive data in chunks of BUF_SIZE bytes
        int bytesReceived = 0;
        int totalReceived = 0;
        int sinceUpdate = 0;
        int toPrint = 0;
        float percentage = 0;

        struct winsize w;
        ioctl(0, TIOCGWINSZ, &w);
        int exclude = strlen(filename) + 16;
        int available = (w.ws_col - exclude);
        float interval = (float)fileSize / available;
        

        char buff[BUF_SIZE];
        memset(buff, '0', sizeof(buff));
        while ((bytesReceived = read(sockfd, buff, BUF_SIZE)) > 0) {
            totalReceived += bytesReceived;
            sinceUpdate += bytesReceived;

            fwrite(buff, 1, bytesReceived, fp);

            if (sinceUpdate >= interval) {

                // update terminal size
                ioctl(0, TIOCGWINSZ, &w);
                available = (w.ws_col - exclude);
                interval = (float)fileSize / available;
                percentage = (float)totalReceived / fileSize;
                toPrint = percentage * available;

                // print progress bar
                printf("\33[2K\r");
                printf("(%i/%i) %s [", pkg_num, pkg_count, filename);

                for (int i = 0; i < toPrint; i++) {
                    printf("=");
                }

                for (int i = 0; i < (available - toPrint); i++) {
                    printf(" ");
                }

                printf("] (%g%%)", round(percentage * 100));

                fflush(stdout);
                sinceUpdate = 0;
            }
        }

        printf("\33[2K\r");
        printf("(%i/%i) %s [", pkg_num, pkg_count, filename);

        for (int i = 0; i < available; i++) {
            printf("=");
        }
        printf("] (100%%)\n");

        if (bytesReceived < 0) {
            printf("Read Error \n");
        }

        fclose(fp);
    }
    else {
        printf("Error: file does not exist on peer\n");
        exit(1);
    }
}

char *get_remote_address(char *pkgname) {

        int trackerfd = 0;
        struct sockaddr_in tracker_addr;

        // Create a socket first
        if ((trackerfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("Error: could not connect to tracker\n");
            exit(1);
        }

        // Tracker socket data structure
        tracker_addr.sin_family = AF_INET;
        tracker_addr.sin_port = htons(5003);
        tracker_addr.sin_addr.s_addr = inet_addr(TRACKER_URL);

        // Attempt a connection to tracker
        if (connect(trackerfd, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0) {
            printf("Error: could not connect to tracker\n");
            exit(1);
        }

        char request[150];
        char *remote_address = malloc(sizeof(char) * 20);

        // send package request to tracker
        strcpy(request, "TRK:");
        strcat(request, pkgname);
        send(trackerfd, &request, strlen(pkgname)+5, 0);

        // receive a response to see if it has it
        recv(trackerfd, remote_address, 20, 0);
        close(trackerfd);


        if (strcmp(remote_address, "NOTPKG") == 0) {
            printf("Error: package '%s' not found\n", pkgname);
            exit(1);
        }

        return remote_address;
}

char *get_remote_checksum(char *pkgname) {

        int trackerfd = 0;
        struct sockaddr_in tracker_addr;

        // Create a socket first
        if ((trackerfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            printf("Error: could not connect to tracker\n");
            exit(1);
        }

        // Tracker socket data structure
        tracker_addr.sin_family = AF_INET;
        tracker_addr.sin_port = htons(5003);
        tracker_addr.sin_addr.s_addr = inet_addr(TRACKER_URL);

        // Attempt a connection to tracker
        if (connect(trackerfd, (struct sockaddr *)&tracker_addr, sizeof(tracker_addr)) < 0) {
            printf("Error: could not connect to tracker\n");
            exit(1);
        }

        char request[150];
        char *checksum = malloc(sizeof(char) * 33);

        // send package request to tracker
        strcpy(request, "MD5:");
        strcat(request, pkgname);
        send(trackerfd, &request, strlen(pkgname)+5, 0);

        // receive a response to see if it has it
        recv(trackerfd, checksum, 33, 0);
        close(trackerfd);

        return checksum;
}

int main(int argc, char** argv)
{
    pkg_count, pkg_num = 0;

    char *packages[256][2];
    char *md5sums[256][2];
    int count = 0;

    // check argument passed
    int action = 0;
    if (strcmp(argv[1], "install") == 0) {
        action = 1;
    }
    else if (strcmp(argv[1], "remove") == 0) {
        action = 2;
    }
    else {
        printf("Error: invalid command\n");
        exit(1);
    }

    switch(action) {

        // they want to install the packages
        case 1:

            // get peer addresses for each package
            for (int i = 2; i < argc; i++) {
                packages[count][0] = argv[i];
                packages[count][1] = get_remote_address(argv[i]);
                count++;
                pkg_count++;
            }

            // download each package
            printf(" :: Downloading packages...\n");
            count = 0;
            for (int i = 2; i < argc; i++) {
                pkg_num++;

                if (!pkg_exists(pkg_path(packages[count][0]))) {
                    download_pkg(packages[count][1], packages[count][0]);
                }
                
                count++;
            }



            // verify checksums
            printf(" :: Verifying checksums...\n");
            count = 0;
            for (int i = 2; i < argc; i++) {
                md5sums[count][0] = argv[i];
                md5sums[count][1] = get_remote_checksum(argv[i]);

                if (!verify_checksum(pkg_path(md5sums[count][0]), md5sums[count][1])) {
                    printf("Error: failed to verify checksum for '%s'\n", md5sums[count][0]);
                    exit(1);
                }

                // TEMPORARY!!
                unlink(pkg_path(md5sums[count][0]));

                count++;
            }

            printf("Checksum verification complete\n");


            break;

        // they want to remove it
        case 2:
            count = 0;
            for (int i = 2; i < argc; i++) {
                printf("Removing: %s\n", argv[i]);
                count++;
            }
            break;
    }
    
}