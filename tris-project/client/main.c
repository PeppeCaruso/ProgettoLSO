#include "connessione.h"
#include "gioco.h"
#include "strutture.h"

char griglia[3][3] = {{0}};
int socket_fd = 0;

const char NO_ERROR = '0';
const char ERROR_CONN = '1';

const char ESITO_NONE = '0';
const char ESITO_VITTORIA = '1';
const char ESITO_SCONFITTA = '2';
const char ESITO_PAREGGIO = '3';

int main() {
    signal(SIGUSR1, SIGUSR1_handler);
    signal(SIGTERM, SIGTERM_handler);

    connetti_al_server();
    loop_client();

    return 0;
}
