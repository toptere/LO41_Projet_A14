#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define NbNiveaux 3					//Nombre de niveaux d'ateliers
#define NbAtelier1 1				//Nombre de processus symbolisant les ateliers 1
#define NbAtelier2 1				//Nombre de processus symbolisant les ateliers 2
#define NbAtelier3 1				//Nombre de processus symbolisant les ateliers 3
#define TAILLE_CONTAINER 10	//Taille d'un container
#define SPEED_ATELIER_1 2		//Duree d'attente lors de la production pour les ateliers de niveau 1
#define SPEED_ATELIER_2 1		//Duree d'attente lors de la production pour les ateliers de niveau 2
#define SPEED_ATELIER_3 1		//Duree d'attente lors de la production pour les ateliers de niveau 3


pthread_t tid[NbAtelier1+NbAtelier2+NbAtelier3+1];
pthread_mutex_t mutex0, mutex1, mutex2, mutex3;
pthread_cond_t atelier1_2, atelier2_3;

int *ressources;

typedef struct arguments_atelier {
		unsigned int numero;
		unsigned int niveau;
		unsigned int vitesse;
};

void mutex_lock(int niveau){
		switch (niveau)
	{
		case 0:
			pthread_mutex_lock(&mutex0);
			break;
		case 1:
			pthread_mutex_lock(&mutex1);
			break;
		case 2:
			pthread_mutex_lock(&mutex2);
			break;
		case 3:
			pthread_mutex_lock(&mutex3);
			break;
	}
}

void mutex_unlock(int niveau){
		switch (niveau)
	{
		case 0:
			pthread_mutex_unlock(&mutex0);
			break;
		case 1:
			pthread_mutex_unlock(&mutex1);
			break;
		case 2:
			pthread_mutex_unlock(&mutex2);
			break;
		case 3:
			pthread_mutex_unlock(&mutex3);
			break;
	}
}

void cond_signal(int niveau){
		switch (niveau)
	{
		case 1:
			pthread_cond_signal(&atelier1_2);
			break;
		case 2:
			pthread_cond_signal(&atelier2_3);
			break;
	}
}

void cond_wait(int niveau){
		switch (niveau)
	{
		case 2:
			pthread_cond_wait(&atelier1_2, &mutex2);
			break;
		case 3:
			pthread_cond_wait(&atelier2_3, &mutex3);
			break;
	}
}

void atelier(void *arguments){
	
	struct arguments_atelier *args = arguments;
	
	int ressource_case = 2*(args->numero)-1;
	
	printf("Demarrage atelier numero: %d de niveau: %d utilisant la ressource de case: %d\n", (int) args->numero, (int) args->niveau, (int) ressource_case);
	
	bool signal_envoye = false;		//Booleen marquant si un signal a deja ete envoye ou non pour eviter de noyer la console
	mutex_lock(args->niveau);
	while ( ressources[ressource_case] > 0 ){
		mutex_unlock(args->niveau);
		
		/* Verification stock input */
		if ( args->niveau != NbNiveaux){	//Si l'atelier est au dernier niveau il ne demande pas de ressources et les consomme jusqu'a epuisement
			mutex_lock(args->niveau);
			if ( ressources[ressource_case] <= TAILLE_CONTAINER ){
				mutex_unlock(args->niveau);
				cond_signal(args->niveau);
				if (signal_envoye == false){
					printf("L'atelier %d de niveau %d exige un autre container.\n", (int) args->numero, (int) args->niveau);
					signal_envoye = true;
				}
			}
			else{
				signal_envoye = false;
			}
			mutex_unlock(args->niveau);
		}
		
		/* Production */
		mutex_lock(args->niveau);
		if ( ressources[ressource_case+1] < TAILLE_CONTAINER ){
			mutex_unlock(args->niveau);
			sleep(args->vitesse);
			mutex_lock(args->niveau);
			ressources[ressource_case]--;
			ressources[ressource_case+1]++;
			printf("L'atelier %d de niveau %d produit. Input: %d Output: %d\n", (int) args->numero, (int) args->niveau, ressources[ressource_case], ressources[ressource_case+1]);
		}
		mutex_unlock(args->niveau);
		
		/* Expedition */
		mutex_lock(args->niveau);
		if ( ressources[ressource_case+1] >= TAILLE_CONTAINER ){
			
			if (args->niveau != 1 ){
				cond_wait(args->niveau); //Attente d'une demande de ressources de la part d'un atelier de niveau superieur
				ressources[ressource_case+1] -= TAILLE_CONTAINER;
				mutex_unlock(args->niveau);
				
				mutex_lock(args->niveau);
				// TEMPORARY WORKAROUND!!! ONLY WORKS FOR 1 WORKSPACE PER LEVEL!!!
				ressources[ressource_case-2] += TAILLE_CONTAINER;
				mutex_unlock(args->niveau);
			}
			else{	// Les ateliers de niveau 1 n'attendent pas d'ordre de niveaux superieurs et expedient immediatement
				ressources[ressource_case+1] -= TAILLE_CONTAINER;
				mutex_unlock(args->niveau);
				
				mutex_lock(args->niveau);
				ressources[0] += TAILLE_CONTAINER;
				mutex_unlock(args->niveau);
			}
			printf("L'atelier %d de niveau %d a expedie un container.\n", (int) args->numero, (int) args->niveau);
		}
		mutex_unlock(args->niveau);
	}
	mutex_unlock(args->niveau);
	printf("L'atelier %d de niveau %d a epuise son stock.\n", (int) args->numero, (int) args->niveau);
}




int main(int argc, char *argv[], char *arge[])
{
	
	/* Creation du tableau de ressources */
	
	//Allocation de la taille du tableau
	int taille_ressources;
	switch(argc)
	{
		case 1:
			//Par defaut, on cree 1 atelier par niveau
			taille_ressources = 7;
			break;
		default:
			//Il faut 1 case pour les produits finis et 2 cases par atelier
			// pour l'entree et la sortie sachant qu'il y a 3 niveaux
			taille_ressources = 2 + 6*atoi(argv[1]);
			break;
	}
	ressources = malloc(sizeof(int*)*taille_ressources);
	
	//Initialisation des ressources de depart
	//Chaque atelier commence avec 2 containers de ressources
	for ( int i = 1; i < taille_ressources ; i += 2)
		ressources[i] = 2*TAILLE_CONTAINER;
	
	printf("Ressources de depart:\n");
	for ( int i = 0; i < taille_ressources ; i++)
		printf("\tRessources[%d] = %d\n", (int)i, (int)ressources[i]);
	
	
	//creation des threads ateliers
	struct arguments_atelier args1;
	args1.numero = 1;
	args1.niveau = 1;
	args1.vitesse = SPEED_ATELIER_1;
	pthread_create(tid,0,(void *(*)())atelier, (void *)&args1);
	
	struct arguments_atelier args2;
	args2.numero = 2;
	args2.niveau = 2;
	args2.vitesse = SPEED_ATELIER_2;
	pthread_create(tid+1,0,(void *(*)())atelier, (void *)&args2);
	
	struct arguments_atelier args3;
	args3.numero = 3;
	args3.niveau = 3;
	args3.vitesse = SPEED_ATELIER_3;
	pthread_create(tid+2,0,(void *(*)())atelier, (void *)&args3);
	
	//attend la fin de toutes les threads ateliers
	pthread_join(tid[0],NULL);
	pthread_join(tid[1],NULL);
	pthread_join(tid[2],NULL);
	
	pthread_mutex_lock(&mutex1);
	printf("Quantite de produits obtenue: %d\n",ressources[0]);
	pthread_mutex_unlock(&mutex1);
	
	
	
	/* liberation des ressources");*/
	pthread_mutex_destroy(&mutex0);
	pthread_mutex_destroy(&mutex1);
	pthread_mutex_destroy(&mutex2);
	pthread_mutex_destroy(&mutex3);
	pthread_cond_destroy(&atelier1_2);
	pthread_cond_destroy(&atelier2_3);

	exit(0);
}
