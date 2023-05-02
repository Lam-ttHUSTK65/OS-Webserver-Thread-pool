#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <pthread.h>
#include "myqueue.h"

#define SERVERPORT 8080
#define BUFSIZE 4096
#define SOCKETERROR (-1)
#define SERVER_BACKLOG 100
#define THREAD_POOL_SIZE 20

pthread_t thread_pool[THREAD_POOL_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;


typedef struct sockaddr_in SA_IN;
typedef struct sockaddr SA;


void * handle_connection(void* p_client_socket);
int check(int exp, const char *msg);
void * thread_function(void *argv);

int main(int argc, char **argv)
{
    int server_socket, client_socket, addr_size;
    SA_IN server_addr, client_addr;

    //first off create a bunch of thread to handle future connections
    for(int i = 0; i < THREAD_POOL_SIZE; i++){
        pthread_create(&thread_pool[i], NULL, thread_function, NULL);
    }
    

    check((server_socket = socket (AF_INET, SOCK_STREAM, 0)), "Failed to create socket");


    //initialize the address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons (SERVERPORT);


    check(bind(server_socket, (SA*)&server_addr, sizeof(server_addr)), "Bind Failed!");
    check(listen (server_socket, SERVER_BACKLOG), "Listen Failed!");


    while (true) {
        printf("Waiting for connections...\n");
        //wait for, and eventually accept an incoming connection
        addr_size = sizeof(SA_IN);
        check (client_socket = accept(server_socket, (SA*)&client_addr, (socklen_t*)&addr_size),"accept failed");
        printf("Connected with client: %s\n", inet_ntoa(client_addr.sin_addr));
        //put the connection somewhere so that an available connection thread can find it
        
        int *pclient = malloc(sizeof(int));
        *pclient = client_socket;

        //make sure only one thread messes with the queue at a time
        pthread_mutex_lock(&mutex);
        enqueue(pclient);
        pthread_cond_signal(&condition_var);
        pthread_mutex_unlock(&mutex);
    } //while


    return 0;
}


int check(int exp, const char *msg){
    if(exp == SOCKETERROR){
        perror (msg);
        exit(1);
    }
    return exp;
}

void * thread_function(void *arg){
    while (true) {
        int *pclient;
        pthread_mutex_lock(&mutex); 

       
        if ((pclient = dequeue()) == NULL){
            pthread_cond_wait(&condition_var, &mutex);
            //try again
            pclient = dequeue();
        }        
        pthread_mutex_unlock(&mutex);
        if( pclient != NULL){
            handle_connection(pclient);
        }

    }
}


void * handle_connection(void* p_client_socket) {
    int client_socket = *(int*)p_client_socket;
    free(p_client_socket);

    char request[BUFSIZE];
    ssize_t bytes_received = recv(client_socket, request, BUFSIZE - 1, 0);
    if (bytes_received < 0) {
        perror("Failed to receive bytes from client");
        close(client_socket);
        return NULL;
    }

    request[bytes_received] = '\0';
    printf("Request received:\n%s\n", request);

    char response[BUFSIZE];
    char *path = strtok(request, " ");
    if (strcmp(path, "GET") == 0) {
        path = strtok(NULL, " ");
        char *file_path = path + 1;  // Bỏ đi ký tự '/' ở đầu đường dẫn
        char full_path[100];
        sprintf(full_path, "/home/lam/Documents/%s.txt", file_path);
        FILE *fp = fopen(full_path, "r");
        if (fp == NULL) {
            sprintf(response, "HTTP/1.1 404 Not Found\r\n\r\n");
        } else {
            // Get the file size
            fseek(fp, 0, SEEK_END);
            size_t file_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            // Build the response header
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\nContent-Disposition: attachment; filename=\"%s\"\r\n\r\n", file_size, file_path);

            // Send the response header
            if (send(client_socket, response, strlen(response), 0) < 0) {
                perror("Failed to send response header to client");
                fclose(fp);
                close(client_socket);
                return NULL;
            }

            // Send the file contents
            char file_buffer[BUFSIZE];
            size_t n;
            while ((n = fread(file_buffer, 1, BUFSIZE, fp)) > 0) {
                if (send(client_socket, file_buffer, n, 0) < 0) {
                    perror("Failed to send file to client");
                    break;
                }
            }

            fclose(fp);
        }
    } else {
        sprintf(response, "HTTP/1.1 400 Bad Request\r\n\r\n");
        if (send(client_socket, response, strlen(response), 0) < 0) {
            perror("Failed to send response to client");
        }
    }

    printf("Response sent:\n%s\n", response);

    close(client_socket);
    return NULL;
}
