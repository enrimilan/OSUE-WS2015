#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char** argv){

	char buffer[20];

	while(fgets(buffer, 20, stdin) != NULL){
		//please implement some conversion here, otherwise numbers will be printed as they are.
		printf("!!!to convert!!! %s", buffer);
	}
		
	return EXIT_SUCCESS;
}
