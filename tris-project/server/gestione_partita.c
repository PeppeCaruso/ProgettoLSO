#include "gestione_partita.h"
#include "gestione_client.h"
#include "strutture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

void esegui_ciclo_lobby(struct nodo_giocatore *dati_giocatore)
{
    if (dati_giocatore == NULL)
    {
        return;
    }
    
    // === DICHIARAZIONI VARIABILI (AGGIUNGI QUESTE RIGHE!) ===
    char comando_ricevuto[MAXIN];
    char statistiche_giocatore[MAXOUT];
    const int sd_giocatore = dati_giocatore->sd_giocatore;
    bool client_connesso = true;
    
    do
    {
        // Inizializzazione buffer per nuovo ciclo di lobby
        memset(comando_ricevuto, 0, MAXIN);
        memset(statistiche_giocatore, 0, MAXOUT);
        dati_giocatore->stato = IN_LOBBY;

        // Conversione delle statistiche numeriche in stringhe
        char str_vittorie[3];
        sprintf(str_vittorie, "%u", dati_giocatore->vittorie);
        char str_sconfitte[3];
        sprintf(str_sconfitte, "%u", dati_giocatore->sconfitte);
        char str_pareggi[3];
        sprintf(str_pareggi, "%u", dati_giocatore->pareggi);

        // Costruzione del messaggio con le statistiche del giocatore
        statistiche_giocatore[0] = '\0';
        strcat(statistiche_giocatore, "\n+-------------+\n| ");
        strcat(statistiche_giocatore, dati_giocatore->nome);
        strcat(statistiche_giocatore, "\n+-------------+\n| vittorie: ");
        strcat(statistiche_giocatore, str_vittorie);
        strcat(statistiche_giocatore, "\n| sconfitte: ");
        strcat(statistiche_giocatore, str_sconfitte);
        strcat(statistiche_giocatore, "\n| pareggi: ");
        strcat(statistiche_giocatore, str_pareggi);
        strcat(statistiche_giocatore, "\n+-------------+\n");
        
        // Invio delle statistiche al client
        if (send(sd_giocatore, statistiche_giocatore, strlen(statistiche_giocatore), MSG_NOSIGNAL) < 0)
        {
            gestisci_errore_connessione(sd_giocatore);
        }

        // Invio della lista delle partite disponibili
        invia_lista_partite_disponibili();
        
        // Ciclo per gestire l'interruzione della recv da parte di segnali (EINTR)
        // Evita che il client crashe quando la recv viene interrotta
        do
        {
            if (errno == EINTR) errno = 0;
            if (recv(sd_giocatore, comando_ricevuto, MAXIN, 0) <= 0 && errno != EINTR)
                gestisci_errore_connessione(sd_giocatore);
        } while (errno == EINTR);
        
        // Conversione del comando in maiuscolo (case-insensitive)
        int i = 0;
        while (comando_ricevuto[i] != '\0')
        {
            comando_ricevuto[i] = toupper(comando_ricevuto[i]);
            i++;
        }
        
        // === GESTIONE COMANDI DEL GIOCATORE ===
        
        // Comando ESCI: disconnessione dalla lobby
        if (strcmp(comando_ricevuto, "ESCI") == 0)
        {
            client_connesso = false;
        }
        // Comando CREA: creazione di una nuova partita
        else if (strcmp(comando_ricevuto, "CREA") == 0) 
        {
            do
            {
                // Creazione della partita con il giocatore come proprietario
                struct nodo_partita *partita_creata = crea_nuova_partita(dati_giocatore->nome, sd_giocatore);
                
                if (partita_creata != NULL) 
                {
                    // Esecuzione della partita
                    esegui_partita_tris(partita_creata);
                    rimuovi_partita_lista(partita_creata);
                }
                else
                {
                    if (send(sd_giocatore, "Impossibile creare partita, attendi e riprova...\n", 50, MSG_NOSIGNAL) < 0)
                        gestisci_errore_connessione(sd_giocatore);
                }
                // Se il giocatore è campione (ha vinto), può scegliere se continuare o tornare in lobby
            } while (dati_giocatore->campione && !richiedi_uscita_lobby(sd_giocatore));
            
            // Ritorno alla lobby dopo la partita
            if (send(sd_giocatore, "Ritorno in lobby\n", 17, MSG_NOSIGNAL) < 0)
                gestisci_errore_connessione(sd_giocatore);
            
            // Notifica agli altri giocatori in lobby
            notifica_fine_partita(dati_giocatore->nome);
        }
        // Comando NUMERO: richiesta di unirsi a una partita esistente
        else 
        {
            // Conversione dell'indice partita da stringa a intero
            int indice_partita = atoi(comando_ricevuto);
            struct nodo_partita *partita_selezionata = NULL;
            
            if (indice_partita != 0)
                partita_selezionata = cerca_partita_per_id(indice_partita);

            if (partita_selezionata == NULL)
            {
                if (send(sd_giocatore, "Partita non trovata\n", 20, MSG_NOSIGNAL) < 0)
                    gestisci_errore_connessione(sd_giocatore);
            }
            else
            {
                // Invio richiesta di unione alla partita selezionata
                dati_giocatore->stato = IN_RICHIESTA;
                
                if (!conferma_ingresso_avversario(partita_selezionata, sd_giocatore, dati_giocatore->nome))
                {
                    // Richiesta rifiutata o partita già piena
                    if (partita_selezionata->richiesta_unione == false)
                    {
                        if (send(sd_giocatore, "Richiesta di unione rifiutata\n", 31, MSG_NOSIGNAL) < 0)
                            gestisci_errore_connessione(sd_giocatore);
                    }
                    else
                    {
                        if (send(sd_giocatore, "Un altro giocatore ha già richiesto di unirsi a questa partita\n", 64, MSG_NOSIGNAL) < 0)
                            gestisci_errore_connessione(sd_giocatore);
                    }
                }
                else // Richiesta accettata: il giocatore entra in partita
                {
                    dati_giocatore->stato = IN_PARTITA;
                    pthread_mutex_lock(&(dati_giocatore->stato_mutex));

                    // Attesa passiva fino alla fine della partita (segnalata dal proprietario)
                    while (dati_giocatore->stato == IN_PARTITA)
                    {
                        pthread_cond_wait(&(dati_giocatore->stato_cv), &(dati_giocatore->stato_mutex));
                    }
                    pthread_mutex_unlock(&(dati_giocatore->stato_mutex));

                    // Se il giocatore diventa campione (vince), crea una nuova partita come proprietario
                    while (dati_giocatore->campione && !richiedi_uscita_lobby(sd_giocatore))
                    {
                        struct nodo_partita *partita_campione = crea_nuova_partita(dati_giocatore->nome, sd_giocatore);
                        
                        if (partita_campione != NULL) 
                        {
                            esegui_partita_tris(partita_campione);
                            rimuovi_partita_lista(partita_campione);
                        }
                        else
                        {
                            if (send(sd_giocatore, "Impossibile creare partita, attendi e riprova...\n", 50, MSG_NOSIGNAL) < 0)
                                gestisci_errore_connessione(sd_giocatore);
                        }
                    }
                    
                    if (send(sd_giocatore, "Ritorno in lobby\n", 17, MSG_NOSIGNAL) < 0)
                        gestisci_errore_connessione(sd_giocatore);
                    
                    // Notifica agli altri giocatori in lobby
                    notifica_fine_partita(dati_giocatore->nome);
                }
            }
        }
    } while (client_connesso);
}

bool conferma_ingresso_avversario(struct nodo_partita *partita, const int sd_avversario, const char *nome_avversario)
{
    // Verifica se è già presente una richiesta in elaborazione per questa partita
    if (partita->richiesta_unione == true) 
        return false;
    
    const int sd_proprietario = partita->sd_proprietario;
    char messaggio_richiesta[MAXOUT];
    memset(messaggio_richiesta, 0, MAXOUT);
    
    // Costruzione del messaggio di richiesta per il proprietario
    strcat(messaggio_richiesta, nome_avversario);
    strcat(messaggio_richiesta, " vuole unirsi alla tua partita, accetti? [s/n]\n");

    char risposta_proprietario = '\0'; // La validazione dell'input è gestita lato client

    // Notifica all'avversario che la richiesta è in elaborazione
    if (send(sd_avversario, "In attesa del proprietario...\n", 30, MSG_NOSIGNAL) < 0)
        gestisci_errore_connessione(sd_avversario);
    
    // Segnala che una richiesta è in corso (evita richieste multiple simultanee)
    partita->richiesta_unione = true;

    // Invio della richiesta al proprietario della partita
    if (send(sd_proprietario, messaggio_richiesta, strlen(messaggio_richiesta), MSG_NOSIGNAL) < 0) 
    {
        gestisci_errore_connessione(sd_proprietario);
        return false;
    }
    
    // Ricezione della risposta dal proprietario
    if (recv(sd_proprietario, &risposta_proprietario, 1, 0) <= 0)
    {
        gestisci_errore_connessione(sd_proprietario);
        return false;
    }

    // Conversione a maiuscolo per gestione case-insensitive
    risposta_proprietario = toupper(risposta_proprietario);
    
    // === GESTIONE RISPOSTA DEL PROPRIETARIO ===
    
    if (risposta_proprietario == 'S') // Richiesta accettata
    {
        // Aggiunta dell'avversario alla partita
        strcpy(partita->avversario, nome_avversario);
        partita->sd_avversario = sd_avversario;
        
        // Notifica ai giocatori dell'inizio della partita
        if (send(sd_proprietario, "*** Inizia la partita come proprietario ***\n", 45, MSG_NOSIGNAL) < 0)
            gestisci_errore_connessione(sd_proprietario);
        if (send(sd_avversario, "*** Inizia la partita come avversario ***\n", 43, MSG_NOSIGNAL) < 0)
            gestisci_errore_connessione(sd_avversario);
        
        // Aggiornamento dello stato della partita e segnalazione al thread proprietario
        if (partita != NULL)
        {
            partita->stato = IN_CORSO;
            partita->richiesta_unione = false;
            
            // Sveglia il thread del proprietario in attesa sulla condition variable
            pthread_cond_signal(&(partita->stato_cv));
        }
        return true;
    }
    else // Richiesta rifiutata
    {
        if (send(sd_proprietario, "Richiesta rifiutata, in attesa di un avversario...\n", 52, MSG_NOSIGNAL) < 0)
            gestisci_errore_connessione(sd_proprietario);
        
        // Reset del flag di richiesta in corso
        partita->richiesta_unione = false;
    }
    
    return false;
}

void esegui_partita_tris(struct nodo_partita *dati_partita)
{
    const int sd_proprietario = dati_partita->sd_proprietario;
    struct nodo_giocatore *proprietario = cerca_giocatore_per_socket(sd_proprietario);
    
    // Inizializzazione stato proprietario
    proprietario->stato = IN_PARTITA;
    proprietario->campione = true;
    
    if (send(sd_proprietario, "In attesa di un avversario...\n", 30, MSG_NOSIGNAL) < 0)
        gestisci_errore_connessione(sd_proprietario);
    
    // Notifica agli altri giocatori della nuova partita disponibile
    notifica_aggiornamento_partite();

    // === ATTESA AVVERSARIO ===
    // Attesa con timeout periodico per verificare se il proprietario si disconnette
    pthread_mutex_lock(&(dati_partita->stato_mutex));
    while (dati_partita->stato != IN_CORSO)
    {
        // Timeout di 1 secondo per controlli periodici della connessione
        struct timespec timeout_attesa;
        clock_gettime(CLOCK_REALTIME, &timeout_attesa);
        timeout_attesa.tv_sec += 1;
        timeout_attesa.tv_nsec = 0;
        
        int risultato = pthread_cond_timedwait(&(dati_partita->stato_cv), &(dati_partita->stato_mutex), &timeout_attesa);
        
        if (risultato == ETIMEDOUT) 
        {
            // Invio messaggio nullo per verificare se la socket è ancora attiva
            if (send(sd_proprietario, "\0", 1, MSG_NOSIGNAL) < 0)
                gestisci_errore_connessione(sd_proprietario);
        }
    }
    pthread_mutex_unlock(&(dati_partita->stato_mutex));

    // Recupero dati avversario
    const int sd_avversario = dati_partita->sd_avversario;
    struct nodo_giocatore *avversario = cerca_giocatore_per_socket(sd_avversario);
    avversario->campione = false;

    unsigned int numero_round = 0;

    // === CICLO PARTITE (gestisce anche le rivincite in caso di pareggio) ===
    do
    {
        dati_partita->stato = IN_CORSO;
        numero_round++;
        notifica_aggiornamento_partite();

        bool errore_connessione = false; // Flag per errori di disconnessione
        char mossa_ricevuta = '\0';
        char esito_proprietario = ESITO_NONE; // Aggiornato dal client: '0'=in corso, '1'=vittoria, '2'=sconfitta, '3'=pareggio
        char esito_avversario = ESITO_NONE;

        // === CICLO TURNI DELLA PARTITA ===
        do
        {
            // Il server invia NO_ERROR per confermare che tutto procede correttamente
            // In caso di errore, l'error_handler invia ERROR_CONN all'altro giocatore
            
            if (numero_round % 2 != 0) // Round dispari: inizia il proprietario
            {
                // === TURNO PROPRIETARIO ===
                if (recv(sd_proprietario, &mossa_ricevuta, 1, 0) <= 0)
                    gestisci_errore_connessione(sd_proprietario);
                if (recv(sd_proprietario, &esito_proprietario, 1, 0) <= 0)
                    gestisci_errore_connessione(sd_proprietario);
                
                // Inoltro mossa all'avversario
                if (send(sd_avversario, &NO_ERROR, 1, MSG_NOSIGNAL) < 0)
                {
                    gestisci_errore_connessione(sd_avversario);
                    errore_connessione = true;
                    break;
                }
                if (send(sd_avversario, &mossa_ricevuta, 1, MSG_NOSIGNAL) < 0)
                {
                    gestisci_errore_connessione(sd_avversario);
                    errore_connessione = true;
                    break;
                }
                
                // Se la partita è finita, esce dal ciclo
                if (esito_proprietario != ESITO_NONE) break;

                // === TURNO AVVERSARIO ===
                if (recv(sd_avversario, &mossa_ricevuta, 1, 0) <= 0)
                {
                    gestisci_errore_connessione(sd_avversario);
                    errore_connessione = true;
                    break;
                }
                if (recv(sd_avversario, &esito_avversario, 1, 0) <= 0)
                {
                    gestisci_errore_connessione(sd_avversario);
                    errore_connessione = true;
                    break;
                }
                
                // Inoltro mossa al proprietario
                if (send(sd_proprietario, &NO_ERROR, 1, MSG_NOSIGNAL) < 0)
                    gestisci_errore_connessione(sd_proprietario);
                if (send(sd_proprietario, &mossa_ricevuta, 1, MSG_NOSIGNAL) < 0)
                    gestisci_errore_connessione(sd_proprietario);
            }
            else // Round pari: inizia l'avversario
            {
                // === TURNO AVVERSARIO ===
                if (recv(sd_avversario, &mossa_ricevuta, 1, 0) <= 0)
                {
                    gestisci_errore_connessione(sd_avversario);
                    errore_connessione = true;
                    break;
                }
                if (recv(sd_avversario, &esito_avversario, 1, 0) <= 0)
                {
                    gestisci_errore_connessione(sd_avversario);
                    errore_connessione = true;
                    break;
                }
                
                // Inoltro mossa al proprietario
                if (send(sd_proprietario, &NO_ERROR, 1, MSG_NOSIGNAL) < 0)
                    gestisci_errore_connessione(sd_proprietario);
                if (send(sd_proprietario, &mossa_ricevuta, 1, MSG_NOSIGNAL) < 0)
                    gestisci_errore_connessione(sd_proprietario);
                
                // Se la partita è finita, esce dal ciclo
                if (esito_avversario != ESITO_NONE) break;

                // === TURNO PROPRIETARIO ===
                if (recv(sd_proprietario, &mossa_ricevuta, 1, 0) <= 0)
                    gestisci_errore_connessione(sd_proprietario);
                if (recv(sd_proprietario, &esito_proprietario, 1, 0) <= 0)
                    gestisci_errore_connessione(sd_proprietario);
                
                // Inoltro mossa all'avversario
                if (send(sd_avversario, &NO_ERROR, 1, MSG_NOSIGNAL) < 0)
                {
                    gestisci_errore_connessione(sd_avversario);
                    errore_connessione = true;
                    break;
                }
                if (send(sd_avversario, &mossa_ricevuta, 1, MSG_NOSIGNAL) < 0)
                {
                    gestisci_errore_connessione(sd_avversario);
                    errore_connessione = true;
                    break;
                }
            }
        } while (esito_proprietario == ESITO_NONE && esito_avversario == ESITO_NONE);

        // === GESTIONE ERRORE DI CONNESSIONE ===
        // Se l'avversario si disconnette, il proprietario vince automaticamente
        if (errore_connessione) 
        {
            proprietario->vittorie++;
            proprietario->campione = true;
            break;
        }

        // === AGGIORNAMENTO STATISTICHE GIOCATORI ===
        
        if (esito_proprietario == ESITO_VITTORIA || esito_avversario == ESITO_SCONFITTA)
        {
            // Vittoria del proprietario
            proprietario->vittorie++;
            avversario->sconfitte++;
            proprietario->campione = true;
            avversario->campione = false;
            
            // Sblocca l'avversario che torna in lobby
            avversario->stato = IN_LOBBY;
            pthread_cond_signal(&(avversario->stato_cv));
            break;
        }
        else if (esito_proprietario == ESITO_SCONFITTA || esito_avversario == ESITO_VITTORIA)
        {
            // Vittoria dell'avversario
            proprietario->sconfitte++;
            avversario->vittorie++;
            proprietario->campione = false;
            avversario->campione = true;
            
            // L'avversario diventa il nuovo campione (può creare una nuova partita)
            avversario->stato = IN_RICHIESTA;
            pthread_cond_signal(&(avversario->stato_cv));
            break;
        }

        // Pareggio: aggiornamento contatori e possibilità di rivincita
        proprietario->pareggi++;
        avversario->pareggi++;

        dati_partita->stato = TERMINATA;
        notifica_aggiornamento_partite();

    // La partita continua solo se entrambi accettano la rivincita (in caso di pareggio)
    } while (richiedi_nuova_partita(sd_proprietario, sd_avversario));
}

bool richiedi_nuova_partita(const int sd_proprietario, const int sd_avversario)
{
    struct nodo_giocatore *avversario = cerca_giocatore_per_socket(sd_avversario);
    char risposta_avversario = '\0';
    char risposta_proprietario = '\0';

    // === RICHIESTA RIVINCITA ALL'AVVERSARIO ===
    if (send(sd_avversario, "Rivincita? [s/n]\n", 17, MSG_NOSIGNAL) < 0)
        gestisci_errore_connessione(sd_avversario);
    if (recv(sd_avversario, &risposta_avversario, 1, 0) <= 0)
        gestisci_errore_connessione(sd_avversario);
    
    // Conversione a maiuscolo per gestione case-insensitive
    risposta_avversario = toupper(risposta_avversario);
    
    if (risposta_avversario != 'S') 
    {
        // Avversario rifiuta la rivincita
        if (send(sd_proprietario, "Rivincita rifiutata dall'avversario\n", 37, MSG_NOSIGNAL) < 0)
            gestisci_errore_connessione(sd_proprietario);
    }
    else // Avversario accetta la rivincita
    {
        // === RICHIESTA CONFERMA AL PROPRIETARIO ===
        if (send(sd_avversario, "In attesa del proprietario...\n", 30, MSG_NOSIGNAL) < 0)
            gestisci_errore_connessione(sd_avversario);
        if (send(sd_proprietario, "L'avversario vuole la rivincita, accetti? [s/n]\n", 49, MSG_NOSIGNAL) < 0)
            gestisci_errore_connessione(sd_proprietario);
        if (recv(sd_proprietario, &risposta_proprietario, 1, 0) <= 0)
            gestisci_errore_connessione(sd_proprietario);
        
        // Conversione a maiuscolo per gestione case-insensitive
        risposta_proprietario = toupper(risposta_proprietario);
    }
    
    // === GESTIONE RISPOSTA FINALE ===
    
    if (risposta_proprietario != 'S') // Rivincita rifiutata: ritorno alla lobby
    {
        if (risposta_proprietario == 'N')
        {
            if (send(sd_avversario, "Rivincita rifiutata dal proprietario\n", 38, MSG_NOSIGNAL) < 0)
                gestisci_errore_connessione(sd_avversario);
        }
        
        // Sblocca il thread dell'avversario per tornare alla lobby
        if (avversario != NULL)
        {
            avversario->stato = IN_RICHIESTA;
            pthread_cond_signal(&(avversario->stato_cv));
        }
        return false;
    }
    else // Rivincita accettata: nuovo round
    {
        if (send(sd_avversario, "Rivincita accettata, pronti per il prossimo round\n", 51, MSG_NOSIGNAL) < 0)
            gestisci_errore_connessione(sd_avversario);
        if (send(sd_proprietario, "Rivincita accettata, pronti per il prossimo round\n", 51, MSG_NOSIGNAL) < 0)
            gestisci_errore_connessione(sd_proprietario);
        return true;
    }
}

bool richiedi_uscita_lobby(const int sd_client)
{
    char risposta_giocatore = '\0';
    
    // Richiesta al giocatore se vuole continuare a giocare o tornare in lobby
    if (send(sd_client, "Vuoi cercare un nuovo avversario? [s/n]\n", 40, MSG_NOSIGNAL) < 0)
        gestisci_errore_connessione(sd_client);
    if (recv(sd_client, &risposta_giocatore, 1, 0) <= 0)
        gestisci_errore_connessione(sd_client);
    
    // Conversione a maiuscolo per gestione case-insensitive
    risposta_giocatore = toupper(risposta_giocatore);
    
    // Ritorna true se il giocatore vuole uscire (risposta != 'S')
    // Ritorna false se il giocatore vuole continuare (risposta == 'S')
    if (risposta_giocatore == 'S') 
        return false; // Non vuole uscire, continua a cercare avversari
    else 
        return true;  // Vuole uscire e tornare in lobby
}

struct nodo_partita* crea_nuova_partita(const char *nome_proprietario, const int sd_proprietario)
{
    // Allocazione memoria per il nuovo nodo partita
    struct nodo_partita *nuovo_nodo = (struct nodo_partita *) malloc(sizeof(struct nodo_partita));
    if (nuovo_nodo == NULL)
        return NULL;

    // Inizializzazione del nodo partita
    memset(nuovo_nodo, 0, sizeof(struct nodo_partita));
    
    // AGGIUNGI QUESTO: Assegna ID univoco e incrementa il contatore
    pthread_mutex_lock(&mutex_partite);
    nuovo_nodo->id = prossimo_id_partita;
    prossimo_id_partita++;
    pthread_mutex_unlock(&mutex_partite);
    
    strcpy(nuovo_nodo->proprietario, nome_proprietario);
    nuovo_nodo->sd_proprietario = sd_proprietario;
    nuovo_nodo->stato = NUOVA_CREAZIONE;
    nuovo_nodo->richiesta_unione = false;
    
    // Inizializzazione mutex e condition variable
    pthread_mutex_init(&(nuovo_nodo->stato_mutex), NULL);
    pthread_cond_init(&(nuovo_nodo->stato_cv), NULL);

    // Inserimento in testa alla lista
    pthread_mutex_lock(&mutex_partite);

    nuovo_nodo->next_node = testa_partite;
    
    if (testa_partite != NULL && testa_partite->stato == NUOVA_CREAZIONE)
        testa_partite->stato = IN_ATTESA;

    testa_partite = nuovo_nodo;
    
    pthread_mutex_unlock(&mutex_partite);

    return nuovo_nodo;
}

struct nodo_partita* cerca_partita_per_socket(const int sd)
{
    struct nodo_partita *nodo_corrente = testa_partite;
    
    // Scansione della lista partite alla ricerca del socket descriptor
    // Verifica sia il proprietario che l'avversario
    while (nodo_corrente != NULL)
    {
        if (nodo_corrente->sd_proprietario == sd || nodo_corrente->sd_avversario == sd)
            return nodo_corrente;
        
        nodo_corrente = nodo_corrente->next_node;
    }
    
    // Socket descriptor non trovato in nessuna partita
    return NULL;
}

struct nodo_partita* cerca_partita_per_id(const unsigned int id_cercato)
{
    // Acquisizione del lock per accesso thread-safe
    pthread_mutex_lock(&mutex_partite);

    struct nodo_partita *nodo_corrente = testa_partite;
    
    // Scansione della lista cercando l'ID
    while (nodo_corrente != NULL)
    {
        // Cerca solo tra partite disponibili
        if ((nodo_corrente->stato == IN_ATTESA || nodo_corrente->stato == NUOVA_CREAZIONE) &&
            nodo_corrente->id == id_cercato)
        {
            pthread_mutex_unlock(&mutex_partite);
            return nodo_corrente;
        }
        
        nodo_corrente = nodo_corrente->next_node;
    }
    
    pthread_mutex_unlock(&mutex_partite);

    // ID non trovato
    return NULL;
}

void rimuovi_partita_lista(struct nodo_partita *nodo)
{
    pthread_mutex_lock(&mutex_partite);

    // === CASO 1: Il nodo da eliminare è la testa della lista ===
    if (nodo != NULL && testa_partite == nodo)
    {
        testa_partite = testa_partite->next_node;
        
        // Distruzione delle primitive di sincronizzazione
        pthread_mutex_destroy(&(nodo->stato_mutex));
        pthread_cond_destroy(&(nodo->stato_cv));
        
        free(nodo);
        
        // Notifica agli altri thread della modifica alla lista partite
        notifica_aggiornamento_partite();
    }
    // === CASO 2: Il nodo da eliminare è nel mezzo o in coda alla lista ===
    else if (nodo != NULL)
    {
        struct nodo_partita *nodo_corrente = testa_partite;
        
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
            
            // Notifica agli altri thread della modifica alla lista partite
            notifica_aggiornamento_partite();
        }
    }

    pthread_mutex_unlock(&mutex_partite);
}