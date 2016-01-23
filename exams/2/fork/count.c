#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <fcntl.h>

int main(int argc, char** argv){

	int fildes[2];
	pipe(fildes);

	if(fork() == 0){
		close(fildes[1]);
		dup2(fildes[0], 0);
		FILE *file = fopen(argv[2], "w");
		dup2(fileno(file), 1);
		execl("./encode", "encode", NULL);
	}
	else{
		close(fildes[0]);
		FILE *file = fopen(argv[1], "r");
		FILE *filew = fdopen(fildes[1], "w");
		char buffer[20];
		while(fgets(buffer, 20, file) != NULL){
			int l = strlen(buffer);
			char c = l + '0';
			fprintf(filew, "%c\n", c);
		}
		fclose(file);
		fclose(filew);
		
		int status;
		wait(&status);
	}
	
	
	return 0;
}
