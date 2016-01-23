/**
 * @file battleships-client.c
 * @author Enri Miho (0929003) <e0929003@student.tuwien.ac.at>
 * @date 10.01.2016
 * @brief the client for the battleships game
 * @details guesses where the ship (length 3) of the opponent could be. If it hits it 3 times, this client won.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <semaphore.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

/* === Constants === */
/// The maximum number of rows and columns of the board (4x4).
#define DIMENSION (4)
/// Name of shared memory object, used by the client to place the ship.
#define SHM_REGISTRATION "battleships_registration_shm"
/// Name of shared memory object, used by the client and the server for the game action.
#define SHM_GAME_ACTION "battleships_game_action_shm"
/// Permission.
#define PERMISSION (0600)
/// Name of the semaphore, which is used for "registration".
#define SEM_1 "/battleships_sem_1"
/// Name of the semaphore, which is used to make the interaction between the clients occur alternately.
#define SEM_2 "/battleships_sem_2"
/// Name of the semaphore, which is used to make the interaction between the clients occur alternately.
#define SEM_3 "/battleships_sem_3"
/// Name of the semaphore, which is used to make the interaction between the client and the server occur alternately.
#define SEM_4 "/battleships_sem_4"
/// Name of the semaphore, which is used to make the interaction between the client and the server occur alternately.
#define SEM_5 "/battleships_sem_5"

/* === Type definitions === */

/// Represents a property for a position.
typedef enum {FREE='.', BUSY='S', SHOT_HIT='X', SHOT_MISS='O'} PositionProp;

/// Positioning mode.
typedef enum {HORIZONTAL, VERTICAL, DIAGONAL1, DIAGONAL2} Positioning;

/// A response from the server.
typedef enum {HIT, MISS, WON, WALKOVER, LOST} Response;

/**
 * A structure to represent the registration data.
 */
struct registration_shm {
	int x;
	int y;
	Positioning positioning;
};

/**
 * A structure to represent the game action data.
 */
struct game_action_shm {
	int x;
	int y;
	int give_up_flag;
	Response response;
	int server_terminated_flag;
};

/* === Global variables === */

/** Name of the program **/
static const char *progname = "battleships-client";

/** The board of the player **/
static PositionProp board[DIMENSION][DIMENSION];

/** Data of shared memory object, used by the client to place the ship. **/
struct registration_shm *registration_data;

/** Data of shared memory object, used by the client and the server for the game action. **/
struct game_action_shm *game_action_data;

/** File descriptor**/
int shm_registration_fd;

/** File descriptor**/
int shm_game_action_fd;

/** Semaphore, which is used for "registration". **/
sem_t *s1;
/** Semaphore, which is used to make the interaction between the clients occur alternately. **/
sem_t *s2;
/** Semaphore, which is used to make the interaction between the clients occur alternately. **/
sem_t *s3;
/** Semaphore, which is used to make the interaction between the client and the server occur alternately. **/
sem_t *s4;
/** Semaphore, which is used to make the interaction between the client and the server occur alternately. **/
sem_t *s5;


/* === Prototypes === */

/**
 * @brief allocate resources
 */
static void allocate_resources(void);

/**
 * @brief join and place the ship
 */
static void join_and_place_ship(void);

/**
 * @brief start playing
 */
static void play(void);

/**
 * @brief print the board (shows misses, hits and so on)
 */
static void print_board(void);

/**
 * @brief terminate program on program error
 * @param exitcode exit code
 * @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

/**
 * @brief free allocated resources
 */
static void free_resources(void);

/* === Implementations === */

int main(int argc, char **argv){

	progname = argv[0];
	
	if(argc != 1){
		bail_out(EXIT_FAILURE,"Usage: %s", progname);
	}

	//register the termination function
   	if(atexit(free_resources) != 0){
   		(void) fprintf(stderr, "%s\n", "atexit error");
   		return EXIT_FAILURE;
   	}
	
	//fill the table
	for(int x=0; x<DIMENSION; x++){
		for(int y=0; y<DIMENSION; y++){
			board[x][y] = FREE;
		}
	}	
	
	allocate_resources();
	join_and_place_ship();
	play();
	
	return EXIT_SUCCESS;
}

static void allocate_resources(void){
	
	shm_registration_fd = shm_open(SHM_REGISTRATION, O_RDWR | O_CREAT, PERMISSION);
	shm_game_action_fd = shm_open(SHM_GAME_ACTION, O_RDWR | O_CREAT, PERMISSION);
	if(shm_registration_fd == -1 || shm_game_action_fd == -1){
		bail_out(EXIT_FAILURE, "shm_open");
	}
	if (ftruncate(shm_registration_fd, sizeof *registration_data) == -1){
		bail_out(EXIT_FAILURE, "ftruncate");
	}
	if (ftruncate(shm_game_action_fd, sizeof *game_action_data) == -1){
		bail_out(EXIT_FAILURE, "ftruncate");
	}
	registration_data = mmap(NULL, sizeof *registration_data, PROT_READ | PROT_WRITE, MAP_SHARED, shm_registration_fd, 0);
	game_action_data = mmap(NULL, sizeof *game_action_data, PROT_READ | PROT_WRITE, MAP_SHARED, shm_game_action_fd, 0);
	if(registration_data == MAP_FAILED || game_action_data == MAP_FAILED){
		bail_out(EXIT_FAILURE, "mmap");
	}
	if (close(shm_registration_fd) == -1 || close(shm_game_action_fd) == -1){
		bail_out(EXIT_FAILURE, "close");
	}

	s1 = sem_open(SEM_1, 0);
	s2 = sem_open(SEM_2, 0);
	s3 = sem_open(SEM_3, 0);
	s4 = sem_open(SEM_4, 0);
	s5 = sem_open(SEM_5, 0);
	if(s1 == SEM_FAILED || s2 == SEM_FAILED || s3 == SEM_FAILED || s4 == SEM_FAILED || s5 == SEM_FAILED){
		bail_out(EXIT_FAILURE, "sem_open");
	}
}

static void join_and_place_ship(void){
	//connect to server
	if(sem_post(s1) == -1){
		bail_out(EXIT_FAILURE, "sem_post");
	}

	//place the ship when ready
	(void) printf("Successfully joined. Waiting to place the ship ...\n");
	if(sem_wait(s3) == -1){
		bail_out(EXIT_FAILURE, "sem_wait");
	}
	if(game_action_data->server_terminated_flag == 1){
		bail_out(EXIT_FAILURE, "server terminated unexpectedly");
	}
	char position_data[10];
	(void) printf("Please enter your ship position and orientation (format x y [0|1|2|3])\n");
	(void) printf("0 = HORIZONTAL\n");
	(void) printf("1 = VERTICAL\n");
	(void) printf("2 = DIAGONAL1\n");
	(void) printf("3 = DIAGONAL2\n");
	(void) printf("Your position data: ");
	(void) fgets (position_data, 10, stdin);
	if(game_action_data->server_terminated_flag == 1){
		bail_out(EXIT_FAILURE, "server terminated unexpectedly");
	}
	//CRITICAL SECTION BEGIN
	registration_data->x = position_data[0] - '0';
	registration_data->y = position_data[2] - '0';
	registration_data->positioning = position_data[4] - '0';
	//CRITICAL SECTION END
	
	if(sem_post(s2) == -1){
		bail_out(EXIT_FAILURE, "sem_post");
	}
}

static void play(void){
	char guess_data[10];
	int hits = 0;
	//start playing
	while(1){

		if(sem_wait(s3) == -1){
			bail_out(EXIT_FAILURE, "sem_wait");
		}

		if(sem_wait(s4) == -1){
			bail_out(EXIT_FAILURE, "sem_wait");
		}

		if(game_action_data->server_terminated_flag == 1){
			bail_out(EXIT_FAILURE, "server terminated unexpectedly");
		}

		if(game_action_data->response == LOST){
			(void) printf("YOU LOST :(\n");
			if(sem_post(s5) == -1){
				bail_out(EXIT_FAILURE, "sem_post");
			}
			break;
		}
		else if(game_action_data->response == WALKOVER){
			(void) printf("Your oppopnent gave up. YOU WON :)\n");
			if(sem_post(s5) == -1){
				bail_out(EXIT_FAILURE, "sem_post");
			}
			break;
		}
		(void) printf("\n");
		(void) printf("It's your turn!\n");
		(void) printf("YOUR BOARD:\n");
		print_board();
		(void) printf("Hits: %d\n", hits);
		(void) printf("POSSIBLE OPTIONS:\n");
		(void) printf("1. Enter position where to shoot (x y)\n");
		(void) printf("2. Give up (q)\n");
		(void) printf("Enter your input: ");
		while(1){
			(void) fgets (guess_data, 10, stdin);
			if(guess_data[0] == 'q'){
				break;
			}
			else if(guess_data[0] - '0'>= 0 && guess_data[0] - '0'<= 3 && guess_data[2] - '0'>= 0 && guess_data[2] - '0'<= 3){
				break;
			}
			else{
				(void) printf("Input invalid. Try again: ");
			}
		}
		if(game_action_data->server_terminated_flag == 1){
			bail_out(EXIT_FAILURE, "server terminated unexpectedly");
		}
		//CRITICAL SECTION BEGIN
		if(guess_data[0] == 'q'){
			game_action_data->give_up_flag = 1;
			if(sem_post(s5) == -1){
				bail_out(EXIT_FAILURE, "sem_post");
			}
			if(sem_post(s2) == -1){
				bail_out(EXIT_FAILURE, "sem_post");
			}
			break;
		}
		else{
			game_action_data->x = guess_data[0] - '0';
			game_action_data->y = guess_data[2] - '0';
		}
		
		//CRITICAL SECTION END
		if(sem_post(s5) == -1){
			bail_out(EXIT_FAILURE, "sem_post");
		}
			
		if(sem_wait(s4) == -1){
			bail_out(EXIT_FAILURE, "sem_wait");
		}
		if(game_action_data->server_terminated_flag == 1){
			bail_out(EXIT_FAILURE, "server terminated unexpectedly");
		}
		//CRITICAL SECTION BEGIN
		if(game_action_data->response == WON){
			(void) printf("***YOU WON***\n");
			board[game_action_data->x][game_action_data->y] = SHOT_HIT;
			print_board();
			hits++;
			if(sem_post(s5) == -1){
				bail_out(EXIT_FAILURE, "sem_post");
			}
			if(sem_post(s2) == -1){
				bail_out(EXIT_FAILURE, "sem_post");
			}
			break;
		}
		else if(game_action_data->response == HIT){
			(void) printf("IT'S A HIT!!!\n");
			board[game_action_data->x][game_action_data->y] = SHOT_HIT;
			print_board();
			hits++;
		}
		else if(game_action_data->response == MISS){
			(void) printf("it's a miss :(\n");
			if(board[game_action_data->x][game_action_data->y] != SHOT_HIT){
				board[game_action_data->x][game_action_data->y] = SHOT_MISS;
			}
			print_board();
		}
		//CRITICAL SECTION END
		
		if(sem_post(s5) == -1){
			bail_out(EXIT_FAILURE, "sem_post");
		}
		if(sem_post(s2) == -1){
			bail_out(EXIT_FAILURE, "sem_post");
		}
	}
}

static void print_board(void){
	for(int x=0; x<DIMENSION; x++){
		for(int y=0; y<DIMENSION; y++){
			printf("%c ", board[y][x]);
		}
		printf("\n");
	}	
}

static void bail_out(int exitcode, const char *fmt, ...){

    va_list ap;

    (void) fprintf(stderr, "%s: ", progname);
    if (fmt != NULL) {
        va_start(ap, fmt);
        (void) vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    if (errno != 0) {
        (void) fprintf(stderr, ": %s", strerror(errno));
    }
    (void) fprintf(stderr, "\n");
    exit(exitcode);
}

static void free_resources(void){
	(void) close(shm_registration_fd); 
	(void) close(shm_game_action_fd);
	(void) munmap(registration_data, sizeof *registration_data);
	(void) munmap(game_action_data, sizeof *game_action_data);
	(void) sem_close(s1);
	(void) sem_close(s2);
	(void) sem_close(s3);
	(void) sem_close(s4);
	(void) sem_close(s5);
}
