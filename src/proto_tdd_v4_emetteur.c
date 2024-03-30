/*************************************************************
* proto_tdd_v0 -  émetteur                                   *
* TRANSFERT DE DONNEES  v4                                   *
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

/*
    Envoie un paquet contenant `message`, de taille `taille_msg`, de numéro de séquence `numseq`
    Et le bufferise dans `tab_paquet`
*/
void envoyer_message(unsigned char * message, int taille_msg, int numseq, paquet_t * tab_paquet) {
    paquet_t paquet_envoi = creer_paquet_vide();
    for (int i=0; i<taille_msg; i++) {
        paquet_envoi.info[i] = message[i];
    }
    paquet_envoi.type = DATA;
    paquet_envoi.lg_info = taille_msg;
    paquet_envoi.num_seq = numseq;
    paquet_envoi.somme_ctrl = generer_somme_ctrl_xor(paquet_envoi);
    tab_paquet[numseq] = paquet_envoi;
    
    printf("[TRP] Envoi de %d octets (numseq %d)\n", taille_msg, numseq);
    vers_reseau(&paquet_envoi);
}

/*
    Trouve le prochain paquet non acquitté pouvant faire office de nouvelle borne inférieure pour 
    la fenêtre d'envoi.
*/
int trouver_prochaine_borne_inf(int * tab_ack, int borne_inf, int taille_fenetre) {
    for (int i = 0; i < taille_fenetre; i++) {
        int i_paquet = (borne_inf + i) % CAPACITE_NUMEROTATION;
        if (tab_ack[i_paquet] == 0) {
            return i_paquet;
        }
    }
    return (borne_inf + taille_fenetre) % CAPACITE_NUMEROTATION;
}

int main(int argc, char* argv[])
{
    int taille_fenetre = 7;
    int curseur = 0;
    int borne_inf = 0;
    int nouvelle_borne_inf; // Utilisé lors de la recherche d'une nouvelle borne inf

    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg; /* taille du message */
    paquet_t paquet_recu = creer_paquet_vide(); /* paquet utilisé par le protocole */
    paquet_t tab_paquet[CAPACITE_NUMEROTATION]; /* paquets bufferisés s'il y a besoin de les renvoyer */
    int tab_ack[CAPACITE_NUMEROTATION]; /* Utilisé pour marquer les packets acquittés */
    for (int i = 0; i < CAPACITE_NUMEROTATION; i++) { // On remplit ce tableau de 0
        tab_ack[i] = 0;
    }

    int evenement_temporisateur = 0;

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* lecture de donnees provenant de la couche application */
    de_application(message, &taille_msg);

    /* tant que l'émetteur a des données à envoyer */
    while ( taille_msg != 0 || borne_inf != curseur ) {
        afficher_fenetre(borne_inf, curseur, taille_fenetre, tab_ack);
        
        if (dans_fenetre(borne_inf, curseur, taille_fenetre) && taille_msg > 0) {
            // On envoie des paquets tant qu'on peut
            printf("[TRP] Envoi de %d octets\n", taille_msg);
            envoyer_message(message, taille_msg, curseur, tab_paquet);
            
            // On démarre un temporisateur pour chaque paquet
            depart_temporisateur_num(curseur, DUREE_TIMEOUT);
            curseur = (curseur + 1) % CAPACITE_NUMEROTATION;

            // On charge le prochain message de l'application
            de_application(message, &taille_msg);

        } else {
            // Plus de crédit
            evenement_temporisateur = attendre();
            if (evenement_temporisateur == PAQUET_RECU) {
                de_reseau(&paquet_recu);

                // Si on reçoit un paquet ACK valide dans la fenêtre
                if (verifier_controle(&paquet_recu) && dans_fenetre(borne_inf, paquet_recu.num_seq, taille_fenetre)) {
                    printf("[TRP] Réception d'un ACK pour le paquet %d\n", paquet_recu.num_seq);

                    // On stoppe le temporisateur et on marque le paquet comme acquitté 
                    arret_temporisateur_num(paquet_recu.num_seq);
                    tab_ack[paquet_recu.num_seq] = 1;
    
                    // On recherche une nouvelle borne inf du prochain paquet non ack
                    nouvelle_borne_inf = trouver_prochaine_borne_inf(tab_ack, borne_inf, taille_fenetre);

                    // On efface le statut des anciens paquets
                    int i_paquet = borne_inf;
                    while (i_paquet != nouvelle_borne_inf) {
                        tab_ack[i_paquet] = 0;
                        i_paquet = (i_paquet + 1) % CAPACITE_NUMEROTATION;
                    }
                    borne_inf = nouvelle_borne_inf;
                }

            } else {
                // Timeout d'un des temporisateurs 
                // On rediffuse le paquet concerné
                depart_temporisateur_num(evenement_temporisateur, DUREE_TIMEOUT);
                printf("[TRP] Timeout du paquet %d de %d octets, on le retransmet\n", evenement_temporisateur, tab_paquet[evenement_temporisateur].lg_info);
                vers_reseau(&(tab_paquet[evenement_temporisateur]));
            }
        }
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
