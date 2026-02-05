#ifndef INTERFACCIA_H
#define INTERFACCIA_H

#include "strutture.h"

void mostra_griglia();
void aggiorna_griglia(unsigned short mossa, char simbolo);
bool mossa_valida(int mossa);
char verifica_esito(const unsigned short *n_giocate);

#endif
