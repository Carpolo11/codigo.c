#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <stdbool.h>

#define SHM_SIZE 1024  // Tamaño de la memoria compartida

typedef struct {
    char message[SHM_SIZE];
    bool turno; // Control de turno: true = escritor, false = lector
} SharedMemory;

void writer() {
    int shmid;
    SharedMemory *shm_ptr;
    key_t key = 1234;

    // Crear el segmento de memoria compartida
    shmid = shmget(key, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Error al crear segmento de memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Adjuntar el segmento de memoria compartida
    shm_ptr = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm_ptr == (SharedMemory *)(-1)) {
        perror("Error al adjuntar la memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Inicializar memoria compartida
    shm_ptr->turno = true;
    
    for (int i = 0; i < 5; i++) {
        while (!shm_ptr->turno) {
            usleep(100000); // Espera activa
        }
        snprintf(shm_ptr->message, SHM_SIZE, "Mensaje %d desde el escritor", i);
        printf("Escritor escribió: %s\n", shm_ptr->message);
        shm_ptr->turno = false;
    }

    shmdt(shm_ptr);
}

void reader() {
    int shmid;
    SharedMemory *shm_ptr;
    key_t key = 1234;

    // Obtener el segmento de memoria compartida
    shmid = shmget(key, sizeof(SharedMemory), 0666);
    if (shmid == -1) {
        perror("Error al obtener el segmento de memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Adjuntar el segmento de memoria compartida
    shm_ptr = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm_ptr == (SharedMemory *)(-1)) {
        perror("Error al adjuntar la memoria compartida");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 5; i++) {
        while (shm_ptr->turno) {
            usleep(100000); // Espera activa
        }
        printf("Lector leyó: %s\n", shm_ptr->message);
        shm_ptr->turno = true;
    }

    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, NULL); // Eliminar memoria compartida al finalizar
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s [writer|reader]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "writer") == 0) {
        writer();
    } else if (strcmp(argv[1], "reader") == 0) {
        reader();
    } else {
        fprintf(stderr, "Opción inválida. Usa 'writer' o 'reader'.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
