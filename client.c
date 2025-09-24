#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MSG_LEN 1024
#define MAX_INPUT_LEN 768  // Since Base64 encoding expands size by ~4/3
#define BASE64_TABLE "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

// Message structure
typedef struct {
    int type;
    char content[MSG_LEN];
} Message;

// Function prototypes
char *encode_base64(const char *input, size_t input_len);
void send_message(int socket, const char *message, int message_type, 
                 struct sockaddr_in *server_addr, int is_tcp);
int receive_ack(int socket, struct sockaddr_in *server_addr, int is_tcp);
void print_message_type(int type);

// Function to print message type
void print_message_type(int type) {
    printf("Message Type: ");
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

// Base64 encoding function
char *encode_base64(const char *input, size_t input_len) {
    if (!input || input_len == 0) return NULL;
    
    // Calculate output size (including padding)
    size_t output_len = 4 * ((input_len + 2) / 3);
    char *output = malloc(output_len + 1);
    if (!output) return NULL;
    
    size_t i = 0, j = 0;
    uint32_t octet_a, octet_b, octet_c;
    uint32_t triple;
    
    while (i < input_len) {
        octet_a = i < input_len ? (unsigned char)input[i++] : 0;
        octet_b = i < input_len ? (unsigned char)input[i++] : 0;
        octet_c = i < input_len ? (unsigned char)input[i++] : 0;
        
        triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        
        output[j++] = BASE64_TABLE[(triple >> 18) & 0x3F];
        output[j++] = BASE64_TABLE[(triple >> 12) & 0x3F];
        output[j++] = (i > input_len + 1) ? '=' : BASE64_TABLE[(triple >> 6) & 0x3F];
        output[j++] = (i > input_len) ? '=' : BASE64_TABLE[triple & 0x3F];
    }
    
    output[j] = '\0';
    return output;
}

// Send message function
void send_message(int socket, const char *message, int message_type,
                 struct sockaddr_in *server_addr, int is_tcp) {
    Message msg;
    msg.type = message_type;
    strncpy(msg.content, message, MSG_LEN - 1);
    msg.content[MSG_LEN - 1] = '\0';
    
    // Print message type being sent
    print_message_type(message_type);
    
    if (is_tcp) {
        send(socket, &msg, sizeof(Message), 0);
        printf("Sent message to server via TCP\n");
    } else {
        socklen_t addr_len = sizeof(struct sockaddr_in);
        sendto(socket, &msg, sizeof(Message), 0, 
              (struct sockaddr *)server_addr, addr_len);
        printf("Sent message to server via UDP\n");
    }
}

// Receive acknowledgment
int receive_ack(int socket, struct sockaddr_in *server_addr, int is_tcp) {
    Message ack_msg;
    
    if (is_tcp) {
        ssize_t bytes_received = recv(socket, &ack_msg, sizeof(Message), 0);
        if (bytes_received <= 0) {
            return 0;
        }
    } else {
        socklen_t addr_len = sizeof(struct sockaddr_in);
        ssize_t bytes_received = recvfrom(socket, &ack_msg, sizeof(Message), 0,
                                        (struct sockaddr *)server_addr, &addr_len);
        if (bytes_received <= 0) {
            return 0;
        }
    }
    
    // Print the message type received
    if (ack_msg.type == 2) {
        print_message_type(ack_msg.type);
        printf("Server response: %s\n", ack_msg.content);
        return 1;
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <server_ip> <server_port> <protocol>\n", argv[0]);
        printf("Protocol can be 'tcp' or 'udp'\n");
        return 1;
    }
    
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int is_tcp = (strcmp(argv[3], "tcp") == 0);
    
    if (!is_tcp && strcmp(argv[3], "udp") != 0) {
        printf("Protocol must be 'tcp' or 'udp'\n");
        return 1;
    }
    
    // Create socket
    int sock;
    if (is_tcp) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
    }
    
    if (sock == -1) {
        perror("Failed to create socket");
        return 1;
    }
    
    // Set up server address structure
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(sock);
        return 1;
    }
    
    // Connect if using TCP
    if (is_tcp) {
        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Failed to connect to server");
            close(sock);
            return 1;
        }
        printf("Connected to server %s:%d using TCP\n", server_ip, server_port);
    } else {
        printf("Using UDP for communication with server %s:%d\n", server_ip, server_port);
    }
    
    // Get client information
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    if (getsockname(sock, (struct sockaddr *)&client_addr, &client_len) == 0) {
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Client running on %s:%d\n", client_ip, ntohs(client_addr.sin_port));
    }
    
    while (1) {
        char input[MAX_INPUT_LEN];
        printf("\nEnter message (or 'quit' to exit): ");
        fgets(input, MAX_INPUT_LEN, stdin);
        
        // Remove trailing newline
        size_t input_len = strlen(input);
        if (input_len > 0 && input[input_len - 1] == '\n') {
            input[input_len - 1] = '\0';
            input_len--;
        }
        
        // Check for quit command
        if (strcmp(input, "quit") == 0) {
            if (is_tcp) {
                printf("Sending termination message...\n");
                send_message(sock, "TERMINATE", 3, &server_addr, is_tcp);
            }
            break;
        }
        
        // Encode the message
        char *encoded = encode_base64(input, input_len);
        if (!encoded) {
            printf("Failed to encode message\n");
            continue;
        }
        
        // printf("Original message: %s\n", input);
        printf("Base64 encoded message: %s\n", encoded);
        
        // Send the message
        send_message(sock, encoded, 1, &server_addr, is_tcp);
        free(encoded);
        
        // Wait for acknowledgment
        if (receive_ack(sock, &server_addr, is_tcp)) {
            printf("Received acknowledgment from server\n");
        } else {
            printf("Failed to receive acknowledgment from server\n");
            if (is_tcp) {
                // For TCP, this indicates a potential connection issue
                break;
            }
        }
    }
    
    // Close socket
    close(sock);
    printf("Connection closed\n");
    
    return 0;
}