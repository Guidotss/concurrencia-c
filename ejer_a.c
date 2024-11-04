#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>

int Compartida = 0;

pthread_mutex_t mutex;

typedef struct {
    int tipo;   // 1 o 2
    int numero;
} hilo_info_t;

void* funcion_hilo(void* arg) {
    hilo_info_t* info = (hilo_info_t*) arg;
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)((uintptr_t)pthread_self());
    // Hilo de tipo 1
    if (info->tipo == 1) {
        printf("Instancia %d del hilo 1\n", info->numero);
        
        int tiempo = rand_r(&seed) % 2000001;
        usleep(tiempo);
        
        pthread_mutex_lock(&mutex);
        Compartida++;
        pthread_mutex_unlock(&mutex);
    }
    // Hilo de tipo 2
    else if (info->tipo == 2) {
        printf("Instancia %d del hilo 2\n", info->numero);
        
        int tiempo = rand_r(&seed) % 2000001;
        usleep(tiempo);
        
        pthread_mutex_lock(&mutex);
        int valor = Compartida;
        pthread_mutex_unlock(&mutex);
        
        printf("Valor de Compartida: %d\n", valor);
    }

    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error: Número incorrecto de argumentos\n");
        fprintf(stderr, "Uso: %s N M\n", argv[0]);
        fprintf(stderr, "Donde:\n");
        fprintf(stderr, "  N: Número de hilos de tipo 1 (incrementadores)\n");
        fprintf(stderr, "  M: Número de hilos de tipo 2 (lectores)\n");
        fprintf(stderr, "Ambos números deben ser enteros positivos\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i++) {
        for (int j = 0; argv[i][j] != '\0'; j++) {
            if (argv[i][j] < '0' || argv[i][j] > '9') {
                fprintf(stderr, "Error: El argumento %d ('%s') debe ser un número entero positivo\n", 
                        i, argv[i]);
                exit(EXIT_FAILURE);
            }
        }
    }

    int N = atoi(argv[1]);
    int M = atoi(argv[2]);

    if (N <= 0 || M <= 0) {
        fprintf(stderr, "Error: Tanto N como M deben ser mayores que 0\n");
        fprintf(stderr, "N = %d, M = %d\n", N, M);
        exit(EXIT_FAILURE);
    }

    pthread_t* hilos = malloc((N + M) * sizeof(pthread_t));
    if (hilos == NULL) {
        perror("Error al asignar memoria para los hilos");
        exit(EXIT_FAILURE);
    }

    hilo_info_t* info_hilos = malloc((N + M) * sizeof(hilo_info_t));
    if (info_hilos == NULL) {
        perror("Error al asignar memoria para la información de los hilos");
        free(hilos);
        exit(EXIT_FAILURE);
    }

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("Error al inicializar el mutex");
        free(hilos);
        free(info_hilos);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < N; i++) {
        info_hilos[i].tipo = 1;
        info_hilos[i].numero = i;
        if (pthread_create(&hilos[i], NULL, funcion_hilo, &info_hilos[i]) != 0) {
            perror("Error al crear el hilo de tipo 1");
        }
    }

    for (int i = 0; i < M; i++) {
        info_hilos[N + i].tipo = 2;
        info_hilos[N + i].numero = i;
        if (pthread_create(&hilos[N + i], NULL, funcion_hilo, &info_hilos[N + i]) != 0) {
            perror("Error al crear el hilo de tipo 2");
        }
    }

    for (int i = 0; i < N + M; i++) {
        if (pthread_join(hilos[i], NULL) != 0) {
            perror("Error al esperar a que un hilo termine");
        }
    }

    pthread_mutex_destroy(&mutex);

    free(hilos);
    free(info_hilos);

    printf("Se ha finalizado la ejecución\n");

    return 0;
}
