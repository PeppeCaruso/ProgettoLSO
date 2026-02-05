#ifndef CONNESSIONE_H
#define CONNESSIONE_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include "strutture.h"

void connetti_al_server();
void gestisci_errore();
void SIGUSR1_handler();
void SIGTERM_handler();

#endif
