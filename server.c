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

#include "server.h"
#include "globals.h"
#include "config.h"
#include "utils.h"

/* Main server loop */
int server() {
    signal(SIGPIPE, SIG_IGN);

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // Keep socket open
    int option = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("192.168.0.15");
    serv_addr.sin_port = htons(5004);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    if (listen(listenfd, 10) == -1) {
        printf("Failed to listen\n");
        return -1;
    }

    for (;;) {
        int connfd = accept(listenfd, (struct sockaddr*)NULL ,NULL);

	    if (fork() == 0) {

    	    // receive requests
        	char requestBuff[100];
    		recv(connfd, &requestBuff, 100, 0);

            // Our response to the client
            char responseBuffer[10];

            // create the full path
            char *fullPath = pkg_path(remove_prefix(requestBuff));

    		// The request is for a package dl
    		if (strstr(requestBuff, "PKG:")) {

                // check if packages exists on server
    			if (pkg_exists(fullPath) == 1) {

                    // tell the client that it exists
                    strcpy(responseBuffer, "EXISTS");
                    send(connfd, &responseBuffer, 10, 0);

                    // Open the file that we wish to transfer
                    FILE *fp = fopen(fullPath, "rb");
                    if (fp == NULL) {
                        printf("File open error");
                        return 1;
                    }

                    // calculate and send file size
                    fseek(fp, 0L, SEEK_END);
                    int fileSize = htonl(ftell(fp));
                    rewind(fp);
                    send(connfd, &fileSize, sizeof(fileSize), 0);

                    // Transfer the file
                    for (;;) {

                        // First read file in chunks of BUF_SIZE bytes
                        unsigned char buff[BUF_SIZE]={0};
                        int nread = fread(buff,1,BUF_SIZE,fp);
                        //printf("Bytes read %d\n", nread);

                        // If read was success, send data
                        if (nread > 0) {
                            write(connfd, buff, nread);
                        }

                        if (nread < BUF_SIZE) {
                            if (feof(fp))
                                //printf("End of file\n");
                            if (ferror(fp))
                                printf("Error reading\n");

                            break;
                        }
                    }

                
    		        fclose(fp);
                }
                else {
                    strcpy(responseBuffer, "NOEXIST");
                    send(connfd, &responseBuffer, 10, 0);
                }
    		}

            // The request is for package existence
            else if (strstr(requestBuff, "EXT:")) {

                // check if packages exists on server
                if (pkg_exists(fullPath) == 1) {

                    // tell the client that it exists
                    strcpy(responseBuffer, "EXISTS");
                    send(connfd, &responseBuffer, 10, 0);
                }
                else {

                    // tell the client that it doesn't exist
                    strcpy(responseBuffer, "NOEXIST");
                    send(connfd, &responseBuffer, 10, 0);
                }
            }

            // free memory
            free(fullPath);
            close(connfd);

            exit(0);
        }
        else {
            close(connfd);
        }

    }

	return 0;
}

int main(int argc, char** argv) {  
    return server();
}
