#ifndef STRUTTURE_H
#define STRUTTURE_H

#include <pthread.h>
#include <stdbool.h>

// Dimensioni buffer
#define MAX_RICEZIONE 512
#define MAX_INPUT 16

// Griglia 3x3
extern char griglia[3][3];
extern int socket_fd;

// Costanti esito partita
extern const char ESITO_NONE;
extern const char ESITO_VITTORIA;
extern const char ESITO_SCONFITTA;
extern const char ESITO_PAREGGIO;

// Costanti errori connessione
extern const char NO_ERROR;
extern const char ERROR_CONN;

// Tipo giocatore
typedef enum {
    PROPRIETARIO,
    AVVERSARIO
} Ruolo;

#endif
