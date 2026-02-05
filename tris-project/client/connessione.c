#include "connessione.h"

void connetti_al_server() {
    struct sockaddr_in server_addr;
    socklen_t len = sizeof(struct sockaddr_in);

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror("Errore creazione socket"), exit(EXIT_FAILURE);

    const int opt = 1;
    struct timeval timer = {300, 0};

    // Timeout e disattivazione del buffering TCP
    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timer, sizeof(timer)) < 0 ||
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timer, sizeof(timer)) < 0 ||
        setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) < 0)
        perror("Errore settaggio socket"), exit(EXIT_FAILURE);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(socket_fd, (struct sockaddr *)&server_addr, len) < 0)
        perror("Errore connessione al server"), exit(EXIT_FAILURE);
}

void gestisci_errore() {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
        printf("⚠️ Disconnesso per inattività\n");
    else
        printf("❌ Errore di connessione\n");

    close(socket_fd);
    exit(EXIT_FAILURE);
}

void SIGUSR1_handler() {
    pthread_exit(NULL);
}

void SIGTERM_handler() {
    close(socket_fd);
    exit(EXIT_SUCCESS);
}
