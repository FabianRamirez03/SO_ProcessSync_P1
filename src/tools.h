#include <stdio.h>
#include <stdlib.h>


#define Negro "\033[0;30m"
#define Rojo "\033[0;31m"
#define Verde "\033[0;32m"
#define Amarillo "\033[0;33m"
#define Azul "\033[0;34m"
#define Morado "\033[0;35m"
#define Cyan "\033[0;36m"
#define Blanco "\033[0;37m"
#define Reset "\033[0m"

void color(char *s_color){
    if (strcmp(s_color, "Negro") == 0){
        printf( Negro);
    }
    else if (strcmp(s_color, "Rojo") == 0){
        printf( Rojo);
    }
    else if (strcmp(s_color, "Verde") == 0){
        printf( Verde);
    }
    else if (strcmp(s_color, "Amarillo") == 0){
        printf( Amarillo);
    }
    else if (strcmp(s_color, "Azul") == 0){
        printf( Azul);
    }
    else if (strcmp(s_color, "Morado") == 0){
        printf( Morado);
    }
    else if (strcmp(s_color, "Cyan") == 0){
        printf( Cyan);
    }
    else if (strcmp(s_color, "Blanco") == 0){
        printf( Blanco);
    }
    else if (strcmp(s_color, "Reset") == 0){
        printf( Reset);
    }
    else{
        printf("No es un color valido");
    }
    
}