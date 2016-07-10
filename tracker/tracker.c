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

#include <mysql/mysql.h>

char *remove_prefix(char *command);

/* Main tracker loop */
int tracker() {

    // connect to mysql
    MYSQL *con = mysql_init(NULL);
    MYSQL_RES *result;
    MYSQL_ROW row;

    if (mysql_real_connect(con, "localhost", "root", "", "testdb", 0, NULL, 0) == NULL) {
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        exit(1);
    }

    // TRACKER SOCKET (for accepting requests)
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    // Keep socket open
    int option = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5003);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    if (listen(listenfd, 10) == -1) {
        printf("Failed to listen\n");
        return -1;
    }

    for (;;) {

        int connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

        if (fork() == 0) {

            printf("REQUEST!\n");

            // receive requests
        	char requestBuff[100];
    		recv(connfd, &requestBuff, 100, 0);

            // Our response to the client
            char responseBuffer[10];

            if (strstr(requestBuff, "TRK:")) {

                char *pkgname = remove_prefix(requestBuff);

                char query[150];
                sprintf(query, "SELECT * FROM `packages` WHERE `name`='%s'", pkgname);

                mysql_query(con, query);
                result = mysql_store_result(con);

                // check if package exists in db
                if (mysql_num_rows(result) == 0) {
                    char response[10];

                    // tell client the package doesn't exist
                    strcpy(response, "NOTPKG");
                    send(connfd, &response, strlen(response)+1, 0);
                }
                else {
                    // get client list
                    mysql_query(con, "SELECT * FROM `clients`");
                    result = mysql_store_result(con);
                    int num_fields = mysql_num_fields(result);
                    int found = 0;

                    // loop through each client
                    while ((row = mysql_fetch_row(result))) {
                        
                        found = 0;
                        char *id = row[0];
                        char *address = row[1];
                        char *lastupdated = row[2];
                        
                        // Create a socket for the node connection
                        int nodefd = 0;
                        if ((nodefd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                            printf("Error : Could not create socket\n");
                            return 1;
                        }

                        // Connect to one of many package nodes
                        struct sockaddr_in node_addr;
                        node_addr.sin_family = AF_INET;
                        node_addr.sin_port = htons(5004); // port
                        node_addr.sin_addr.s_addr = inet_addr(address);

                        struct timeval timeout;      
                        timeout.tv_sec = 0;
                        timeout.tv_usec = 50000;

                        setsockopt(nodefd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
                        setsockopt(nodefd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));

                        // Attempt a connection with node
                        if (connect(nodefd, (struct sockaddr *)&node_addr, sizeof(node_addr)) >= 0) {
                            char request[150];
                            char response[10];

                            // send package request to server
                            strcpy(request, "EXT:");
                            strcat(request, pkgname);
                            send(nodefd, &request, strlen(pkgname)+5, 0);

                            // receive a response to see if it has it
                            recv(nodefd, &response, 10, 0);

                            // reply with new IP address
                            if (strcmp(response, "EXISTS") == 0) {
                                char remote_address[20];
                                strcpy(remote_address, address);
                                printf("%s: %s\n", remote_address, pkgname);

                                send(connfd, &remote_address, strlen(remote_address)+1, 0);
                                found = 1;
                                close(nodefd);
                                break;
                            }

                        }
                       
                        


                    }

                    mysql_free_result(result);

                    // if still not found, use mirror
                    if (found == 0) {
                        char remote_address[20];
                        strcpy(remote_address, "MIRROR_URL");
                        send(connfd, &remote_address, strlen(remote_address)+1, 0);
                    }
                }

                
            }

            else if (strstr(requestBuff, "MD5:")) {

                char *pkgname = remove_prefix(requestBuff);

                char query[150];
                sprintf(query, "SELECT `hash` FROM `packages` WHERE `name`='%s'", pkgname);

                mysql_query(con, query);
                result = mysql_store_result(con);

                // check if package exists in db
                if (mysql_num_rows(result) == 0) {
                    char response[10];

                    // tell client the package doesn't exist
                    strcpy(response, "NOTPKG");
                    send(connfd, &response, strlen(response)+1, 0);
                }
                else {
                    
                    // return hash
                    while ((row = mysql_fetch_row(result))) {
                        char db_hash[33];
                        strcpy(db_hash, row[0]);
                        printf("returning hash: %s", row[0]);
                        send(connfd, &db_hash, strlen(db_hash)+1, 0);
                        break;
                    }
                }
                
            }

            exit(0);
        }
        else {
           close(connfd); 
        }
    }

    mysql_close(con);
    return 0;
}

/* Strip prefix from socket command */
char *remove_prefix(char *command) {
    return &command[4];
}

int main(int argc, char** argv) {
    return tracker();
}