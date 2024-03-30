/*************************************************************
* proto_tdd_v0 -  récepteur                                  *
* TRANSFERT DE DONNEES  v1                                   *
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
            /* extraction des donnees du paquet recu */
            for (int i=0; i<paquet_recu.lg_info; i++) {
                message[i] = paquet_recu.info[i];
            }
            /* remise des données à la couche application */
            fin = vers_application(message, paquet_recu.lg_info);

            paquet_envoi.type = ACK;

            vers_reseau(&paquet_envoi);
        } else {
            /* Erreur détectée grâce à la somme de contrôle */
            paquet_envoi.type = NACK;
            vers_reseau(&paquet_envoi);
        }

    }

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
