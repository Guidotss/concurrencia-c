#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

typedef struct {
    int Compartida;
    sem_t *mutex;
} memoria_compartida_t;

void dormir_aleatorio(unsigned int seed) {
    int tiempo_micro = rand_r(&seed) % 2000001;
    usleep(tiempo_micro);
}

void proceso_tipo1(int numero, memoria_compartida_t* memoria) {
    unsigned int seed = time(NULL) ^ getpid();

    printf("Instancia %d del proceso 1\n", numero);
    fflush(stdout);

    dormir_aleatorio(seed);

    if (sem_wait(memoria->mutex) == -1) {
        fprintf(stderr, "Proceso 1-%d: Error al esperar el semáforo: %s\n", numero, strerror(errno));
        exit(EXIT_FAILURE);
    }

    memoria->Compartida++;

    if (sem_post(memoria->mutex) == -1) {
        fprintf(stderr, "Proceso 1-%d: Error al liberar el semáforo: %s\n", numero, strerror(errno));
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}

void proceso_tipo2(int numero, memoria_compartida_t* memoria) {
    unsigned int seed = time(NULL) ^ getpid();

    printf("Instancia %d del proceso 2\n", numero);
    fflush(stdout);

    dormir_aleatorio(seed);

    if (sem_wait(memoria->mutex) == -1) {
        fprintf(stderr, "Proceso 2-%d: Error al esperar el semáforo: %s\n", numero, strerror(errno));
        exit(EXIT_FAILURE);
    }

    int valor = memoria->Compartida;

    if (sem_post(memoria->mutex) == -1) {
        fprintf(stderr, "Proceso 2-%d: Error al liberar el semáforo: %s\n", numero, strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("Valor de Compartida: %d\n", valor);
    fflush(stdout);

    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s N M\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int N = atoi(argv[1]); 
    int M = atoi(argv[2]); 

    if (N < 0 || M < 0) {
        fprintf(stderr, "Los valores de N y M deben ser enteros no negativos.\n");
        exit(EXIT_FAILURE);
    }

    // Crear memoria compartida
    memoria_compartida_t* memoria = mmap(NULL, sizeof(memoria_compartida_t),
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (memoria == MAP_FAILED) {
        perror("Error al crear la memoria compartida con mmap");
        exit(EXIT_FAILURE);
    }

    memoria->Compartida = 0;

    // Inicializar el semáforo mutex (1 indica que es compartido entre procesos)
    memoria->mutex = sem_open("/mi_semaforo", O_CREAT | O_EXCL, 0644, 1);
    if (memoria->mutex == SEM_FAILED) {
        perror("Error al crear el semáforo");
        munmap(memoria, sizeof(memoria_compartida_t));
        exit(EXIT_FAILURE);
    }

    pid_t pid;
    int total_procesos = N + M;

    // Crear procesos de tipo 1
    for (int i = 0; i < N; i++) {
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Error al crear el proceso de tipo 1-%d: %s\n", i, strerror(errno));
            continue;
        }
        else if (pid == 0) {
            proceso_tipo1(i, memoria);
        }
    }

    // Crear procesos de tipo 2
    for (int i = 0; i < M; i++) {
        pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Error al crear el proceso de tipo 2-%d: %s\n", i, strerror(errno));
            continue;
        }
        else if (pid == 0) {
            proceso_tipo2(i, memoria);
        }
    }

    // Esperar a que todos los procesos hijos finalicen
    for (int i = 0; i < total_procesos; i++) {
        pid_t wpid = wait(NULL);
        if (wpid == -1) {
            if (errno == ECHILD) {
                break;
            }
            else {
                perror("Error al esperar a un proceso hijo");
            }
        }
    }

    // Destruir el semáforo
    if (sem_close(memoria->mutex) == -1) {
        perror("Error al cerrar el semáforo");
    }
    if (sem_unlink("/mi_semaforo") == -1) {
        perror("Error al eliminar el semáforo");
    }

    // Liberar la memoria compartida
    if (munmap(memoria, sizeof(memoria_compartida_t)) == -1) {
        perror("Error al liberar la memoria compartida con munmap");
    }

    printf("Se ha finalizado la ejecución\n");
    fflush(stdout);

    return EXIT_SUCCESS;
}
