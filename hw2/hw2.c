#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <stdlib.h>
#include "hw2_functions.h"

#define BUF_SIZE 128
#define MAGIC_NUMBER "P2CRYPTAR"
#define DIRLIST_EXEC "./dirlist"
#define P2ARCH_EXEC "./p2archive"
#define P2CRYPT_EXEC "./p2crypt"
#define P2UNARCH_EXEC "./p2unarchive"

int create_encr_archive(char *dir_name, char *encr_key, char *archive_name);
int extract_encr_data(char *dir_name, char *encr_key, char *archive_name);
void print_help();
int kill_all_children(pid_t processes[], int proc_number, char *file, int line);
int check_magic_number(int archive_fd);
void redirected_exec(char *name, char *arguement, int input_fd, int output_fd, char *file, int line);
int wait_all_processes(pid_t processes[], int proc_number);
int close_multiple_fd(char *file, int line, int number, ...);

int main(int argc, char *argv[]) {
	if (argc < 5) { //An den exoun dwthei ola ta orismata
		/*An deinei > 2 orismata, kai to deutero (mode) einai to "-H", 
		  ektypwnei tis odhgies xrhshs tou programatos. Alliws
		  thewrei pws yparxei sfalma kai termatizei. */
		if ((argc < 2) || (argc >=2 && strcmp(argv[1], "-H"))) {
			fprintf(stderr, "hw2.c:34: Incorrect number of arguements!\n");
			fprintf(stderr, "       Run with -H for more information.\n");
			return(0);
		}
	}
	
	if (!strcmp(argv[1], "-H")) { //An o xrhsths zhthse boitheia
		print_help();
	}
	else if (!strcmp(argv[1], "-E")) { //An zhta na dhmiourghsei kryptografhmeno archive
		if (create_encr_archive(argv[2], argv[3], argv[4]) < 0) {
			return(1);
		}
	}
	else if (!strcmp(argv[1], "-D")) { //An zhta na anaparagei ta periexomena enos kryptografhmenou archive
		if (extract_encr_data(argv[2], argv[3], argv[4]) < 0) {
			return(1);
		}
	}
	else { //An edwse lathos mode
		fprintf(stderr, "hw2.c:54: \"%s\" is not valid mode arguement! ", argv[1]);
		fprintf(stderr, "Run with -H for more information.\n");
		return(0);
	}
	
	return(0);
}

/* create_encr_archive():
 * Dhmiourgei mia kryptografhmenh apothikh sthn opoia apothikeuei to 
 * periexomeno twn kanonikwn arxeiwn enos katalogou kathws kai 
 * metadata gia to kathena apo auta.
 * Parametroi:
 * > dir_name: Monopati tou katalogou pros arxeiothethsh
 * > encr_key: To alfarithmitiko kleidi me bash to opoio tha ginei h kryptografhsh
 * > archve_name: To monopati ths apothikhs pou tha dhmiourghthei 
 * Epistrofh:
 * 0, an ola pane kala
 * -1, ean yparxei sfalma 
*/

int create_encr_archive(char *dir_name, char *encr_key, char *archive_name) {	
	int arch_fd;	
	/* Oi processes arxikopoiountai se 0, dioti polles synarthseis doulevoun me thn
	ypothesh oti mia thesh tou pinaka processes einai adeia an periexei to 0 */
	pid_t processes[3] = {0};
	int dir_pipe[2], arch_pipe[2]; 
	
	//Anoigma tou arxeiou archive pou tha dhmiourghthei 
	arch_fd = open(archive_name, O_WRONLY | O_EXCL | O_CREAT, 0664);
	if (arch_fd < 0) {
		if (errno == EEXIST) { //An yparxei hdh, na mhn dhmiourghthei
			fprintf(stderr, "hw2.c:70: A file correpsonding to the specified filepath already exists!\n");
			return(0);
		}
		perror("hw2.c:70: Error at open");
		return(-1);
	}
	if (my_write(arch_fd, MAGIC_NUMBER, strlen(MAGIC_NUMBER), __FILE__, __LINE__) < 0) { //write magic number
		return(-1);
	}
	/* Dhmiourgia tou argwgou pou metaferei ta monopatia twn arxeiwn apo thn diergasia 
	 pou xrhsimopoiei to dirlist se auth pou vriskei orismena metadata kai to periexomeno
	 tous mesw ths p2archive */
	if (my_pipe(dir_pipe, __FILE__, __LINE__) < 0) { 
		my_close(arch_fd, __FILE__, __LINE__);
		return(-1);
	}
	/* Dhmiourgia tou argwgou pou metaferei to periexomeno + metadata twn arxeiwn
	 sthn diergasia kryptografhshs */
	if (my_pipe(arch_pipe, __FILE__, __LINE__) < 0) { 
		close_multiple_fd(__FILE__, __LINE__, 3, arch_fd, dir_pipe[0], dir_pipe[1]);
		return(-1);
	}
	
	/* Dhmiourgia ths diergasias pou trofodotei tis alles me ta monopatia twn arxeiwn 
	tou katalogou pros eisagwgh */
	processes[0] = my_fork(__FILE__, __LINE__);
	if (processes[0] < 0) { //sfalma ths fork()
		close_multiple_fd(__FILE__, __LINE__, 5, arch_fd, dir_pipe[0], dir_pipe[1], 
					arch_pipe[0], arch_pipe[1]);
		return(-1);
	}
	else if (processes[0] == 0) { //Kwdikas tou paidiou
		//Kleisimo twn axreiastwn file descriptor
		if (close_multiple_fd(__FILE__, __LINE__, 4, arch_fd, dir_pipe[0], arch_pipe[0], arch_pipe[1]) < 0) {
			exit(EXIT_FAILURE);
		}
		//Ektelei to programma dirlist, me anakateuthinsh ths katheirwmenhs exodou ston agwgo dir_pipe
	 redirected_exec(DIRLIST_EXEC, dir_name, -1, dir_pipe[1], __FILE__, __LINE__); 
	}
	
	/* Kleisimo tou akrou tou agwgou pou den xreiazetai pleon (efoson an meinei anoixto den 
	epitrepei thn anixneush tou end-of-input */
	if (my_close(dir_pipe[1], __FILE__, __LINE__) < 0) {
		close_multiple_fd(__FILE__, __LINE__, 4, arch_fd, dir_pipe[0], arch_pipe[0], arch_pipe[1]);
		kill_all_children(processes, 1, __FILE__, __LINE__); //Den afhnei orfana paidia
		return(-1);
	}
	
	/* Dhmiourgia ths diergasias pou dexetai ta monopatia twn arxeiwn apo thn prwth,
	kai trofodwtei thn epomenh me to periexeomeno tous + metadedomena */
	processes[1] = my_fork(__FILE__, __LINE__);
	if (processes[1] < 0) { //sfalma
		close_multiple_fd(__FILE__, __LINE__, 4, arch_fd, dir_pipe[0], arch_pipe[0], arch_pipe[1]);
		kill_all_children(processes, 1, __FILE__, __LINE__);
		return(-1);
	}
	else if (processes[1] == 0) { //Kwdikas tou paidiou
		if (close_multiple_fd(__FILE__, __LINE__, 2, arch_fd, arch_pipe[0]) < 0) { //kleinei oti den xreiazetai
			exit(EXIT_FAILURE);
		} 
		/* Ektelesh tou programmatos p2archive, me anakateuthinsh ths eisodou
		sto akro diabasmatos tou agwgou dir_pipe kai ths exodou sto akro 
		grapsimatos tou arch_pipe */
	 redirected_exec(P2ARCH_EXEC, NULL, dir_pipe[0], arch_pipe[1], __FILE__, __LINE__);
	}
	//Kleisimo twn axreiastwn fd
	if (close_multiple_fd(__FILE__, __LINE__, 2, dir_pipe[0], arch_pipe[1]) < 0) {
		close_multiple_fd(__FILE__, __LINE__, 2, arch_fd, arch_pipe[0]);
		kill_all_children(processes, 2, __FILE__, __LINE__); //Den afhnei orfana
		return(-1);
	}

	/* Dhmiourgia ths diergasias pou dexetai to periexomeno twn 
	arxeiwn + metadata apo authn me pid processes[1], kai to kryptografei
	apothikeountas to sto archive pou dhmiourghithike */
	processes[2] = my_fork(__FILE__, __LINE__);
	if (processes[1] < 0) {
		close_multiple_fd(__FILE__, __LINE__, 2, arch_fd, arch_pipe[0]);
		kill_all_children(processes, 2, __FILE__, __LINE__);
		return(-1);
	}
	else if (processes[2] == 0) { //paidi
		/* Ektelei to p2crypt, me eisodo na anakateuthinetai sto akro diabasmatos
		tou arch_pipe, kai thn exodo sto arch_fd (perigrafeas tou archive pou dhmiourgeitai) */
	 redirected_exec(P2CRYPT_EXEC, encr_key, arch_pipe[0], arch_fd, __FILE__, __LINE__);
	}
	//Kleisimo twn axreiastwn fd
	if (close_multiple_fd(__FILE__, __LINE__, 2, arch_fd, arch_pipe[0]) < 0) {
		kill_all_children(processes, 3, __FILE__, __LINE__);
		return(-1);
	}
	//Anamonh gia oles tis diergasies (na mhn meinoun zombies)
	if (wait_all_processes(processes, 3) < 0) {
		return(-1);
	}

	return(0);
}

/* extract_encr_data():
 * Dhmiourgei ena neo katalogo, mesa ston opoio apothikeuontai ta arxeia pou
 * periexontai sthn kryptomegrafhmenh apothikh ths opoias to onoma dinetai ws orisma
 * me tous xronous teleutaias prosbashs, tropopoishshs, onomata, modes na einai ta
 * idia me auta pou eixan prin arxeiotheththoun.
 * Parametroi:
 * > dir_name: Monopati tou neou katalogou pou tha dhmiourghthei
 * > encr_key: To alfarithmitiko kleidi me bash to opoio tha ginei h apokryptografhsh
 * > archve_name: To monopati ths apothikhs mesw ths opoias anaparagontai ta arxeia 
 * Epistrofh:
 * 0, an ola pane kala
 * -1, ean yparxei sfalma 
*/

int extract_encr_data(char *dir_name, char *encr_key, char *archive_name) {
	int arch_fd;	
	pid_t processes[2] = {0}; //arxiokopihsh se 0 gia thn kill_all_children
	int unarch_pipe[2];
	int check;
	
	//Anoigma enos kryptografhmenou arxeiou archive
	arch_fd = open(archive_name, O_RDONLY, 0664);
	if (arch_fd < 0) {
		if (errno == ENOENT) { //Den yparxei
			fprintf(stderr, "hw2.c:179: A file correpsonding to the specified filepath does not exist!\n");
			return(0);
		}
		perror("hw2.c:179: Error at open"); 
		return(-1);
	}
	//Elegxos gia thn yparxh tou magic number P2CRYPTAR
	if ((check = check_magic_number(arch_fd)) < 0) {
		perror("hw2.c:189: Error at check_magic number");
		my_close(arch_fd, __FILE__, __LINE__);
		return(-1);
	}
	else if (check) { //Den yparxei
		fprintf(stderr, "hw2.c:189: Invalid file format, magic number \"P2CRYPTAR\" is missing!\n");
		my_close(arch_fd, __FILE__, __LINE__);
		return(0);
	}
	/* Dhmiourgia tou agwgou pou metaferei dedomena apo thn diergasia apokryptografhshs (p2crypt)
     se auth pou anaparagei ta epimerous arxeia pou apothikeuontai sto archive (mesw p2unarchive) */
	if (my_pipe(unarch_pipe, __FILE__, __LINE__) < 0) { 
		my_close(arch_fd, __FILE__, __LINE__);
		return(-1);
	}
	/* Dhmiourgia ths diergasias pou apokryptografei ta periexomena tou archive */
	processes[0] = my_fork(__FILE__, __LINE__);
	if (processes[0] < 0) { //Sfalma
		close_multiple_fd(__FILE__, __LINE__, 3, unarch_pipe[0], unarch_pipe[1], arch_fd);
		return(-1);
	}
	else if (processes[0] == 0) { //Paidi
		if (my_close(unarch_pipe[0], __FILE__, __LINE__) < 0) { //kleinei oti den xreiazetai
			exit(EXIT_FAILURE);
		}
		/* Ektelesh tou p2crypt me anakateuthinsh tou stdin sto arch_fd (fd sto archive)
		kai ths stdout sto akro grapismatos tou unarch_pipe */
	 	redirected_exec(P2CRYPT_EXEC, encr_key, arch_fd, unarch_pipe[1], __FILE__, __LINE__);
	}
	
	//Kleinei oti den xreiazetai pia
	if (close_multiple_fd(__FILE__, __LINE__, 2, unarch_pipe[1], arch_fd) < 0) {
		my_close(unarch_pipe[0], __FILE__, __LINE__);
		kill_all_children(processes, 1, __FILE__, __LINE__);
		return(-1);
	}

	/* Dhmiourgia ths diergasias pou anaparagei ta arxeia poy einai apothikeumena sto archive */
	processes[1] = my_fork(__FILE__, __LINE__);
	if (processes[1] < 0) { //Sfalma
		my_close(unarch_pipe[0], __FILE__, __LINE__);
		kill_all_children(processes, 1, __FILE__, __LINE__);
		return(-1);
	}
	else if (processes[1] == 0) { //paidi 
		/* Ektelei to p2unarchive, me anakateuthinsh ths kathierwmenhs eisodou
		 sto akro diabasmatos tou unarch_pipe */
		redirected_exec(P2UNARCH_EXEC, dir_name, unarch_pipe[0], -1, __FILE__, __LINE__);
	}
	//Kleinei oti den xreiazetai
	if (my_close(unarch_pipe[0], __FILE__, __LINE__) < 0) {
		kill_all_children(processes, 2, __FILE__, __LINE__);
		return(-1);
	}
	//Anamonh olwn twn paidiwn
	if (wait_all_processes(processes, 2) < 0) {
		return(-1);
	}
	
	return(0);
}

/* close_multiple_fd():
 * Kleinei osous perigrafeis arxeiou ths dothoun ws orismata,
 * kai ektypwnei mhnyma lathous se periptwsh sfalmatos.
 * Parametroi:
 * > file: to onoma tou phgaiou arxeiou
 * > line: o arithmos ths grammhs sto phgaio
 * > number: o arithmos twn perigrafewn arxeiou pros kleisimo
 * > ... : number perigrafeis arxeiou pros kleisimo
 * Epistrofh:
 * 0, an ola pane kala
 * -1, ean yparxei sfalma twn system calls
*/

int close_multiple_fd(char *file, int line, int number, ...) {
	int k, fd;
	va_list fd_list;
	
	va_start(fd_list, number);

	for (k = 0 ; k < number ; k++) {
		fd = va_arg(fd_list, int);
		if (my_close(fd, __FILE__, __LINE__) < 0) {
			return(-1);
		}
	}
	
	va_end(fd_list);
	
	return(0);
}

/* wait_all_processes():
 * Perimenei allagh katastashs (sto pneuma ths waitpid), twn
 * proc_number diergasiwn pou dinontai mesw tou pinaka processes
 * Parametroi:
 * > processes: pinakas diergasiwn pros anamonh 
 * > proc_number: Arithmos twn diergasiwn pros anamonh
 * Epistrofh:
 * 0, an ola pane kala
 * -1, ean yparxei sfalma twn system calls
*/

int wait_all_processes(pid_t processes[], int proc_number) {
	int k, j;
	pid_t waited;
	int status;
	
	for (k = 0 ; k < proc_number ; k++) {
		waited = waitpid(-1, &status, 0);
		if (waited < 0) {
			perror("hw2.c: Error at wait_all_processes at waitpid");
			kill_all_children(processes, proc_number, __FILE__, __LINE__);
			return(-1);
		}
			
		if (WIFEXITED(status)) {
			for (j = 0 ; j < proc_number ; j++) { //afairesh diergasias apo ton pinaka (gia na mhn th stoxeusei h kill)
				if (processes[j] == waited) {
					processes[j] = 0;
					break;
				}
			}
			if (WEXITSTATUS(status)) { //An den phgan kala ta pragamata (exit(1))
				kill_all_children(processes, proc_number, __FILE__, __LINE__);
				return(-1);
			}
		}
		else { //efyge xwris exit ara kati periergo paixthke
			fprintf(stderr, "hw2.c: Process %d exited abnormally!\n",
					 waited);
			kill_all_children(processes, proc_number, __FILE__, __LINE__);
			return(-1);
		}
	}
	
	return(0);
}

/* kill_all_children():
 * Skotwnei oles tis diergasies twn opoiwn ta anagnwristika dinontai ws orisma
 * Parametroi:
 * > processes: pinakas anagnwristikwn twn diergasiwn pou tha skotwthoun 
 * > proc_number: Arithmos twn diergasiwn pou tha skotwthoun
 * Epistrofh:
 * 0, an ola pane kala
 * -1, ean yparxei sfalma twn system calls
*/

int kill_all_children(pid_t processes[], int proc_number, char *file, int line) {
	int k;
	char message[BUF_SIZE] = {'\0'};
	
	sprintf(message, "%s: Error in kill_all_children at line %d", file, line);
	
	for (k = 0 ; k < proc_number ; k++) {
		if (processes[k] != 0) {
			if (kill(processes[k], SIGKILL) < 0) {
				perror(message);
				return(-1);
			}
		}
	}
	
	return(0);
}

/* redirected_exec():
 * Anakateuthinei tis stdin, stdout mias diergasias stous fd pou dinontai, kai epeita 
 * ektelei to ektelesimo name mesw exec, metaferontas ena orisma pou dinetai, kai
   typwnontas mhnyma kai kanontas exit se periptwsh sfalmatos
 * Parametroi:
 * > name: To onoma tou ektelesimou
 * > arguement: To orisna pou tha dwthei sto neo programma mesw ths exec
 * > input_fd: O fd ston opoio tha ginei anakateuthinsh ths kathierwmenhs eisodou,
 	an dwthei -1, den ginetai anakateuthinsh
   > output_fd: Omoiws me ton input_fd, mono pou ginetai anakateuthinsh ths kathieromenhs
   	exodou.
 */
void redirected_exec(char *name, char *arguement, int input_fd, int output_fd, char *file, int line) {
	char message[BUF_SIZE] = {'\0'};
	
	sprintf(message, "%s: Error in redirected_exec at line %d", file, line);
 	
	if (input_fd != -1) { //An dwsw -1, den xreiazetai anakateythinsh 
		if (my_dup(input_fd, STDIN_FILENO, __FILE__, __LINE__) < 0) { //Anakateuthinsh katieromenhs eisodou
			exit(EXIT_FAILURE);
		}
	}
	if (output_fd != -1) {
		if (my_dup(output_fd, STDOUT_FILENO, __FILE__, __LINE__) < 0) { //Anakateuthinsh katieromenhs exodou
			exit(EXIT_FAILURE);
		}
	}
	if (execlp(name, name, arguement, NULL) < 0) {
		perror(message);
		exit(EXIT_FAILURE); //einai entaxei na mhn gyrna tipota afou an apotyxei, tha kanei exit, kai
				    //tha to katalabei to kyriws programma apo to EXIT_FAILURE
	}
}

/* print_help():
	Typwnei tis odhgies xrhshs tou programmatos sthn kathieromenh exodo
*/

void print_help() {
	printf("P2 Homework 2: encrypted archive generator and extractor\n\n");
	printf("Arguements: <program_name> <mode> <target_directory> <cryptographic_key> <archive_filename>\n\n");
	printf("Modes: \n");
	printf("> \"-E\": Create an encrypted archive of a directory's contents.\n");
	printf("> \"-D\": Unarchive an already existing archive into a new directory.\n");
	printf("> \"-H\": Program instruction manual (you are already there).\n\n");
}

/* check_magic_number():
 * Elegxei yparxei o magic number sthn arxh tou arxeiou archive
 * 
 * Epistrofh:
 * 0, ean to magic number yparxei
 * 1, ean oxi
*/
int check_magic_number(int archive_fd) { //OK
	char buffer[BUF_SIZE] = {'\0'};
	int check;

	if ((check = my_read(archive_fd, buffer, strlen(MAGIC_NUMBER), __FILE__, __LINE__) < 0)) {
		return(-1);
	}
	else if (check == END_INPUT) { //periptwsh adeiou arxeiou
		return(1);
	}
	if (!strcmp(buffer, MAGIC_NUMBER)) { //elegxei ta prwta MAGIC_NUMBER bytes
		return(0);
	}
	
	return(1);
}
