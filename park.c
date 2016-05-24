#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "defines.h"

int free_places;
int closed;

int log_park;

void write_park_log(Vehicle * v, int state);

void * tpark_helper(void * avg)
{
  
  Vehicle vehicle = *(Vehicle *) avg; 
  int fd_write;
  int park_state = 0;
  
  pthread_detach(pthread_self());
  
  fd_write = open(vehicle.fifo, O_WRONLY);
  
  if(free_places > 0 && closed != 1)
  {
    
    park_state = ENTERING;
    write_park_log(&vehicle, park_state);
    write(fd_write, &park_state, sizeof(int));
    free_places--;
    sleep(vehicle.park_time);
    free_places++;
    park_state = EXITING;
    write_park_log(&vehicle, park_state);
    write(fd_write, &park_state, sizeof(int));
  }
  else if(closed == 1)
  {
    park_state = CLOSED;
    write_park_log(&vehicle, park_state);
    write(fd_write, &park_state, sizeof(int));
  }
  else if(free_places <= 0)
  {
    park_state = FULL;
    write_park_log(&vehicle, park_state);
    write(fd_write, &park_state, sizeof(int));
  }
  
  
  close(fd_write);
  return NULL;
}

void * tcontroller_N(void * avg)
{
 
  int fd;
  Vehicle vehicle;
 
  
  mkfifo(FIFON, 0600);
  fd = open(FIFON, O_RDONLY); 

  
  while(read(fd, &vehicle, sizeof(vehicle)) != 0)
  {
     if(vehicle.id == 0)
     {
      closed = 1;
      break;
     }
     else
     {
        pthread_t tid;
        pthread_create(&tid, NULL, tpark_helper, &vehicle);      
     }
  }
 

  close(fd);
  printf("exiting N\n");
  pthread_exit(0);
  return NULL;
}

void * tcontroller_E(void * avg)
{
  int fd;
  Vehicle vehicle; 
   
  mkfifo(FIFOE, 0600);
  fd = open(FIFOE, O_RDONLY);   

  while(read(fd, &vehicle, sizeof(vehicle)) != 0)
  {

     if(vehicle.id == 0)
     {
     closed = 1;
      break;
     }
     else
     {
        pthread_t tid;
        pthread_create(&tid, NULL, tpark_helper, &vehicle);      
     }
  }
  
  
  close(fd);
  printf("exiting E\n");
  pthread_exit(0);
  return NULL;
}

void * tcontroller_W(void * avg)
{
  int fd;
  Vehicle vehicle; 
   
  mkfifo(FIFOW, 0600);
  fd = open(FIFOW, O_RDONLY);   

  while(read(fd, &vehicle, sizeof(vehicle)) != 0)
  {

     if(vehicle.id == 0)
     {
      closed = 1;
      break;
     }
     else
     {
        pthread_t tid;
        pthread_create(&tid, NULL, tpark_helper, &vehicle);      
     }
  }
  
  
  close(fd);
  printf("exiting W\n");
  pthread_exit(0);
  return NULL;
}

void * tcontroller_S(void * avg)
{
  int fd;
  Vehicle vehicle; 
   
  mkfifo(FIFOS, 0600);
  
  fd = open(FIFOS, O_RDONLY);   

  while(read(fd, &vehicle, sizeof(vehicle)) != 0)
  {
     if(vehicle.id == 0)
     {
      closed = 1;
      break;
     }
     else
     {
        pthread_t tid;
        pthread_create(&tid, NULL, tpark_helper, &vehicle);      
     }
  }
  
  close(fd);
  printf("exiting S\n");
  pthread_exit(0);
  return NULL;
}

void create_park_log()
{
  FILE* file = fopen(PARK_LOG, "w");
  fclose(file);
  log_park = open(PARK_LOG, O_WRONLY | O_CREAT, 0600);
  char str[] = "t(ticks) ; nlug ; id_viat ; observ\n";
  write(log_park, str, strlen(str));
   
}

void write_park_log(Vehicle * v, int state)
{
  long ticks = v->initial_tick;
  int id = v->id;
  char observ[MAX_BUF];
  
  char str[LONG_BUF];
  
  
  switch(state)
  {
    case ENTERING:
    strcpy(observ, "entered");
    break;
    case EXITING:
    strcpy(observ, "exit");
    break;
    case FULL:
    strcpy(observ, "full");
    break;
    case CLOSED:
    strcpy(observ, "close");
    break;
    default:
    return;
    break;
  }
  
  if(state == EXITING)
  {
    sprintf(str, "%7li ; %5d ; %7d ; %s\n", ticks, free_places, id, observ);
  }
  else
  {
    sprintf(str, "%7li ; %5d ; %7d ; %s\n", ticks, free_places, id,observ);
  }
  
  write(log_park, str, strlen(str));
  
}

int main(int argc, const char * argv[])
{
    //variables declaration
    int fplaces;
    int duration;
    int fdN, fdS, fdW, fdE;
    pthread_t tidN, tidS, tidW, tidE;
    Vehicle* vehicle_stop = (Vehicle*) malloc(sizeof(Vehicle));
    
    //test arguments
    if(argc != 3)
    {
      printf("Usage %s <FREE_SPOTS_NUMBER> <TIME_UNTIL_CLOSE>\n", argv[0]);
      free(vehicle_stop);
      return 1;
    }
    closed = 0;
    
    //variable initialization
    free_places = fplaces = atoi(argv[1]);
    duration = atoi(argv[2]);
    create_park_log();
        
    //creating threads and executing
    pthread_create(&tidN, NULL, tcontroller_N, NULL); 
    pthread_create(&tidS, NULL, tcontroller_S, NULL); 
    pthread_create(&tidW, NULL, tcontroller_W, NULL); 
    pthread_create(&tidE, NULL, tcontroller_E, NULL); 
   
   
    sleep(duration);
    closed = 1;
    
    fdN = open(FIFON, O_WRONLY);
    fdS = open(FIFOS, O_WRONLY);
    fdW = open(FIFOW, O_WRONLY);
    fdE = open(FIFOE, O_WRONLY);
    
    //creating the stop vehicle and closing program
    vehicle_stop->id = 0;
    write(fdN, &vehicle_stop, sizeof(Vehicle));
    write(fdS, &vehicle_stop, sizeof(Vehicle));
    write(fdW, &vehicle_stop, sizeof(Vehicle));
    write(fdE, &vehicle_stop, sizeof(Vehicle));

    free(vehicle_stop);
    close(fdN);
    close(fdS);
    close(fdW);
    close(fdE);
    
    pthread_join(tidN,NULL);
    pthread_join(tidS,NULL);
    pthread_join(tidW,NULL);
    pthread_join(tidE,NULL);
    
    unlink(FIFON); unlink(FIFOS); unlink(FIFOE); unlink(FIFOW);
    pthread_exit(NULL);
    return 0;
}
