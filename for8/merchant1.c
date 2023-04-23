#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>


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

int main()
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