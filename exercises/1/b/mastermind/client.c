/**
 * @module client.c
 * @author Enri Miho - 0929003
 * @brief the client for the mastermind game
 * @date 05.11.2015
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>

/* === Constants === */

#define SLOTS (5)
#define READ_BYTES (1)
#define WRITE_BYTES (2)
#define PARITY_ERR_BIT (6)
#define GAME_LOST_ERR_BIT (7)
#define EXIT_PARITY_ERROR (2)
#define EXIT_GAME_LOST (3)
#define EXIT_MULTIPLE_ERRORS (4)

/* === Global variables === */

/* Name of the program */
static const char *progname = "client";

/*File descriptor for client socket*/
static int sockfd = -1;

/* === Type definitions === */

struct opts{
	char *server_hostname;
	char *server_portno;
};

/* === Prototypes === */

/**
 * @brief Parse command line options
 * @param argc The argument counter
 * @param argv The argument vector
 * @param options Struct where parsed arguments are stored
 */
static void parse_args(int argc, char **argv, struct opts *options);

/**
 * @brief Connects to the server
 * @param options Struct where parsed arguments are stored
 * @details If the socket is setup correctly, sockfd ist set
 */
static void connect_to_server(struct opts *options);

/**
 * @brief Generates a random 16 bit number, where bit 15 is the parity bit of bits 0-14
 * @return The generated random number
 */
static uint16_t generate_random_number(void);

/**
 * @brief Terminate the program
 * @param exitcode
 * @param fmt
 * @details The name of the program, stored in the variable progname is used here
 */
static void bail_out(int exitcode, const char *fmt, ...);

/**
 * @brief free allocated resources
 */
static void free_resources(void);

/* === Implementations === */

static void parse_args(int argc, char **argv, struct opts *options){
	
	if(argc > 0) {
        progname = argv[0];
    }
	
	if(argc != 3){
		bail_out(EXIT_FAILURE, "Usage: %s <server-hostname> <server-port>", progname);
	}
	
	//parse server hostname and port
	options->server_hostname = argv[1];
	options->server_portno = argv[2];
	
	//verify the port
	char *endptr;
	errno = 0;
	long int port = strtol(argv[2],&endptr,10);
	if(errno == ERANGE || errno !=0){
		bail_out(EXIT_FAILURE, "strtol");
	}
	if(endptr == argv[2]){
		bail_out(EXIT_FAILURE, "port has no digits");
	}
	if (*endptr != '\0') {
        bail_out(EXIT_FAILURE, "Further characters after <server-port>: %s", endptr);
    }
	if(port<1 || port>65535){
		bail_out(EXIT_FAILURE, "invalid port range");
	}
}

static void connect_to_server(struct opts *options){
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		bail_out(EXIT_FAILURE, "socket");
    }
	
	struct addrinfo hints;
	(void) memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;    
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
	struct addrinfo *server;
	int s = getaddrinfo(options->server_hostname, options->server_portno, &hints, &server);
    if (s != 0) {
		bail_out(EXIT_FAILURE, "getaddrinfo: %s\n", gai_strerror(s));
    }
	errno = 0; //workaround, for some reason errno is set here to ENOENT when using localhost
	if(connect(sockfd, server->ai_addr, server->ai_addrlen)<0){
		bail_out(EXIT_FAILURE, "connect");
	}
	freeaddrinfo(server); /* No longer needed */
}

static uint16_t generate_random_number(void){ 
	
	uint16_t random_number = rand() % 32768; // 32767 = 0111111111111111
	uint16_t one = 1; //0000000000000001 (used for masking) 
	uint16_t parity = one & random_number; //most right bit
	for(int i = 1; i<15; i++){
		parity = ((random_number >> i) & one) ^ parity;
	}
	random_number = random_number | (parity << 15);
	return random_number;
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

    free_resources();
    exit(exitcode);
}

static void free_resources(void){
    /* clean up resources */
    if(sockfd >= 0) {
        (void) close(sockfd);
    }
}

/**
 * @brief Program entry point
 * @param argc The argument counter
 * @param argv The argument vector
 * @return EXIT_SUCCESS on success, EXIT_PARITY_ERROR in case of an parity
 * error, EXIT_GAME_LOST in case client needed to many guesses,
 * EXIT_MULTIPLE_ERRORS in case multiple errors occured in one round
 */
int main(int argc, char **argv){
	
	struct opts options;
	parse_args(argc, argv, &options);
	connect_to_server(&options);
	srand(time(NULL));
	uint8_t response = 0;
	int round = 1;
	
	while(1){
		
		uint16_t random_number = generate_random_number();
		//send guess to the server
		if(send(sockfd, &random_number, WRITE_BYTES, 0)< WRITE_BYTES){
			bail_out(EXIT_FAILURE, "send_to_server");
    	}
    	//receive response from the server
    	if(recv(sockfd, &response, READ_BYTES, 0) < READ_BYTES){
			bail_out(EXIT_FAILURE, "read_from_server");
    	}
		
		//check the bits
		if(((response & (1 << PARITY_ERR_BIT))>0) && ((response & (1 << GAME_LOST_ERR_BIT))>0)){
			bail_out(EXIT_MULTIPLE_ERRORS,"Parity error\nGame lost");
		}
		if((response & (1 << PARITY_ERR_BIT))>0){
			bail_out(EXIT_PARITY_ERROR,"Parity error");
		}
		if((response & (1 << GAME_LOST_ERR_BIT))>0){
			bail_out(EXIT_GAME_LOST,"Game lost");
		}
		
		//check if the client won the game
		if((7&response) == SLOTS){
			(void) printf("Runden: %d\n", round);
			break;
		}
		
		round++;
	}
	
	free_resources();
	return EXIT_SUCCESS;
}
