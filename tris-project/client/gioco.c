#include "gioco.h"
#include "connessione.h"
#include "interfaccia.h"

void loop_client() {
    char buffer[MAX_RICEZIONE] = {0};

    pthread_t tid_input;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&tid_input, &attr, gestione_input, NULL);

    for (int i = 5; i >= 0; i--)
    {
        sleep(1);
        printf("%d\n",i);
    }

    printf("TRIS-LSO\n");

    while (recv(socket_fd, buffer, MAX_RICEZIONE, 0) > 0) {
        if (strcmp(buffer, "*** Inizia la partita come proprietario ***\n") == 0) {
            pthread_kill(tid_input, SIGUSR1);
            avvia_partita(buffer, PROPRIETARIO);
            pthread_create(&tid_input, &attr, gestione_input, NULL);
        } else if (strcmp(buffer, "*** Inizia la partita come avversario ***\n") == 0) {
            pthread_kill(tid_input, SIGUSR1);
            avvia_partita(buffer, AVVERSARIO);
            pthread_create(&tid_input, &attr, gestione_input, NULL);
        } else {
            printf("%s", buffer);
        }
        memset(buffer, 0, MAX_RICEZIONE);
    }

    pthread_kill(tid_input, SIGUSR1);
    if (errno) gestisci_errore();
    else close(socket_fd);
}

void* gestione_input() {
    char msg[MAX_INPUT] = {0};

    while (1) {
        memset(msg, 0, MAX_INPUT);
        if (fgets(msg, MAX_INPUT, stdin) && msg[strlen(msg)-1] == '\n') {
            if (msg[0] != '\n')
                send(socket_fd, msg, strlen(msg)-1, 0);
            else
                printf("❗ Input non valido\n");
        } else {
            printf("⛔ Max 15 caratteri consentiti\n");
            int c; while ((c = getchar()) != '\n' && c != EOF);
        }
    }
    return NULL;
}

void avvia_partita(char *buffer, Ruolo ruolo) {
    printf("\n--- Nuova partita ---\n");
    printf(ruolo == PROPRIETARIO ? "Inizi tu!\n" : "Attendi il turno...\n");

    unsigned int round = 0;
    bool rivincita = false;

    do {
        memset(griglia, 0, 9);
        round++;
        unsigned short n_giocate = 0;
        char esito = ESITO_NONE, flag = NO_ERROR;

        do {
            if (((ruolo == PROPRIETARIO && round % 2 == 1) ||
                 (ruolo == AVVERSARIO && round % 2 == 0)) && n_giocate == 0) {
                mostra_griglia();
                printf("Tocca a te!\n");
                esito = turno_X(&n_giocate);
                printf("Turno avversario...\n");
            } else {
                if (recv(socket_fd, &flag, 1, 0) <= 0) gestisci_errore();
                if (flag == ERROR_CONN) { esito = ESITO_VITTORIA; break; }

                if ((esito = turno_O(&n_giocate)) != ESITO_NONE) break;
                printf("Tocca a te!\n");
                if ((esito = turno_X(&n_giocate)) != ESITO_NONE) break;
                printf("Turno avversario...\n");
            }
        } while (esito == ESITO_NONE);

        if (flag == ERROR_CONN) { printf("L’avversario si è disconnesso\n"); break; }
        if (esito != ESITO_PAREGGIO) break;

        rivincita = (ruolo == PROPRIETARIO) ? rivincita_host() : rivincita_guest();
    } while (rivincita);
}

char turno_X(unsigned short *n_giocate) {
    int c;
    char input[3] = {0};
    int mossa = 0;
    bool valida = false;
    char esito = ESITO_NONE;

    do {
        printf("Scrivi un numero da 1 a 9 (0 per arrenderti):\n");
        input[0] = getchar();

        if (input[0] != '\n') {
            mossa = atoi(input);
            valida = mossa_valida(mossa);
            while ((c = getchar()) != '\n' && c != EOF);
        }

        if (!valida) printf("Mossa non valida\n");
    } while (!valida);

    send(socket_fd, input, 1, 0);

    if (mossa != 0)
        aggiorna_griglia(mossa, 'O');

    mostra_griglia();

    if (mossa == 0) {
        printf("\n ═══════════════════════════ \n");
        printf("   Ti sei arreso, hai perso.\n");
        printf(" ═══════════════════════════ \n\n");
        send(socket_fd, &ESITO_SCONFITTA, 1, 0);
        return ESITO_SCONFITTA;
    }

    (*n_giocate)++;

    if (*n_giocate >= 5)
        esito = verifica_esito(n_giocate);

        if (esito == ESITO_VITTORIA) {
        printf("\n ═══════════════════════════ \n");
        printf("         HAI VINTO!\n");
        printf(" ═══════════════════════════ \n\n");
    } else if (esito == ESITO_PAREGGIO) {
        printf("\n ═══════════════════════════ \n");
        printf("          PAREGGIO!\n");
        printf(" ═══════════════════════════ \n\n");
    }

    send(socket_fd, &esito, 1, 0);
    return esito;
}

char turno_O(unsigned short *n_giocate) {
    char input[2] = {0};
    int mossa = 0;
    char esito = ESITO_NONE;

    if (recv(socket_fd, input, 1, 0) <= 0)
        gestisci_errore();

    mossa = atoi(input);

    if (mossa != 0)
        aggiorna_griglia(mossa, 'X');

    mostra_griglia();

    if (mossa == 0) {
        printf("\n ═══════════════════════════ \n");
        printf(" L'avversario si è arreso!\n");
        printf("         HAI VINTO!\n");
        printf(" ═══════════════════════════ \n\n");
        return ESITO_VITTORIA;
    }

    (*n_giocate)++;

    esito = verifica_esito(n_giocate);

    if (esito == ESITO_SCONFITTA) {
        printf("\n ═══════════════════════════ \n");
        printf("         HAI PERSO!\n");
        printf(" ═══════════════════════════ \n\n");
    } else if (esito == ESITO_PAREGGIO) {
        printf("\n ═══════════════════════════ \n");
        printf("          PAREGGIO!\n");
        printf(" ═══════════════════════════ \n\n");
    }

    return esito;
}

bool rivincita_host() {
    char buffer[256] = {0};
    char risposta;
    int c;
    bool valida = false;

    printf("L’avversario sta decidendo sulla rivincita...\n");

    if (recv(socket_fd, buffer, sizeof(buffer), 0) <= 0)
        gestisci_errore();

    printf("%s", buffer);

    if (strcmp(buffer, "Rivincita rifiutata dall'avversario\n") == 0)
        return false;

    do {
        risposta = getchar();
        if (risposta != '\n') {
            while ((c = getchar()) != '\n' && c != EOF);
            risposta = toupper(risposta);
            if (risposta == 'S' || risposta == 'N')
                valida = true;
            else
                printf("Risposta non valida. Usa S o N.\n");
        }
    } while (!valida);

    send(socket_fd, &risposta, 1, 0);

    if (risposta == 'N') {
        printf("Rivincita rifiutata.\n");
        return false;
    }

    memset(buffer, 0, sizeof(buffer));
    if (recv(socket_fd, buffer, sizeof(buffer), 0) <= 0)
        gestisci_errore();

    printf("%s", buffer);
    return true;
}

bool rivincita_guest() {
    char buffer[256] = {0};
    char risposta;
    int c;
    bool valida = false;

    if (recv(socket_fd, buffer, sizeof(buffer), 0) <= 0)
        gestisci_errore();

    printf("%s", buffer);

    do {
        risposta = getchar();
        if (risposta != '\n') {
            while ((c = getchar()) != '\n' && c != EOF);
            risposta = toupper(risposta);

            if (risposta == 'S' || risposta == 'N')
                valida = true;
            else
                printf("Risposta non valida. Usa S o N.\n");
        }
    } while (!valida);

    send(socket_fd, &risposta, 1, 0);

    if (risposta == 'N') {
        printf("Hai rifiutato la rivincita.\n");
        return false;
    }

    memset(buffer, 0, sizeof(buffer));
    if (recv(socket_fd, buffer, sizeof(buffer), 0) <= 0)
        gestisci_errore();
    printf("%s", buffer);

    memset(buffer, 0, sizeof(buffer));
    if (recv(socket_fd, buffer, sizeof(buffer), 0) <= 0)
        gestisci_errore();
    printf("%s", buffer);

    if (strcmp(buffer, "Rivincita rifiutata dal proprietario\n") == 0)
        return false;

    return true;
}

