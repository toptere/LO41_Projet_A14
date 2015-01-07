#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define NbNiveaux 3					//Nombre de niveaux d'atelierss
#define NbAtelier1 1      	//Nombre de processus symbolisant les ateliers 1
#define NbAtelier2 1      	//Nombre de processus symbolisant les ateliers 2
#define NbAtelier3 1      	//Nombre de processus symbolisant les ateliers 3
#define TAILLE_CONTAINER 10	//Taille d'un container
#define SPEED_ATELIER_1 2		//Duree d'attente lors de la production pour les ateliers de niveau 1
#define SPEED_ATELIER_2 1		//Duree d'attente lors de la production pour les ateliers de niveau 2
#define SPEED_ATELIER_3 1		//Duree d'attente lors de la production pour les ateliers de niveau 3


pthread_t tid[NbAtelier1+NbAtelier2+NbAtelier3+1];
pthread_mutex_t mutex1, mutex1_2, mutex2_3, mutex3;
pthread_cond_t atelier1_2, atelier2_3;


/* Initialisation */

// Ressources generiques
int NbInput = 50;		// Nombre de ressources du stock originel
int NbOutput = 0;		// Nombre de produits finis

// Atelier 1
int NbInput1 = 20;	// Nombre de ressources en stock pour la production de l'atelier 1
int NbOutput1 = 0;	// Nombre de produits en stock prets a etre expedies de l'atelier 1

// Atelier 2
int NbInput2 = 20;	// Nombre de ressources en stock pour la production de l'atelier 2
int NbOutput2 = 0;	// Nombre de produits en stock prets a etre expedies de l'atelier 2

// Atelier 3
int NbInput3 = 20;	// Nombre de ressources en stock pour la production de l'atelier 3
int NbOutput3 = 0;	// Nombre de produits en stock prets a etre expedies de l'atelier 3



void atelier3(int i){
	printf("Demarrage atelier 3\n");
	pthread_mutex_lock(&mutex3);
	while ( NbInput3 > 0 ){
		pthread_mutex_unlock(&mutex3);
		
		/* Verification stock input */
		// Unecessary for now
		pthread_mutex_lock(&mutex3);
		if ( NbInput3 == 0 ){
			printf("L'atelier %d de niveau 3 a epuise son stock.\n", (int) i);
		}
		pthread_mutex_unlock(&mutex3);
		
		/* Production */
		pthread_mutex_lock(&mutex3);
		if ( NbOutput3 < TAILLE_CONTAINER ){
			sleep(SPEED_ATELIER_3);
			NbInput3--;
			NbOutput3++;
			printf("L'atelier %d de niveau 3 produit. Input: %d Output: %d\n", (int) i, NbInput3, NbOutput3);
		}
		pthread_mutex_unlock(&mutex3);
		
		/* Expedition */
		pthread_mutex_lock(&mutex3);
		if ( NbOutput3 >= TAILLE_CONTAINER ){
			pthread_cond_wait(&atelier2_3, &mutex3);
			NbOutput3 -= TAILLE_CONTAINER;
			pthread_mutex_unlock(&mutex3);
			
			pthread_mutex_lock(&mutex2_3);
			NbInput2 += TAILLE_CONTAINER;
			pthread_mutex_unlock(&mutex2_3);
			printf("L'atelier %d de niveau 3 a expedie un container.\n", (int) i);
		}
		pthread_mutex_unlock(&mutex3);
	}
	pthread_mutex_unlock(&mutex3);
	printf("L'atelier %d de niveau 3 a epuise son stock.\n", (int) i);
}

void atelier2(int i){
	printf("Demarrage atelier 2\n");
	
	bool signal_sent = false;		//Booleen marquant si un signal a deja ete envoye ou non
	
	pthread_mutex_lock(&mutex2_3);
	while ( NbInput2 > 0 ){
		pthread_mutex_unlock(&mutex2_3);
		
		/* Verification stock input */
		pthread_mutex_lock(&mutex2_3);
		if ( NbInput2 <= TAILLE_CONTAINER ){
			if (signal_sent == false){
				pthread_mutex_unlock(&mutex2_3);
				printf("L'atelier %d de niveau 2 exige un autre container.\n", (int) i);
				pthread_cond_signal(&atelier2_3);
				signal_sent = true;
			}
		}
		else{
			signal_sent = false;
		}
		pthread_mutex_unlock(&mutex2_3);
		
		
		/* Production */
		pthread_mutex_lock(&mutex2_3);
		if ( NbOutput2 < TAILLE_CONTAINER ){
			pthread_mutex_unlock(&mutex2_3);
			sleep(SPEED_ATELIER_2);
			pthread_mutex_lock(&mutex2_3);
			NbInput2--;
			NbOutput2++;
			printf("L'atelier %d de niveau 2 produit. Input: %d Output: %d\n", (int) i, NbInput2, NbOutput2);
		}
		pthread_mutex_unlock(&mutex2_3);
		
		/* Expedition */
		pthread_mutex_lock(&mutex2_3);
		if ( NbOutput2 >= TAILLE_CONTAINER ){
			pthread_cond_wait(&atelier1_2, &mutex2_3);
			NbOutput2 -= TAILLE_CONTAINER;
			pthread_mutex_unlock(&mutex2_3);
			
			pthread_mutex_lock(&mutex1_2);
			NbInput1 += TAILLE_CONTAINER;
			pthread_mutex_unlock(&mutex1_2);
			printf("L'atelier %d de niveau 2 a expedie un container.\n", (int) i);
		}
		pthread_mutex_unlock(&mutex2_3);
	}
	pthread_mutex_unlock(&mutex2_3);
	printf("L'atelier %d de niveau 2 a epuise son stock.\n", (int) i);
}



void atelier1(int i) {
	printf("Demarrage atelier 1\n");
	
	bool signal_sent = false;		//Booleen marquant si un signal a deja ete envoye ou non
	
	pthread_mutex_lock(&mutex1_2);
	while ( NbInput1 > 0 ){
		pthread_mutex_unlock(&mutex1_2);
		
		
		/* Verification stock input */
		pthread_mutex_lock(&mutex1_2);
		if ( NbInput1 <= TAILLE_CONTAINER ){
			if (signal_sent == false){
				pthread_mutex_unlock(&mutex1_2);
				printf("L'atelier %d de niveau 1 exige un autre container.\n", (int) i);
				pthread_cond_signal(&atelier1_2);
				signal_sent = true;
			}
		}
		else{
			signal_sent = false;
		}
		pthread_mutex_unlock(&mutex1_2);
		
		/* Production */
		sleep(SPEED_ATELIER_1);
		pthread_mutex_lock(&mutex1_2);
		NbInput1--;
		NbOutput1++;
		pthread_mutex_unlock(&mutex1_2);
		printf("L'atelier %d de niveau 1 produit. Input: %d Output: %d\n", (int) i, NbInput1, NbOutput1);
  	
  	/* Expedition */
  	pthread_mutex_lock(&mutex1_2);
  		if ( NbOutput1 >= TAILLE_CONTAINER ){
  	  	printf("L'atelier %d de niveau 1 expedie un container.\n", (int) i);
  	    NbOutput1 -= TAILLE_CONTAINER;
				pthread_mutex_unlock(&mutex1_2);
				
				pthread_mutex_lock(&mutex1);
  	    NbOutput += TAILLE_CONTAINER;
				pthread_mutex_unlock(&mutex1);
  	}
  	pthread_mutex_unlock(&mutex1_2);
  	
		}
  printf("L'atelier %d de niveau 1 a epuise son stock.\n", (int) i);
	pthread_mutex_unlock(&mutex1_2);
}



int main()
{
  //creation des threads ateliers
	
  pthread_create(tid,0,(void *(*)())atelier1,(void*)1);
  pthread_create(tid+1,0,(void *(*)())atelier2,(void*)2);
  pthread_create(tid+2,0,(void *(*)())atelier3,(void*)3);

  //attend la fin de toutes les threads voitures
	pthread_join(tid[0],NULL);
	pthread_join(tid[1],NULL);
  
  pthread_mutex_lock(&mutex1);
  printf("Quantite de produits obtenue: %d\n",NbOutput);
  pthread_mutex_unlock(&mutex1);
  
  /* liberation des ressources");*/
  exit(0);
}
