#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <math.h>
#include <time.h>

#define PORT 8054
#define BUFFER_SIZE 256
#define MAX_CLIENTS 12


// Shared memory and synchronization
unsigned long slot[MAX_CLIENTS]; 
char serverflag[MAX_CLIENTS] = {0}; 
int rotation_count[MAX_CLIENTS] = {0};
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int outstanding_requests = 0; 

// Function to right rotate n by d bits
unsigned int rightRotate(int n, unsigned int d) {
    return (n >> d) | (n << (32 - d));
}

// Function to check if a number is prime
int isPrime(int num) {
    if (num <= 1) return 0;
    for (int j = 2; j <= sqrt(num); j++) {
        if (num % j == 0) return 0;
    }
    return 1;
}

// Function to find prime factors and send them immediately
void primeFactors(int num, int sock) {
    if (num <= 1) {
        send(sock, "None\n", 5, 0);
        return;
    }

    char factors[BUFFER_SIZE] = "";  
    int found = 0;

    for (int i = 2; i <= num; i++) {
        if (num % i == 0 && isPrime(i)) {
            snprintf(factors + strlen(factors), BUFFER_SIZE - strlen(factors), "%d ", i);
            found = 1;
        }
    }

    if (!found) {
        strcat(factors, "None");
    } else {
        factors[strlen(factors) - 1] = '\0'; 
    }

    send(sock, factors, strlen(factors), 0);
    send(sock, "\n", 1, 0); 
}

// Function to handle each thread's behavior for test mode
void *test_mode_thread(void *arg) {
    int thread_id = *(int *)arg;
    int sock = *(int *)(arg + sizeof(int));

    for (int i = 0; i < 10; i++) {
        int number_to_send = (thread_id * 10) + i;
        char message[BUFFER_SIZE];
        snprintf(message, sizeof(message), "Test number: %d\n", number_to_send);

        send(sock, message, strlen(message), 0);

        // Introduce random delay between 10ms and 100ms
        usleep((rand() % 91 + 10) * 1000);
    }
    return NULL;
}

// Function to handle client requests
void *handle_client(void *socket_desc) {
    int sock = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];
    int client_index = -1;

    pthread_mutex_lock(&mutex);
    // Find an available slot
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (slot[i] == 0) { // Slot is available
            slot[i] = 1; // Mark as occupied
            client_index = i; // Assign this slot to the client
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    if (client_index == -1) {
        send(sock, "Server is busy, please try again later.\n", 40, 0);
        close(sock);
        free(socket_desc);
        return NULL;
    }

    while (recv(sock, buffer, BUFFER_SIZE, 0) > 0) {
        buffer[strcspn(buffer, "\n")] = 0;

        // Check for test mode input
        if (strcmp(buffer, "0") == 0) { // Check for test mode input
            pthread_mutex_lock(&mutex);
            if (outstanding_requests == 0) {
                outstanding_requests += 3;
                pthread_mutex_unlock(&mutex);

                printf("Entering test mode...\n");
                pthread_t threads[3];
                int thread_ids[3];

                for (int i = 0; i < 3; i++) {
                    thread_ids[i] = i; // Assign thread ID
                    pthread_create(&threads[i], NULL, test_mode_thread, (void *)&thread_ids[i]); // Create thread
                    }

                    for (int i = 0; i < 3; i++) {
                        pthread_join(threads[i], NULL);
                    }

                    pthread_mutex_lock(&mutex);
                    outstanding_requests -= 3;
                    pthread_mutex_unlock(&mutex);
                } else {
                    pthread_mutex_unlock(&mutex);
                    send(sock, "Server is busy, please wait.\n", 31, 0); // Send busy message if server is busy
                }
                continue;
        }

        int n = atoi(buffer);  

        pthread_mutex_lock(&mutex);
        if (outstanding_requests < MAX_CLIENTS) {
            clock_t start_time = clock();
            outstanding_requests++;
            rotation_count[client_index] = 0; // Reset rotation count
            pthread_mutex_unlock(&mutex);

            for (int i = 0; i < 32; i++) {
                unsigned int rotatedValue = rightRotate(n, i);
                char response[BUFFER_SIZE];
                snprintf(response, sizeof(response), "Slot %d - Rotation %d: %u | Prime Factors: ", client_index, i+1, rotatedValue);
                send(sock, response, strlen(response), 0);
                primeFactors(rotatedValue, sock);
                
                // Increment and display the progress for the client slot
                rotation_count[client_index]++;
                printf("Slot %d - Progress: %d%%\n", client_index, (rotation_count[client_index] * 100) / 32);
            }

            // Send completion message to client
            clock_t end_time = clock(); // End timing
            double time_taken = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
            char completion_message[BUFFER_SIZE];
            snprintf(completion_message, sizeof(completion_message), "Slot %d - All factors found. Time taken: %.2f seconds.\n", client_index, time_taken);
            send(sock, completion_message, strlen(completion_message), 0);

            pthread_mutex_lock(&mutex);
            outstanding_requests--; // Decrease outstanding requests count
            slot[client_index] = 0; // Mark slot as free
            pthread_mutex_unlock(&mutex);
        } else {
            send(sock, "System is busy, please wait.\n", 31, 0);
            pthread_mutex_unlock(&mutex);
        }
    }

    close(sock);
    free(socket_desc);
    return NULL;
}

int main() {
    int server_fd, new_socket, *new_sock;
    struct sockaddr_in server, client;
    socklen_t client_len = sizeof(client);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Could not create socket");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Bind failed");
        return 1;
    }

    listen(server_fd, MAX_CLIENTS);
    printf("Waiting for incoming connections...\n");

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&client, &client_len))) {
            printf("Connection accepted\n");

            new_sock = malloc(sizeof(int));
            *new_sock = new_socket;

            pthread_t client_thread;
            if (pthread_create(&client_thread, NULL, handle_client, (void *)new_sock) < 0) {
                perror("Could not create thread");
                close(new_socket); 
                free(new_sock); 
                continue; 
            }

            pthread_detach(client_thread);
        }
    }

    close(server_fd);
    return 0;
}
