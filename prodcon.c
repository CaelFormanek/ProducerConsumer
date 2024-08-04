#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>   
#include <fcntl.h>
#include <sys/types.h>          
#include <semaphore.h>  
#include "buffer.h"
    
/* global variables */
int front = 0;
int back = 0;
int num_items_in_buffer = 0;
sem_t* empty;
sem_t* full;
pthread_mutex_t mutex;
BUFFER_ITEM buffer[BUFFER_SIZE];

/* function declarations */
void insert_item(BUFFER_ITEM* item);
void remove_item(BUFFER_ITEM* item);
void* producer(void* param);
void* consumer(void* param);
uint16_t checksum(char *addr, uint32_t count);
int initialize_mutex_and_semaphores();

int main(int argc, char** argv)
{
  /* 1. Get command line arguments argv[1], argv[2], argv[3] */
  printf("Getting command line arguments...\n");
  if (argc != 4)
  {
    printf("You must provide the name of the program and sleep length, number of producer threads, and number of consumer threads, nothing more or less.\n");
    return 1;
  }
  
  /* make sure all arguments are integers */
  int a = 0, b = 0;
  for (a = 1; a < 4; a++)
  {
    for (b = 0; b < strlen(argv[a]); b++)
    {
      if (!isdigit(argv[a][b]))
      {
        printf("You entered %s, but you must enter an integer number (no decimals or extra characters).\n", argv[a]);
        return 1;
      }
    }
  }

  int sleep_time_seconds = atoi(argv[1]);
  int num_producer_threads = atoi(argv[2]);
  int num_consumer_threads = atoi(argv[3]);
  
  /* 2. Initialize  */
  printf("Initializing...\n");
  
  if (initialize_mutex_and_semaphores() == -1)
  {
    return -1;
  }

  /* 3. Create producer threads */
  printf("Creating producer threads...\n");
  pthread_t producer_threads[num_producer_threads];
  pthread_attr_t producer_thread_attrs[num_producer_threads];
  int i  = 0;
  for (i = 0; i < num_producer_threads; i++)
  {
    pthread_attr_init(&producer_thread_attrs[i]);
    pthread_create(&producer_threads[i], &producer_thread_attrs[i], producer, NULL);
  }

  /* 4. Create consumer threads */
  printf("Creating consumer threads...\n");
  pthread_t consumer_threads[num_consumer_threads];
  pthread_attr_t consumer_thread_attrs[num_consumer_threads];
  int j  = 0;
  for (j = 0; j < num_consumer_threads; j++)
  {
    pthread_attr_init(&consumer_thread_attrs[j]);
    pthread_create(&consumer_threads[j], &consumer_thread_attrs[j], consumer, NULL);
  }

  /* Sleep */
  printf("Sleeping for %d...\n", sleep_time_seconds);
  sleep(sleep_time_seconds);

  /* Exit */
  printf("About to exit...\n");

  return 0;
}

void insert_item(BUFFER_ITEM *item)
{
  // semaphore empty, and pthread mutex lock
  sem_wait(empty);
  pthread_mutex_lock(&mutex);

  memcpy(&buffer[back], item, sizeof(BUFFER_ITEM));
  back = (back+1) % BUFFER_SIZE;

  //pthread mutex unlock, sempost full
  pthread_mutex_unlock(&mutex);
  sem_post(full);
}

void remove_item(BUFFER_ITEM* item)
{
  // wait full, pthreadmutex lock
  sem_wait(full);
  pthread_mutex_lock(&mutex);

  int calculated_checksum = checksum((char*)&buffer[front].data, 30); // move to consumer
  if (calculated_checksum != buffer[front].cksum)
  {
    printf("cksum does not match\n");
    printf("actual checksum is %d, but calculated %d", buffer[front].cksum, calculated_checksum);
    exit(1);
  }
  else
  {
    memcpy(item, &buffer[front], sizeof(BUFFER_ITEM)); /* place object from buffer in item*/
    front = (front+1) % BUFFER_SIZE;
    --num_items_in_buffer;
  }
  // mutex unlock, post empty
  pthread_mutex_unlock(&mutex);
  sem_post(empty);
}

void* producer(void* param)
{
  BUFFER_ITEM item;

  while (1)
  {
    /* sleep for a random period of time */
    int randnum = rand() % (2 - 1 + 1) + 1;
    sleep(randnum);

    /* generate random data for item */
    int i = 0;
    for (i = 0; i < 30; i++)
    {
      item.data[i] = rand();
    }
    /* calculate checksum */
    item.cksum = checksum((char*)item.data, 30);

    insert_item(&item);

    printf("producer produced %d\n", item.cksum);
  }
  pthread_exit(0);
}

void* consumer(void* param)
{
   BUFFER_ITEM item;

   while (1)
   {
    /* sleep for a random period of time */
    int randnum = rand() % (2 - 1 + 1) + 1;
    sleep(randnum);

    remove_item(&item);

    printf("consumer consumed %d\n", item.cksum);
   }
   pthread_exit(0);
}

uint16_t checksum(char *addr, uint32_t count)
{
    register uint32_t sum = 0;

    uint16_t *buf = (uint16_t *) addr;

    // Main summing loop
    while(count > 1)
    {
        sum = sum + *(buf)++;
        count = count - 2;
    }

    // Add left-over byte, if any
    if (count > 0)
        sum = sum + *addr;

    // Fold 32-bit sum to 16 bits
    while (sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return(~sum);
}

int initialize_mutex_and_semaphores()
{
  sem_unlink("empty");
  sem_unlink("full");
  pthread_mutex_unlock(&mutex);

  empty = sem_open("empty", O_CREAT | O_EXCL, 0644, BUFFER_SIZE);
  if (empty == SEM_FAILED) 
  {
    printf("could not initialize empty semaphore: %s\n", strerror(errno));
    return -1;
  }

  full = sem_open("full", O_CREAT | O_EXCL, 0644, 0);
  if (full == SEM_FAILED) 
  {
    printf("could not initialize full semaphore: %s\n", strerror(errno));
    return -1;
  }

  if (pthread_mutex_init(&mutex,NULL) != 0)
  {
    fprintf(stderr,"Unable to initialize mutex\n");
    return -1;
  }
  
  return 1;
}
