#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define N_PARADAS 5
// número de paradas de la ruta
#define EN_RUTA 0
// autobús en ruta
#define EN_PARADA 1
// autobús en la parada
#define MAX_USUARIOS 40 // capacidad del autobús
#define USUARIOS 4
// numero de usuarios
// estado inicial
int estado = EN_RUTA;

int parada_actual = 0;
// parada en la que se encuentra el autobus

int n_ocupantes = 0;
// ocupantes que tiene el autobús

// personas que desean subir en cada parada
int esperando_parada[N_PARADAS]; //= {0,0,...0};

// personas que desean bajar en cada parada
int esperando_bajar[N_PARADAS]; //= {0,0,...0};

pthread_mutex_t mtx1[N_PARADAS], mtx2[N_PARADAS];
pthread_cond_t suben, bajan, hanAcabadoSubir, hanAcabadoBajar;
pthread_t autobus;
pthread_t usuarios[USUARIOS];

// Otras definiciones globales (comunicación y sincronización)
void * thread_autobus(void * args) {
while (1) {
// esperar a que los viajeros suban y bajen
	Autobus_En_Parada();
// conducir hasta siguiente parada
	Conducir_Hasta_Siguiente_Parada();
}
}
void * thread_usuario(int i) {
	int id_usuario = i, a, b;
	// obtener el id del usario
	while (1) {
		a=rand() % N_PARADAS;
		do{
			b=rand() % N_PARADAS;
		} while(a==b);
		Usuario(id_usuario,a,b);
	}
}
void Usuario(int id_usuario, int origen, int destino) {
// Esperar a que el autobus esté en parada origen para subir
	Subir_Autobus(id_usuario, origen);
// Bajarme en estación destino
	Bajar_Autobus(id_usuario, destino);
}
int main(int argc, char *argv[]) {
	int i;
	
	// Definición de variables locales a main
	// Opcional: obtener de los argumentos del programa la capacidad del
	// autobus, el numero de usuarios y el numero de paradas
	// Crear el thread Autobus
	for(int i = 0; i < N_PARADAS; i++){
		pthread_mutex_init(&(mtx1[i]), NULL);
		pthread_mutex_init(&(mtx2[i]), NULL);
	}

	pthread_cond_init(&suben, NULL);
	pthread_cond_init(&bajan, NULL);
	pthread_cond_init(&hanAcabadoSubir, NULL);
	pthread_cond_init(&hanAcabadoBajar, NULL);

	pthread_create(&autobus, NULL, thread_autobus, NULL);
	


	for (i = 0; i < USUARIOS; i++){
		// Crear thread para el usuario i
		pthread_create(&(usuarios[i]), NULL, thread_usuario, (void*)i);
		// Esperar terminación de los hilos
	}

	for(int i = 0; i < USUARIOS; i++){
		pthread_join(usuarios[i], NULL);
	}
	pthread_join(autobus, NULL);
	for(int i = 0; i < N_PARADAS; i++){
		pthread_mutex_destroy(&(mtx1[i]));
		pthread_mutex_destroy(&(mtx2[i]));
	}

	pthread_cond_destroy(&suben);
	pthread_cond_destroy(&bajan);
	pthread_cond_destroy(&hanAcabadoSubir);
	pthread_cond_destroy(&hanAcabadoBajar);

	return 0;
}

void Autobus_En_Parada(){
/* Ajustar el estado y bloquear al autobús hasta que no haya pasajeros que
quieran bajar y/o subir la parada actual. Después se pone en marcha */

	// Version 1 para esta parte. Paralela
	estado = EN_PARADA;

	/* Cogemos los 2 mutexes para hacer los broadcast tanto a los que se 
	quiere subir como bajar */

	pthread_mutex_lock(&(mtx1[parada_actual]));
	pthread_mutex_lock(&(mtx2[parada_actual]));

	printf("Autobus ha llegado a la parada %d \n" , parada_actual);

	/* Avisamos tanto a los que se quieen subir como bajar a la parada para
	que lo hagan (de forma paralela se pueden estar subiendo y bajando) */

	pthread_cond_broadcast(&suben);
	pthread_cond_broadcast(&bajan);


	/* Tenemos cuidado a la hora de bloqear este hilo para que sea despertado */
	/* Para volver a poner el autobus en ruta tenemos que asegurnos de que todos
	los que s tuviesen que bajar y subir lo hayan hecho */


	while(esperando_bajar[parada_actual] > 0 || esperando_parada[parada_actual] > 0){


		/* Si solo habia personas para bajarse solo soltamos el mutex de bajada
		para que se bajen los que toca, pero no soltamos el de subida, puesto que no hay
		nadie que quiera hacerlo*/

		if(esperando_bajar[parada_actual] > 0 && esperando_parada[parada_actual] == 0){
			pthread_cond_wait(&hanAcabadoBajar, &(mtx2[parada_actual]));
		}
		/* Análogo en caso de que solo haya gente para subirse en la parada*/

		else if(esperando_bajar[parada_actual] == 0 && esperando_parada[parada_actual] >0){
			pthread_cond_wait(&hanAcabadoSubir, &(mtx1[parada_actual]));
		}
		/* Si hay gente para subirse y bajarse, tenemosque soltar los 2 mutexes. 
		No podemos hacerlo con 2 wait porque no sabemos qué se va a terminar antes, subir o bajar,
		y podriamos no salir de los waits jamás. Como se tienen que cumplir ambas condiciones,
		soltamos un mutex y esperamos a que nos avisen de que se ha cumplido la condicion del otro
		Si la condicion del ue soltamos sin saber nada se había cumplido, pues perfecto, y si no
		volvera a entrar en el while y a bloquearse hasta que se cumpla realmente
		*/
		
		else{
			pthread_mutex_unlock(&(mtx1[parada_actual]));
			pthread_cond_wait(&hanAcabadoBajar, &(mtx2[parada_actual]));
			pthread_mutex_lock(&(mtx1[parada_actual]));
		}
		
	}
	pthread_mutex_unlock(&(mtx1[parada_actual]));
	pthread_mutex_unlock(&(mtx2[parada_actual]));
	estado = EN_RUTA;
	printf("Autobus en ruta\n");



	//Versión 2

	/*Otra opcion más limpia aunque menos concurrente es coger el mutex de los que quieren
	subir. Despertarlos, bloquearse y esperar a que me avisen de que han acabado.
	Y después hacerlo propio con los que se quieren bajar. A diferencia de la versió anterior
	no estamos permitiendo que se suban y bajen a la vez pasajeros. En un autobus de verdad
	hay varias puertas y esto normalmente sí puede suceder*/

	/*estado = EN_PARADA;
	printf("Autobus ha llegado a la parada %d \n" , parada_actual);
	pthread_mutex_lock(&(mtx1[parada_actual]));
	pthread_cond_broadcast(&suben);
	while(esperando_parada[parada_actual] > 0){
		pthread_cond_wait(&hanAcabadoSubir, &(mtx1[parada_actual]));
	}
	pthread_mutex_unlock(&(mtx1[parada_actual]));

	pthread_mutex_lock(&(mtx2[parada_actual]));
	pthread_cond_broadcast(&bajan);
	while(esperando_bajar[parada_actual] > 0){
		pthread_cond_wait(&hanAcabadoBajar, &(mtx2[parada_actual]));
	}
	pthread_mutex_unlock(&(mtx2[parada_actual]));
	estado = EN_RUTA;
	printf("Autobus en ruta\n");*/
}

void Conducir_Hasta_Siguiente_Parada(){
/* Establecer un Retardo que simule el trayecto y actualizar numero de parada */
	sleep(1);
	parada_actual = (parada_actual + 1) % N_PARADAS;
}
void Subir_Autobus(int id_usuario, int origen){
/* El usuario indicará que quiere subir en la parada ’origen’, esperará a que
el autobús se pare en dicha parada y subirá. El id_usuario puede utilizarse para
proporcionar información de depuración */
	pthread_mutex_lock(&(mtx1[origen]));
	esperando_parada[origen]++;
	printf("Pasajero %d esperando en %d \n", id_usuario, origen);

	while(estado != EN_PARADA || parada_actual != origen){

		pthread_cond_wait(&suben, &(mtx1[origen]));
	}

	esperando_parada[origen]--;
	n_ocupantes++;
	printf("Pasajero %d se ha subido al bus \n", id_usuario);
	if(esperando_parada[origen] == 0){
		pthread_cond_signal(&hanAcabadoSubir);
	}
	pthread_mutex_unlock(&(mtx1[origen]));
}

void Bajar_Autobus(int id_usuario, int destino){
/* El usuario indicará que quiere bajar en la parada ’destino’, esperará a que
el autobús se pare en dicha parada y bajará. El id_usuario puede utilizarse para
proporcionar información de depuración */
	pthread_mutex_lock(&(mtx2[destino]));
	esperando_bajar[destino]++;
	printf("Pasajero %d esperando para bajar en %d \n", id_usuario, destino);
	
	while(estado != EN_PARADA || parada_actual != destino){
		pthread_cond_wait(&bajan, &(mtx2[destino]));
	}
	esperando_bajar[destino]--;
	n_ocupantes--;
	printf("Pasajero %d se ha bajado del bus \n", id_usuario);
	if(esperando_bajar[destino] == 0){
		pthread_cond_signal(&hanAcabadoBajar);
	}
	pthread_mutex_unlock(&(mtx2[destino]));
}
