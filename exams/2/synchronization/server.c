#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <signal.h>

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
   shm_unlink(SHM_NUMBERS);
   sem_close(sem_clients);
   sem_unlink(SEM_CLIENTS);
   sem_close(sem_server);
   sem_unlink(SEM_SERVER);
   sem_close(sem_queue);
   sem_unlink(SEM_QUEUE);
}

void handle_signal(int signal){
	printf("caught signal\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv){

	atexit(terminate);

	struct sigaction s;
	s.sa_handler = handle_signal;
	s.sa_flags = 0;
	sigaction(SIGINT, &s, NULL);
	sigaction(SIGTERM, &s, NULL);

	//shm
	int fd = shm_open(SHM_NUMBERS, O_RDWR | O_CREAT, PERMISSION);
	ftruncate(fd, sizeof *shared);
	shared = mmap(NULL, sizeof *shared, PROT_READ | PROT_WRITE, PERMISSION, fd, 0);
	close(fd);

	//sema
	sem_clients = sem_open(SEM_CLIENTS, O_CREAT | O_EXCL, PERMISSION, 0);
	sem_server = sem_open(SEM_SERVER, O_CREAT | O_EXCL, PERMISSION, 0);
	sem_queue = sem_open(SEM_QUEUE, O_CREAT | O_EXCL, PERMISSION, 1);

	while(1){
		//wait until client wrote the numbers
		sem_wait(sem_clients);
		printf("sorting [%d, %d, %d, %d]\n", shared->toSort[0],shared->toSort[1],shared->toSort[2],shared->toSort[3]);
		//critical section start
		//NOTE: should generate here some random numbers, this is just for demonstration.
		shared->toSort[0] = 1;
		shared->toSort[1] = 4;
		shared->toSort[2] = 5;
		shared->toSort[3] = 6;
		//critical section end
	
		//client can now read the sorted numbers
		sem_post(sem_server);
	}
	
	
	return 0;
}
