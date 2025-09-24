#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>
#include <sys/select.h>

#define MSG_LEN 1024
#define BASE64_TABLE "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

// Message structure
typedef struct {
    int type;
    char content[MSG_LEN];
} Message;

// Thread argument structure
typedef struct {
    int client_socket;
    struct sockaddr_in client_addr;
} ThreadArgs;

// Function prototypes
void *handle_tcp_client(void *arg);
void handle_udp_message(int udp_socket, struct sockaddr_in *client_addr);
char *decode_base64(const char *input);
void send_ack(int socket, struct sockaddr_in *addr, int is_tcp);
void print_message_type(int type);

// Function to print message type with client information
void print_message_type(int type) {
    printf("\t>>> ");
    switch (type) {
        case 1:
            printf("Type 1: Base64-encoded message\n");
            break;
        case 2:
            printf("Type 2: Acknowledgment (ACK) message\n");
            break;
        case 3:
            printf("Type 3: Termination message\n");
            break;
        default:
            printf("Unknown message type: %d\n", type);
    }
}

// Base64 decoding function
char *decode_base64(const char *input) {
    if (!input) return NULL;
    
    size_t input_len = strlen(input);
    if (input_len == 0) return NULL;
    
    // Find the position of each character in the BASE64_TABLE
    int char_index[256];
    memset(char_index, -1, sizeof(char_index));
    for (int i = 0; i < 64; i++) {
        char_index[(int)BASE64_TABLE[i]] = i;
    }
    
    // Calculate output length (accounting for padding)
    size_t padding = 0;
    if (input_len > 0 && input[input_len - 1] == '=') padding++;
    if (input_len > 1 && input[input_len - 2] == '=') padding++;
    size_t output_len = (input_len * 3) / 4 - padding;
    
    // Allocate memory for decoded output
    char *output = malloc(output_len + 1);
    if (!output) return NULL;
    
    // Decode the input
    size_t i = 0, j = 0;
    uint32_t sextet_a, sextet_b, sextet_c, sextet_d;
    uint32_t triple;
    
    while (i < input_len) {
        // Get the sextets from four consecutive base64 chars
        sextet_a = input[i] == '=' ? 0 : char_index[(int)input[i]]; i++;
        sextet_b = input[i] == '=' ? 0 : char_index[(int)input[i]]; i++;
        sextet_c = (i < input_len && input[i] != '=') ? char_index[(int)input[i]] : 0; i++;
        sextet_d = (i < input_len && input[i] != '=') ? char_index[(int)input[i]] : 0; i++;
        
        // Combine the sextets into a 24-bit number
        triple = (sextet_a << 18) + (sextet_b << 12) + (sextet_c << 6) + sextet_d;
        
        // Extract the original three bytes
        if (j < output_len) output[j++] = (triple >> 16) & 0xFF;
        if (j < output_len) output[j++] = (triple >> 8) & 0xFF;
        if (j < output_len) output[j++] = triple & 0xFF;
    }
    
    output[output_len] = '\0';
    return output;
}

// TCP client handler thread function
void *handle_tcp_client(void *arg) {
    ThreadArgs *thread_args = (ThreadArgs *)arg;
    int client_socket = thread_args->client_socket;
    struct sockaddr_in client_addr = thread_args->client_addr;
    free(thread_args);
    
    // Get client IP and port for logging
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr.sin_port);
    
    Message msg;
    while (1) {
        // Receive message from client
        ssize_t bytes_received = recv(client_socket, &msg, sizeof(Message), 0);
        if (bytes_received <= 0) {
            printf("\n[TCP Client %s:%d] Disconnected unexpectedly\n", client_ip, client_port);
            break;
        }
        
        // Display message type with client information
        printf("\n[TCP Client %s:%d] \n", client_ip, client_port);
        print_message_type(msg.type);
        
        if (msg.type == 1) {
            // Decode Base64 message
            printf("\t>>> Encoded message: %s\n", msg.content);
            char *decoded = decode_base64(msg.content);
            if (decoded) {
                printf("\t>>> Decoded message: %s\n", decoded);
                free(decoded);
            } else {
                printf("\t>>> Failed to decode Base64 message\n");
            }
            
            // Send ACK
            Message ack_msg;
            ack_msg.type = 2;
            strcpy(ack_msg.content, "ACK");
            send(client_socket, &ack_msg, sizeof(Message), 0);
            printf("\t>>> Sent Type 2: Acknowledgment (ACK) message\n");
        } else if (msg.type == 3) {
            printf("\t>>> Requested termination\n");
            break;
        }
    }
    
    close(client_socket);
    printf("\t>>> Connection closed\n");
    pthread_exit(NULL);
}

// Handle UDP message
void handle_udp_message(int udp_socket, struct sockaddr_in *client_addr) {
    Message msg;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    
    // Receive message from client
    ssize_t bytes_received = recvfrom(udp_socket, &msg, sizeof(Message), 0, 
                                    (struct sockaddr *)client_addr, &addr_len);
    
    if (bytes_received <= 0) {
        printf("Failed to receive UDP message\n");
        return;
    }
    
    // Get client IP and port for logging
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(client_addr->sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr->sin_port);
    
    // Display message type with client information
    printf("\n[UDP Client %s:%d] \n", client_ip, client_port);
    print_message_type(msg.type);
    
    if (msg.type == 1) {
        // Decode Base64 message
        printf("\t>>> Encoded message: %s\n", msg.content);
        
        char *decoded = decode_base64(msg.content);
        if (decoded) {
            printf("\t>>> Decoded message: %s\n", decoded);
            free(decoded);
        } else {
            printf("\t>>> Failed to decode Base64 message\n");
        }
        
        // Send ACK
        Message ack_msg;
        ack_msg.type = 2;
        strcpy(ack_msg.content, "ACK");
        sendto(udp_socket, &ack_msg, sizeof(Message), 0, 
              (struct sockaddr *)client_addr, addr_len);
        printf("\t>>> Sent Type 2: Acknowledgment (ACK) message\n");
    }
    // Note: Type 3 (termination) messages are not relevant for UDP as it's connectionless
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    
    // Create TCP socket
    int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket == -1) {
        perror("Failed to create TCP socket");
        return 1;
    }
    
    // Create UDP socket
    int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket == -1) {
        perror("Failed to create UDP socket");
        close(tcp_socket);
        return 1;
    }
    
    // Set up server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind TCP socket
    if (bind(tcp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to bind TCP socket");
        close(tcp_socket);
        close(udp_socket);
        return 1;
    }
    
    // Bind UDP socket
    if (bind(udp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Failed to bind UDP socket");
        close(tcp_socket);
        close(udp_socket);
        return 1;
    }
    
    // Listen for TCP connections
    if (listen(tcp_socket, 10) == -1) {
        perror("Failed to listen on TCP socket");
        close(tcp_socket);
        close(udp_socket);
        return 1;
    }
    
    printf("Server started on port %d\n", port);
    // printf("Listening for TCP and UDP connections...\n");
    
    // Set up for select()
    fd_set read_fds;
    int max_fd = (tcp_socket > udp_socket) ? tcp_socket : udp_socket;
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(tcp_socket, &read_fds);
        FD_SET(udp_socket, &read_fds);
        
        // Wait for activity on either socket
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select() failed");
            break;
        }
        
        // Check for TCP connections
        if (FD_ISSET(tcp_socket, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            
            int client_socket = accept(tcp_socket, (struct sockaddr *)&client_addr, &addr_len);
            if (client_socket == -1) {
                perror("Failed to accept TCP connection");
                continue;
            }
            
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
            int client_port = ntohs(client_addr.sin_port);
            
            printf("New TCP connection from %s:%d - ", client_ip, client_port);
            printf("Connection Established\n");
            
            // Create thread to handle this client
            pthread_t thread_id;
            ThreadArgs *args = malloc(sizeof(ThreadArgs));
            args->client_socket = client_socket;
            args->client_addr = client_addr;
            
            if (pthread_create(&thread_id, NULL, handle_tcp_client, args) != 0) {
                perror("Failed to create thread");
                free(args);
                close(client_socket);
            } else {
                pthread_detach(thread_id);
            }
        }
        
        // Check for UDP messages
        if (FD_ISSET(udp_socket, &read_fds)) {
            struct sockaddr_in client_addr;
            handle_udp_message(udp_socket, &client_addr);
        }
    }
    
    // Close sockets
    close(tcp_socket);
    close(udp_socket);
    
    return 0;
}