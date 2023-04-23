#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>

#define SECOND_MERCHANT "/s_merchant_sem"
#define SHM_NAME "/shared_memory_2"

typedef struct {
    int count;
} shared_data;

sem_t *second_m_sem;
shared_data* shared_queue;

void sigint_handler(int signum) {
    printf("\nHere");
    // освобождение ресурсов семафора
    sem_close(second_m_sem);
    sem_unlink(SECOND_MERCHANT);
    // освобождение разделяемой памяти
    munmap(shared_queue, sizeof(shared_data));
    shm_unlink(SHM_NAME);
    exit(-1);
}

int main() {
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    // создание разделяемой памяти
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return 1;
    }
    if (ftruncate(shm_fd, sizeof(shared_data)) == -1) {
        perror("ftruncate");
        return 1;
    }

    shared_queue = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_queue== MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // создание семафора продавца
    second_m_sem = sem_open(SECOND_MERCHANT, O_CREAT, 0666, 1);
    if (second_m_sem == SEM_FAILED) {
        perror("Error sem_open: second_m_sem");
        return 1;
    }
    shared_queue->count = 0;

    int l = 0;

    while (l < 10000) {
        sleep(5);
        sem_wait(second_m_sem);
        if (shared_queue->count > 0) {
            printf("Second merchant is working!\n");
            if (shared_queue->count > 0) {
                printf("Now one client is not in queue! Good job!\n");
                shared_queue->count--;
            }
        } else {
            printf("Second merchant is sleeping!\n");
        }
        sem_post(second_m_sem);
    }
}