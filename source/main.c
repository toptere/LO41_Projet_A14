#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NbAtelier1 1      //Nombre de processus symbolisant les ateliers 1
#define NbAtelier2 1      //Nombre de processus symbolisant les ateliers 2
#define NbAtelier3 1      //Nombre de processus symbolisant les ateliers 3
#define TAILLE_CONTAINER 10	//Taille d'un container

pthread_t tid[NbAtelier1+NbAtelier2+NbAtelier3+1];
pthread_mutex_t mutex;
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
int NbOutput2 = 20;	// Nombre de produits en stock prets a etre expedies de l'atelier 2

// Atelier 3
int NbInput3 = 20;	// Nombre de ressources en stock pour la production de l'atelier 3
int NbOutput3 = 0;	// Nombre de produits en stock prets a etre expedies de l'atelier 3



void atelier2(int i){
	printf("Demarrage atelier 2\n");
	pthread_mutex_lock(&mutex);
	while ( NbInput2 > 0 ){
		pthread_mutex_unlock(&mutex);
		
		/* Verification stock input */
		// Unecessary for now
		pthread_mutex_lock(&mutex);
		if ( NbInput2 == 0 ){
			printf("L'atelier %d de niveau 2 a epuise son stock.\n", (int) i);
		}
		pthread_mutex_unlock(&mutex);
		
		/* Production */
		pthread_mutex_lock(&mutex);
		if ( NbOutput2 < TAILLE_CONTAINER ){
			sleep(1);
			NbInput2--;
			NbOutput2++;
			printf("L'atelier %d de niveau 2 produit. Input: %d Output: %d\n", (int) i, NbInput2, NbOutput2);
		}
		pthread_mutex_unlock(&mutex);
		
		/* Expedition */
		pthread_mutex_lock(&mutex);
		if ( NbOutput2 >= TAILLE_CONTAINER ){
			pthread_cond_wait(&atelier1_2, &mutex);
			printf("L'atelier %d de niveau 2 expedie un container.\n", (int) i);
			NbOutput2 -= TAILLE_CONTAINER;
			NbInput1 += TAILLE_CONTAINER;
		}
		pthread_mutex_unlock(&mutex);
	}
	pthread_mutex_unlock(&mutex);
	printf("L'atelier %d de niveau 2 a epuise son stock.\n", (int) i);
}



void atelier1(int i) {
	printf("Demarrage atelier 1\n");
	pthread_mutex_lock(&mutex);
	while ( NbInput1 > 0 ){
		pthread_mutex_unlock(&mutex);


		/* Verification stock input */
		pthread_mutex_lock(&mutex);
		if ( NbInput1 <= TAILLE_CONTAINER ){
			printf("L'atelier %d de niveau 1 exige un autre container.\n", (int) i);
			pthread_cond_signal(&atelier1_2);
		}
		pthread_mutex_unlock(&mutex);
		
		/* Production */
		pthread_mutex_lock(&mutex);
		sleep(1);
		NbInput1--;
		NbOutput1++;
		pthread_mutex_unlock(&mutex);
		printf("L'atelier %d de niveau 1 produit. Input: %d Output: %d\n", (int) i, NbInput1, NbOutput1);
  	
  	/* Expedition */
  	pthread_mutex_lock(&mutex);
  		if ( NbOutput1 >= TAILLE_CONTAINER ){
  	  	printf("L'atelier %d de niveau 1 expedie un container.\n", (int) i);
  	    NbOutput1 -= TAILLE_CONTAINER;
  	    NbOutput += TAILLE_CONTAINER;
  	}
  	pthread_mutex_unlock(&mutex);
  	
		}
  printf("L'atelier %d de niveau 1 a epuise son stock.\n", (int) i);
	pthread_mutex_unlock(&mutex);
}



int main()
{
  //creation des threads ateliers
	
  pthread_create(tid,0,(void *(*)())atelier1,(void*)1);
  pthread_create(tid+1,0,(void *(*)())atelier2,(void*)2);
  //pthread_create(tid+2,0,(void *(*)())atelier3,(void*)3);

  //attend la fin de toutes les threads voitures
	pthread_join(tid[0],NULL);
	pthread_join(tid[1],NULL);
  
  pthread_mutex_lock(&mutex);
  printf("Quantite de produits obtenue: %d\n",NbOutput);
  pthread_mutex_unlock(&mutex);
  
  /* liberation des ressources");*/
  exit(0);
}
