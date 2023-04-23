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

int all_waist[5][5] = {{0, 0, 0, 1, 1}, {0, 1, 0, 1, 0}, {1, 0, 1, 0, 1}, {0, 0, 0, 0, 0}, {1, 1, 1, 1, 1}};

int main()
{
  signal(SIGINT, sigint_handler);
  signal(SIGTERM, sigint_handler);
  int num_customers = 5;

  sleep(5);

  // открытие семафоров торговцев
  if ((first_m_sem = semget(1, 1, 0666)) == -1)
  {
    perror("Error sem get: first_m_sem");
    return 1;
  }

  if ((second_m_sem = semget(2, 1, 0666)) == -1)
  {
    perror("Error sem get: first_m_sem");
    return 1;
  }

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
  sigint_handler(0);
  return 0;
}