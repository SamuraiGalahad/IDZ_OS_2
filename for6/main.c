#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <pthread.h>


int first_m_sem, second_m_sem;
key_t key_1;
key_t key_2;

struct sembuf sem_wait = {0, -1, SEM_UNDO};
struct sembuf sem_signal = {0, 1, SEM_UNDO};

void sigint_handler(int signum)
{
  if (semctl(first_m_sem, 0, IPC_RMID) == -1)
  {
    perror("Ошибка при удалении семафора для записи");
    exit(1);
  }
  if (semctl(second_m_sem, 0, IPC_RMID) == -1)
  {
    perror("Ошибка при удалении семафора для чтения");
    exit(1);
  }
  exit(-1);
}

void *merchant1(void *thread_data)
{
  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);
  // создание семафора продавца

  if ((first_m_sem = semget(1, 1, IPC_CREAT | 0666)) == -1)
  {
    perror("Error sem get: first_m_sem");
    return 1;
  }

  if (semctl(first_m_sem, 0, SETVAL, 1) == -1)
  {
    perror("Ошибка при инициализации семафора 1");
    exit(1);
  }

  int l = 0;

  while (l < 100)
  {
    sleep(1);
    if (semctl(first_m_sem, 0, GETVAL, 0) > 1)
    {
      printf("First merchant is working!\n");
      printf("Now one client is not in queue! Good job!\n");
      semop(first_m_sem, &sem_wait, 1);
    }
    else
    {
      if (semctl(first_m_sem, 0, GETVAL, 0) <= 0) {
        semop(first_m_sem, &sem_signal, 1);
      }
      printf("First merchant is sleeping!\n");
    }
    l++;
  }
}

void *merchant2(void *thread_data)
{
  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);
  // создание семафора продавца

  if ((second_m_sem = semget(2, 1, IPC_CREAT | 0666)) == -1)
  {
    perror("Error sem get: first_m_sem");
    return 1;
  }

  if (semctl(second_m_sem, 0, SETVAL, 1) == -1)
  {
    perror("Ошибка при инициализации семафора 1");
    exit(1);
  }

  int l = 0;

  while (l < 100)
  {
    sleep(1);
    if (semctl(second_m_sem, 0, GETVAL, 0) > 1)
    {
      printf("Seoncd merchant is working!\n");
      printf("Now one client is not in queue! Good job!\n");
      semop(second_m_sem, &sem_wait, 1);
    }
    else
    {
      if (semctl(second_m_sem, 0, GETVAL, 0) <= 0) {
        semop(second_m_sem, &sem_signal, 1);
      }
      printf("Second merchant is sleeping!\n");
    }
    l++;
  }
}

int all_waist[5][5] = {{0, 0, 0, 1, 1}, {0, 1, 0, 1, 0}, {1, 0, 1, 0, 1}, {0, 0, 0, 0, 0}, {1, 1, 1, 1, 1}};

int main()
{
  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);
  int num_customers = 5;

  pthread_t thread1, thread2;

  pthread_create(&thread1, NULL, merchant1, NULL);
  pthread_create(&thread2, NULL, merchant2, NULL);

  sleep(5);

  // создание покупателей
  for (int i = 0; i < 5; ++i)
  {
    pid_t pid;
    switch (pid = fork())
    {
    case -1:
      printf("Error creating process");
      return 1;
      break;
    case 0:
      // i покупатель пробигается по j покупке и добавляет себя к j+1 продавцу
      for (int j = 0; j < 5; ++j)
      {
        sleep(i + 1);
        if (all_waist[i][j] == 0)
        {
          semop(first_m_sem, &sem_signal, 1);
          printf("%d in first queue\n", i);
          sleep(i + 1);
          semop(first_m_sem, &sem_wait, 1);
        }
        else
        {
          semop(second_m_sem, &sem_signal, 1);
          printf("%d in second queue\n", i);
          semop(first_m_sem, &sem_wait, 1);
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