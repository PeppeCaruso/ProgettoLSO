#include "gestione_client.h"
#include "strutture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <sys/socket.h>       
#include <netinet/in.h>

struct nodo_giocatore* aggiungi_nuovo_giocatore(const int sd_client)
{
    printf("[DEBUG-AGGIUNGI] Inizio registrazione cliente socket: %d\n", sd_client);
    fflush(stdout);
    
    // Validazione e ricezione del nome del giocatore dal client
    char *nome_giocatore = valida_registrazione_giocatore(sd_client);
    
    printf("[DEBUG-AGGIUNGI] Nome ricevuto: %s\n", nome_giocatore);
    fflush(stdout);
    
    // Creazione del nodo giocatore e inserimento in testa alla lista globale
    struct nodo_giocatore *nuovo_giocatore = inserisci_giocatore_lista(nome_giocatore, sd_client);
    
    printf("[DEBUG-AGGIUNGI] Giocatore inserito in lista: %p\n", (void*)nuovo_giocatore);
    fflush(stdout);
    
    // Liberazione della memoria allocata dinamicamente per il nome
    free(nome_giocatore);
    
    printf("[DEBUG-AGGIUNGI] Fine registrazione\n");
    fflush(stdout);
    
    return nuovo_giocatore;
}

char* valida_registrazione_giocatore(const int sd_client)
{
    printf("[DEBUG-VALIDA] Inizio validazione per socket: %d\n", sd_client);
    fflush(stdout);
    
    // Allocazione memoria per il nome del giocatore
    char *nome_giocatore = (char *) malloc(MAXPLAYER * sizeof(char));
    if (nome_giocatore == NULL)
    {
        printf("[DEBUG-VALIDA] ERRORE: malloc fallito!\n");
        fflush(stdout);
        send(sd_client, "errore\n", 7, MSG_NOSIGNAL);
        close(sd_client);
        pthread_exit(NULL);
    }

    printf("[DEBUG-VALIDA] Invio messaggio di benvenuto...\n");
    fflush(stdout);
    
    // Richiesta iniziale del nome al client
    if (send(sd_client, "Inserisci il tuo nome per registrarti (max 15 caratteri)\n", 58, MSG_NOSIGNAL) < 0) 
    {
        printf("[DEBUG-VALIDA] ERRORE: send fallito!\n");
        fflush(stdout);
        close(sd_client);
        free(nome_giocatore);
        pthread_exit(NULL);
    }

    printf("[DEBUG-VALIDA] Messaggio inviato, in attesa nome...\n");
    fflush(stdout);
    
    bool nome_valido = false;

    // Ciclo di validazione
    do
    {
        memset(nome_giocatore, 0, MAXPLAYER);
        
        printf("[DEBUG-VALIDA] In attesa di recv...\n");
        fflush(stdout);
        
        // Ricezione del nome dal client
        int bytes_ricevuti = recv(sd_client, nome_giocatore, MAXPLAYER, 0);
        
        printf("[DEBUG-VALIDA] Recv completato, bytes: %d, nome: %s\n", bytes_ricevuti, nome_giocatore);
        fflush(stdout);
        
        if (bytes_ricevuti <= 0)
        {
            printf("[DEBUG-VALIDA] ERRORE: recv fallito o connessione chiusa\n");
            fflush(stdout);
            close(sd_client);
            free(nome_giocatore);
            pthread_exit(NULL);
        }

        // Controllo univocità del nome
        printf("[DEBUG-VALIDA] Controllo univocità nome...\n");
        fflush(stdout);
        
        if (!nome_giocatore_gia_usato(nome_giocatore))
        {
            nome_valido = true;
            printf("[DEBUG-VALIDA] Nome valido!\n");
            fflush(stdout);
        }
        else
        {
            printf("[DEBUG-VALIDA] Nome già in uso\n");
            fflush(stdout);
            // Nome già esistente
            if (send(sd_client, "Il nome inserito è già utilizzato, prova un altro nome (max 15 caratteri)\n", 76, MSG_NOSIGNAL) < 0)
            {
                close(sd_client);
                free(nome_giocatore);
                pthread_exit(NULL);
            }
        }
    } while (!nome_valido);

    // Conferma della registrazione
    if (send(sd_client, "Registrazione completata\n", 25, MSG_NOSIGNAL) < 0)
    {
        close(sd_client);
        free(nome_giocatore);
        pthread_exit(NULL);
    }

    printf("[DEBUG-VALIDA] Validazione completata con successo\n");
    fflush(stdout);
    
    return nome_giocatore;
}

bool nome_giocatore_gia_usato(const char *nome_giocatore)
{
    // Acquisizione del lock per accesso thread-safe alla lista giocatori
    pthread_mutex_lock(&mutex_giocatori);
    
    struct nodo_giocatore *nodo_corrente = testa_giocatori;

    // Scansione della lista giocatori alla ricerca di un nome duplicato
    while (nodo_corrente != NULL)
    {
        // Confronto del nome: se trovato, il nome è già in uso
        if (strcmp(nodo_corrente->nome, nome_giocatore) == 0) 
        {
            pthread_mutex_unlock(&mutex_giocatori);
            return true;
        }
        nodo_corrente = nodo_corrente->next_node;
    }
    
    // Nome non trovato: è disponibile per la registrazione
    pthread_mutex_unlock(&mutex_giocatori); 
    return false;
}

struct nodo_giocatore* inserisci_giocatore_lista(const char *nome_giocatore, const int sd_client)
{
    // Allocazione memoria per il nuovo nodo giocatore
    struct nodo_giocatore *nuovo_nodo = (struct nodo_giocatore *) malloc(sizeof(struct nodo_giocatore));
    if (nuovo_nodo == NULL)
    {
        send(sd_client, "errore\n", 7, MSG_NOSIGNAL); 
        close(sd_client);
        printf("Errore: impossibile allocare memoria per il nuovo giocatore\n");
        pthread_exit(NULL);
    }

    // Inizializzazione del nodo giocatore
    memset(nuovo_nodo, 0, sizeof(struct nodo_giocatore)); // Pulizia memoria per sicurezza
    
    strcpy(nuovo_nodo->nome, nome_giocatore);
    nuovo_nodo->vittorie = 0;
    nuovo_nodo->sconfitte = 0;
    nuovo_nodo->pareggi = 0;
    nuovo_nodo->campione = false;
    nuovo_nodo->stato = IN_LOBBY;
    nuovo_nodo->sd_giocatore = sd_client;
    nuovo_nodo->tid_giocatore = pthread_self();
    
    // Inizializzazione mutex e condition variable per la sincronizzazione
    pthread_mutex_init(&(nuovo_nodo->stato_mutex), NULL);
    pthread_cond_init(&(nuovo_nodo->stato_cv), NULL);

    // === INSERIMENTO IN TESTA ALLA LISTA (thread-safe) ===
    pthread_mutex_lock(&mutex_giocatori);
    
    // Il nuovo nodo punta all'attuale testa della lista
    nuovo_nodo->next_node = testa_giocatori;
    
    // Aggiornamento della testa della lista globale
    testa_giocatori = nuovo_nodo;
    
    pthread_mutex_unlock(&mutex_giocatori);

    return nuovo_nodo;
}

struct nodo_giocatore* cerca_giocatore_per_socket(const int sd)
{
    struct nodo_giocatore *nodo_corrente = testa_giocatori;

    // Scansione della lista giocatori alla ricerca del socket descriptor
    while (nodo_corrente != NULL)
    {
        if (nodo_corrente->sd_giocatore == sd)
            return nodo_corrente;
        
        nodo_corrente = nodo_corrente->next_node;
    }
    
    // Socket descriptor non trovato (situazione improbabile in condizioni normali)
    return NULL;
}

struct nodo_giocatore* cerca_giocatore_per_thread(const pthread_t tid)
{
    pthread_mutex_lock(&mutex_giocatori);  // LOCK

    struct nodo_giocatore *nodo_corrente = testa_giocatori;

    // Scansione della lista giocatori alla ricerca del thread ID
    while (nodo_corrente != NULL)
    {
        if (pthread_equal(nodo_corrente->tid_giocatore, tid))
        {
            pthread_mutex_unlock(&mutex_giocatori);  // UNLOCK prima di ritornare
            return nodo_corrente;
        }

        nodo_corrente = nodo_corrente->next_node;
    }
    
    pthread_mutex_unlock(&mutex_giocatori);  // UNLOCK
    // Thread ID non trovato (situazione improbabile in condizioni normali)
    return NULL;
}

void rimuovi_giocatore_lista(struct nodo_giocatore *nodo)
{
    pthread_mutex_lock(&mutex_giocatori);
    
    // === CASO 1: Il nodo da eliminare è la testa della lista ===
    if (nodo != NULL && testa_giocatori == nodo)
    {
        testa_giocatori = testa_giocatori->next_node;
        
        // Distruzione delle primitive di sincronizzazione
        pthread_mutex_destroy(&(nodo->stato_mutex));
        pthread_cond_destroy(&(nodo->stato_cv));
        
        free(nodo);
    }
    // === CASO 2: Il nodo da eliminare è nel mezzo o in coda alla lista ===
    else if (nodo != NULL)
    {
        struct nodo_giocatore *nodo_corrente = testa_giocatori;
        
        // Ricerca del nodo precedente a quello da eliminare
        while (nodo_corrente != NULL && nodo_corrente->next_node != nodo)
        {
            nodo_corrente = nodo_corrente->next_node;
        }
        
        // Rimozione del nodo dalla lista (bypass del collegamento)
        if (nodo_corrente != NULL)
        {
            nodo_corrente->next_node = nodo->next_node;
            
            // Distruzione delle primitive di sincronizzazione
            pthread_mutex_destroy(&(nodo->stato_mutex));
            pthread_cond_destroy(&(nodo->stato_cv));
            
            free(nodo);
        }
    }
    
    pthread_mutex_unlock(&mutex_giocatori);
}

void notifica_aggiornamento_partite()
{
    const pthread_t tid_thread_corrente = pthread_self();
    pthread_t tid_destinatario = 0;

    // Acquisizione del lock per garantire consistenza durante la scansione
    pthread_mutex_lock(&mutex_giocatori);
    struct nodo_giocatore *nodo_corrente = testa_giocatori;
    pthread_mutex_unlock(&mutex_giocatori);

    // Scansione di tutti i giocatori per notificare quelli in lobby
    while (nodo_corrente != NULL)
    {
        tid_destinatario = nodo_corrente->tid_giocatore;
        
        // Invia SIGUSR1 solo ai giocatori in lobby, escludendo il thread corrente
        // (evita di inviare il segnale a se stesso)
        if (tid_destinatario != tid_thread_corrente && nodo_corrente->stato == IN_LOBBY)
        {
            pthread_kill(tid_destinatario, SIGUSR1);
        }
        
        nodo_corrente = nodo_corrente->next_node;
    }
}

void notifica_fine_partita(const char *nome_giocatore)
{
    if (nome_giocatore == NULL) return;

    char messaggio[MAXOUT];
    sprintf(messaggio, "\n🏁 %s ha terminato la partita ed è tornato in lobby!\n\n", nome_giocatore);

    // Invia il messaggio a tutti i giocatori in lobby
    pthread_mutex_lock(&mutex_giocatori);
    struct nodo_giocatore *nodo_corrente = testa_giocatori;
    
    while (nodo_corrente != NULL)
    {
        // Invia solo ai giocatori in lobby (escludendo chi ha appena finito che non è ancora in lobby)
        if (nodo_corrente->stato == IN_LOBBY && 
            strcmp(nodo_corrente->nome, nome_giocatore) != 0)
        {
            send(nodo_corrente->sd_giocatore, messaggio, strlen(messaggio), MSG_NOSIGNAL);
        }
        nodo_corrente = nodo_corrente->next_node;
    }
    
    pthread_mutex_unlock(&mutex_giocatori);
}

void invia_lista_partite_disponibili()
{
    // Recupero del giocatore corrente tramite il thread ID
    struct nodo_giocatore *giocatore_corrente = cerca_giocatore_per_thread(pthread_self());
    const int sd_client = giocatore_corrente->sd_giocatore;

    // Acquisizione del lock per accesso thread-safe alla lista partite
    pthread_mutex_lock(&mutex_partite);
    struct nodo_partita *nodo_corrente = testa_partite;

    char buffer_partita[MAXOUT];
    memset(buffer_partita, 0, MAXOUT);

    char str_indice[3];
    memset(str_indice, 0, 3);

    char descrizione_stato[28];

    // === CASO 1: Nessuna partita disponibile ===
    if (nodo_corrente == NULL)
    {
        if (send(sd_client, "\nNon ci sono partite attive al momento, scrivi \"crea\" per crearne una o \"esci\" per uscire\n", 91, MSG_NOSIGNAL) < 0)
            gestisci_errore_connessione(sd_client);
    }
    // === CASO 2: Invio lista partite disponibili ===
    else
    {
        if (send(sd_client, "\n\nLISTA PARTITE", 15, MSG_NOSIGNAL) < 0)
            gestisci_errore_connessione(sd_client);
        
        // Scansione e invio di tutte le partite
        while (nodo_corrente != NULL)
        {
            memset(buffer_partita, 0, MAXOUT);
            memset(descrizione_stato, 0, 28);
            
            // Determinazione descrizione dello stato della partita
            switch (nodo_corrente->stato)
            {
                case NUOVA_CREAZIONE:
                    strcpy(descrizione_stato, "Creata da poco\n");
                    break;
                case IN_ATTESA:
                    strcpy(descrizione_stato, "In attesa di un giocatore\n");
                    break;
                case IN_CORSO:
                    strcpy(descrizione_stato, "In corso\n");
                    break;
                case TERMINATA:
                    strcpy(descrizione_stato, "Terminata\n");
                    break;
            }
            
            // Costruzione messaggio informativo sulla partita
            strcat(buffer_partita, "\nPartita di ");
            strcat(buffer_partita, nodo_corrente->proprietario);
            strcat(buffer_partita, "\nAvversario: ");
            strcat(buffer_partita, nodo_corrente->avversario);
            strcat(buffer_partita, "\nStato: ");
            strcat(buffer_partita, descrizione_stato);

            // Aggiunta ID solo per partite disponibili (in attesa o appena create)
            if (nodo_corrente->stato == NUOVA_CREAZIONE || nodo_corrente->stato == IN_ATTESA)
            {
                sprintf(str_indice, "%u\n", nodo_corrente->id);
                strcat(buffer_partita, "ID: "); 
                strcat(buffer_partita, str_indice);
            }
            
            // Invio informazioni della partita al client
            if (send(sd_client, buffer_partita, strlen(buffer_partita), MSG_NOSIGNAL) < 0)
                gestisci_errore_connessione(sd_client);

            nodo_corrente = nodo_corrente->next_node;
        }
        
        // Invio istruzioni per il giocatore
        if (send(sd_client, "\nUnisciti a una partita in attesa scrivendo il relativo ID, scrivi \"crea\" per crearne una o \"esci\" per uscire\n", 111, MSG_NOSIGNAL) < 0)
            gestisci_errore_connessione(sd_client);
    }
    
    pthread_mutex_unlock(&mutex_partite);
}

void notifica_giocatore_connesso()
{
    pthread_mutex_lock(&mutex_giocatori);
    struct nodo_giocatore *ultimo_giocatore = testa_giocatori;
    pthread_mutex_unlock(&mutex_giocatori);

    if (ultimo_giocatore == NULL) return;

    // Attendi che il nuovo giocatore completi la prima recv
    sleep(1);

    char messaggio[MAXOUT];
    sprintf(messaggio, "\n🔔 %s è appena entrato in lobby!\n\n", ultimo_giocatore->nome);

    const pthread_t tid_nuovo = ultimo_giocatore->tid_giocatore;

    // Invia il messaggio a tutti gli altri giocatori in lobby
    pthread_mutex_lock(&mutex_giocatori);
    struct nodo_giocatore *nodo_corrente = testa_giocatori;
    
    while (nodo_corrente != NULL)
    {
        // Invia a tutti tranne al nuovo giocatore E che siano effettivamente in lobby
        if (!pthread_equal(nodo_corrente->tid_giocatore, tid_nuovo) && 
            nodo_corrente->stato == IN_LOBBY)
        {
            send(nodo_corrente->sd_giocatore, messaggio, strlen(messaggio), MSG_NOSIGNAL);
        }
        nodo_corrente = nodo_corrente->next_node;
    }
    
    pthread_mutex_unlock(&mutex_giocatori);
}

void gestisci_errore_connessione(const int sd_giocatore)
{
    pthread_t tid_giocatore = 0;
    
    // Recupero informazioni del giocatore disconnesso
    struct nodo_giocatore *giocatore_disconnesso = cerca_giocatore_per_socket(sd_giocatore);
    if (giocatore_disconnesso != NULL)
        tid_giocatore = giocatore_disconnesso->tid_giocatore;
    
    // Verifica se il giocatore era in una partita
    struct nodo_partita *partita_attiva = cerca_partita_per_socket(sd_giocatore);

    if (partita_attiva != NULL)
    {
        // === GESTIONE DISCONNESSIONE DURANTE PARTITA IN CORSO ===
        if (partita_attiva->stato == IN_CORSO)
        {
            // Le send non hanno error checking per evitare ricorsione infinita
            
            if (sd_giocatore == partita_attiva->sd_proprietario)
            {
                // Il proprietario si è disconnesso: l'avversario vince automaticamente
                send(partita_attiva->sd_avversario, &ERROR_CONN, 1, MSG_NOSIGNAL);
                
                struct nodo_giocatore *avversario = cerca_giocatore_per_socket(partita_attiva->sd_avversario);
                if (avversario != NULL)
                {
                    // Aggiornamento statistiche avversario
                    avversario->vittorie++;
                    avversario->campione = true;
                    avversario->stato = IN_RICHIESTA;
                    
                    // Sblocco del thread avversario
                    pthread_cond_signal(&(avversario->stato_cv));
                }
                
                // Rimozione della partita dalla lista
                rimuovi_partita_lista(partita_attiva);
            }
            else
            {
                // L'avversario si è disconnesso: notifica al proprietario
                send(partita_attiva->sd_proprietario, &ERROR_CONN, 1, MSG_NOSIGNAL);
            }
        }
        else
        {
            // === PARTITA NON IN CORSO: rimozione semplice ===
            rimuovi_partita_lista(partita_attiva);
        }
    }
    
    // Invio segnale SIGALRM al thread per terminazione
    if (giocatore_disconnesso != NULL)
        pthread_kill(tid_giocatore, SIGALRM);
}

void gestisci_timeout_connessione()
{
    // Recupero del giocatore corrente tramite thread ID
    struct nodo_giocatore *giocatore_corrente = cerca_giocatore_per_thread(pthread_self());
    
    // Chiusura della socket e rimozione del giocatore dalla lista
    close(giocatore_corrente->sd_giocatore);
    rimuovi_giocatore_lista(giocatore_corrente);
    
    // Terminazione del thread
    pthread_exit(NULL);
}

void gestisci_chiusura_server()
{
    // Gestione del segnale SIGTERM inviato da Docker durante lo stop del container
    // Termina il processo server in modo pulito
    exit(EXIT_SUCCESS);
}