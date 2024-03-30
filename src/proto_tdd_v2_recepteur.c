/*************************************************************
* proto_tdd_v0 -  récepteur                                  *
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
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[])
{
    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t paquet_recu = creer_paquet_vide(); /* paquet utilisé par le protocole */
    paquet_t paquet_envoi = creer_paquet_vide(); /* paquet utilisé par le protocole */

    int fin = 0; /* condition d'arrêt */
    int numseq = 0; /* Numéro de séquence du paquet */

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* tant que le récepteur reçoit des données */
    while ( !fin ) {

        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&paquet_recu);

        int somme_recue = paquet_recu.somme_ctrl;
        int somme_calculee = generer_somme_ctrl_xor(paquet_recu);
        
        paquet_envoi = creer_paquet_vide();
        if (somme_calculee == somme_recue) {
            if (paquet_recu.num_seq == numseq) {
                /* extraction des donnees du paquet recu */
                for (int i=0; i<paquet_recu.lg_info; i++) {
                    message[i] = paquet_recu.info[i];
                }

                /* remise des données à la couche application */
                fin = vers_application(message, paquet_recu.lg_info);
                numseq = (numseq + 1) % 2;
            }

            /* On acquitte le paquet */
            paquet_envoi.type = ACK;

        } else {
            // Somme de contrôle calculée incorrecte, on envoie un NACK
            paquet_envoi.type = NACK;

        }
        vers_reseau(&paquet_envoi);
    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
