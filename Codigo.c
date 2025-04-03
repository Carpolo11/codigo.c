#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>

#define SHM_SIZE 1024  // Tamaño de la memoria compartida

typedef struct {
    char message[SHM_SIZE];
    bool turno; // Control de turno: true = escritor1, false = escritor2
} SharedMemory;

void processA() {
    int shmid;
    SharedMemory *shm_ptr;
    key_t key = 1234;

    // Crear o obtener el segmento de memoria compartida
    shmid = shmget(key, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("Error al crear segmento de memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Adjuntar el segmento al espacio de direcciones
    shm_ptr = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm_ptr == (SharedMemory *)(-1)) {
        perror("Error al adjuntar la memoria compartida");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 5; i++) {
        while (!shm_ptr->turno) {
            usleep(100000); // Espera activa
        }
        snprintf(shm_ptr->message, SHM_SIZE, "Mensaje %d desde Proceso A", i);
        printf("Proceso A escribió: %s\n", shm_ptr->message);
        shm_ptr->turno = false;
    }

    // Desadjuntar la memoria compartida
    shmdt(shm_ptr);
}

void processB() {
    int shmid;
    SharedMemory *shm_ptr;
    key_t key = 1234;

    // Obtener el ID del segmento de memoria compartida
    shmid = shmget(key, sizeof(SharedMemory), 0666);
    if (shmid == -1) {
        perror("Error al obtener el segmento de memoria compartida");
        exit(EXIT_FAILURE);
    }

    // Adjuntar el segmento al espacio de direcciones
    shm_ptr = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm_ptr == (SharedMemory *)(-1)) {
        perror("Error al adjuntar la memoria compartida");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 5; i++) {
        while (shm_ptr->turno) {
            usleep(100000); // Espera activa
        }
        printf("Proceso B leyó: %s\n", shm_ptr->message);
        snprintf(shm_ptr->message, SHM_SIZE, "Mensaje %d desde Proceso B", i);
        printf("Proceso B escribió: %s\n", shm_ptr->message);
        shm_ptr->turno = true;
    }

    // Desadjuntar la memoria compartida
    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, NULL); // Eliminar memoria compartida al finalizar
}

int main() {
    int pid = fork();

    if (pid < 0) {
        perror("Error al crear el proceso");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        processA();
    } else {
        processB();
        wait(NULL); // Esperar a que termine el proceso hijo
    }

    return 0;
}
