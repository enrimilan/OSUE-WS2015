/**
 * @file forksort.c
 * @author Enri Miho (0929003) <e0929003@student.tuwien.ac.at>
 * @date 2015-11-22
 * @brief A forksort implementation which sorts a given number of strings.
 * @details Forksort works like mergesort. Everytime we split the inputs, the actual process is forked. This is done 
 * until the number of strings is equal to one. Then the sorted strings are merged back. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>

/* === Constants === */

/// The maximum length of a string. The 62th character can be used for null termination if needed
#define MAX_LENGTH (62+1)

/* === Global Variables === */

/** Name of the program **/
static const char *progname = "forksort";

/** 
 * fd1[0] is used by the first child to read (redirected stdin);
 * fd1[1] is used by the parent to write to the redirected stdin of the first child
 */
static int fd1[2];

/** 
 * fd2[0] is used by the parent to read from the redirected stdout of the first child;
 * fd2[1] is used by the first child to write (redirected stdout) 
 */
static int fd2[2]; 

/** 
 * fd3[0] is used by the second child to read (redirected stdin);
 * fd3[1] is used by the parent to write to the redirected stdin of the second child
 */
static int fd3[2];

/** 
 * fd4[0] is used by the parent to read from the redirected stdout of the second child;
 * fd4[1] is used by the second child to write (redirected stdout) 
 */
static int fd4[2];

/** The read strings **/
static char** strings;

/** The first half of the read strings **/
static char** sub_strings1;

/** The second half of the read strings **/
static char **sub_strings2;

/* === Prototypes === */

/**
 * @brief Forks the process and redirects stdin and stdout of the first child to file descriptors.
 * @param strings The entire list of strings
 * @param sub_strings1 The first of half of the entire list of strings which is written to the pipe from which the first child reads
 * @param strings_size The size of the entire list of strings
 * @details The forked child will use redirected stdin and stdout and recall the forksort program from the beginning. 
 * The parent will write through a pipe the first half of the entire list of strings and will wait until the child has sorted it. 
 * After the child has finished the parent will read through another pipe the result and will store it to sub_strings1.
 */
static void forked_child1(char **strings, char **sub_strings1, long int strings_size);

/**
 * @brief Forks the process and redirects stdin and stdout of the second child to file descriptors.
 * @param strings The entire list of strings
 * @param sub_strings2 The second of half of the entire list of strings which is written to the pipe from which the second child reads
 * @param strings_size The size of the entire list of strings
 * @details The forked child will use redirected stdin and stdout and recall the forksort program from the beginning. 
 * The parent will write through a pipe the second half of the entire list of strings and will wait until the child has sorted it. 
 * After the child has finished the parent will read through another pipe the result and will store it to sub_strings2.
 */
static void forked_child2(char **strings, char **sub_strings2, long int strings_size);

/**
 * @brief Performs the mergesort algorithm on 2 given arrays of strings.
 * @param sub_strings1 the first array of strings
 * @param sub_strings1_size the number of elements of the first array
 * @param sub_strings2 the second array of strings
 * @param sub_strings2_size the number of elements of the second array
 * @details The two given arrays are merged, sorted and printed out to the stream using filedescriptor number 1
 */
static void merge_sort(char **sub_strings1, long int sub_strings1_size, char **sub_strings2, long int sub_strings2_size);

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

/**
 * @brief Program entry point
 * @param argc The argument counter
 * @param argv The argument vector
 * @return EXIT_SUCCESS on success, EXIT_FAILURE in case of an error
 * @details Parses the number of strings and reads the strings
 */
int main(int argc, char **argv){

	progname = argv[0];

	//check program has only 1 argument
	if(argc != 1){
		bail_out(EXIT_FAILURE,"Usage: %s", progname);
	}
	//parse the number of strings
	char num_of_elements_str[MAX_LENGTH];
	if((fgets(num_of_elements_str, MAX_LENGTH, stdin))==NULL){
		bail_out(EXIT_FAILURE, "fgets");
	}
	
	char *endptr;
	long int num_of_elements = strtol(num_of_elements_str ,&endptr, 10);

	if((errno == ERANGE && (num_of_elements == LONG_MAX || num_of_elements == LONG_MIN)) || errno != 0){
		bail_out(EXIT_FAILURE, "strtol");
	}

	if (endptr == num_of_elements_str){
		bail_out(EXIT_FAILURE, "No digits were found");
	}

	if(*endptr != '\n'){
		bail_out(EXIT_FAILURE, "Further characters after number found: %s", endptr);
	}

	if(num_of_elements<1){
		bail_out(EXIT_FAILURE, "Number of elements has to be greater than 0");
	}

	//read the strings
	strings = malloc(num_of_elements * sizeof(char*));
	if(strings == NULL){
		bail_out(EXIT_FAILURE, "malloc");
	}
	long int count = 0;
	while(count<num_of_elements){
		strings[count] = malloc(MAX_LENGTH * sizeof(char));
		if(strings[count] == NULL){
			bail_out(EXIT_FAILURE, "malloc");
		}
		if(fgets(strings[count], MAX_LENGTH, stdin)==NULL){
			bail_out(EXIT_FAILURE, "fgets");
		}
		count++;
	}

	if(num_of_elements == 1){
		(void) fprintf(stdout, "%s", strings[0]);
	}
	else{
		//create 4 unnamed pipes for reading and writing
		if(pipe(fd1)<0 || pipe(fd2)<0 || pipe(fd3)<0 || pipe(fd4)<0){
			bail_out(EXIT_FAILURE, "pipe");
		}
		sub_strings1 = malloc((num_of_elements/2)*sizeof(char*));
		sub_strings2 = malloc((num_of_elements-num_of_elements/2)*sizeof(char*));
		if(sub_strings1 == NULL || sub_strings2 == NULL){
			bail_out(EXIT_FAILURE, "malloc");
		}

		//fork
		forked_child1(strings, sub_strings1, num_of_elements);
		forked_child2(strings, sub_strings2, num_of_elements);
		
		//merge the results
		merge_sort(sub_strings1, num_of_elements/2, sub_strings2, num_of_elements-num_of_elements/2);
	}
	
	free_resources();
	return EXIT_SUCCESS;
}

static void forked_child1(char **strings, char **sub_strings1, long int strings_size){

	long int sub_strings1_size = strings_size/2;
	pid_t pid = fork();

	if(pid==0){
		//redirect stdin and stdout to the file descriptors
		if(dup2(fd1[0], fileno(stdin))<0){
			bail_out(EXIT_FAILURE, "dup2");
		}
		if(dup2(fd2[1], fileno(stdout))<0){
			bail_out(EXIT_FAILURE, "dup2");
		}
		//close unused endpoints
		(void) close(fd1[0]);
		(void) close(fd1[1]);
		(void) close(fd2[0]);
		(void) close(fd2[1]);
		if(execlp("./forksort", "forksort", NULL)<0){
			bail_out(EXIT_FAILURE, "execlp");
		}
	}
	else if(pid>0){
		//close unused endpoints
		(void) close(fd1[0]);
		(void) close(fd2[1]);
		
		//write number of sub_strings1 to the redirected stdin of child1
		char number[MAX_LENGTH];
		(void) sprintf(number, "%li\n", sub_strings1_size);
		if(write(fd1[1], number, strlen(number))<0){
			bail_out(EXIT_FAILURE, "write");
		}
		
		//now write half of the data to the redirected stdin of child1
		for(long int i=0; i<sub_strings1_size; i++){
			if(write(fd1[1], strings[i], strlen(strings[i]))<0){
				bail_out(EXIT_FAILURE, "write");
			}
		}
		(void) close(fd1[1]);
		
		//wait until child1 finished
		int status;
		pid_t wpid;
		while((wpid = wait(&status)) != pid){
        	if(wpid != -1){
				continue;
        	}
			if(errno == EINTR){
				continue;
			}
		}

		//at this point, child1 finished work. now get from its stdout the data
		FILE *file = fdopen(fd2[0], "r");
		if(file == NULL){
			bail_out(EXIT_FAILURE, "fdopen");
		}
		long int count = 0;
		while(count<sub_strings1_size){
			sub_strings1[count] = malloc(MAX_LENGTH*sizeof(char));
			if(sub_strings1[count] == NULL){
				bail_out(EXIT_FAILURE, "malloc");
			}
			if(fgets(sub_strings1[count], MAX_LENGTH, file) == NULL){
				bail_out(EXIT_FAILURE, "fgets");
			}
			count++;
		}
		(void) fclose(file);
		(void) close(fd2[0]);
		
	}
	else{
		bail_out(EXIT_FAILURE, "fork");
	}
}

static void forked_child2(char **strings, char **sub_strings2, long int strings_size){

	long int sub_strings2_size = strings_size-strings_size/2;
	pid_t pid = fork();

	if(pid==0){
		//redirect stdin and stdout to the file descriptors
		if(dup2(fd3[0], fileno(stdin))<0){
			bail_out(EXIT_FAILURE, "dup2");
		}
		if(dup2(fd4[1], fileno(stdout))<0){
			bail_out(EXIT_FAILURE, "dup2");
		}
		//close unused endpoints
		(void) close(fd3[0]);
		(void) close(fd3[1]);
		(void) close(fd4[0]);
		(void) close(fd4[1]);
		if(execlp("./forksort", "forksort", NULL)<0){
			bail_out(EXIT_FAILURE, "execlp");
		}
	}
	else if(pid>0){
		(void) close(fd3[0]);
		(void) close(fd4[1]);
		
		//write number of sub_strings2 to the redirected stdin of child2
		char number[MAX_LENGTH];
		(void) sprintf(number, "%li\n", sub_strings2_size);
		if(write(fd3[1], number, strlen(number))<0){
			bail_out(EXIT_FAILURE, "write");
		}
		
		//now write half of the data to the redirected stdin of child2
		for(long int i=strings_size/2; i<strings_size; i++){
			if(write(fd3[1], strings[i], strlen(strings[i]))<0){
				bail_out(EXIT_FAILURE, "write");
			}
		}
		(void) close(fd3[1]);
		
		//wait until child2 finished
		int status;
		pid_t wpid;
		while((wpid = wait(&status)) != pid){
        	if(wpid != -1){
				continue;
        	}
			if(errno == EINTR){
				continue;
			}
		}

		//at this point, child2 finished work. now get from its stdout the data
		FILE *file = fdopen(fd4[0], "r");
		if(file == NULL){
			bail_out(EXIT_FAILURE, "fdopen");
		}
		long int count = 0;
		while(count<sub_strings2_size){
			sub_strings2[count] = malloc(MAX_LENGTH*sizeof(char));
			if(sub_strings2[count] == NULL){
				bail_out(EXIT_FAILURE, "malloc");
			}
			if(fgets(sub_strings2[count], MAX_LENGTH, file) == NULL){
				bail_out(EXIT_FAILURE, "fgets");
			}
			count++;
		}
		(void) fclose(file);
		(void) close(fd4[0]);
		
	}
	else{
		bail_out(EXIT_FAILURE, "fork");
	}
}


static void merge_sort(char **sub_strings1, long int sub_strings1_size, char **sub_strings2, long int sub_strings2_size){

	int i = 0;
	int j = 0;

	while(i<sub_strings1_size && j<sub_strings2_size){
		if(strcmp(sub_strings1[i], sub_strings2[j])<0){
			(void) fprintf(stdout, sub_strings1[i]);
			i++;
		}
		else{
			(void) fprintf(stdout, sub_strings2[j]);
			j++;
		}
	}

	while(i<sub_strings1_size){
		(void) fprintf(stdout, sub_strings1[i]);
		i++;
	}

	while(j<sub_strings2_size){
		(void) fprintf(stdout, sub_strings2[j]);
		j++;
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

    free_resources();
    exit(exitcode);
}

static void free_resources(void){

	free(strings);
	free(sub_strings1);
	free(sub_strings2);
	(void) close(fd1[0]);
	(void) close(fd1[1]);
	(void) close(fd2[0]);
	(void) close(fd2[1]);
	(void) close(fd3[0]);
	(void) close(fd3[1]);
	(void) close(fd4[0]);
	(void) close(fd4[1]);
}
