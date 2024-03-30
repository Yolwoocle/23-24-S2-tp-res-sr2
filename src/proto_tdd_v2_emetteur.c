/*************************************************************
* proto_tdd_v0 -  émetteur                                   *
* TRANSFERT DE DONNEES  v2                                   *
*                                                            *
* Protocole sans contrôle de flux, sans reprise sur erreurs  *
*                                                            *
* E. Lavinal - Univ. de Toulouse III - Paul Sabatier         *
**************************************************************/

#include <stdio.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"

/* =============================== */
/* Programme principal - émetteur  */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message de l'application */
    int taille_msg; /* taille du message */
    paquet_t paquet_envoi = creer_paquet_vide(); /* paquet utilisé par le protocole */
    paquet_t paquet_recu = creer_paquet_vide(); /* paquet utilisé par le protocole */
    
    int numseq = 0; // Numéro de séquence
    int evenement_temporisateur = 0; // Événement du temporisateur

    init_reseau(EMISSION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* lecture de donnees provenant de la couche application */
    de_application(message, &taille_msg);

    /* tant que l'émetteur a des données à envoyer */
    while ( taille_msg != 0 ) {

        /* construction paquet */
        paquet_envoi = creer_paquet_vide();
        for (int i=0; i<taille_msg; i++) {
            paquet_envoi.info[i] = message[i];
        }
        paquet_envoi.lg_info = taille_msg;
        paquet_envoi.num_seq = numseq;
        paquet_envoi.type = DATA;
        paquet_envoi.somme_ctrl = generer_somme_ctrl_xor(paquet_envoi);

        do {
            do {
                // Envoi du paquet
                printf("[TRP] Envoi de %d octets\n", paquet_envoi.lg_info);
                vers_reseau(&paquet_envoi);
                
                // Attente d'une réponse
                depart_temporisateur(DUREE_TIMEOUT);
                evenement_temporisateur = attendre();
                if (evenement_temporisateur != -1) {
                    printf("[TRP] Le temporisateur n°%d a expiré, on retente l'envoi\n", evenement_temporisateur);
                }
                arret_temporisateur();

            } while (evenement_temporisateur != -1);

            // On vérifie que le paquet reçu est bien un ACK
            de_reseau(&paquet_recu);
            if (paquet_recu.type != ACK) {
                printf("[TRP] NACK reçu, on retente l'envoi\n");
            }
        } while (paquet_recu.type != ACK);
        
        numseq = (numseq + 1) % 2;

        /* lecture des donnees suivantes de la couche application */
        de_application(message, &taille_msg);
    }

    printf("[TRP] Fin execution protocole transfert de donnees (TDD).\n");
    return 0;
}
