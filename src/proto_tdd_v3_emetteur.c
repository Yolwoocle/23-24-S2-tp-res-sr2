/*************************************************************
* proto_tdd_v0 -  émetteur                                   *
* TRANSFERT DE DONNEES  v3                                   *
*                                                            *
* Protocole sans contrôle de flux, sans reprise sur erreurs  *
*                                                            *
* E. Lavinal - Univ. de Toulouse III - Paul Sabatier         *
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"

/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char* argv[])
{
    int taille_fenetre = 7;
    if (argc == 2) {
        taille_fenetre = atoi(argv[1]);
    }
    int curseur = 0;
    int borne_inf = 0;

    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg; /* taille du message */
    paquet_t paquet_envoi = creer_paquet_vide(); /* paquet utilisé par le protocole */
    paquet_t paquet_recu = creer_paquet_vide(); /* paquet utilisé par le protocole */
    paquet_t tab_paquet[CAPACITE_NUMEROTATION]; 

    int evenement_temporisateur = 0;

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* lecture de donnees provenant de la couche application */
    de_application(message, &taille_msg);

    /* tant que l'émetteur a des données à envoyer */
    while ( taille_msg != 0 || borne_inf != curseur ) {
        if (dans_fenetre(borne_inf, curseur, taille_fenetre) && taille_msg > 0) {
            /* Si on a toujours des paquets qui peuvent êtres envoyés dans la fenêtre... */
            /* construction paquet */
            paquet_envoi = creer_paquet_vide();
            for (int i=0; i<taille_msg; i++) {
                paquet_envoi.info[i] = message[i];
            }
            paquet_envoi.type = DATA;
            paquet_envoi.lg_info = taille_msg;
            paquet_envoi.num_seq = curseur;
            paquet_envoi.somme_ctrl = generer_somme_ctrl_xor(paquet_envoi);
            
            /* remise à la couche reseau */
            printf("[TRP] (Numseq %d) Envoi de %d octets\n", paquet_envoi.num_seq, paquet_envoi.lg_info);
            vers_reseau(&paquet_envoi);
            tab_paquet[curseur] = paquet_envoi;

            /* On démarre le temporisateur */
            if (curseur == borne_inf) {
                depart_temporisateur(DUREE_TIMEOUT);
            }

            /* On incrémente le curseur */
            curseur = (curseur + 1) % CAPACITE_NUMEROTATION;

            /* On charge le prochain message à envoyer */
            de_application(message, &taille_msg);

        } else {
            /* Plus de crédit */
            evenement_temporisateur = attendre();
            if (evenement_temporisateur == PAQUET_RECU) {
                /* On reçoit un ACK dans la fenêtre de réception */
                de_reseau(&paquet_recu);
                if (verifier_controle(&paquet_recu) && dans_fenetre(borne_inf, paquet_recu.num_seq, taille_fenetre)) {
                    borne_inf = (paquet_recu.num_seq + 1) % CAPACITE_NUMEROTATION; 
                    if (borne_inf == curseur) {
                        arret_temporisateur();
                    }
                }

            } else {
                // Timeout : on rediffuse tous les paquets jusqu'au curseur
                depart_temporisateur(DUREE_TIMEOUT);
                int i = borne_inf;
                while (i != curseur) {
                    vers_reseau(&(tab_paquet[i]));
                    i = (i + 1) % CAPACITE_NUMEROTATION;
                }
            }
        }
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
