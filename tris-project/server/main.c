#include "connessione.h"
#include "gestione_client.h"
#include "gestione_partita.h"
#include "strutture.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Inizializzazione delle liste globali
struct nodo_partita *testa_partite = NULL;
struct nodo_giocatore *testa_giocatori = NULL;

// Contatore ID partite
unsigned int prossimo_id_partita = 1;

// Inizializzazione dei mutex globali
pthread_mutex_t mutex_partite = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_giocatori = PTHREAD_MUTEX_INITIALIZER;

// Costanti di errore per gestire le disconnessioni
const char NO_ERROR = '0';
const char ERROR_CONN = '1';

// Costanti per l'esito delle partite
const char ESITO_NONE = '0';
const char ESITO_VITTORIA = '1';
const char ESITO_SCONFITTA = '2';
const char ESITO_PAREGGIO = '3';

// Socket server globale
int socket_server;

// Chiusura server con CTRL+C
void chiudi_server(int sig) 
{
    (void)sig;  // Evita warning per parametro inutilizzato
    printf("\n[SERVER] Arresto in corso...\n");
    close(socket_server);
    exit(0);
}

int main()
{
    signal(SIGINT, chiudi_server);
    
    // Creazione e configurazione del socket server
    socket_server = crea_socket_server();
    printf("[SERVER] Server avviato sulla porta 8080\n");
    
    struct sockaddr_in indirizzo_client;
    socklen_t lunghezza_indirizzo = sizeof(struct sockaddr_in);

    // Configurazione attributi thread: creazione in stato detached
    pthread_attr_t attributi_thread;    
    pthread_attr_init(&attributi_thread);
    pthread_attr_setdetachstate(&attributi_thread, PTHREAD_CREATE_DETACHED);

    // Configurazione handler per i segnali gestiti dai thread
    struct sigaction *azione_segnale = (struct sigaction *) malloc(sizeof(struct sigaction));
    memset(azione_segnale, 0, sizeof(struct sigaction));
    azione_segnale->sa_flags = SA_RESTART;

    azione_segnale->sa_handler = invia_lista_partite_disponibili;   
    sigaction(SIGUSR1, azione_segnale, NULL);
    
    // azione_segnale->sa_handler = gestisci_notifica_nuovo_giocatore;
    // sigaction(SIGUSR2, azione_segnale, NULL);
    
    signal(SIGALRM, gestisci_timeout_connessione);
    signal(SIGTERM, gestisci_chiusura_server);
    
    free(azione_segnale);
    
    printf("[SERVER] In attesa di connessioni...\n");
    printf("[DEBUG-MAIN] Server pronto, mutex inizializzati\n");
    fflush(stdout);
    
    // Ciclo principale
    while (true)
{
    printf("[DEBUG-MAIN] Ciclo accept in attesa...\n");
    fflush(stdout);
    
    int *socket_client_ptr = malloc(sizeof(int));
    if (socket_client_ptr == NULL)
    {
        printf("[DEBUG-MAIN] ERRORE: malloc fallito!\n");
        fflush(stdout);
        perror("[ERRORE] Impossibile allocare memoria per socket descriptor");
        continue;
    }
    printf("[DEBUG-MAIN] Malloc riuscito: %p\n", (void*)socket_client_ptr);
    fflush(stdout);

    *socket_client_ptr = accept(socket_server, (struct sockaddr *) &indirizzo_client, &lunghezza_indirizzo);
    
    printf("[DEBUG-MAIN] Accept completato, socket: %d\n", *socket_client_ptr);
    fflush(stdout);
    
    if (*socket_client_ptr < 0)
    {
        printf("[DEBUG-MAIN] Accept fallito\n");
        fflush(stdout);
        perror("[ERRORE] Errore nell'accettazione della connessione");
        free(socket_client_ptr);
        continue;
    }

    printf("[DEBUG-MAIN] Nuova connessione accettata (socket: %d)\n", *socket_client_ptr);
    fflush(stdout);

    pthread_t tid_client;
    printf("[DEBUG-MAIN] Creazione thread...\n");
    fflush(stdout);
    
    if (pthread_create(&tid_client, &attributi_thread, gestisci_connessione_client, (void*)socket_client_ptr) != 0)
    {
        printf("[DEBUG-MAIN] pthread_create fallito!\n");
        fflush(stdout);
        perror("[ERRORE] Errore nella creazione del thread client");
        close(*socket_client_ptr);
        free(socket_client_ptr);
        continue;
    }
    
    printf("[DEBUG-MAIN] Thread creato con successo\n");
    fflush(stdout);
}
}