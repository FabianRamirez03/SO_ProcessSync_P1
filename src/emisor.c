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
#include <time.h>
#include <signal.h>

// semaforos por inicializar
sem_t *sem_emisores;
sem_t *sem_receptores;
sem_t *sem_info_compartida;

// funcion de inicializar
int inicializarSemaforos(char *nombre_sem_emisores, char *nombre_sem_receptores, char *nombre_sem_info_compartida);
int obtenerValoresCompartidos(char *nombreMemComp);
void ejecutar(void);
int modoEjecucion(int modo);
void finalizarSignal(int sig);
void finalizar(void);


// variables para lectura
int indiceLectura;
char letraLeyendo;
int tamanioArchivo;

// informacion que puede acceder el emisor
struct informacionCompartida *informacion_compartida_emisor;
int *puntero_mem_compartida;
int cantidadCeldas;
int llave = -1;

int descriptor_global;
char buffer_name_global[50];
int tamano_global;


int main(int argc, char *argv[])
{
    signal(SIGINT, finalizarSignal);
    // Inicializa las variables
    char nombre_buffer[50];
    char nombre_sem_emisores[50];
    char nombre_sem_receptores[50];
    char nombre_sem_info_compartida[50];
    // Inicializa los parametros a valores incorrectos que deberan ser cambiados
    strcpy(nombre_buffer, "");

    int modo = 1;

    // Obtiene el valor de los argumentos pasados en la inicializacion del programa
    for (int i = 0; i < argc; i++)
    {
        // nombres de semaforos
        if (strcmp(argv[i], "-n") == 0)
        {
            strcpy(nombre_buffer, argv[i + 1]);
            strcpy(buffer_name_global, argv[i + 1]);

            strcpy(nombre_sem_emisores, nombre_buffer);
            strcat(nombre_sem_emisores, "_emisor");

            strcpy(nombre_sem_receptores, nombre_buffer);
            strcat(nombre_sem_receptores, "_receptor");

            strcpy(nombre_sem_info_compartida, nombre_buffer);
            strcat(nombre_sem_info_compartida, "_info");
        }

        // asigna el modo al semaforo
        if (strcmp(argv[i], "-m") == 0)
        {
            modo = 0;
        }

        // llave para los semaforos
        if (strcmp(argv[i], "-k") == 0)
        {
            llave = atoi(argv[i + 1]);
        }
    }

    // Se asegura que los parametros fueron correctamente proporcionados y cambiados, sino falla
    if (strcmp(nombre_buffer, "") == 0 || llave == -1)
    {
        printf("Error al determinar el nombre o la llave del emisor\n");
        return 1;
    }

    inicializarSemaforos(nombre_sem_emisores, nombre_sem_receptores, nombre_sem_info_compartida);

    // se toma el control del semaforo para aumentar el contador de creacion de emisores
    sem_wait(sem_info_compartida);
    obtenerValoresCompartidos(nombre_buffer);
    informacion_compartida_emisor->emisores_creados++;
    informacion_compartida_emisor->emisores_vivos++;
    tamanioArchivo = informacion_compartida_emisor->tamano_archivo;
    sem_post(sem_info_compartida);
    modoEjecucion(modo);
}

// Inicializacion de los emisores
int inicializarSemaforos(char *nombre_sem_emisores, char *nombre_sem_receptores, char *nombre_sem_info_compartida)
{
    // valida si el semaforo de los emisores existe
    sem_emisores = sem_open(nombre_sem_emisores, O_EXCL);
    if (sem_emisores == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de emisores\n");
        return 1;
    }
    else
    {
        printf("Semaforo de emisor recibido correctamente desde el emisor\n");
    }

    // valida si el semaforo de los receptores existe
    sem_receptores = sem_open(nombre_sem_receptores, O_EXCL);
    if (sem_receptores == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de receptores\n");
        return 1;
    }
    else
    {
        printf("Semaforo de receptor recibido correctamente desde el emisor\n");
    }

    // valida si el semaforo de la informacion compartida existe
    sem_info_compartida = sem_open(nombre_sem_info_compartida, O_EXCL);
    if (sem_info_compartida == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de informacion compartida\n");
        return 1;
    }
    else
    {
        printf("Semaforo de informacion compartida recibido correctamente desde el emisor\n");
    }

    return 0;
}

int obtenerValoresCompartidos(char *nombreMemComp)
{

    struct stat sb;                    // estructura de información de archivo
    int memoria_compartida_descriptor; // descriptor de archivo

    // abrir o crear la memoria compartida
    memoria_compartida_descriptor = shm_open(nombreMemComp, O_RDWR, S_IRWXU);
    descriptor_global = memoria_compartida_descriptor;

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
    tamano_global = size;

    puntero_mem_compartida = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, memoria_compartida_descriptor, 0);
    informacion_compartida_emisor = (struct informacionCompartida *)puntero_mem_compartida;
    cantidadCeldas = informacion_compartida_emisor->celdas_buffer;
    return 0;
}

int modoEjecucion(int modo)
{
    int ejecucion = informacion_compartida_emisor->terminacionProcesos;
    if (modo == 1)
    {
        printf("Modo Automatico\n*****************************\n");

        while (ejecucion == 0)
        {
            sleep(3);
            ejecutar();
            ejecucion = informacion_compartida_emisor->terminacionProcesos;
        }
        finalizar();
    }

    else
    {
        printf("Modo Manual\n*****************************\n");
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);

        while (ejecucion == 0)
        {
            ejecucion = informacion_compartida_emisor->terminacionProcesos;
            printf("Ejecucion: %d\n", ejecucion);
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
                    ejecutar();
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
        finalizar();
    }
}

void ejecutar(void)
{
    int contador = 0;
    int espacioEscritura = 0;
    char letra;
    char letraEncriptada;

    // logica de lectura y escritura con semaforos
    sem_wait(sem_info_compartida);
    letra = ((char *)puntero_mem_compartida + informacion_compartida_emisor->tamano_info_compartida)[informacion_compartida_emisor->contador_emisores];
    contador = informacion_compartida_emisor->contador_emisores;
    espacioEscritura = informacion_compartida_emisor->contador_emisores % cantidadCeldas;
    informacion_compartida_emisor->contador_emisores++;
    sem_post(sem_info_compartida);

    if (contador < tamanioArchivo)
    {	
		int recep, emisores;
		sem_getvalue(sem_emisores, &emisores);
		sem_getvalue(sem_receptores, &recep);

        color("Negro");
		printf("Valor del semaforo emisores antes: %d\n", emisores);
		printf("Valor del semaforo receptores antes: %d\n", recep);

        // down al semaforo de receptores
        sem_post(sem_receptores);
        // up al semaforo de emisores
        sem_wait(sem_emisores);
        char *buffer = (char *)puntero_mem_compartida + informacion_compartida_emisor->tamano_archivo + informacion_compartida_emisor->tamano_info_compartida;

        // se encripta la letra
        letraEncriptada = letra ^ llave;

        *(buffer + espacioEscritura) = letraEncriptada;

		sem_getvalue(sem_emisores, &emisores);
		sem_getvalue(sem_receptores, &recep);

		printf("Valor del semaforo emisores despues: %d\n", emisores);
		printf("Valor del semaforo receptores despues: %d\n", recep);

        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        color("Azul");
        printf("****************************************************************************************\n");

        color("Morado");
        printf("  %s ",asctime(tm_info));

        printf("\033[0;36m Llave: \033[0;32m 0x%x \n\033[0;36m  Letra original: \033[0;32m %c (0x%x)\n \033[0;36m Letra encriptada: \033[0;32m %c (0x%x) \n \033[0;36m Índice: \033[0;32m%d\n \033[0;36m Espacio escritura: \033[0;32m%d\n \033[0;36m Buffer: \033[0;32m%s\n", llave, letra, letra, letraEncriptada, letraEncriptada,  contador, espacioEscritura, buffer);
        color("Azul");
        printf("****************************************************************************************\n");
        color("Negro");
    }
}


void finalizarSignal(int sig) {
    finalizar();
}

void finalizar(void) {
    printf("Finalizando emisor.\n");

    sem_wait(sem_info_compartida);
    informacion_compartida_emisor->emisores_vivos --;
    sem_post(sem_info_compartida);

    // Cierro los semaforos
    sem_close(sem_emisores);
    sem_close(sem_receptores);
    sem_close(sem_info_compartida);
    
    /*
    // Unlink semaforos
    sem_unlink(strcat(buffer_name_global, "_emisor"));
    sem_unlink(strcat(buffer_name_global, "_receptor"));
    sem_unlink(strcat(buffer_name_global, "_info"));
*/

    // Liberar memoria compartida
    if (munmap(puntero_mem_compartida, tamano_global) == -1) {
        perror("Error al liberar memoria compartida");
        exit(0);
    }
    close(descriptor_global);
    
    shm_unlink(buffer_name_global);
    

    exit(0);
}