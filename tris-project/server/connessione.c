#include "connessione.h"
#include "gestione_client.h"
#include "gestione_partita.h"
#include "strutture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

// Inizializza il socket del server e ritorna il file descriptor
int crea_socket_server() // Crea la socket TCP, la configura e la mette in ascolto sulla porta 8080
{
    int socket_server;
    int abilita_riutilizzo = 1; // 1 = abilita SO_REUSEADDR, 0 = disabilita
    struct sockaddr_in indirizzo_server;
    socklen_t lunghezza_indirizzo = sizeof(struct sockaddr_in);

    // Creazione della socket con protocollo TCP (AF_INET = IPv4, SOCK_STREAM = TCP)
    if ((socket_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        perror("Errore nella creazione della socket"), exit(EXIT_FAILURE);

    // Configurazione SO_REUSEADDR: permette di riutilizzare immediatamente la porta
    // dopo la chiusura del server
    if (setsockopt(socket_server, SOL_SOCKET, SO_REUSEADDR, &abilita_riutilizzo, sizeof(abilita_riutilizzo)) < 0) 
    {
        perror("Errore nella configurazione setsockopt");
        close(socket_server);
        exit(EXIT_FAILURE);
    }

    // Inizializzazione e configurazione dell'indirizzo del server
    memset(&indirizzo_server, 0, lunghezza_indirizzo);
    indirizzo_server.sin_family = AF_INET;           // Famiglia di indirizzi IPv4
    indirizzo_server.sin_port = htons(8080);         // Porta 8080 in network byte order
    indirizzo_server.sin_addr.s_addr = INADDR_ANY;   // Accetta connessioni da qualsiasi interfaccia di rete

    // Binding: associa la socket all'indirizzo IP e alla porta specificati
    if (bind(socket_server, (struct sockaddr *) &indirizzo_server, lunghezza_indirizzo) < 0)
        perror("Errore nel binding della socket"), exit(EXIT_FAILURE);

    // Mette la socket in modalità ascolto con coda di massimo 5 connessioni pendenti
    if (listen(socket_server, 5) < 0)
        perror("Errore nella messa in ascolto della socket"), exit(EXIT_FAILURE);

    return socket_server;
}


void* gestisci_connessione_client(void *socket_descriptor)
{
    // DEBUG: Verifica che il puntatore non sia NULL
    if (socket_descriptor == NULL)
    {
        pthread_exit(NULL);
    }

    const int sd_client = *((int *)socket_descriptor);
    
    free(socket_descriptor);
    
    struct nodo_giocatore *giocatore = aggiungi_nuovo_giocatore(sd_client);

    notifica_giocatore_connesso();

    if (giocatore == NULL)
    {
        close(sd_client);
        pthread_exit(NULL);
    }

    // Ciclo principale: gestisce le attività del giocatore nella lobby
    esegui_ciclo_lobby(giocatore);
    
    close(sd_client);
    rimuovi_giocatore_lista(giocatore);
    
    pthread_exit(NULL);
}