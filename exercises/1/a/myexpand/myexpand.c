/**
 * @module myexpand.c  
 * @author Enri Miho - 0929003
 * @brief a simplified version of the unix command expand
 * @date 15.04.2015
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <assert.h>

/* === Global Variables === */

/* Name of the program */
static const char *progname = "myexpand"; /* default name */

/* tabstop value */
char* tabstop;

/* Number of occurrences of option -t */
int opt_t = 0;

/* Number of files */
int nr_of_files = 0;


/* === Prototypes === */

/**
 * @brief reads options and filenames 
 * @param argc The argument counter
 * @param argv The argument vector
 * @param the list of filenames
 */
static char **read_options_and_filenames(int argc, char **argv);

/**
 * @brief checks if a string contains only digits 
 * @param s The argument counter
 * @return 1 if the string contains only digits, otherwise 0
 */
static int tabstop_contains_numbers_only(char *s);

/**
 * @brief checks if a string contains only digits 
 * @param file the stream to read
 * @param t the tabstop value
 */
static void replace_tabs_with_spaces(FILE* file,int t);

/**
 * @brief prints a usage message and terminate program on program error
 */
static void usage(void);

int main(int argc, char **argv){
	char **filenames = read_options_and_filenames(argc,argv);
	int t = 8;
	if(opt_t>0){
		sscanf(tabstop, "%d", &t);
	}
	if(nr_of_files>0){
		for(int i = 0; i<nr_of_files; i++){
			FILE* file = fopen(filenames[i],"r");
			if(file!=NULL){
				replace_tabs_with_spaces(file,t);
				fclose(file);
			}
			else{
				fprintf(stderr,"%s: File %s not found\n",progname,filenames[i]);
			}
		}
	}
	else{
		replace_tabs_with_spaces(stdin,t);
	}
	free(filenames);
	
	return EXIT_SUCCESS;
}

static char **read_options_and_filenames(int argc, char **argv){
	char c;
	while((c=getopt(argc,argv,"t:"))!=-1){
		switch(c){
			case '?':
			 usage();
			 break;
			case 't':
			 if(opt_t>0){
				usage();
			 }
			 if(!tabstop_contains_numbers_only(optarg)){
			 	usage();
			 }
			 tabstop = optarg;
			 opt_t++;
			 break;
			default: 
		 	 assert(0);
		 	 break;
		}
	}
	nr_of_files = argc - optind;
	char **filenames;
	filenames = malloc(nr_of_files*sizeof(char*));
	
	for(int i=0; i<nr_of_files; i++){
		filenames[i] = argv[i+optind];
	}
	return filenames;
}

static int tabstop_contains_numbers_only(char *s){
    while (*s) {
        if (!isdigit(*s)){
        	return 0;
        } 
        s++;
    }
    return 1;
}


static void replace_tabs_with_spaces(FILE* file,int t){
      int x = 0;
      int c;
      while( (c=fgetc(file)) != EOF){
      	if(c=='\n'){
        	x = -1;
        	fputc(c,stdout);
      	}
      	else if(c=='\t'){
      		int p = t * ((x/t)+1);
      		while(p>x){
      			fputc(' ',stdout);
      			p--;
      		}
      	}
      	else{
      		fputc(c,stdout);
      	}
      	x++;
      }    
     
}

static void usage(void){
	fprintf(stderr,"Usage: %s [-t tabstop] [file...]\n",progname);
	exit(EXIT_FAILURE);
}
