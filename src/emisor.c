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


// semaforos por inicializar
sem_t *sem_emisores;
sem_t *sem_receptores;
sem_t *sem_info_compartida;

// funcion de inicializar
int inicializarSemaforos(char* nombre_sem_emisores, char* nombre_sem_receptores, char* nombre_sem_info_compartida);
int obtenerValoresCompartidos(char* nombreMemComp);

int main(int argc, char* argv[]){
    // Inicializa las variables
    char nombre_buffer[50];
    char nombre_sem_emisores[50];
    char nombre_sem_receptores[50];
    char nombre_sem_info_compartida[50];
    // Inicializa los parametros a valores incorrectos que deberan ser cambiados
    strcpy(nombre_buffer, "");

    int modo = 1;
    int llave = -1;

    // Obtiene el valor de los argumentos pasados en la inicializacion del programa
    for (int i = 0; i < argc; i++) {
        // nombres de semaforos
        if (strcmp(argv[i], "-n") == 0) {
            strcpy(nombre_buffer, argv[i + 1]);

            strcpy(nombre_sem_emisores, nombre_buffer);
            strcat(nombre_sem_emisores, "_emisor");

            strcpy(nombre_sem_receptores, nombre_buffer);
            strcat(nombre_sem_receptores, "_receptor");
            

            strcpy(nombre_sem_info_compartida, nombre_buffer);
            strcat(nombre_sem_info_compartida, "_info");
        }
        
        // asigna el modo al semaforo
        if (strcmp(argv[i], "-m") == 0) {
            modo = 0;
        }

        // llave para los semaforos
        if (strcmp(argv[i], "-k") == 0) { 
            llave = atoi(argv[i + 1]);
        }
    }

    // Se asegura que los parametros fueron correctamente proporcionados y cambiados, sino falla
    if (strcmp(nombre_buffer, "") == 0 || llave == -1) {
        printf("Error al determinar el nombre o la llave del emisor\n");
        return 1;
    }
    
    inicializarSemaforos(nombre_sem_emisores, nombre_sem_receptores, nombre_sem_info_compartida);
    printf("Nombre buffer %s \n:", nombre_buffer);
    obtenerValoresCompartidos(nombre_buffer);

}

// Inicializacion de los emisores
int inicializarSemaforos(char* nombre_sem_emisores, char* nombre_sem_receptores, char* nombre_sem_info_compartida)
{
    // valida si el semaforo de los emisores existe
    sem_emisores = sem_open(nombre_sem_emisores, O_EXCL);
    if (sem_emisores == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de emisores\n");
        return 1;
    }
    else{
        printf("Semaforo de emisor recibido correctamente desde el emisor\n");
    }

    // valida si el semaforo de los receptores existe
    sem_receptores = sem_open(nombre_sem_emisores, O_EXCL);
    if (sem_receptores == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de receptores\n");
        return 1;
    }
    else{
        printf("Semaforo de receptor recibido correctamente desde el emisor\n");
    }

    // valida si el semaforo de la informacion compartida existe
    sem_info_compartida = sem_open(nombre_sem_info_compartida, O_EXCL);
    if (sem_info_compartida == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de informacion compartida\n");
        return 1;
    }
    else{
        printf("Semaforo de informacion compartida recibido correctamente desde el emisor\n");
    }

    return 0;
}

int obtenerValoresCompartidos(char* nombreMemComp){
    struct stat sb; // estructura de información de archivo
    int memoria_compartida_descriptor; // descriptor de archivo
    void *ptr; // puntero a la dirección de memoria compartida

    // abrir o crear la memoria compartida
    memoria_compartida_descriptor = shm_open(nombreMemComp, O_RDWR, S_IRWXU);
    
    // verificar si la llamada fue exitosa
    if (memoria_compartida_descriptor < 0) {
        perror("Error en el file descriptor de memoria compartida"); // imprimir un mensaje de error
        return 1;
    }

        // obtener la información del archivo
    if (fstat(memoria_compartida_descriptor, &sb) == -1) {
        perror("Error al obtener la informacion del archivo"); 
        return 1;
    }

    // obtener el tamaño de la memoria compartida
    int size = sb.st_size;

    // imprimir el tamaño de la memoria compartida
    printf("El tamaño de la memoria compartida es: %d\n", size);

    int* puntero_mem_compartida = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, memoria_compartida_descriptor, 0);

    struct informacionCompartida* informacion_compartida_emisor = (struct informacionCompartida*) puntero_mem_compartida;

    printf("Tamano archivo desde el emisor: %d \n",informacion_compartida_emisor->tamano_archivo);

    return 0;
}