#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "hw2_functions.h"

#define BUF_SIZE 2048

int main(int argc, char *argv[]) {
	char buffer[BUF_SIZE];
	ssize_t read_bytes;
	int k, key_idx = 0, key_len;

	if (argc < 2) { //An den exei dwthei kleidi 
		fprintf(stderr, "p2crypt.c:14: A key wasn't given!\n");
		return(1);
	}
	
	key_len = strlen(argv[1]); //Mhkos kleidiou

	//Diabazei bytes, mexri na brei to end-of-input
	do {
		//Prospathei na diabasei BUF_SIZE bytes apo thn kathierwmenh eisodo
		if ((read_bytes = read(STDIN_FILENO, buffer, BUF_SIZE)) < 0) {  
			perror("p2crypt:24: Error in main, at read");
			return(1);
		}
		else if (read_bytes == 0) { //End of input
			break;
		}		
		
		for (k = 0 ; k < read_bytes ; k++) { 
			buffer[k] ^= argv[1][key_idx]; //argv[1] to kleidi
			key_idx++;
			//An xeperastei to teleutaio byte tou kleidiou, ephstrefei sto prwto (kykliko buffer)
			if (key_idx == key_len) { 
				key_idx = 0;
			}
		}
		if (my_write(STDOUT_FILENO, buffer, read_bytes, __FILE__, __LINE__) < 0) {
			return(1); 
		}
	} while (1);
	
	return(0); 
}

