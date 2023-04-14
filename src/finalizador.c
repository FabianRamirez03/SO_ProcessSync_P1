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
#include <sys/select.h>
#include <time.h>
#include "tools.h"

// instancia del semaforo
sem_t *sem_emisores;
sem_t *sem_receptores;
sem_t *sem_info_compartida;
sem_t *sem_archivo_salida;

// funciones implementadas
int inicializarSemaforos(char* nombre_sem_emisores, char* nombre_sem_receptores, char* nombre_sem_archivo_salida, char* nombre_sem_info_compartida);
int obtenerValoresCompartidos(char *nombreMemComp);
int detectarFinalizacion(char* nombre_buffer);
void ejecutar(char* nombre_buffer);
int finalizarRecursosCompartida(char* nombre_buffer);

// informacion para la memoria compartida
struct informacionCompartida *informacion_compartida_finalizador;
int *puntero_mem_compartida;
int ejecucion = 0;

int main(int argc, char *argv[])
{
    // Inicializa las variables
    char nombre_buffer[50];
    char nombre_sem_emisores[50];
    char nombre_sem_receptores[50];
	char nombre_sem_archivo_salida[50];
    char nombre_sem_info_compartida[50];
    // Inicializa los parametros a valores incorrectos que deberan ser cambiados
    strcpy(nombre_buffer, "");

    // Obtiene el valor de los argumentos pasados en la inicializacion del programa
    for (int i = 0; i < argc; i++)
    {
        // nombres de semaforos
        if (strcmp(argv[i], "-n") == 0)
        {
            strcpy(nombre_buffer, argv[i + 1]);

            strcpy(nombre_sem_emisores, nombre_buffer);
            strcat(nombre_sem_emisores, "_emisor");

            strcpy(nombre_sem_receptores, nombre_buffer);
            strcat(nombre_sem_receptores, "_receptor");

            strcpy(nombre_sem_info_compartida, nombre_buffer);
            strcat(nombre_sem_info_compartida, "_info");

            strcpy(nombre_sem_archivo_salida, nombre_buffer);
            strcat(nombre_sem_archivo_salida, "_salida");
        }
    }

    // Se asegura que los parametros fueron correctamente proporcionados y cambiados, sino falla
    if (strcmp(nombre_buffer, "") == 0)
    {
        color("Rojo");
        printf("Error al determinar el nombre o la llave del emisor\n");
        return 1;
    }

    inicializarSemaforos(nombre_sem_emisores, nombre_sem_receptores, nombre_sem_archivo_salida, nombre_sem_info_compartida);

    // Se detectan los inputs del teclado
    detectarFinalizacion(nombre_buffer);
}

int inicializarSemaforos(char* nombre_sem_emisores, char* nombre_sem_receptores, char* nombre_sem_archivo_salida, char* nombre_sem_info_compartida)
{
    // valida si el semaforo de la informacion compartida existe
    sem_info_compartida = sem_open(nombre_sem_info_compartida, O_EXCL);
    if (sem_info_compartida == SEM_FAILED)
    {
        color("Rojo");
        perror("Fallo al inicializar semaforo de informacion compartida\n");
        return 1;
    }

    sem_archivo_salida = sem_open(nombre_sem_archivo_salida, O_EXCL);

    if (sem_archivo_salida == SEM_FAILED)
    {
        color("Rojo");
        perror("Fallo al inicializar semaforo del archivo de salida\n");
        return 1;
    }

    else
    {
        printf("Semaforos de informacion compartida y archivo de salida recibidos correctamente\n");
    }

    return 0;
}

int obtenerValoresCompartidos(char *nombreMemComp)
{

    struct stat sb;                    // estructura de información de archivo
    int memoria_compartida_descriptor; // descriptor de archivo

    // abrir o crear la memoria compartida
    memoria_compartida_descriptor = shm_open(nombreMemComp, O_RDWR, S_IRWXU);

    // verificar si la llamada fue exitosa
    if (memoria_compartida_descriptor < 0)
    {
        color("Rojo");
        perror("Error en el file descriptor de memoria compartida"); // imprimir un mensaje de error
        return 1;
    }

    // obtener la información del archivo
    if (fstat(memoria_compartida_descriptor, &sb) == -1)
    {
        color("Rojo");
        perror("Error al obtener la informacion del archivo");
        return 1;
    }

    // obtener el tamaño de la memoria compartida
    int size = sb.st_size;

    puntero_mem_compartida = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, memoria_compartida_descriptor, 0);
    informacion_compartida_finalizador = (struct informacionCompartida *)puntero_mem_compartida;
    return 0;
}

void ejecutar(char* nombre_buffer)
{
    obtenerValoresCompartidos(nombre_buffer);
    // variables para desplegar la informacion
    int emisoresVivos, emisoresCreados, receptoresVivos, 
        receptoresCreados, caracteresTransferidos,
        caracteresEnBuffer, memoria_utilizada;

    // obtiene los procesos que se terminaron de ejecutar
    sem_wait(sem_info_compartida);
    emisoresVivos = informacion_compartida_finalizador->emisores_vivos;
    emisoresCreados = informacion_compartida_finalizador->emisores_creados;
    receptoresVivos = informacion_compartida_finalizador->receptores_vivos;
    receptoresCreados = informacion_compartida_finalizador->receptores_creados;
    caracteresTransferidos = informacion_compartida_finalizador->contador_receptores;
    caracteresEnBuffer = informacion_compartida_finalizador->contador_emisores;
    memoria_utilizada = informacion_compartida_finalizador->memoriaUtilizada;
    informacion_compartida_finalizador->terminacionProcesos = 1;
    sem_post(sem_info_compartida);

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    color("Azul");
    printf("****************************************************************************************\n");
    
    
    printf("\033[0;36m Emisores Vivos:  \033[0;32m%d\n \033[0;36m Emisores Creados: \033[0;32m %d\n \033[0;36m Receptores Vivos:  \033[0;32m %d\n \033[0;36m Receptores Creados:  \033[0;32m%d\n \033[0;36m Caracteres Transferidos: \033[0;32m %d\n \033[0;36m Caracteres en el Buffer: \033[0;32m%d\n \033[0;36m Memoria utilizada: \033[0;32m %d\n",
            emisoresVivos, emisoresCreados, receptoresVivos, receptoresCreados,
            caracteresTransferidos, caracteresEnBuffer, memoria_utilizada);
    color("Azul");
    printf("****************************************************************************************\n");
    color("Negro");


    ejecucion = 1;

    finalizarRecursosCompartida(nombre_buffer);
    

    
}

int detectarFinalizacion(char* nombre_buffer )
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    while (ejecucion == 0)
    {
        // Monitorear la entrada estándar y la salida estándar
        int ready = select(STDIN_FILENO + 1, &fds, NULL, NULL, NULL);
        if (ready == -1)
        {
            perror("select");
            return 1;
        }

        // Si se detectó un evento en la entrada estándar, leer y procesar la entrada
        if (FD_ISSET(STDIN_FILENO, &fds))
        {
            char input[256];
            ssize_t bytes_read = read(STDIN_FILENO, input, sizeof(input));
            if (bytes_read == -1)
            {
                perror("Error al leer la entrada");
                return 1;
            }
            // Procesar la entrada recibida
            if (bytes_read == 1 && input[0] == '\n')
            {
                ejecutar(nombre_buffer);
            }
            else if (input[0] == 'x')
            {
                printf("Se deberia de terminar el proceso\n");
            }
            
            else
            {
                printf("Ingreso %s y no es una letra valida", input);
            }
        }
    }

}

int finalizarRecursosCompartida(char* nombre_buffer){

    int emisoresCreados, receptoresVivos;

    emisoresCreados = informacion_compartida_finalizador->emisores_creados;
    receptoresVivos = informacion_compartida_finalizador->receptores_vivos;

    while (emisoresCreados  != 0 || receptoresVivos  != 0)
    {
        emisoresCreados = informacion_compartida_finalizador->emisores_creados;
        receptoresVivos = informacion_compartida_finalizador->receptores_vivos;
    }
    

    // Eliminar la región de memoria compartida

    if (shm_unlink(nombre_buffer) == -1)
    {
        perror("Error al eliminar la región de memoria compartida");
        exit(1);
    }
    printf("Región de memoria compartida eliminada\n");
    


    // Eliminando semaforos
    sem_destroy(sem_emisores);
    sem_destroy(sem_receptores);
    sem_destroy(sem_info_compartida);
    sem_destroy(sem_archivo_salida);
    
    return 0;
}