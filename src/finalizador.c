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
#include "tools.h"

// instancia del semaforo
sem_t *sem_info_compartida;
sem_t *sem_archivo_salida;

// funciones implementadas
int inicializarSemaforos(char *nombre_sem_archivo_salida, char *nombre_sem_info_compartida);
int obtenerValoresCompartidos(char *nombreMemComp);
void detectarFinalizacion();

// informacion para la memoria compartida
struct informacionCompartida *informacion_compartida_finalizador;
int *puntero_mem_compartida;

int main(int argc, char *argv[])
{
    // Inicializa las variables
    char nombre_buffer[50];
    char nombre_sem_info_compartida[50];
    char nombre_sem_archivo_salida[50];
    // Inicializa los parametros a valores incorrectos que deberan ser cambiados
    strcpy(nombre_buffer, "");

    // Obtiene el valor de los argumentos pasados en la inicializacion del programa
    for (int i = 0; i < argc; i++)
    {
        // nombres de semaforos
        if (strcmp(argv[i], "-n") == 0)
        {
            strcpy(nombre_buffer, argv[i + 1]);

            strcpy(nombre_sem_info_compartida, nombre_buffer);
            strcat(nombre_sem_info_compartida, "_info");

            strcpy(nombre_sem_archivo_salida, nombre_buffer);
            strcat(nombre_sem_archivo_salida, "_salida");
        }
    }

    // Se asegura que los parametros fueron correctamente proporcionados y cambiados, sino falla
    if (strcmp(nombre_buffer, "") == 0 || llave == -1)
    {
        printf("Error al determinar el nombre o la llave del emisor\n");
        return 1;
    }

    inicializarSemaforos(nombre_sem_archivo_salida, nombre_sem_info_compartida);

    // Se detectan los inputs del teclado
    obtenerValoresCompartidos(nombre_buffer);
    detectarFinalizacion();
}

int inicializarSemaforos(char *nombre_sem_emisores, char *nombre_sem_receptores, char *nombre_sem_info_compartida)
{
    // valida si el semaforo de la informacion compartida existe
    sem_info_compartida = sem_open(nombre_sem_info_compartida, O_EXCL);
    if (sem_info_compartida == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de informacion compartida\n");
        return 1;
    }

    sem_archivo_salida = sem_open(nombre_sem_archivo_salida, O_EXCL);

    if (sem_archivo_salida == SEM_FAILED)
    {
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
        perror("Error en el file descriptor de memoria compartida"); // imprimir un mensaje de error
        return 1;
    }

    // obtener la información del archivo
    if (fstat(memoria_compartida_descriptor, &sb) == -1)
    {
        perror("Error al obtener la informacion del archivo");
        return 1;
    }

    // obtener el tamaño de la memoria compartida
    int size = sb.st_size;

    puntero_mem_compartida = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, memoria_compartida_descriptor, 0);
    informacion_compartida_finalizador = (struct informacionCompartida *)puntero_mem_compartida;
    return 0;
}

void ejecutar(void)
{
    // variables para desplegar la informacion
    int emisoresVivos, emisoresCreados, receptoresVivos, 
        receptoresCreados, caracteresTransferidos,
        caracteresEnBuffer, memoriaUtilizada;



    // obtiene los procesos que se terminaron de ejecutar
    sem_wait(sem_info_compartida);
    emisoresVivos = informacion_compartida_finalizador->emisores_vivos;
    emisoresCreados = informacion_compartida_finalizador->emisores_creados;
    receptoresVivos = informacion_compartida_finalizador->receptores_vivos;
    receptoresCreados = informacion_compartida_finalizador->receptores_creados;
    caracteresTransferidos = informacion_compartida_finalizador->contador_receptores;
    caracteresEnBuffer = informacion_compartida_finalizador->contador_emisores;
    memoriaUtilizada = informacion_compartida_finalizador->tamano_info_compartida;
    sem_post(sem_info_compartida);

    printf("Emisores Vivos ---> %d\nEmisores Creados ---> %d\n
            Receptores Vivos ---> %d\nReceptores Creados ---> %d\n
            Caracteres Transferidos ---> %d\n Caracteres en el Buffer ---> %d\n
            Memoria utilizada ---> %d\n",
            emisoresVivos, emisoresCreados, receptoresVivos, receptoresCreados,
            caracteresTransferidos, caracteresEnBuffer, memoriaUtilizada);

}

void detectarFinalizacion()
{
    // Monitorear la entrada estándar y la salida estándar
    printf("Presiones x y despues enter para finalizar la ejecucion del programa\n");
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
        if (input[0] == 'x')
        {
            ejecutar();
        }

        else
        {
            printf("Ingreso %s y no es una letra valida", input);
        }
    }
}