#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8054
#define BUFFER_SIZE 256

// Shared variables for synchronization
char message[BUFFER_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int exit_program = 0;
int processing_request = 0;

// Function to handle receiving server responses
void *receive_messages(void *sock) {
    int socket = *(int *)sock;
    char response[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(socket, response, sizeof(response) - 1, 0);
        if (bytes_received <= 0) {
            printf("Server disconnected.\n");
            exit_program = 1; // Set exit flag
            break;
        }
        response[bytes_received] = '\0'; 
        printf("%s", response); // Print the server response
        fflush(stdout); // Ensure the output is printed immediately
        
        if (strstr(response, "All factors found") != NULL) {
            processing_request = 0;
        }
    }
    return NULL;
}

int main() {
    int sock;
    struct sockaddr_in server;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        return 1;
    }

    // Set up the server structure
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");  // Localhost

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        return 1;
    }

    printf("Connected to server.\n");

    // Create a thread for receiving messages from the server
    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, (void *)&sock);

    while (1) {
        printf("\nEnter a number to factor (or 'q' to quit): ");
        fflush(stdout);

        while (processing_request) {
            usleep(10000);
        }

        fgets(message, BUFFER_SIZE, stdin);

        message[strcspn(message, "\n")] = 0;

        // Check for exit condition
        if (strcmp(message, "q") == 0) {
            exit_program = 1; // Set the flag to exit the program
            break;
        }

        processing_request = 1;
        
        // Send the input to the server
        send(sock, message, strlen(message), 0);
    }

    // Clean up and close the socket
    close(sock);
    pthread_cancel(receive_thread);
    pthread_join(receive_thread, NULL);
    return 0;
}
