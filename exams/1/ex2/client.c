/**
 * @file client.c
 * @author OSUE Team <osue-team@vmars.tuwien.ac.at>
 * @date 17.10.2012
 *
 * @brief TCP/IP client for Exercise 2
 *
 * This program implements a TCP/IP client, which either sends a status request or a
 * shutdown command to a server. If communication with the main server fails, the
 * client tries to send the message to the backup server.
 **/
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "common.h"

/** textual representation of the server's IP address */
#define SERVER_IPADDR_STR				"127.0.0.1"

/** type of a client message */
enum mode_t { mode_unset, mode_request, mode_shutdown };

/** name of the executable (for printing messages) */
static char *program_name = "client";

/** print usage message and exit */
static void usage(void);


/** parse string describing port number
 * @param str the textual representation of the port number (1-65535) in base 10
 * @return a positive integer if successful, and zero on failure
 */
static uint16_t parse_port_number(char *str);

/** program entry point */
int main(int argc, char **argv)
{
    enum mode_t mode = mode_unset;      /* whether client sends status request or shutdown */
    uint16_t server_port = 0;   /* port of main server */
    uint16_t backup_port = 0;   /* port of backup server */
    msg_t message;              /* client message */

    /* handle options */
    if (argc > 0) {
        program_name = argv[0];
    }
    int getopt_result;
    while ((getopt_result = getopt(argc, argv, "p:b:rs")) != -1) {
        switch (getopt_result) {
        case 'r':
        	if (mode != mode_unset) {
                usage();
            }
            mode = (getopt_result == 's') ? mode_shutdown : mode_request;
            break;
        case 's':
            if (mode != mode_unset) {
                usage();
            }
            mode = (getopt_result == 'r') ? mode_request : mode_shutdown;
            break;
        case 'p':
            if (server_port > 0) {
                usage();
            }
            if ((server_port = parse_port_number(optarg)) == 0) {
                (void) fprintf(stderr, "Bad port number: %s\n", optarg);
                usage();
            }
            break;
        case 'b':
            if (backup_port > 0) {
                usage();
            }
            if ((backup_port = parse_port_number(optarg)) == 0) {
                (void) fprintf(stderr, "Bad port number: %s\n", optarg);
                usage();
            }
            break;
        case '?':
            usage();
            break;
        default:
            assert(0);
        }
    }
    /********************************/
    /* TODO: insert your code below */
    /********************************/

    /* Check that there are no additional (superfluous) arguments and
       that all required options (-r or -s, and -p) are present. */
       if(mode == mode_unset){
			usage();
       }
       if(server_port==0){
			usage();
       }
       if(argc-optind>0){
			usage();
       }

    /* Create socket */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
     
    /* Resolve host / convert IP String */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    inet_pton(AF_INET, SERVER_IPADDR_STR, &addr.sin_addr);
    

    /* Connect to server on server_port */
    /* On error, try to connect to backup_port (if specified) */
    if(connect(sockfd, (struct sockaddr *) &addr, sizeof(addr))<0){
		if(backup_port!=0){
			addr.sin_port = htons(backup_port);
			if(connect(sockfd, (struct sockaddr *) &addr, sizeof(addr))<0){
				usage();
			}
		}
		else{
			usage();
		}
    }


    /* prepare message, and send it to the server */
    (void) memset(message, 0x0, sizeof(msg_t));
    if(mode == mode_shutdown){
    	strncpy(message, "shutdown", 8);
		send(sockfd, message, sizeof(msg_t) ,0);
    }
    /* In mode -r, receive and print reply (followed by newline) */
    if(mode == mode_request){
		strncpy(message, "request", 7);
		send(sockfd, message, sizeof(msg_t) ,0);
		(void) memset(message, 0x0, sizeof(msg_t));
		recv(sockfd, message, sizeof(msg_t), 0);
		printf("%s\n", message);
    }


    return EXIT_SUCCESS;
}

/* Do not edit source code below this line */
/*-----------------------------------------*/

static void usage(void)
{
    (void) fprintf(stderr, "Usage: %s -p PORT [-b PORT] [-r|-s]\n",
                   program_name);
    exit(EXIT_FAILURE);
}

static uint16_t parse_port_number(char *str)
{
    long val;
    char *endptr;

    val = strtol(str, &endptr, 10);
    if (endptr == str || *endptr != '\0') {
        return 0;
    }
    if (val <= 0 || val > 65535) {
        return 0;
    }
    return (uint16_t) val;
}
