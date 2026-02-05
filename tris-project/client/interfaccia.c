#include "interfaccia.h"
#include <stdio.h>
#include <stdlib.h>

void mostra_griglia() {
    printf("\033[2J\033[H"); // pulisce schermo

    printf("\n╔═══════════════════════════╗\n");
    printf("║   MAPPA DELLE CASELLE     ║\n");
    printf("╚═══════════════════════════╝\n\n");
    printf("  Le caselle sono numerate così:\n\n");
    printf("     | 1 | 2 | 3 |\n");
    printf("     | 4 | 5 | 6 |\n");
    printf("     | 7 | 8 | 9 |\n\n");
    printf("─────────────────────────────\n\n");

    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            char cell = griglia[r][c] ? griglia[r][c] : ' ';
            printf("| %c ", cell);
        }
        printf("|\n");
    }
    printf("\n");
}

void aggiorna_griglia(unsigned short mossa, char simbolo) {
    int idx = mossa - 1;
    griglia[idx/3][idx%3] = simbolo;
}

bool mossa_valida(int mossa) {
    if (mossa < 0 || mossa > 9) return false;
    if (mossa == 0) return true;
    int idx = mossa - 1;
    return griglia[idx/3][idx%3] == 0;
}

char verifica_esito(const unsigned short *n_giocate) {
    char e = ESITO_NONE;
    for (int i = 0; i < 3; i++) {
        if (griglia[i][0] && griglia[i][0] == griglia[i][1] && griglia[i][1] == griglia[i][2])
            e = (griglia[i][0] == 'O') ? ESITO_VITTORIA : ESITO_SCONFITTA;
        if (griglia[0][i] && griglia[0][i] == griglia[1][i] && griglia[1][i] == griglia[2][i])
            e = (griglia[0][i] == 'O') ? ESITO_VITTORIA : ESITO_SCONFITTA;
    }
    if (griglia[0][0] && griglia[0][0] == griglia[1][1] && griglia[1][1] == griglia[2][2])
        e = (griglia[0][0] == 'O') ? ESITO_VITTORIA : ESITO_SCONFITTA;
    if (griglia[0][2] && griglia[0][2] == griglia[1][1] && griglia[1][1] == griglia[2][0])
        e = (griglia[0][2] == 'O') ? ESITO_VITTORIA : ESITO_SCONFITTA;

    if (e == ESITO_NONE && *n_giocate == 9) e = ESITO_PAREGGIO;
    return e;
}
