/**
 * @module ispalindrom.c
 * @author Enri Miho - 0929003
 * @brief A program that checks if a given word is a palindrome or not
 * @date 05.11.2015
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>

/* === Constants === */

#define MAX_LENGTH (40)
#define MIN_LENGTH (2)

/* === Type Definitions === */

struct arguments {
	char *progname;
	int opt_s;
	int opt_i;
};

/* === Prototypes === */

/**
 * @brief Parse command line arguments
 * @param argc The argument counter
 * @param argv The argument vector
 * @param args Struct where parsed arguments are stored
 */
static void parse_args(int argc, char **argv, struct arguments *args);

/**
 * @brief Reads user input from stdin according to the options and then checks if that input is a palindrome
 * @param args Struct where parsed arguments are stored
 */
static void read_input_and_check_if_palindrome(struct arguments *args);

/**
 * @brief Checks if a given word is a palindrome
 * @param word The given word
 * @return 1 if the word is a palindrome, otherwise 0
 */
static int is_palindrome(char* word);

/**
 * @brief terminate program
 * @param exitcode exit code
 * @param fmt format string
 */
static void bail_out(int exitcode, const char *fmt, ...);

/* === Implementations === */

/**
 * @brief Program entry point
 * @param argc The argument counter
 * @param argv The argument vector
 * @return EXIT_SUCCESS on success, EXIT_FAILURE on error
 */
int main(int argc, char** argv){
	
	struct arguments args;
	parse_args(argc, argv, &args);
	read_input_and_check_if_palindrome(&args);
	return EXIT_SUCCESS;
}

static void parse_args(int argc, char **argv, struct arguments *args){
	
	if(argc>0){
		args->progname = argv[0];
	}
	int opt_s = 0;
	int opt_i = 0;
	int c;
	while((c = getopt(argc,argv,"si")) != -1){
		switch(c){
			case 's':
				opt_s++;
				break;
		
			case 'i':
				opt_i++;
				break;
		
			default:
				assert(0);
		}
	}
	if(opt_i>1 || opt_s>1 || argc-opt_i-opt_s>1){
		bail_out(EXIT_FAILURE, "Usage: %s [-s] [-i]", args->progname);
	}
	args->opt_s = opt_s;
	args->opt_i = opt_i;
}

static void read_input_and_check_if_palindrome(struct arguments *args){
	
	char* input = NULL;
	char c;
	char word[MAX_LENGTH+1];
	(void) memset(word,0,MAX_LENGTH+1);
	int i = 0; //index of the final word
	int j = 0; //index of the input read from stdin ((j+1)th character)
	while(1){
		c=fgetc(stdin);
		if(c == EOF || c == '\n'){
			if(c == EOF && i>0){
				(void) printf("\n");
			}
			if(i>=MAX_LENGTH){
				(void) printf("%s: Eingabe zu lang, max 40 Zeichen!\n", args->progname);
			}
			else if(i<MIN_LENGTH){
				(void) printf("%s: Eingabe muss mindestens 2 Zeichen lang sein\n", args->progname);
			}
			else{
				//check if our input is a palindrome
				int result = is_palindrome(word);
				if(result){
					(void) printf("%s ist ein palindrom\n",input);
				}
				else{
					(void) printf("%s ist kein palindrom\n",input);
				}
			}
			
			//free the memory and start over
			(void) memset(word,0,MAX_LENGTH+1);
			free(input);
			input = NULL;
			i = 0;
			j = 0;
			if(c == EOF){
				break;
			}
		}
		else{
			//we haven't finished yet, so allocate space for the next character
			char* tmp = realloc(input,j+1);
			if(tmp==NULL){
				bail_out(EXIT_FAILURE, "%s: allocating memory failed");
				break;
			}
			input = tmp;
			input[j] = c;
			j++;
			
			if(i<MAX_LENGTH){
				if(c==' ' && args->opt_s==0){
					word[i]=c;
					i++;
				}
				else if(isalnum(c)){
					if(args->opt_i==1){
						word[i]=tolower(c);
					}
					else{
						word[i]=c;
					}
					i++;
				}
			}
		}	
		
	}
}

static int is_palindrome(char* word){
	
	int len = strlen(word);
	int j = len-1;
	for(int i=0; i<len; i++){
		if(word[i]!=word[j]){
			return 0;
		}
		j--;
	}
	
	return 1;
}

static void bail_out(int exitcode, const char *fmt, ...){
	
	va_list ap;

    if (fmt != NULL) {
        va_start(ap, fmt);
        (void) vfprintf(stderr, fmt, ap);
        va_end(ap);
    }
    (void) fprintf(stderr, "\n");
	
    exit(exitcode);
}