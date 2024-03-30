/*************************************************************
* proto_tdd_v0 -  récepteur                                  *
* TRANSFERT DE DONNEES  v4                                   *
*                                                            *
* Protocole sans contrôle de flux, sans reprise sur erreurs  *
*                                                            *
* E. Lavinal - Univ. de Toulouse III - Paul Sabatier         *
**************************************************************/

#include <stdio.h>
#include "application.h"
#include "couche_transport.h"
#include "services_reseau.h"

void envoyer_ack(int num_seq) {
    paquet_t paquet_envoi = creer_paquet_vide();
    paquet_envoi.type = ACK;
    paquet_envoi.num_seq = num_seq;
    paquet_envoi.somme_ctrl = generer_somme_ctrl_xor(paquet_envoi);
    vers_reseau(&paquet_envoi);
}

/* =============================== */
/* Programme principal - récepteur */
/* =============================== */
int main(int argc, char* argv[])
{
    
    unsigned char message[MAX_INFO]; /* message pour l'application */
    paquet_t paquet_recu = creer_paquet_vide(); /* paquet utilisé par le protocole */
    
    // Fenêtre de réception
    paquet_t tab_paquet[CAPACITE_NUMEROTATION]; 
    int tab_ack[CAPACITE_NUMEROTATION]; 
    for (int i = 0; i < CAPACITE_NUMEROTATION; i++) {
        tab_ack[i] = 0;
    }

    int taille_fenetre = 7; 
    int borne_inf = 0;

    int fin = 0; /* condition d'arrêt */
    int evenement_temporisateur = 0;

    init_reseau(RECEPTION);

    printf("[TRP] Initialisation reseau : OK.\n");
    printf("[TRP] Debut execution protocole transport.\n");

    /* tant que le récepteur reçoit des données */
    while ( !fin ) {
        de_reseau(&paquet_recu);

        if (verifier_controle(&paquet_recu)) {
            // On bufferise le paquet s'il est dans la fenêtre
            if (dans_fenetre(borne_inf, paquet_recu.num_seq, taille_fenetre)) {
                printf("[TRP] J'ai reçu numseq %d contenant %d octets, je le bufferise\n", paquet_recu.num_seq, paquet_recu.lg_info);
                tab_paquet[paquet_recu.num_seq] = paquet_recu;
                tab_ack[paquet_recu.num_seq] = 1;
            }

            printf("[TRP] Envoi d'un ACK de numseq %d\n", paquet_recu.num_seq);
            envoyer_ack(paquet_recu.num_seq);
        }

        // On transmet vers l'application les prochains messages bufferisés
        while (tab_ack[borne_inf] == 1) {
            tab_ack[borne_inf] = 0;

            paquet_t * p_paquet = &(tab_paquet[borne_inf]);
            printf("[TRP] Je transmet vers l'application numseq %d contenant %d octets\n", p_paquet->num_seq, p_paquet->lg_info);
            copier_info_paquet_dans_message(p_paquet, message);
            fin = vers_application(p_paquet->info, p_paquet->lg_info); 
            
            borne_inf = (borne_inf + 1) % CAPACITE_NUMEROTATION;
        }
    }

    // On retransmet à la fin si le dernier paquet n'est pas reçu, pour éviter que l'émetteur ne boucle à l'infini
    depart_temporisateur(1000);
    evenement_temporisateur = attendre(); 
    while ( evenement_temporisateur == PAQUET_RECU ) {
        arret_temporisateur();
        de_reseau(&paquet_recu);

        envoyer_ack(paquet_recu.num_seq);
        printf("[TRP] Renvoi du dernier paquet de ACK car c'est possible que l'émetteur ne l'ait pas reçu\n");

        depart_temporisateur(1000);
        evenement_temporisateur = attendre(); 
    }
    // arret_temporisateur();

    printf("[TRP] Fin execution protocole transport.\n");
    return 0;
}
