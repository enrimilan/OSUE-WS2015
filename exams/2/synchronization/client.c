#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <fcntl.h>

#define SHM_NUMBERS "/shm_numbers"
#define PERMISSION (0600)
#define SEM_CLIENTS "/sem_clients"
#define SEM_SERVER "/sem_server"
#define SEM_QUEUE "/sem_queue"

struct numbers {
	int toSort[4];
};

struct numbers *shared;
sem_t *sem_clients;
sem_t *sem_server;
sem_t *sem_queue;

void terminate(){
   munmap(shared, sizeof *shared);
   sem_close(sem_clients);
   sem_close(sem_server);
   sem_close(sem_queue);
}

int main(int argc, char** argv){

	atexit(terminate);

	//shm
	int fd = shm_open(SHM_NUMBERS, O_RDWR | O_CREAT, PERMISSION);
	ftruncate(fd, sizeof *shared);
	shared = mmap(NULL, sizeof *shared, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	//sema
	sem_clients = sem_open(SEM_CLIENTS, 0);
	sem_server = sem_open(SEM_SERVER, 0);
	sem_queue = sem_open(SEM_QUEUE, 0);

	sem_wait(sem_queue);
	
	//write the numbers
	//critical section start
	//NOTE: the numbers should be sorted here, this is just for demonstration.
	shared->toSort[0] = 5;
	shared->toSort[1] = 4;
	shared->toSort[2] = 6;
	shared->toSort[3] = 1;
	//critical section end

	sem_post(sem_clients);

	//wait for the server to sort
	sem_wait(sem_server);
	//critical section start
	printf("sorted [%d, %d, %d, %d]\n", shared->toSort[0],shared->toSort[1],shared->toSort[2],shared->toSort[3]);
	//critical section end

	sem_post(sem_queue);
		
	return 0;
}
