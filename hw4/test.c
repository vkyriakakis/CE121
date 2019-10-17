#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/* Global metablhth o counter, wste na ton metabalei o handler tou SIGUSR1 */
volatile sig_atomic_t counter;

/* Handler tou SIGUSR1 */
void sigusr1_handler(int signum) {
	counter = -1;
}

/* Loipes synarthseis */
int handle_arguements(int argc, char *argv[], int *repetition_ptr, int *mode_ptr);
int printer_loop(int block_mode, int repetitions, sigset_t proc_mask);

int main(int argc, char *argv[]) {
	int repetitions;
	//An 0, den mplokaretai to SIGUSR1, an 1, mplokaretai gia tis prwtes mises epanalhpseis mono
	int block_mode;
	struct itimerval timer_info = {{0}};
	struct sigaction sig_disposition = {{0}};
	sigset_t proc_mask;

	//Diaxeirhsh twn orismatwn
	if (handle_arguements(argc, argv, &repetitions, &block_mode) < 0) {
		return(1);
	}
    //Mplokarei to SIGALRM, prokeimenou na mporei na xrhsimopoihthei h sigwait argotera
    sigemptyset(&proc_mask);
    //An dwthike "-b 1", mplokarei kai to SIGUSR1, alliws oxi
    if (block_mode) {
        sigaddset(&proc_mask, SIGUSR1);
    }
    sigaddset(&proc_mask, SIGALRM);
    sigprocmask(SIG_SETMASK, &proc_mask, NULL);

	//Thetei ton handler tou SIGUSR1
    sig_disposition.sa_handler = sigusr1_handler;
    sig_disposition.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sig_disposition, NULL);

	//Thetei to xypnhthri, to opoio xtypa ana 5 deuterolepa (real time)
	timer_info.it_interval.tv_sec = timer_info.it_value.tv_sec = 5;
	setitimer(ITIMER_REAL, &timer_info, NULL);

	if (printer_loop(block_mode, repetitions, proc_mask) < 0) {
		return(1);
	}

	return(0);
}

/* Synarthsh ektypwshs twn mhnymatwn */
int printer_loop(int block_mode, int repetitions, sigset_t proc_mask) {
	pid_t self_pid;
	int signum;

	/* O logos pou ylopoihsa to programma me thn sigwait, anti na dialexw
	 mia poly pio aplh ylopoihsh me thn sleep einai diplos:
	  a. Gia ekpedeutikous skopous
	  b. Gia na mhn diakoptei to SIGUSR1 thn anamonh twn 5 deuteroleptwn,
	   alla oi allages na shmeiwthoun meta to peras ths */

	sigdelset(&proc_mask, SIGUSR1);

	//Pairnei to idio ths to pid, gia na to ektypwnei
	self_pid = getpid();

	//Ektypwsh tou mhnymatos "PID: %d, counter: 0, max value: %d\n" se loop
	for (counter = 0 ; counter < repetitions ; counter++) {

		/* Perimenei mexri na xtyphsei to xypnhthri (ana 5 deuterolepta), ara kai
		 na tethei se ekkremothta to SIGALRM */
		sigwait(&proc_mask, &signum);
		if (signum == SIGALRM) {
			//An otan xtyphse to xypnhthri, ston handler tou SIGUSR1 allaxe o counter
			if (counter == -1) {
				printf("PID: %d, counter: 0, max value: %d\n", self_pid, repetitions-1);
				counter = 0;
			}
			else {
				printf("PID: %d, counter: %d, max value: %d\n", self_pid, counter, repetitions-1);
			}
			//An dwthike "-b 1", meta apo tis mises epanalhspeis, to SIGUSR1 xemplokaretai
			if (block_mode) {
				if ((repetitions % 2 == 0 && counter == repetitions/2 - 1) ||
					(repetitions % 2 != 0 && counter == repetitions/2)) {
					sigprocmask(SIG_SETMASK, &proc_mask, NULL);
				}
			}
		}
	}

	return(0);
}

/* Synarthsh diaxeirhshs twn orismatwn */
int handle_arguements(int argc, char *argv[], int *repetition_ptr, int *mode_ptr) {
	//An den dwthoun arketa orismata, gyrna sfalma
	if (argc < 5) {
		fprintf(stderr, "Invalid number of arguements!\n");
		return(-1);
	}
	//An to prwto orisma den einai "-m" gyrna sfalma
	if (strcmp(argv[1], "-m")) {
		fprintf(stderr, "First arguement must be \"-m\".\n");
		return(-1);
	}
	//Me atoi exagetai o arithmos twn epanalhpsewn
	*repetition_ptr = atoi(argv[2]);
	if (*repetition_ptr <= 0) {
		fprintf(stderr, "Second arguement must be a positive integer!\n");
		return(-1);
	}
	//To trito orisma prepei na einai "-b"
	if (strcmp(argv[3], "-b")) {
		fprintf(stderr, "Third arguement must be \"-b\".\n");
		return(-1);
	}
	//Ean to tetarto orisma den einai 0 h 1, tote yparxei sfalma
	if (strcmp(argv[4], "0") != 0 && strcmp(argv[4], "1") != 0) {
		fprintf(stderr, "Fourth arguement must be 0 or 1!\n");
		return(-1);
	}
	//Epistrofh mesw deikth tou tetartou orismatos
	*mode_ptr = atoi(argv[4]);

	return(0);
}
