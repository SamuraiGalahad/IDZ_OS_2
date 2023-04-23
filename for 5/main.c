#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <pthread.h>

#define FIRST_MERCHANT "/f_merchant_sem"
#define SECOND_MERCHANT "/s_merchant_sem"
#define SHM_NAME_1 "/shared_memory"
#define SHM_NAME_2 "/shared_memory_2"

typedef struct {
    int count;
} shared_data;

sem_t *first_m_sem, *second_m_sem;
shared_data* shared_queue_1, *shared_queue_2;

void sigint_handler(int signum) {
    // освобождение ресурсов семафора
    sem_close(first_m_sem);
    sem_unlink(FIRST_MERCHANT);
    // освобождение разделяемой памяти
    munmap(shared_queue_1, sizeof(shared_data));
    shm_unlink(SHM_NAME_1);

    // освобождение ресурсов семафора
    sem_close(second_m_sem);
    sem_unlink(SECOND_MERCHANT);
    // освобождение разделяемой памяти
    munmap(shared_queue_2, sizeof(shared_data));
    shm_unlink(SHM_NAME_2);
    exit(-1);
}

void *merchant1(void* thread_data) {
  shared_data* shared_queue;
  // создание разделяемой памяти
    int shm_fd = shm_open(SHM_NAME_1, O_CREAT | O_RDWR, 0666);
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
    first_m_sem = sem_open(FIRST_MERCHANT, O_CREAT, 0666, 1);
    if (first_m_sem == SEM_FAILED) {
        perror("Error sem_open: first_m_sem");
        return 1;
    }
    shared_queue->count = 0;

    int l = 0;

    while (l < 1000) {
        sleep(1);
        sem_wait(first_m_sem);
        if (shared_queue->count > 0) {
            printf("First merchant is working!\n");
            if (shared_queue->count > 0) {
                printf("Now one client is not in queue! Good job!\n");
                shared_queue->count--;
            }
        } else {
            printf("First merchant is sleeping!\n");
        }
        sem_post(first_m_sem);
    }
}

void *merchant2(void* thread_data) {
  shared_data* shared_queue;
  int shm_fd = shm_open(SHM_NAME_2, O_CREAT | O_RDWR, 0666);
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

    while (l < 1000) {
      sleep(1);
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

int all_waist[5][5] = {{0, 0, 0, 1, 1}, {0, 1, 0, 1, 0}, {1, 0, 1, 0, 1}, {0, 0, 0, 0, 0}, {1, 1, 1, 1, 1}};

int main() {
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);
    int num_customers = 5;

    pthread_t thread1, thread2;

    pthread_create(&thread1, NULL, merchant1, NULL);
    pthread_create(&thread2, NULL, merchant2, NULL);


    // открытие именованных семафоров
    first_m_sem = sem_open(FIRST_MERCHANT, O_CREAT, 0666, 1);
    if (first_m_sem == SEM_FAILED) {
        perror("Error sem_open: first_m_sem");
        return 1;
    }
    second_m_sem = sem_open(SECOND_MERCHANT, O_CREAT, 0666, 1);
    if (second_m_sem == SEM_FAILED) {
        perror("Error sem_open: second_m_sem");
        return 1;
    }

    // открытие области памяти первого торговца
    int shm_fd_1 = shm_open(SHM_NAME_1, O_CREAT | O_RDWR, 0666);
    if (shm_fd_1 == -1) {
        perror("shm_open 1");
        return 1;
    }

    shared_queue_1 = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_1, 0);
    if (shared_queue_1== MAP_FAILED) {
        perror("mmap 1");
        return 1;
    }

    // открытие области памяти второго торговца
    int shm_fd_2 = shm_open(SHM_NAME_2, O_CREAT | O_RDWR, 0666);
    if (shm_fd_2 == -1) {
        perror("shm_open 2");
        return 1;
    }

    shared_queue_2 = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_2, 0);
    if (shared_queue_2 == MAP_FAILED) {
        perror("mmap 2");
        return 1;
    }
    
    // создание покупателей
    for (int i = 0; i < 5; ++i) {
        pid_t pid;
        switch (pid = fork())
        {
        case -1:
          printf("Error creating process");
          return 1;
          break;
        case 0:
          // i покупатель пробигается по j покупке и добавляет себя к j+1 продавцу
            for (int j = 0; j < 5; ++j) {
                if (all_waist[i][j] == 0) {
                    sem_wait(first_m_sem);
                    printf("%d in first queue\n",i);
                    shared_queue_1->count++;
                    sem_post(first_m_sem);
                } else {
                    sem_wait(second_m_sem);
                    printf("%d in second queue\n",i);
                    shared_queue_2->count++;
                    sem_post(second_m_sem);
                }
                sleep(10);
            }
            exit(EXIT_SUCCESS);
        default:
          continue;
          break;
        }
    }
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    sigint_handler(0);
    return 0;
}