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
sem_t *sem_archivo_salida;

// funcion de inicializar
int inicializarSemaforos(char* nombre_sem_emisores, char* nombre_sem_receptores, char* nombre_sem_archivo_salida, char* nombre_sem_info_compartida);
int obtenerValoresCompartidos(char *nombreMemComp);
int ejecutar(void);
int modoEjecucion(int modo);
int actualizarArchivoSalida(int i, char letra);
void finalizarSignal(int sig);
void finalizar(void);


// variables para lectura
int indiceLectura;
char letraLeyendo;
int tamanioArchivo;

// informacion que puede acceder el emisor
struct informacionCompartida *informacion_compartida_receptor;
int *puntero_mem_compartida;
int cantidadCeldas;
int llave = -1;

int main(int argc, char *argv[])
{
    signal(SIGINT, finalizarSignal);
    // Inicializa las variables
    char nombre_buffer[50];
    char nombre_sem_emisores[50];
    char nombre_sem_receptores[50];
	char nombre_sem_archivo_salida[50];
    char nombre_sem_info_compartida[50];
    color("Negro");
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

            strcpy(nombre_sem_emisores, nombre_buffer);
            strcat(nombre_sem_emisores, "_emisor");

            strcpy(nombre_sem_receptores, nombre_buffer);
            strcat(nombre_sem_receptores, "_receptor");

            strcpy(nombre_sem_info_compartida, nombre_buffer);
            strcat(nombre_sem_info_compartida, "_info");

            strcpy(nombre_sem_archivo_salida, nombre_buffer);
            strcat(nombre_sem_archivo_salida, "_salida");
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
    inicializarSemaforos(nombre_sem_emisores, nombre_sem_receptores, nombre_sem_archivo_salida, nombre_sem_info_compartida);
    // se toma el control del semaforo para aumentar el contador de creacion de emisores
    sem_wait(sem_info_compartida);
	obtenerValoresCompartidos(nombre_buffer);
    informacion_compartida_receptor->receptores_creados++;
   	informacion_compartida_receptor->receptores_vivos++;
    tamanioArchivo = informacion_compartida_receptor->tamano_archivo;
	sem_post(sem_info_compartida);
    modoEjecucion(modo);
}


int inicializarSemaforos(char* nombre_sem_emisores, char* nombre_sem_receptores, char* nombre_sem_archivo_salida, char* nombre_sem_info_compartida) {

    sem_emisores = sem_open(nombre_sem_emisores, O_EXCL);

    if (sem_emisores == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de emisores\n");
        return 1;
    }

    sem_receptores = sem_open(nombre_sem_receptores, O_EXCL);

    if (sem_receptores == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de receptores\n");
        return 1;
    }

    sem_archivo_salida = sem_open(nombre_sem_archivo_salida, O_EXCL);

    if (sem_archivo_salida == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo del archivo de salida\n");
        return 1;
    }

    sem_info_compartida = sem_open(nombre_sem_info_compartida, O_EXCL);
    if (sem_archivo_salida == SEM_FAILED)
    {
        perror("Fallo al inicializar semaforo de la informacion compartida\n");
        return 1;
    }

    return 0;
}

int obtenerValoresCompartidos(char *nombreMemComp)
{
	printf("Obteniendo valores compartidos\n");
    struct stat sb;                    // estructura de información de archivo
    int memoria_compartida_descriptor; // descriptor de archivo

    // abrir o crear la memoria compartida
    memoria_compartida_descriptor = shm_open(nombreMemComp, O_RDWR, S_IRWXU);

	printf("Se obtuve el file descriptor\n");

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
    informacion_compartida_receptor = (struct informacionCompartida *)puntero_mem_compartida;
    cantidadCeldas = informacion_compartida_receptor->celdas_buffer;
    
	return 0;
}

int modoEjecucion(int modo)
{
    int ejecucion = informacion_compartida_receptor->terminacionProcesos;
    if (modo == 1)
    {
        printf("Modo Automatico\n*****************************\n");

        while (ejecucion == 0)
        {
            sleep(3);
            ejecutar();
            ejecucion = informacion_compartida_receptor->terminacionProcesos;
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
            
            ejecucion = informacion_compartida_receptor->terminacionProcesos;
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
                    finalizar();
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

int ejecutar(void)
{
    int contador_receptores = 0;
    int espacioLectura= 0;
    char letra;
    char letraEncriptada;
	time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    color("Azul");
    printf("****************************************************************************************\n");

    color("Morado");
    printf("  %s ",asctime(tm_info));
    // logica de lectura y escritura con semaforos
    sem_wait(sem_info_compartida);
    contador_receptores = informacion_compartida_receptor->contador_receptores;
    espacioLectura = contador_receptores % cantidadCeldas;
	if (contador_receptores < tamanioArchivo){
		informacion_compartida_receptor->contador_receptores++;
	}
    sem_post(sem_info_compartida);

	printf("\033[0;36m Contador Receptores: \033[0;32m%d\n \033[0;36m Espacio Lectura: \033[0;32m%d\n \033[0;36m Tamaño archivo: \033[0;32m%d\n",contador_receptores, espacioLectura, tamanioArchivo);

    if (contador_receptores < tamanioArchivo)
    {	
        // up al semaforo de emisores

		int recep, emisores;
		sem_getvalue(sem_emisores, &emisores);
		sem_getvalue(sem_receptores, &recep);

		printf("\033[0;36m  Valor del semaforo emisores antes: \033[0;32m%d\n", emisores);
		printf("\033[0;36m  Valor del semaforo receptores antes: \033[0;32m%d\n", recep);
        sem_post(sem_emisores);
        // post al semaforo de receptores
        sem_wait(sem_receptores);
        char *buffer = (char *)puntero_mem_compartida + informacion_compartida_receptor->tamano_archivo + informacion_compartida_receptor->tamano_info_compartida;

		// Obtengo la letra encriptada del buffer
		letraEncriptada = buffer[espacioLectura];
		printf("\033[0;36m  Letra encriptada: \033[0;32m %c\n", letraEncriptada);
		// Vacio el buffer
        buffer[espacioLectura] = ' ';

		// desencripto la letra
        letra = letraEncriptada ^ llave;

		printf("\033[0;36m  Letra desencriptada: \033[0;32m %c\n", letra);

		actualizarArchivoSalida(contador_receptores, letra);

		sem_getvalue(sem_emisores, &emisores);
		sem_getvalue(sem_receptores, &recep);

		printf("\033[0;36m  Valor del semaforo emisores despues: \033[0;32m %d\n", emisores);
		printf("\033[0;36m  Valor del semaforo receptores despues: \033[0;32m%d\n", recep);

        color("Azul");
        printf("****************************************************************************************\n");
        color("Negro");
		return 0;
	}
}

int actualizarArchivoSalida(int i, char letra){
	FILE *file;
	int file_size = tamanioArchivo;
	
	printf("\033[0;36m  Índice: \033[0;32m%d\n", i);

	// Modifica el carater en el archivo de salida
	sem_wait(sem_archivo_salida);

	file = fopen("texto_salida.txt", "r+");
    if (file == NULL) {
        printf("Error al abrir el archivo. Por favor, verifique si existe o si tiene permisos.\n");
        exit(EXIT_FAILURE);
    }

	if (i < 0 || i >= file_size) {
        printf("El índice i proporcionado está fuera de los límites del archivo.\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

	fseek(file, i, SEEK_SET);
    fputc(letra, file);
	fclose(file);

	// Libera el semaforo
	sem_post(sem_archivo_salida);

	return 0;
}

void finalizarSignal(int sig) {
    finalizar();
}

void finalizar(void) {
    printf("Finalizando receptor.\n");
    sem_wait(sem_info_compartida);
    informacion_compartida_receptor->receptores_vivos --;
    sem_post(sem_info_compartida);
    exit(0);
}