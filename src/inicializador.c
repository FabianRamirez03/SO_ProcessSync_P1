#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/stat.h>
#include "informacionCompartida.h"


void inicializarInformacionCompartida (struct informacionCompartida* informacion_compartida);

int inicializarSemaforos(char* nombre_sem_emisores, char* nombre_sem_receptores, char* nombre_sem_archivo_salida, char* nombre_sem_info_compartida, int tamano_buffer);

int main(int argc, char* argv[]) {

    // Inicializa las variables
    char nombre_buffer[50];
    char nombre_sem_emisores[50];
    char nombre_sem_receptores[50];
    char nombre_sem_archivo_salida[50];
    char nombre_sem_info_compartida[50];
    // Inicializa los parametros a valores incorrectos que deberan ser cambiados
    strcpy(nombre_buffer, "");

    int texto_entrada_descriptor;
    char texto_entrada_buffer[8192];

    int celdas_buffer = -1;
    int llave = -1;


    // Obtiene el valor de los argumentos pasados en la inicializacion del programa
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            strcpy(nombre_buffer, argv[i + 1]);

            strcpy(nombre_sem_emisores, nombre_buffer);
            strcat(nombre_sem_emisores, "emisor");

            strcpy(nombre_sem_receptores, nombre_buffer);
            strcat(nombre_sem_receptores, "receptor");
            
            strcpy(nombre_sem_archivo_salida, nombre_buffer);
            strcat(nombre_sem_archivo_salida, "salida");

            strcpy(nombre_sem_info_compartida, nombre_buffer);
            strcat(nombre_sem_info_compartida, "info");
        }
        if (strcmp(argv[i], "-b") == 0) { 
            celdas_buffer = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "-k") == 0) { 
            llave = atoi(argv[i + 1]);
        }
    }
    // Se asegura que los parametros fueron correctamente proporcionados y cambiados, sino falla
    if (celdas_buffer == -1 || strcmp(nombre_buffer, "") == 0 || llave == -1) {
        printf("No se pudo determinar el nombre o el largo del buffer\n");
        return 1;
    }

    // 0666 setea permisos de lectura y escritura para todos
    int mem_compartida_descriptor = shm_open(nombre_buffer, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (mem_compartida_descriptor < 0) {
        shm_unlink(nombre_buffer); 
        perror("No se pudo crear la memoria compartida del buffer\n");
        return 1;
    }


	//-------------------------------------------------------------- Abre el archivo --------------------------------------------------------------
	FILE* archivo_entrada_ptr = fopen("texto_entrada.txt", "r");  // Abre el archivo en modo lectura
    if (archivo_entrada_ptr == NULL) {  // Verifica si el archivo se abrió correctamente
        printf("No se pudo abrir el archivo\n");
        return 1;
    }

    // Mueve el puntero al final del archivo
    fseek(archivo_entrada_ptr, 0, SEEK_END);

	//-----------------------------------------------Calcula el tamano de la memoria compartida --------------------------------------------------------------
	// Obtiene la posición actual del puntero, que es el tamaño del archivo en bytes
    long int tamano_entrada = ftell(archivo_entrada_ptr);
	fseek(archivo_entrada_ptr, 0, SEEK_SET); // Desplazar el puntero al inicio del archivo

    printf("El tamaño del archivo es %ld bytes\n", tamano_entrada);

    int tamano_info_compartida = sizeof(struct informacionCompartida);

	printf("El tamaño de la info compartida es %d bytes\n", tamano_info_compartida);


    // Maximo de ascii = 256
    int tamano_buffer = 2 * celdas_buffer;

	printf("El tamaño del buffer es %d bytes\n", tamano_buffer);


    int tamano_total = tamano_info_compartida + tamano_buffer + tamano_entrada;

	//-----------------------------------------------Actualiza el espacio de la memoria compartida y lo mapea --------------------------------------------------------------
    
	// Se le actualiza el tamano a la memoria compartida con el tamano de la info compartida, mas el tamano total del buffer 
    // segun las celdas a utilizar
    ftruncate(mem_compartida_descriptor, tamano_total);
    

    // Accede al contenido del objeto de memoria compartida como si fuera parte de su propio espacio de direcciones. 
    // La función devuelve un puntero al área de memoria mapeada.
    int* puntero_mem_compartida = mmap(NULL, tamano_total, PROT_READ | PROT_WRITE, MAP_SHARED, mem_compartida_descriptor, 0);


	//-----------------------------------------------Inicializa la memoria compartida--------------------------------------------------------------
    
	// Redirecciona esta estructura al puntero anterior
    struct informacionCompartida* informacion_compartida = (struct informacionCompartida*) puntero_mem_compartida;
    
    // Inicializa los valores de la informacion compartida todo en 0
    inicializarInformacionCompartida(informacion_compartida);
	// Actualiza el dato de la cantidad de celdas de la estructura con el argumento ingresado por el usuario
    informacion_compartida->celdas_buffer = celdas_buffer;
	informacion_compartida->tamano_archivo = tamano_entrada;
	informacion_compartida->tamano_info_compartida = tamano_info_compartida;

	//---------------------------------------------------------Lectura del archivo y guarda en buffer ----------------------------------------------------
	
	char* buffer_entrada;
	buffer_entrada = malloc(tamano_entrada); // Reservar memoria para el array
	
	if (!buffer_entrada) {
        printf("Error al reservar memoria para el buffer\n");
        return 1;
    }

	fread(buffer_entrada, 1, tamano_entrada, archivo_entrada_ptr); // Leer el archivo y almacenarlo en el array
	// Cierra el archivo
	fclose(archivo_entrada_ptr);

	char* dest_archivo_entrada_ptr =  (char*) puntero_mem_compartida + tamano_info_compartida;

	memcpy(dest_archivo_entrada_ptr, buffer_entrada, tamano_entrada);

	
	if (!dest_archivo_entrada_ptr) {
        printf("Error al reservar memoria para el buffer\n");
        return 1;
    }

	printf("Puntero info compartida: %p\n", (void*)puntero_mem_compartida);
	printf("Puntero info Archivo: %p\n", (void*)dest_archivo_entrada_ptr);



		// Recorrer e imprimir el contenido del array
    for (int i = 0; i < tamano_entrada; i++) {
        printf("%c", dest_archivo_entrada_ptr[i]);
    }
	printf("\n");

    // Se inicializan los semaforos y se comprueba que se haga correctamente
    if (inicializarSemaforos(nombre_sem_emisores, nombre_sem_receptores, nombre_sem_archivo_salida, nombre_sem_info_compartida, celdas_buffer) > 0) {
        printf("No se pudieron inicializar los semaforos\n");
        close(mem_compartida_descriptor);
        shm_unlink(nombre_buffer); 
        return 1;
    }

    printf("Buffer y semaforos creados exitosamente\n");

    return 0;
}

void inicializarInformacionCompartida(struct informacionCompartida* informacion_compartida) {

    informacion_compartida->emisores_vivos = 0;
    informacion_compartida->emisores_creados = 0; 

    informacion_compartida->receptores_vivos = 0; 
    informacion_compartida->receptores_creados = 0; 

    informacion_compartida->celdas_buffer = 0; 

    informacion_compartida->contador_emisores = 0; 
    informacion_compartida->contador_receptores = 0; 

	informacion_compartida->tamano_archivo = 0;
	informacion_compartida->tamano_info_compartida = 0;
	informacion_compartida->llave = 0;

    return;
}


int inicializarSemaforos(char* nombre_sem_emisores, char* nombre_sem_receptores, char* nombre_sem_archivo_salida, char* nombre_sem_info_compartida, int tamano_buffer) {

    sem_t* sem_emisores = sem_open(nombre_sem_emisores, O_CREAT, 0644, tamano_buffer);

    if (sem_emisores == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de emisores\n");
        return 1;
    }

    sem_t* sem_receptores = sem_open(nombre_sem_receptores, O_CREAT, 0644, 0);

    if (sem_receptores == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de receptores\n");
        return 1;
    }



    sem_t* sem_archivo_salida = sem_open(nombre_sem_archivo_salida, O_CREAT, 0644, 1);

    if (sem_archivo_salida == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo del archivo de salida\n");
        return 1;
    }

    sem_t* sem_info_compartida = sem_open(nombre_sem_info_compartida, O_CREAT, 0644, 1);

    if (sem_archivo_salida == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de la informacion compartida\n");
        return 1;
    }

    return 0;
}