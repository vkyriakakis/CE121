#include <stdio.h>
#include <unistd.h>
#include "hw2_functions.h"

/* my_close():
 * Kleinei enan file descriptor, kai ektypwnei mhnyma lathous an xreiastei, 
 * analoga me thn topothesia pou edwse o xrhsths
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias
 * -1, se sfalma twn sys-calls
*/
int my_close(int fd, char *file, int line) { 
	char message[MESSAGE_SIZE] = {'\0'};
	
	sprintf(message, "%s: Error in my_close at line %d", file, line);
	if (close(fd) < 0) {
		perror(message);
		return(-1);
	}
	return(0);
}		

/* my_write():
 * Grafei count bytes tou buffer, oses fores xreiastei wspou na grafoun ola
 * kai ektypwnei katallhlo mhnyma se periptwsh sfalmatos
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias
 * -1, se sfalma twn sys-calls
*/
int my_write(int fd, char *buffer, off_t count, char *file, int line) {
	ssize_t written; 
	off_t idx = 0;
	char message[MESSAGE_SIZE] = {'\0'};
	
	sprintf(message, "%s: Error in my_write at line %d", file, line);
	
	do {
		if ((written = write(fd, &buffer[idx], (size_t)count)) < 0) {
			perror(message);
			return(-1);
		}
		count -= (off_t)written;
		idx += written;
	} while (count != 0); //Epanalhpsh oso den ta exei grapsei ola

	return(0);
}

/* my_read():
 * Diabazei count bytes kai ta grafei sto buffer, oses fores xreiastei wspou na diabastoun ola
 * kai ektypwnei katallhlo mhnyma se periptwsh sfalmatos
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias
 * -1, se sfalma twn sys-calls
*/
int my_read(int fd, char *buffer, off_t count, char *file, int line) {
	ssize_t read_bytes; 
	off_t idx = 0;
	char message[MESSAGE_SIZE] = {'\0'};
	
	sprintf(message, "%s: Error in my_read at line %d", file, line);
	
	do {
		if ((read_bytes = read(fd, &buffer[idx], (size_t)count)) < 0) {
			perror(message);
			return(-1);
		}
		else if (read_bytes == 0) { //Elegxos gia to an eftase sto telos
			return(END_INPUT);
		}
		count -= (off_t)read_bytes;
		idx += read_bytes;
	} while (count != 0); //Epanalhpsh oso den ta exei grapsei ola

	return(0);
}

/* my_dup():
 * Apotelei wrapper gia thn dup2(),
 * kai ektypwnei katallhlo mhnyma se periptwsh sfalmatos
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias
 * -1, se sfalma twn sys-calls
*/

int my_dup(int fd, int id, char *file, int line) {
	char message[MESSAGE_SIZE] = {'\0'};

	sprintf(message, "%s: Error in my_dup at line %d", file, line);

	if (dup2(fd, id) < 0) {
		perror(message);
		return(-1);
	}
	
	if (my_close(fd, __FILE__, __LINE__) < 0) {
       		perror(message);
        	return(-1);
    	}
	
	return(0);
}

/* my_fork():
 * Apotelei ena wrapper gia thn fork(), ektypwnontas katallhlo
 * mhnyma se periptwsh sfalmatos
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias, sto paidi
 *  pid > 0, se periptwsh epityxias, ston gonio
 * -1, se sfalma twn sys-calls
*/

pid_t my_fork(char *file, int line) {
	char message[MESSAGE_SIZE] = {'\0'};
	pid_t pid;

	sprintf(message, "%s: Error in my_fork at line %d", file, line);

	pid = fork();
	if (pid < 0) {
		perror(message);
		return(-1);
	}
	
	return(pid);
}

/* my_pipe():
 * Apotelei ena wrapper gia o system call pipe(), ektypwnontas katallhlo
 * mhnyma se periptwsh sfalmatos
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias, sto paidi
 * -1, se sfalma twn sys-calls
*/
		
int my_pipe(int pipe_fd[2], char *file, int line) {
	char message[MESSAGE_SIZE] = {'\0'};

	sprintf(message, "%s: Error in my_pipe at line %d", file, line);

	if (pipe(pipe_fd) < 0) {
		perror(message);
		return(-1);
	}
	
	return(0);
}

/* transfer_data():
 * Metaferei data_size bytes apo mia thesh arxeiou se mia allh  h apo ena arxeio se ena allo
 * se tmhmata to polu BLOCK_SIZE bytes
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias
 * -1, se sfalma twn sys-calls
*/
int transfer_data(int source_fd, int destination_fd, off_t data_size) { 
	off_t blocks; //BLOCK_SIZE Byte blocks
	int remainder;
	off_t k; //remainder
	char buffer[BLOCK_SIZE];

	blocks = data_size/BLOCK_SIZE; //ypologismos tou aritmou twn tmhmatwn
	remainder = data_size % BLOCK_SIZE; //to ypoloipo
	
	if (blocks > 0) { //ean yparxei estw ena tmhma 512MB
		for (k = 0 ; k < blocks ; k++) {
			if (my_read(source_fd, buffer, BLOCK_SIZE, __FILE__, __LINE__) < 0) {
				return(-1);
			}
			if (my_write(destination_fd, buffer, BLOCK_SIZE, __FILE__, __LINE__) < 0) {
				return(-1);
			}
		}
	}
	if (remainder > 0) {
		if (my_read(source_fd, buffer, remainder, __FILE__, __LINE__) < 0) {
	       		return(-1);
	    	}
	   	if (my_write(destination_fd, buffer, remainder, __FILE__, __LINE__) < 0) {
	       		return(-1);
	    	}
	}
    				
    return(0);
}
