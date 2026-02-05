#ifndef STRUTTURE_H
#define STRUTTURE_H

#include <pthread.h>
#include <stdbool.h>

#define MAXPARTITA 64
#define MAXPLAYER 16
#define MAXOUT 128
#define MAXIN 16

// Stati
enum stato_partita {
    NUOVA_CREAZIONE,
    IN_ATTESA,
    IN_CORSO,
    TERMINATA
};

enum stato_giocatore {
    IN_LOBBY,
    IN_RICHIESTA,
    IN_PARTITA
};

// Nodo giocatore
struct nodo_giocatore {
    char nome[MAXPLAYER];
    enum stato_giocatore stato;

    pthread_cond_t stato_cv;
    pthread_mutex_t stato_mutex;

    bool campione;
    unsigned int vittorie;
    unsigned int sconfitte;
    unsigned int pareggi;

    pthread_t tid_giocatore;
    int sd_giocatore;

    struct nodo_giocatore *next_node;
};

// Nodo partita
struct nodo_partita {
    unsigned int id;

    char proprietario[MAXPLAYER];
    int sd_proprietario;

    char avversario[MAXPLAYER];
    int sd_avversario;

    enum stato_partita stato;
    bool richiesta_unione;

    pthread_cond_t stato_cv;
    pthread_mutex_t stato_mutex;

    struct nodo_partita *next_node;
};

// Liste
extern struct nodo_partita *testa_partite;
extern struct nodo_giocatore *testa_giocatori;

// Contatore globale per gli ID delle partite
extern unsigned int prossimo_id_partita;

// Mutex globali
extern pthread_mutex_t mutex_partite;
extern pthread_mutex_t mutex_giocatori;

// Costanti
extern const char NO_ERROR;
extern const char ERROR_CONN;

extern const char ESITO_NONE;
extern const char ESITO_VITTORIA;
extern const char ESITO_SCONFITTA;
extern const char ESITO_PAREGGIO;

#endif
