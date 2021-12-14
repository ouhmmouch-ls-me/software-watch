#include "sys/alt_stdio.h"
#include "system.h"
#include <unistd.h>
#include <stdio.h>
#include "sys/alt_irq.h"


volatile int edge_capture;
volatile int * timer_ptr = (int *) TIMER_0_BASE;
volatile int * hex_ptr   = (int *) HEX_BASE;
volatile int * key_ptr = (int *) KEY_BASE;


unsigned char seven_seg_table[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71};


void handle_key_interrupts(void* context, alt_u32 id);
void handle_timer_interrupts(void* context, alt_u32 id);
void init_timer();
void init_key();

void AFFICHER_HEX(int a,int b);

// AFFICHER_HEX fonction qui affiche la montre , chronometre , sur 4 les 7-segments.

int i = 0, h = 0, sec = 0, minc = 0, minm = 0;/* i compteur de secondes pour le chrono , j compteur de secondes pour la montre,

h Les heures affichées par la montre

minm Les minutes affichées par la montre,

sec Les secondes affichées par le chronometre,

minc les minutes affichées par le chronometre

*/
int j=0;
int counter=0; // compteur qui compte les minutes qu'on a ajouté que le mode réglage est activé.

int chrono; // chrono = 1 si le mode chrono est activé chrono = 0 si le mode montre est activé.par default le mode est mode montre.

int playc= 0; // playc=1 quand le chronometre est declenché , quand le chronometre est en pause plyac=0

int presskey2 = 0; /*(utilisable seulement en mode montre pour activé le mode réglage )

presskey=1 quand on a appuyé sur KEY2 pour la premier fois,(pour régler les minutes de la montre)

presskey=2 quand on a appuyé sur KEY2 pour la deuxième  fois,(pour régler les heures de la montres)

presskey=0 si on appuye jamais sur key2 ou quand on a appuyé sur KEY2 pour la troisième fois pour sortir du mode réglage  */

int main()
{

	init_timer();//initialisation de timer

	init_key();// initialisation de keys

  while (1){
	 sec=i%60;
	 minc = (i / 60)%60;
	 minm = (j / 60)%60;
	 h = (j / 3600)%24;
	 if(minm==0&&counter<60&&counter!=0){
			 counter=60;
		 }
	 else if(counter%60==0&&counter!=0){
		 		j-=3600;
		 		counter=0;
		 				}
	 else if(chrono){
		 AFFICHER_HEX(minc,sec);
	 }else{
		 AFFICHER_HEX(h,minm);
	 }
  };

  return 0;
}


void AFFICHER_HEX(int a,int b){

	int dd = a / 10;
	int d = a % 10;
	int cc = b / 10;
	int c = b % 10;
	*hex_ptr = (seven_seg_table[dd] << 24)|(seven_seg_table[d] << 16)|(seven_seg_table[cc] << 8)|(seven_seg_table[c]);
	if(presskey2==1){// si le mode réglage des minutes est activé.
		*hex_ptr = (seven_seg_table[dd] << 24)|(seven_seg_table[d] << 16)|(0<< 8)|0;
		usleep(80000);
		*hex_ptr = (seven_seg_table[dd] << 24)|(seven_seg_table[d] << 16)|(seven_seg_table[cc] << 8)|(seven_seg_table[c]);
		usleep(80000);
		//les deux premiers 7-segments des minutes clignotent pour montrer qu'on est on mode reglage des minutes.
	}
	if(presskey2==2){// si le mode réglage des heures est activé.
		*hex_ptr = (0 << 24)|(0 << 16)|(seven_seg_table[cc] << 8)|(seven_seg_table[c]);
		usleep(80000);
		*hex_ptr = (seven_seg_table[dd] << 24)|(seven_seg_table[d] << 16)|(seven_seg_table[cc] << 8)|(seven_seg_table[c]);
		usleep(80000);
		//les deux derniers 7-segments des heures clignotent pour montrer qu'on est on mode reglage des heures.
		}
}

void handle_timer_interrupts(void* context, alt_u32 id){
	*(timer_ptr) = 0;
	if(playc)i++;
	j++;
	printf("montre %d:%d ; chrono %d:%d \n",(j/3600)%24,(j/60)%60,(i/60)%60,(i%60));// affichage de la montre et du chronometre sur le console pour la vérification.

}

void init_timer()
{
	    /* Recast the edge_capture pointer to match the alt_irq_register() function
	     * prototype. */
	    void* edge_capture_ptr = (void*) &edge_capture;
	    /* set the interval timer period for scrolling the HEX displays */
		*(timer_ptr + 1) = 0x7;	// STOP = 0, START = 1, CONT = 1, ITO = 1
	    alt_irq_register( TIMER_0_IRQ, edge_capture_ptr,handle_timer_interrupts );
}



void handle_key_interrupts(void* context, alt_u32 id){
	int key_s;
	key_s = *(key_ptr + 3);
	if(key_s&8){// si on a appyué sur KEY3.

		if(chrono){//en mode chronometre.
			i=0;	//reset de chronometre
			playc=0; //arreter le chronometre.
		}
		else{// en mode montre.
			if(presskey2==1){//si mode reglage des minutes est activé.
				counter=minm+1;
				j+=60;//ajouter une minute.

			}
			else if(presskey2==2){//si mode reglage des hueres est activé.
				j+=3600;// ajouter une heures.
			}
		}

	}
	else if (key_s & 4){// si on a appyué sur KEY2.
		if(!chrono){ // en mode montre.
			if(presskey2==2){
				presskey2=0;//sortir de mode reglage.
			}
			else if(presskey2==1){
				presskey2=2;//activer le mode reglage des heures.
				counter=0;
				}
			else{
				presskey2=1;//activer le mode reglage des minutes.
				counter=minm;
			}
				}
		else{// en mode chronometre
			if(playc==1){
			playc=0;  // si le chronometre a été declenché arreter le chronometre.
			}
			else{
				playc=1; //si le chronometre est en pause declencher le chronometre.
			}
          }
		}

	else if(key_s & 2){ //si on a appyué sur KEY1.
		chrono=!chrono;// changer le mode (mode chronometre/ mode montre)
		presskey2=0; // sortir de mode réglege si on a changer le mode entre (montre/chronometre)
	}
	* (key_ptr + 3) = 0;


}


void init_key()
{
	void* edge_capture_ptr = (void*) &edge_capture;
	* (key_ptr + 2) = 0xe;
	alt_irq_register( KEY_IRQ, edge_capture_ptr,handle_key_interrupts );
}
