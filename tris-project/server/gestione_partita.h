#ifndef GESTIONE_PARTITA_H
#define GESTIONE_PARTITA_H

#include "strutture.h"
#include <stdbool.h>

//----------GESTIONE LOBBY----------
void esegui_ciclo_lobby(struct nodo_giocatore *dati_giocatore);

bool conferma_ingresso_avversario(struct nodo_partita *partita, const int sd_avversario, const char *nome_avversario);

//----------GESTIONE PARTITA----------
void esegui_partita_tris(struct nodo_partita *dati_partita);

bool richiedi_nuova_partita(const int sd_proprietario, const int sd_avversario);

bool richiedi_uscita_lobby(const int client_sd);

//----------GESTIONE LISTA PARTITE----------
struct nodo_partita* crea_nuova_partita(const char *nome_proprietario, const int id_proprietario);

struct nodo_partita* cerca_partita_per_socket(const int sd);

struct nodo_partita* cerca_partita_per_id(const unsigned int indice);

void rimuovi_partita_lista(struct nodo_partita *nodo);

#endif
