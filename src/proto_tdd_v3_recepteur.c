/*************************************************************
* proto_tdd_v0 -  récepteur                                  *
* TRANSFERT DE DONNEES  v3                                   *
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
    int num_seq = 0;
    int evenement_temporisateur = 0;

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* tant que le récepteur reçoit des données */
    while ( !fin ) {
        // attendre(); /* optionnel ici car de_reseau() fct bloquante */
        de_reseau(&paquet_recu);

        if (verifier_controle(&paquet_recu)) {
            // On reçoit un paquet valide
            printf("[TRP] (numseq %d) paquet reçu de %d octets.\n", num_seq, paquet_recu.lg_info);

            // On transmet le contenu du paquet à l'application s'il est dans le bon ordre
            if (paquet_recu.num_seq == num_seq) {
                num_seq = (num_seq + 1) % CAPACITE_NUMEROTATION;

                copier_info_paquet_dans_message(&paquet_recu, message);

                fin = vers_application(paquet_recu.info, paquet_recu.lg_info);  
            } 

            // On envoie un ACK
            paquet_envoi = creer_paquet_vide();
            paquet_envoi.type = ACK;
            paquet_envoi.num_seq = (num_seq - 1) % CAPACITE_NUMEROTATION;
            paquet_envoi.somme_ctrl = generer_somme_ctrl_xor(paquet_envoi);
            printf("[TRP] Envoi d'un ACK de numseq %d\n", paquet_envoi.num_seq);
            vers_reseau(&paquet_envoi);
        }
    }

    // On retransmet les ACK à la fin si le dernier paquet n'est pas reçu, 
    // pour éviter que l'émetteur ne boucle à l'infini
    depart_temporisateur(1000);
    evenement_temporisateur = attendre(); 
    while ( evenement_temporisateur == PAQUET_RECU ) {
        arret_temporisateur();
        de_reseau(&paquet_recu);

        paquet_envoi = creer_paquet_vide();
        paquet_envoi.type = ACK;
        paquet_envoi.num_seq = (num_seq - 1) % CAPACITE_NUMEROTATION;
        paquet_envoi.somme_ctrl = generer_somme_ctrl_xor(paquet_envoi);
        printf("[TRP] Renvoi du dernier paquet car c'est possible que l'émetteur ne l'ait pas reçu\n");

        vers_reseau(&paquet_envoi);

        depart_temporisateur(1000);
        evenement_temporisateur = attendre(); 
    }
    arret_temporisateur();

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
