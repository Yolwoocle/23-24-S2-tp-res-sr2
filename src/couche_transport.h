#ifndef __COUCHE_TRANSPORT_H__
#define __COUCHE_TRANSPORT_H__

#include <stdint.h> /* uint8_t */
#include <stdbool.h>
#define MAX_INFO 96

/*************************
* Structure d'un paquet *
*************************/

typedef struct paquet_s {
    uint8_t type;         /* type de paquet, cf. ci-dessous */
    uint8_t num_seq;      /* numéro de séquence */
    uint8_t lg_info;      /* longueur du champ info */
    uint8_t somme_ctrl;   /* somme de contrôle */
    unsigned char info[MAX_INFO];  /* données utiles du paquet */
} paquet_t;

/******************
* Types de paquet *
******************/
#define DATA          1  /* données de l'application */
#define ACK           2  /* accusé de réception des données */
#define NACK          3  /* accusé de réception négatif */
#define CON_REQ       4  /* demande d'établissement de connexion */
#define CON_ACCEPT    5  /* acceptation de connexion */
#define CON_REFUSE    6  /* refus d'établissement de connexion */
#define CON_CLOSE     7  /* notification de déconnexion */
#define CON_CLOSE_ACK 8  /* accusé de réception de la déconnexion */
#define OTHER         9  /* extensions */


/* Capacite de numerotation */
#define SEQ_NUM_SIZE 16

/* Timeout */
#define DUREE_TIMEOUT 200

#define CAPACITE_NUMEROTATION 16

/* ************************************** */
/* Fonctions utilitaires couche transport */
/* ************************************** */

int generer_somme_ctrl_xor(paquet_t paquet);
paquet_t creer_paquet_vide();
bool verifier_controle(paquet_t * paquet);
void copier_info_paquet_dans_message(paquet_t* paquet, unsigned char* message);
void afficher_fenetre(int borne_inf, int curseur, int taille_fenetre, int * tab_ack);

/*--------------------------------------*
* Fonction d'inclusion dans la fenetre *
*--------------------------------------*/
int dans_fenetre(unsigned int inf, unsigned int pointeur, int taille);

#endif
