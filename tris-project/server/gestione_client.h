#ifndef GESTIONE_CLIENT_H
#define GESTIONE_CLIENT_H

#include "strutture.h"
#include <pthread.h>
#include <stdbool.h>

//----------REGISTRAZIONE----------
struct nodo_giocatore* aggiungi_nuovo_giocatore(const int client_sd);

char* valida_registrazione_giocatore(const int client_sd);

bool nome_giocatore_gia_usato(const char *nome_giocatore);

//----------GESTIONE LISTA GIOCATORI----------
struct nodo_giocatore* inserisci_giocatore_lista(const char *nome_giocatore, const int client_sd);

struct nodo_giocatore* cerca_giocatore_per_socket(const int sd);

struct nodo_giocatore* cerca_giocatore_per_thread(const pthread_t tid);

void rimuovi_giocatore_lista(struct nodo_giocatore *nodo);

//----------SIGNAL HANDLING----------
void notifica_aggiornamento_partite();

void notifica_fine_partita(const char *nome_giocatore);

void invia_lista_partite_disponibili();

void notifica_giocatore_connesso();

void gestisci_errore_connessione(const int sd_giocatore);

void gestisci_timeout_connessione();

void gestisci_chiusura_server();

#endif
