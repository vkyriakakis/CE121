#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <utime.h>
#include <sys/time.h>
#include "hw2_functions.h"

#define MAX_FILENAME 256

int transfer_data(int source_fd, int destination_fd, off_t data_size);
void add_path_info(char *basename, char *pathinfo, char *filepath);
int reproduce_file(char *dir_path);
int set_time_info(int fd, struct timespec last_access, struct timespec last_mod, char *file, int line);

int main(int argc, char *argv[]) {
	int check = 0;

	if (argc < 2) { //Den dwthike onoma tou neou katalogou
		fprintf(stderr, "p2unarchive.c:24: A directory name was not given.\n");
		return(1);
	}
	
	errno = 0; //Arxikopoihsh ths errno
	
	if (mkdir(argv[1], 0764) < 0) { //Dhmiourgia tou neou katalogou
		perror("p2unarchive.c:31: Error at mkdir");
		return(1);
	}
	
	do {
		//Kalei thn reproduce_file gia na anaparagei to epomeno arxeio
		if ((check = reproduce_file(argv[1])) < 0) { 
			return(1);
		}
		else if (check == END_INPUT) { //End of input reached
			break;
		}
	} while(1);

	return(0);
}

/* reproduce_file():
 * Diabazei plhrofories gia ena arxeio, kai to periexomeno tou apo thn kathieromenh
 * eisodo, kai to anaparagei me bash auta.
 * Parametroi:
 * > dir_path: To monopati tou katalogou ston opoio tha topotheththei to neo arxeio
 * Epistrofh:
 * > Me return:
 *  0, se periptwsh epityxias
 * -1, se sfalma twn sys-calls
 *	1, se periptwsh pou ftasei sto end of input
*/

int reproduce_file(char *dir_path) { //dir_path to monopati tou neou katalogou 
	char basename[MAX_FILENAME] ={'\0'};
	char filepath[MAX_FILENAME] = {'\0'};
	size_t name_length;
	struct timespec last_mod; //Xronos teleutaias tropopoihshs
	struct timespec last_access; //Xronos teleutaias prospelashs
	mode_t mode; //Mode tou arxeiou
	off_t file_size;
	int reproduced_fd; //fd tou neou arxeiou
	int check;
	
	//Diabazei mhkos onomatos
	if ((check = my_read(STDIN_FILENO, (char*)&name_length, sizeof(size_t), __FILE__, __LINE__)) < 0) { 
		return(-1);
	}
	//Eftase sto end-of-input (me thn ypothesh, oti den prokeitai na dwthoun merikws ta dedomena gia ena arxeio
	else if (check == END_INPUT) { 
		return(END_INPUT);
	}
	//Diabazei onoma, xrhsimopoiwntas to mhkos pou diabasthke prohgoumenws
	if (my_read(STDIN_FILENO, basename, name_length, __FILE__, __LINE__) < 0) { 
		return(-1);
	}
	//Diabazei xrono teleutaias prospelashs (binary)
	if (my_read(STDIN_FILENO, (char*)&last_access, sizeof(struct timespec), __FILE__, __LINE__) < 0) {
		return(-1);
	}
	//Diabazei xrono teleutaias tropopoihshs (binary)
	if (my_read(STDIN_FILENO, (char*)&last_mod, sizeof(struct timespec), __FILE__, __LINE__) < 0) {
		return(-1);
	}
	//Diabazei to mode tou arxeiou
	if (my_read(STDIN_FILENO, (char*)&mode, sizeof(mode_t), __FILE__, __LINE__) < 0) {
		return(-1);
	}
	//Diabazei to megethos tou arxeiou
	if (my_read(STDIN_FILENO, (char*)&file_size, sizeof(off_t), __FILE__, __LINE__) < 0) {
		return(-1);
	}
	//Emploutizei to basename me thn plhroforia tou katalogou, wste na to anoixei sto swsto katalogo
	add_path_info(basename, dir_path, filepath);
	
	//Anoigma tou fd tou arxeiou pros eisgwgh periexomenou
	reproduced_fd = open(filepath, O_WRONLY | O_CREAT, 0664);  
	if (reproduced_fd < 0) {
		perror("p2unarchive: Error in reproduce_file(), at open");
		return(-1);
	}
	//Diabazei to periexomeno apo to stdin kai epeita to grafei sto arxeio-antigrafo
	if (transfer_data(STDIN_FILENO, reproduced_fd, file_size) < 0) {
		my_close(reproduced_fd, __FILE__, __LINE__);
		return(-1);
	}
	//Metabolh xronikwn metadedomenwn
	if (set_time_info(reproduced_fd, last_access, last_mod, __FILE__, __LINE__) < 0) {
		my_close(reproduced_fd, __FILE__, __LINE__);        
		return(-1);
	}
	//Thetei to mode pou diabasthke sto neo arxeio
	if (fchmod(reproduced_fd, mode) < 0) {
		perror("p2unarchive: Error in reproduce_file(), at fchmod");
		my_close(reproduced_fd, __FILE__, __LINE__);        
		return(-1);
	}
	//Kleisimo tou file descriptor
	if (my_close(reproduced_fd, __FILE__, __LINE__) < 0) {
		return(-1);
	}
	
	return(0);
}

/* add_path_info():
 * Emploutizei to basename enos arxeiou me plhroforia arxeiou
 * Parametroi:
 * > basename: To basename tou arxeiou
 * > pathinfo: H plhroforia arxeiou pou prepei na prostethei
 * > filepath: Pinakas xarakthrwn (opou tha apothikeutei to emploutismeno onoma)
 * Epistrofh:
   > Mesaw tou pointer filepath, to emploutismeno onoma
*/
void add_path_info(char *basename, char *pathinfo, char *filepath) { 
	strcpy(filepath, pathinfo);
	strcat(filepath, "/");
	strcat(filepath, basename);
}

/* set_time_info():
 * Thetei tous xronous teleutaias prospelashs kai tropopoihshs enos arxeiou
 * Parametroi:
 * > fd: Perigrafeas arxeiou
 * > last_access: O xronos teleutaias prospelashs
 * > last_mod: O xronos teleutaias tropopoihshs
 * > file: Onoma tou arxeiou (phgaiou kwdika)
 * > line: Arithmos ths grammhs apo thn opoia kaleitai
 * Epistrofh:
   > 0, an ola pane kala
   > -1 an oxi
*/
int set_time_info(int fd, struct timespec last_access, struct timespec last_mod, char *file, int line) {
	struct timespec times[2];
	char message[MESSAGE_SIZE] = {'\0'};
	
	sprintf(message, "%s: Error in set_time_info at line %d", file, line);
	times[0] = last_access;
	times[1] = last_mod;

	//Xrhsh ths futimens gia na thesei tous xronou t. prospelashs/eggrafhs sto arxeio
	if (futimens(fd, times) < 0) {
		perror(message);
		return(-1);
	}
	
	return(0);
}
