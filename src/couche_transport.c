#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "couche_transport.h"
#include "services_reseau.h"
#include "application.h"

/* ************************************************************************** */
/* *************** Fonctions utilitaires couche transport ******************* */
/* ************************************************************************** */

// RAJOUTER VOS FONCTIONS DANS CE FICHIER...

/*
    Générer une somme de contrôle pour le paquet donné en paramètre.
*/
int generer_somme_ctrl_xor(paquet_t paquet) {
    int s = paquet.type ^ paquet.num_seq ^ paquet.lg_info;
    for (int i = 0; i < paquet.lg_info; i++) {
        s = s ^ paquet.info[i];
    }
    return s;
}

/*
    Construit un paquet vide pouvant être utilisé par la suite.
    Le champ info est rempli de '\0', et tous les autres champs sont initialisés à 0.
*/
paquet_t creer_paquet_vide() {
    /* construction paquet */
    paquet_t paquet;

    for (int i=0; i < MAX_INFO; i++) {
        paquet.info[i] = '\0';
    }
    paquet.lg_info = 0;
    paquet.num_seq = 0;
    paquet.type = 0;
    paquet.somme_ctrl = 0;

    return paquet;
}

/*
    Vérifie si la somme de contrôle d'un paquet est cohérente.
*/
bool verifier_controle(paquet_t * paquet) {
    int somme_recue = paquet->somme_ctrl;
    int somme_calculee = generer_somme_ctrl_xor(*paquet);
    return (somme_recue == somme_calculee);
}

/*
    Copie le contenu de `paquet->info` vers la chaîne de caractères `message`.
*/
void copier_info_paquet_dans_message(paquet_t* paquet, unsigned char* message) {
    for (int i=0; i < paquet->lg_info; i++) {
        message[i] = paquet->info[i];
    }
}

/* Utilisé dans la V4 uniquement: affiche la fenêtre courante sous la forme:
  0   1   2   3   4   5   6 [ 7   8   9  10  11 (12) 13] 14  15
  0   0   0   0   0   0   0   0   1   1   1   0   0   0   0   0

    - Les [] sont les bornes de la fenêtre
    - Le nombre entre () est le curseur
    - Les lignes en bas sont le statut des paquets : acquittés ou non  
 */
void afficher_fenetre(int borne_inf, int curseur, int taille_fenetre, int * tab_ack) {
    for (int i = 0; i < CAPACITE_NUMEROTATION; i++) {
        if (dans_fenetre(borne_inf, i, taille_fenetre)) {
            printf("\033[4m");
        }
        if (i == borne_inf) {
            if (i == curseur)
                printf("[%2d)", i);
            else
                printf("[%2d ", i);
        } else if (i == (borne_inf + taille_fenetre - 1) % CAPACITE_NUMEROTATION) {
            if (i == curseur)
                printf("(%2d]", i);
            else
                printf(" %2d]", i);
        } else {
            if (i == curseur)
                printf("(%2d)", i);
            else
                printf(" %2d ", i);
        }
        if (dans_fenetre(borne_inf, i, taille_fenetre)) {
            printf("\033[0m");
        }
    }
    printf("\n");

    for (int i = 0; i < CAPACITE_NUMEROTATION; i++) {
        printf(" %2d ", tab_ack[i]);
    }
    printf("\n");
    printf("\n");
}

/* ===================== Fenêtre d'anticipation ============================= */

/*--------------------------------------*/
/* Fonction d'inclusion dans la fenetre */
/*--------------------------------------*/
int dans_fenetre(unsigned int inf, unsigned int pointeur, int taille) {

    unsigned int sup = (inf+taille-1) % SEQ_NUM_SIZE;

    return
        /* inf <= pointeur <= sup */
        ( inf <= sup && pointeur >= inf && pointeur <= sup ) ||
        /* sup < inf <= pointeur */
        ( sup < inf && pointeur >= inf) ||
        /* pointeur <= sup < inf */
        ( sup < inf && pointeur <= sup);
}
