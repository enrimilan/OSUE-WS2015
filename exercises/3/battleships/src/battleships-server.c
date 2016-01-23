/**
 * @file battleships-server.c
 * @author Enri Miho (0929003) <e0929003@student.tuwien.ac.at>
 * @date 10.01.2016
 * @brief the server for the battleships game
 * @details manages games between 2 clients, where they have to guess the position of each ohter's ship.
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
#include <signal.h>

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

/* === Type Definitions === */

/// Represents a property for a position.
typedef enum {FREE='.', BUSY='S'} PositionProp;

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

/* === Global Variables === */

/** Name of the program **/
static const char *progname = "battleships-server";

/** Board of player 1 **/
static PositionProp board1[DIMENSION][DIMENSION];

/** Board of player 2 **/
static PositionProp board2[DIMENSION][DIMENSION];

/** Data of shared memory object, used by the client to place the ship. **/
static struct registration_shm *registration_data;

/** Data of shared memory object, used by the client and the server for the game action. **/
static struct game_action_shm *game_action_data;

/** File descriptor**/
static int shm_registration_fd;

/** File descriptor**/
static int shm_game_action_fd;

/** Semaphore, which is used for "registration". **/
static sem_t *s1;
/** Semaphore, which is used to make the interaction between the clients occur alternately. **/
static sem_t *s2;
/** Semaphore, which is used to make the interaction between the clients occur alternately. **/
static sem_t *s3;
/** Semaphore, which is used to make the interaction between the client and the server occur alternately. **/
static sem_t *s4;
/** Semaphore, which is used to make the interaction between the client and the server occur alternately. **/
static sem_t *s5;

/* === Prototypes === */

/**
 * @brief clear the boards of both players
 */
static void clear_boards(void);

/**
 * @brief print the board of a player
 * @param board the board of the player to print
 */
static void print_board(PositionProp board[DIMENSION][DIMENSION]);

/**
 * @brief places the ship on a board, if possible
 * @param board the board of the played
 * @param x the x coordinate
 * @param y the y coordinate
 * @param positioning the positioning, may be {HORIZONTAL, VERTICAL, DIAGONAL1, DIAGONAL2}
 */
static void place_ship_on_board(PositionProp board[DIMENSION][DIMENSION], int x, int y, Positioning positioning);

/**
 * @brief allocate resources
 */
static void allocate_resources(void);

/**
 * @brief wait for the players to join
 */
static void wait_for_players_to_join(void);

/**
 * @brief wait for the players to place their ship
 */
static void wait_for_players_to_place_ship(void);

/**
 * @brief start playing
 */
static void play(void);

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

/**
 * @brief Signal handler
 * @param sig Signal number catched
 */
static void signal_handler(int sig);

/* === Implementations === */

int main(int argc, char *argv[]) {

	progname = argv[0];
	
	if(argc != 1){
		bail_out(EXIT_FAILURE,"Usage: %s", progname);
	}

	//setup signal handlers
    const int signals[] = {SIGINT, SIGTERM};
    struct sigaction s;

    s.sa_handler = signal_handler;
    s.sa_flags   = 0;
    if(sigfillset(&s.sa_mask) < 0) {
        bail_out(EXIT_FAILURE, "sigfillset");
    }
    for(int i = 0; i < 2; i++) {
        if (sigaction(signals[i], &s, NULL) < 0) {
            bail_out(EXIT_FAILURE, "sigaction");
        }
    }

	//register the termination function
   	if(atexit(free_resources) != 0){
   		(void) fprintf(stderr, "%s\n", "atexit error");
   		return EXIT_FAILURE;
   	}
	
	while(1){
		clear_boards();
		allocate_resources();
		(void) printf("New game\n");
		wait_for_players_to_join();
		wait_for_players_to_place_ship();
		play();
		free_resources();
	}

    return EXIT_SUCCESS;
}

static void clear_boards(void){
	for(int x=0; x<DIMENSION; x++){
		for(int y=0; y<DIMENSION; y++){
			board1[x][y] = FREE;
			board2[x][y] = FREE;
		}
	}	
}

static void print_board(PositionProp board[DIMENSION][DIMENSION]){
	for(int x=0; x<DIMENSION; x++){
		for(int y=0; y<DIMENSION; y++){
			printf("%c ", board[y][x]);
		}
		printf("\n");
	}	
}

static void place_ship_on_board(PositionProp board[DIMENSION][DIMENSION], int x, int y, Positioning positioning){
	if(x<0 || x>3 || y<0 || y>3){
		return;
	}
	if(positioning == HORIZONTAL){
		if(x-1>=0 && x-1<=3 && x+1>=0 && x+1<=3){
			board[x-1][y] = BUSY;
			board[x][y] = BUSY;
			board[x+1][y] = BUSY;
		}
	}
	else if(positioning == VERTICAL){
		if(y-1>=0 && y-1<=3 && y+1>=0 && y+1<=3){
			board[x][y+1] = BUSY;
			board[x][y] = BUSY;
			board[x][y-1] = BUSY;
		}
	}
	/*
	 * example:
	 *     s
	 *    s
	 *   s
	 */
	else if(positioning == DIAGONAL1){
		if(x+1>=0 && x+1<=3 && y-1>=0 && y-1<=3 && x-1>=0 && x-1<=3 && y+1>=0 && y+1<=3){
			board[x+1][y-1] = BUSY;
			board[x][y] = BUSY;
			board[x-1][y+1] = BUSY;
		}
	}
	/*
	 * example:
	 *     s
	 *      s
	 *       s
	 */
	else if(positioning == DIAGONAL2){
		if(x-1>=0 && x-1<=3 && y-1>=0 && y-1<=3 && x+1>=0 && x+1<=3 && y+1>=0 && y+1<=3){
			board[x-1][y-1] = BUSY;
			board[x][y] = BUSY;
			board[x+1][y+1] = BUSY;
		}
	}
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

	game_action_data->give_up_flag = 0;
	game_action_data->server_terminated_flag = 0;

	s1 = sem_open(SEM_1, O_CREAT | O_EXCL, PERMISSION, 0);
	s2 = sem_open(SEM_2, O_CREAT | O_EXCL, PERMISSION, 1);
	s3 = sem_open(SEM_3, O_CREAT | O_EXCL, PERMISSION, 0);
	s4 = sem_open(SEM_4, O_CREAT | O_EXCL, PERMISSION, 1);
	s5 = sem_open(SEM_5, O_CREAT | O_EXCL, PERMISSION, 0);
	if(s1 == SEM_FAILED || s2 == SEM_FAILED || s3 == SEM_FAILED || s4 == SEM_FAILED || s5 == SEM_FAILED){
		bail_out(EXIT_FAILURE, "sem_open");
	}
}

static void wait_for_players_to_join(void){
	(void) printf("Waiting for players to join ...\n");
	if(sem_wait(s1) == -1){
		bail_out(EXIT_FAILURE, "sem_wait");
	}
	(void) printf("Player 1 joined\n");
	
	if(sem_wait(s1) == -1){
		if(errno == EINTR){
			game_action_data->server_terminated_flag = 1;
			sem_post(s3);
		}
		bail_out(EXIT_FAILURE, "sem_wait");
	}
	(void) printf("Player 2 joined\n");
	
	//semaphore not needed anymore
	if(sem_close(s1) == -1){
		bail_out(EXIT_FAILURE, "sem_close");
	}
	if(sem_unlink(SEM_1) == -1){
		bail_out(EXIT_FAILURE, "sem_unlink");
	}
}

static void wait_for_players_to_place_ship(void){
	if(sem_wait(s2) == -1){
		bail_out(EXIT_FAILURE, "sem_wait");
	}
	if(sem_post(s3) == -1){
		bail_out(EXIT_FAILURE, "sem_post");
	}

	//first player
	if(sem_wait(s2) == -1){
		if(errno == EINTR){
			game_action_data->server_terminated_flag = 1;
			sem_post(s3);
		}
		bail_out(EXIT_FAILURE, "sem_wait");
	}
	place_ship_on_board(board1, registration_data->x, registration_data->y, registration_data->positioning);
	(void) printf("-PLAYER 1-\n");
	print_board(board1);
	if(sem_post(s3) == -1){
		bail_out(EXIT_FAILURE, "sem_post");
	}

	//second player
	if(sem_wait(s2) == -1){
		if(errno == EINTR){
			game_action_data->server_terminated_flag = 1;
			sem_post(s3);
		}
		bail_out(EXIT_FAILURE, "sem_wait");
	}
	place_ship_on_board(board2, registration_data->x, registration_data->y, registration_data->positioning);
	(void) printf("-PLAYER 2-\n");
	print_board(board2);
	if(sem_post(s3) == -1){
		bail_out(EXIT_FAILURE, "sem_post");
	}
}

static void play(void){
	int turn = 0;
	int player1_hits = 0;
	int player2_hits = 0;
	int playing = 1;
	while(playing){
		//read the guess
		if(sem_wait(s5) == -1){
			if(errno == EINTR){
				game_action_data->server_terminated_flag = 1;
				sem_post(s3);
				sem_post(s4);
			}
			bail_out(EXIT_FAILURE, "sem_wait");
		}
		game_action_data->response = MISS;
		if(turn == 0){
			if(board2[game_action_data->x][game_action_data->y] == BUSY){
				game_action_data->response = HIT;
				board2[game_action_data->x][game_action_data->y] = FREE;
				player1_hits++;
			}
			turn = 1;
		}
		else{
			if(board1[game_action_data->x][game_action_data->y] == BUSY){
				game_action_data->response = HIT;
				board1[game_action_data->x][game_action_data->y] = FREE;
				player2_hits++;
			}
			turn = 0;
		}
		if(player1_hits == 3 || player2_hits == 3){
			game_action_data->response = WON;
			playing = 0;
		}
		if(sem_post(s4) == -1){
			bail_out(EXIT_FAILURE, "sem_post");
		}


		if(game_action_data->give_up_flag == 1){
			game_action_data->response = WALKOVER;
			playing = 0;
			if(sem_post(s4) == -1){
				bail_out(EXIT_FAILURE, "sem_post");
			}
		}
		else{
			if(sem_wait(s5) == -1){
				bail_out(EXIT_FAILURE, "sem_wait");
			}
			if(sem_post(s4) == -1){
				bail_out(EXIT_FAILURE, "sem_post");
			}
		}
		
		if(player1_hits == 3 || player2_hits == 3){
			playing = 0;
			game_action_data->response = LOST;
		}

		if(sem_wait(s2) == -1){
			bail_out(EXIT_FAILURE, "sem_wait");
		}
		if(sem_post(s3) == -1){
			bail_out(EXIT_FAILURE, "sem_post");
		}
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
	(void) shm_unlink(SHM_REGISTRATION);
	(void) shm_unlink(SHM_GAME_ACTION);
	(void) sem_close(s1);
	(void) sem_unlink(SEM_1);
	(void) sem_close(s2);
	(void) sem_unlink(SEM_2);
	(void) sem_close(s3);
	(void) sem_unlink(SEM_3);
	(void) sem_close(s4);
	(void) sem_unlink(SEM_4);
	(void) sem_close(s5);
	(void) sem_unlink(SEM_5);
}

static void signal_handler(int sig){
	switch(sig){
      case SIGINT:
         fprintf(stderr, "\ncaught signal SIGINT\n");
         break;
      case SIGTERM:
         fprintf(stderr, "\ncaught signal SIGTERM\n");
         break;
   }
}
