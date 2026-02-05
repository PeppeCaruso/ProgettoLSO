#ifndef CONNESSIONE_H
#define CONNESSIONE_H

#include "strutture.h"

// Avvia il socket server e ritorna il file descriptor
int crea_socket_server();

// Accetta un nuovo client e lo inserisce nella struttura Giocatore
void* gestisci_connessione_client(void *sd_client);

#endif
