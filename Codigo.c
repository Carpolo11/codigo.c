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

void processA(SharedMemory *shm_ptr) {
    for (int i = 0; i < 5; i++) {
        while (!shm_ptr->turno) {
            usleep(100000); // Espera activa
        }
        snprintf(shm_ptr->message, SHM_SIZE, "Mensaje %d desde Proceso A", i);
        printf("Proceso A escribió: %s\n", shm_ptr->message);
        shm_ptr->turno = false;
    }
}

void processB(SharedMemory *shm_ptr) {
    for (int i = 0; i < 5; i++) {
        while (shm_ptr->turno) {
            usleep(100000); // Espera activa
        }
        printf("Proceso B leyó: %s\n", shm_ptr->message);
        snprintf(shm_ptr->message, SHM_SIZE, "Mensaje %d desde Proceso B", i);
        printf("Proceso B escribió: %s\n", shm_ptr->message);
        shm_ptr->turno = true;
    }
}

int main() {
    int shmid;
    SharedMemory *shm_ptr;
    key_t key = 1234;

    // Crear el segmento de memoria compartida antes del fork
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

    // Inicializar la estructura compartida
    shm_ptr->turno = true;
    memset(shm_ptr->message, 0, SHM_SIZE);

    int pid = fork();
    if (pid < 0) {
        perror("Error al crear el proceso");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        processA(shm_ptr);
    } else {
        processB(shm_ptr);
        wait(NULL); // Esperar a que termine el proceso hijo
        
        // Desadjuntar y eliminar la memoria compartida al finalizar
        shmdt(shm_ptr);
        shmctl(shmid, IPC_RMID, NULL);
    }

    return 0;
}
