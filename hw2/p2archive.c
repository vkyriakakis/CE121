#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "hw2_functions.h"

#define MAX_FILENAME 256

int archive_file(char *filepath);
void remove_path_info(char *filepath, char *basename);

int main(int argc, char *argv[]) {
	char filename[MAX_FILENAME];
	
	/*Grafei plhrofories + periexomena arxeiwn, wspou na stamathsoun
	  na dinontai apo thn katheirwmenh eisodo */
	do {
		errno = 0; //Arxikopoish ths errno
		if (fgets(filename, MAX_FILENAME, stdin) == NULL) { 
			if (!errno) { //NULL xwris na tethei h errno, ara eftase sto end-of-input
				break;
			}
			else { //Sfalma
				perror("p2archive.c:27: Error in main at fgets()");
				return(1);
			}
		}
		filename[strlen(filename)-1] = '\0'; //Afairesh tou newline pou afhnei h fgets

		if (archive_file(filename) < 0) { //An yparxei sfalma sthn archive_file
			return(1);
		}
	} while (1);
	
	return(0);
}
	
/* archive_file():
 * Afou labei to monopati enos arxeiou ws orisma, grafei sthn kathieromenh
 * exodo kapoia dedomena pou to aforoun (mhkos basename, basename, megethos
 * xronoi teleutaias prosbashs kai tropopoihshs, mode) kai to periexomeno
 * tou.
 * Parametroi:
 * > To monopati filepath enos arxeiou
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias (h arxeiou gia to opoio den exw prosbash diavasmatos)
 * -1, sthn periptwsh opoioudhpote sfalmatos
*/

int archive_file(char *filepath) {
	struct stat stats;
	char basename[MAX_FILENAME] ={'\0'};
	size_t name_length;
	int target_fd;

	target_fd = open(filepath, O_RDONLY); //Anoigma tou arxeiou
	if (target_fd < 0) {
		if (errno == EACCES) { //An den exw prosbash, epistrefw sth main, kai proxwraw sto epomeno
			return(0);
		} //Yparxei ontws sfalma
		perror("p2archive:65: Error in archive_file(), at open");
		return(-1);
	}
	//Apokthsh plhroforiwn sxetika me to arxeio me thn fstat
	if (fstat(target_fd, &stats) < 0) {
		perror("p2archive:70: Error in archive_file(), at stat");
		my_close(target_fd, __FILE__, __LINE__);
		return(-1);
	}
	
	remove_path_info(filepath, basename); //Apomononwsh tou basename tou arxeiou
	name_length = strlen(basename); //Ypologismos mhkous tou basename
	
	//Grafei sthn kathierwmenh exodo tou mhkos tou basename
	if (my_write(STDOUT_FILENO, (char*)&name_length, sizeof(size_t), __FILE__, __LINE__) < 0) { 
		return(-1);
	}
	//Grafei to basename
	if (my_write(STDOUT_FILENO, basename, strlen(basename), __FILE__, __LINE__) < 0) { 
		return(-1);
	}
	//Grafei ton xrono teleutaias prospelashs (binary)
	if (my_write(STDOUT_FILENO, (char*)&(stats.st_atim), sizeof(struct timespec), __FILE__, __LINE__) < 0) {
		my_close(target_fd, __FILE__, __LINE__);
		return(-1);
	}
	//Grafei xrono teleutaias tropopoihshs (binary)
	if (my_write(STDOUT_FILENO, (char*)&(stats.st_mtim), sizeof(struct timespec), __FILE__, __LINE__) < 0) { 
		my_close(target_fd, __FILE__, __LINE__);
		return(-1);
	}	
	//Grafei to mode tou arxeiou (binary)
	if (my_write(STDOUT_FILENO, (char*)&(stats.st_mode), sizeof(mode_t), __FILE__, __LINE__) < 0) { 
		my_close(target_fd, __FILE__, __LINE__);
		return(-1);
	}
	//Grafei to megethos tou arxeioy (se binary)
	if (my_write(STDOUT_FILENO, (char*)&(stats.st_size), sizeof(off_t), __FILE__, __LINE__) < 0) { 
		my_close(target_fd, __FILE__, __LINE__);
		return(-1);
	}
	//Grafei to periexomeno tou arxeiou me xrhsh ths transfer_data
	if (transfer_data(target_fd, STDOUT_FILENO, stats.st_size) < 0) {
		my_close(target_fd, __FILE__, __LINE__);
		return(-1);
	}
	//Kleisimo tou perigrafea arxeiou
	if (my_close(target_fd, __FILE__, __LINE__) < 0) {
		return(-1);
	}
	
	return(0);
}
	
/* remove_path_info():
 * Apomonwnei to onoma tou arxeiou apo to monopati
 */
void remove_path_info(char *filepath, char *basename) { 
	char *slash_ptr, *prev_ptr;
	
	slash_ptr = strchr(filepath, '/');
	if (slash_ptr == NULL) {
		strcpy(basename, filepath); //Periptwsh pou to arxeio vrisketai ston trexon katalogo
		return;
	}

	do { 
		prev_ptr = slash_ptr;
		slash_ptr = strchr(slash_ptr, '/');
		if (slash_ptr == NULL) {
			break;
		}
		slash_ptr++;
	} while (1);
		
	strcpy(basename, prev_ptr);
}
