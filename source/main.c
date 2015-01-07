#define _XOPEN_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>

#define nombre_niveaux 3					// Nombre de niveaux d'ateliers
#define TAILLE_CONTAINER 10	// Taille d'un container
#define SPEED_ATELIER [3] = {3, 2, 1}	// Duree d'attente lors de la production pour les ateliers de niveau 1, 2 et 3

pthread_t *tid;
pthread_mutex_t mutex0, mutex1, mutex2, mutex3, mutex1_2, mutex2_3;
pthread_cond_t atelier1_2, atelier2_3;

int nombre_ateliers_niveau = 1;	// Par defaut il y a 1 thread donc 1 atelier par niveau
int *transfert_containers1_2, *transfert_containers2_3;		// Tableaux utilises pour transferer les containers entre niveaux
int *ressources;	// Tableau contenant toutes les ressources des ateliers

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

void imprimer_container(int niveau){
	switch (niveau)
	{
		case 2:
			printf("Transfert de container 2 -> 1: ");
			pthread_mutex_lock(&mutex1_2);
			for (int i = 0 ; i < nombre_ateliers_niveau ; i++)
				printf("%d ",(int)transfert_containers1_2[i]);
			printf("\n");
			pthread_mutex_unlock(&mutex1_2);
			break;
		case 3:
			printf("Transfert de container 3 -> 2: ");
			pthread_mutex_lock(&mutex2_3);
			for (int i = 0 ; i < nombre_ateliers_niveau ; i++)
				printf("%d ",(int)transfert_containers2_3[i]);
			printf("\n");
			pthread_mutex_unlock(&mutex2_3);
			break;
	}
}

bool recuperer_container(int numero, int niveau){
	switch (niveau)
	{
		case 1:
			pthread_mutex_lock(&mutex1_2);
			for (int i = 0 ; i < nombre_ateliers_niveau; i++){
				if (transfert_containers1_2[i] > 0){
					transfert_containers1_2[i]--;
					pthread_mutex_unlock(&mutex1_2);
					imprimer_container(niveau+1);
					return true;
				}
			}
			pthread_mutex_unlock(&mutex1_2);
			imprimer_container(niveau);
			break;
		case 2:
			pthread_mutex_lock(&mutex2_3);
			for (int i = 0 ; i < nombre_ateliers_niveau; i++){
				if (transfert_containers2_3[i] > 0){
					transfert_containers2_3[i]--;
					pthread_mutex_unlock(&mutex2_3);
					imprimer_container(niveau+1);
					return true;
				}
			}
			pthread_mutex_unlock(&mutex2_3);
			imprimer_container(niveau);
			break;
	}
	return false;
}

void transferer_container(int numero, int niveau){
	switch (niveau)
	{
		case 2:
			pthread_mutex_lock(&mutex1_2);
			transfert_containers1_2[numero-nombre_ateliers_niveau*(niveau-1)-1]++;
			pthread_mutex_unlock(&mutex1_2);
			imprimer_container(niveau);
			break;
		case 3:
			pthread_mutex_lock(&mutex2_3);
			transfert_containers2_3[numero-nombre_ateliers_niveau*(niveau-1)-1]++;
			pthread_mutex_unlock(&mutex2_3);
			imprimer_container(niveau);
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
		if ( args->niveau != nombre_niveaux){	//Si l'atelier est au dernier niveau il ne demande pas de ressources et les consomme jusqu'a epuisement
			mutex_lock(args->niveau);
			if ( ressources[ressource_case] <= TAILLE_CONTAINER ){	// L'atelier demande des ressources s'il entame son dernier container
				mutex_unlock(args->niveau);
				cond_signal(args->niveau);	// On renvoit le signal a chaque iteration au cas ou aucun atelier de niveau inferieur n'etait pret a expedier un container
				if (signal_envoye == false){
					printf("L'atelier %d de niveau %d exige un autre container.\n", (int) args->numero, (int) args->niveau);
					signal_envoye = true;
				}
				else if(recuperer_container(args->numero, args->niveau)){
					mutex_lock(args->niveau);
					ressources[ressource_case] += TAILLE_CONTAINER;
					mutex_unlock(args->niveau);
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
				
				transferer_container(args->numero, args->niveau);
			}
			else{	// Les ateliers de niveau 1 n'attendent pas d'ordre de niveaux superieurs et expedient immediatement
				ressources[ressource_case+1] -= TAILLE_CONTAINER;
				mutex_unlock(args->niveau);
				
				mutex_lock(args->niveau);
				ressources[0] += TAILLE_CONTAINER;	// Expedition vers la case des produits finis
				mutex_unlock(args->niveau);
			}
			printf("L'atelier %d de niveau %d a expedie un container.\n", (int) args->numero, (int) args->niveau);
		}
		mutex_unlock(args->niveau);
	}
	mutex_unlock(args->niveau);
	printf("L'atelier %d de niveau %d a epuise son stock.\n", (int) args->numero, (int) args->niveau);
	exit(0);
}

void liberation_ressources(){
	/* liberation des ressources */
	pthread_mutex_destroy(&mutex0);
	pthread_mutex_destroy(&mutex1);
	pthread_mutex_destroy(&mutex2);
	pthread_mutex_destroy(&mutex3);
	pthread_mutex_destroy(&mutex1_2);
	pthread_mutex_destroy(&mutex2_3);
	pthread_cond_destroy(&atelier1_2);
	pthread_cond_destroy(&atelier2_3);
}

void traitantSIGINT(int num) {

	printf("\n\nInteruption du programme!!!\n");
	
	// Envoyer au thread un signal pour qu'ils s'arretent
	for ( int i = 0; i < nombre_ateliers_niveau*3; i++)
		pthread_cancel(tid[i]);
	
	for ( int i = 0; i < nombre_ateliers_niveau*3 ; i++)
		pthread_join(tid[i],NULL);
	
	// Presenter resume de l'etat des ressources
	printf("Ressources actuelles:\n");
	for ( int i = 1; i < (1 + 6*nombre_ateliers_niveau) ; i++)
		printf("\tRessources[%d] = %d\n", (int)i, (int)ressources[i]);
		
	printf("\nNombre de produits obtenue: %d\n\n",ressources[0]);
	
	liberation_ressources();
	
	//signal(SIGINT, SIG_DFL);
	
	exit(0);
}

int main(int argc, char *argv[], char *arge[])
{
	/* Gestion des signaux */
	
	signal(SIGINT,traitantSIGINT);
	
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
			nombre_ateliers_niveau = atoi(argv[1]);
			taille_ressources = 1 + 2*nombre_niveaux*nombre_ateliers_niveau;
			break;
	}
	ressources = malloc(sizeof(int*)*taille_ressources);
	
	// Allocation des tableaux de transfert de containers a raison d'une case par ateliers
	transfert_containers1_2 = malloc(sizeof(int*)*nombre_ateliers_niveau);
	transfert_containers2_3 = malloc(sizeof(int*)*nombre_ateliers_niveau);
	// Initialisation des tableaux
	for (int i = 0 ; i < nombre_ateliers_niveau ; i++){
		transfert_containers1_2[i] = 0;
		transfert_containers2_3[i] = 0;
	}
	
	//Initialisation des ressources de depart
	//Chaque atelier commence avec 2 containers de ressources
	for ( int i = 1; i < taille_ressources ; i += 2)
		ressources[i] = 2*TAILLE_CONTAINER;
	
	printf("Ressources de depart:\n");
	for ( int i = 0; i < taille_ressources ; i++)
		printf("\tRessources[%d] = %d\n", (int)i, (int)ressources[i]);
	
	//Allocation du tableau de tid
	tid = malloc(sizeof(int*)*(nombre_ateliers_niveau*nombre_niveaux+1));
	
	//creation des threads ateliers
	struct arguments_atelier args;

	int numero=0;
	for(int i=0;i<nombre_niveaux;i++)
		for(int j=0;j<nombre_ateliers_niveau;j++){
			numero = = j + i*nombre_ateliers_niveau + 1;
			args.numero = numero;
			args.niveau = 1;
			args.vitesse = SPEED_ATELIER[i];
			pthread_create(tid,numero,(void *(*)())atelier, (void *)&args);
		}
	
	//attend la fin de toutes les threads ateliers
	for(int i=0;i<nombre_niveaux;i++)
		for(int j=0;j<nombre_ateliers_niveau;j++){
			pthread_join(tid[j + i*nombre_ateliers_niveau + 1],NULL);
	
	// Vu que tous les autres threads ont ete tues, il n'est plus necessaire d'utiliser un mutex
	printf("Nombre de produits obtenue: %d\n\n",ressources[0]);
	
	
	liberation_ressources();

	exit(0);
}
