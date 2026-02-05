#ifndef GIOCO_H
#define GIOCO_H

#include "strutture.h"

void loop_client();
void* gestione_input();
void avvia_partita(char *buffer, Ruolo ruolo);
char turno_X(unsigned short *n_giocate);
char turno_O(unsigned short *n_giocate);
bool rivincita_host();
bool rivincita_guest();

#endif
