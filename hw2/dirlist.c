#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

#define MAX_FILENAME 256

int main(int argc, char *argv[]) {
	DIR *dir_stream;
	struct dirent *dir_entry;
	struct stat stats;
	char filename[MAX_FILENAME];

	errno = 0; //Arxikopoihsh ths errno
	
	if (argc < 2) { //An den dwthei to onoma tou katalogou
		fprintf(stderr, "No directory name was given!\n");
		return(1);
	}
	
	dir_stream = opendir(argv[1]); //Anoigma tou directory stream
	if (dir_stream == NULL) {
		if (errno == ENOENT) { //Den yparxei to directory
			fprintf(stderr, "dirlist.c:27: A directory named %s does not exist.\n",argv[1]);
		}
		else if (errno == ENOTDIR) { //To onoma pou dwthike den antistoixei se katalogo
			fprintf(stderr, "dirlist.c:30: The specified name doesn't correspond to a directory!\n");
		}
		else if (errno == EACCES) { //Den epitrepetai h prosbash ston katalogo
			fprintf(stderr, "dirlist.c:33: Reading permission denied!\n");
		}
		else { //Opoiodhpote allo sfalma
			perror("dirlist.c:36: Error at main in opendir");
		}
		return(1);
	}
	
	do {
		dir_entry = readdir(dir_stream); //diabasma ths epomenhs kataxwrhshs tou directory stream
		if (dir_entry == NULL) {
			if (!errno) { //Eftase sto telos tou directory stream, ara stamata to diabasma
				break;
			}
			else { //Telika yphrxe sfalma
				perror("dirlist.c:48:: Error at main in readdir");
				if (closedir(dir_stream) < 0) {
					perror("dirlist.c:50: Error at closedir");
				}
				return(1);
			}
		}
		/* Sto struct dirent apothikeuetai to basename tou kathe stoixeiou
		 tou katalogou, ara tha prepei sthn stat na dwsw
		 to sxetiko monopati tou kathe stoixeiou ws pros ton trexon katalogo
		 wste na mporesei na to entopisei */
		
		strcpy(filename, argv[1]); //Monopati tou katalogou ws pros ton trexon katalogo
		strcat(filename, "/"); //Append to diaxwristiko '/'
		strcat(filename, dir_entry->d_name); //Append to basename tou arxeiou 
	
		if (stat(filename, &stats) < 0) { 
			/*Me bash to pedio onoma d_name tou struct dirent, 
			kalei thn stat gia na brei ton typo tou arxeiou */
			perror("dirlist.c:67: Error at main in stat");
			if (closedir(dir_stream) < 0) {
				perror("dirlist.c:69: Error at closedir");
			}
			return(1);
		}
		if ((stats.st_mode & S_IFMT) == S_IFREG) { //An einai kanoniko arxeio
			//Typwnei to onoma tou me plhroforia monopatiou 
			printf("%s\n", filename); 
		}
	} while (1);
	
	if (closedir(dir_stream) < 0) { //Kleisimo tou directory stream
		perror("dirlist.c:80: Error at closedir");
		return(1);
	}
	
	return(0);
}