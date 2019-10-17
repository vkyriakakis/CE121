#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdarg.h>


/*  Symbolikes Statheres */

/*
 EXECUTION_INTERVAL: O xronos se deuterolepta ston opoio trexei mia diergasia
 prin ginei enallagh

 MAX_LINE: H megisth grammh pou mporei na dwthei ws input apo ton xrhsth
*/
#define EXECUTION_INTERVAL 20 
#define MAX_LINE 256
#define TRUE 1
#define FALSE 0

/* Dhlwseis typwn */

/* 
 status_t: Kathe mia apo tis diergasies mporei na brisketai se mia apo treis 
 katastaseis:
 	a. RUNNING: Na einai h trexousa diergasia
 	b. WAITING: Na brisketai se anamonh, mporei na ginei h trexousa
 	c. DELETE: Na einai shmademeh gia diagrafh apo thn lista diergasiwn
	
 srcmode_t: Ta modes pou xrhsimopoiei h synarthsh anazhthshs se lista:
 	a. SRC_PID: Anazhthsh diergasias sthn lista me bash to pid ths
 	b. SRC_RUNNING: Anazhthsh ths trexousas diergasias
 	c. SRC_WAITING: Anazhthsh ths epomenhs ,(diasxizontas thn lista mesw twn
 	   deiktwn next), diergasias se anamonh

 command_t: Xrhsimeuei gia thn metafora plhroforias gia thn entolh pou edwse o 
 xrhsths ston exw kosmo:
 	a. EXEC: Ektelesh diergasias
 	b. TERM: Apostolh tou SIGTERM se diergasia
 	c. SIG: Apostolh tou SIGUSR1 se diergasia
 	d. LIST: Ektypwsh twn mh shmademenwn gia diagrafh diergasiwn, emfanizontas eidika thn
 	  trexousa diergasia.
 	e. QUIT: Apoxwrhsh apo to programma
 	f. NONE: Den antiproswpeuei mia entolh, tithetai ws placeholder otan o xrhsths dwsei
 	  akyrh entolh

  process_node_t: Kombos dipla diasyndedemenhs kyklikhs listas pou periexei ta dedomena
  olwn twn diergasiwn pou dhmiourgei to periballon ekteleshs.
 */


typedef enum {WAITING, RUNNING, DELETE} status_t;
typedef enum {SRC_PID, SRC_RUNNING, SRC_WAITING} srcmode_t;
typedef enum {EXEC, TERM, SIG, LIST, QUIT, NONE} command_t;

struct process {
	pid_t pid; //To anagnwristiko ths diergaisias
	char **argv; /* Pinakas me ta orismata mesw grammhs entolwn me argv[0] onoma tou
				    kata symbash, o opoios termatizetai me NULL (logw ths exec) */ 
	int argc; //O arithmos twn orismatwn
	status_t status; //H katastash ths diergasias

	//Deiktes gia na thn diasxish ths listas
	struct process *next;
	struct process *prev;
};
typedef struct process process_node_t;


/* Katholikes Metablhtes */

//H kefalh ths listas twn diergasiwn
process_node_t *process_list = NULL;

/* Flag pou xrhsimopoihtai ston handler ths SIGCHLD, exei thn timh
 TRUE otan yparxoun komboi pros diagrafh sthn lista diergasiwn, FALSE otan oxi */
volatile sig_atomic_t del_flag = FALSE;

/* Flag pou xrhsimopoihtai ston handler twn shmatwn termatismou SIGINT, SIGHUP, 
 SIGTERM, exei thn timh TRUE otan stalthoun ta shmata, thn FALSE otan oxi */
volatile sig_atomic_t termsig_flag = FALSE;



/* Epikefalides synarthsewn */

/* Signal Handlers */
static void sigchld_handler(int signum);
static void sigusr1_handler(int signum);
static void termsig_handler(int signum);
static void sigalrm_handler(int signum);

/* Synarthseis diaxeirhshs listwn */
int process_list_init(void);
int process_list_add(pid_t pid, char *argv[], int argc);
process_node_t *process_list_src(srcmode_t mode, pid_t pid, process_node_t *running);
void free_process_node(process_node_t *process);
int process_list_print(void);
void process_list_destroy(void);
void process_list_clean(void);

/* Bohthitikes synarthseis shmatwn */
int set_signal_handlers(void);
sigset_t build_sigset(int set_num, ...);
int my_sigaction(int signum, int flags, void (*handler)(int), sigset_t *mask);

/* Synarthseis analyshs eisodou kai ekteleshs entolwn */
int parse_command(char *line, command_t *command_ptr, pid_t *pid_ptr, int *argc_ptr, char ***argv_ptr);
int start_process(char **process_argv, int process_argc);
int obtain_pid(pid_t *pid_ptr);
int obtain_argv(char ***argv_ptr, int *argc_ptr);
void free_argv(char **argv, int argc);

/* Katharisma twn zombies */
void kill_all_children();


int main(int argc, char *argv[]) {
	char line[MAX_LINE]; //Apothikeush twn entolwn tou xrhsth
	command_t command; //Kwdikos pou afora to eidos ths entolhs pou dwthike
	char **process_argv; //Pinakas orismatwn ths diergasias pros ektelesh
	int process_argc; //Arithmos orismatwn synarthshs pros ektelesh
	pid_t target_pid; //Anagnwsristiko diergasias (gia term kai sig)
	int check; //Metablhth apothikeushs epistrofhs (gia elegxo)

	//Arxikopoihsh ths listas twn diergasiwn
	if (process_list_init() < 0){ 
		return(1);
	}
	//Topothethsh twn signal handler tou programmatos 
	if (set_signal_handlers() < 0) {
		return(1);
	}

	//Xekinhma tou xypnhthriou, me xrono lhxhs 20 deuterolepta (real time)
	alarm(EXECUTION_INTERVAL);

	/* Kyrio loop tou programmatos, o xrhsths dinei entoles, oi opoies analyontai
	 kai ektelountai, enw meta thn ektelesh elegxetai h lista gia kombous pros diagrafh. */
	do {
		command = NONE; //Arxikopoihsh tou command

		//Diabasma grammhs
		if (!fgets(line, MAX_LINE, stdin)) {
			//An yphrxe diakoph logw shmatos
			if (errno == EINTR) {
				/*  Na agnoohsei thn switch an diakophke apo shma */
				command = NONE; 		
			}
		}
		//An to diabasma egine kanonika
		else {
			line[strlen(line)-1] = '\0'; //Afairesh ths allaghs grammhs
			//Analysh ths entolhs
			check = parse_command(line, &command, &target_pid, &process_argc, &process_argv);

			//An h entolh akyrh
			if (check == 1) {
				command = NONE; //Na mhn asxolhthei me thn switch
			}
			//Yphrxe sfalma
			else if (check < 0) {
				//Ara prin thn exodo ginetai katharisma 
				//Skotwnontai ta paidia
				kill_all_children();
				//Apeleutherwnetai h mnhmh ths listas
				process_list_destroy();
				return(1);
			}
			//Analoga to eidos thns entolhs pou proekypse apo thn analysh
			switch(command) {
				/* Dhmiourgia ths diergasias pou tha ektelesei to ektelesimo
				  pou edwse o xrhsths, me ta orismata pou edwse o xrhsths */
				case EXEC:
					if (start_process(process_argv, process_argc) < 0) {
						kill_all_children();
						process_list_destroy();
						return(1);
					}
					break;
				/* Apostolh tou SIGUSR1 ston stoxo */
				case SIG:
					kill(target_pid, SIGUSR1);
					break;
				/* Apostolh tou SIGTERM ston stoxo */
				case TERM:
					/* Efoson exei stalei ston stoxo to SIGSTOP, an steilw to SIGTERM,
					  auto tha ekremei wspou na xanaxekinhsei. Thewrw pws o termatismos
					  prepei na einai amesos, etsi xanaxekinaw to stoxo prin ba stalei to SIGTERM */
					kill(target_pid, SIGCONT);
					kill(target_pid, SIGTERM);
					break;
					// Ektypwsh twn diergasiwn
				case LIST:
					process_list_print();
					break;
					// Apoxwrhsh
				case QUIT:
					break;
				default:
					break;
			}
		}
		//Opote dinetai entolh, elegxetai h lista gia kombous pros diagrafh
		if (del_flag) {
			process_list_clean(); 
			//Thn elegxe gia diagrafes, ara to flag xana 0
			del_flag = FALSE;
		}
		//An stalthike shma termatismou, na apoxwrhsei  
		if (termsig_flag) {
			command = QUIT;
		}
	} while(command != QUIT);

	//Katharisma:
	kill_all_children(); //Skotonwntai ola ta paidia
	process_list_destroy(); //Apodesmeush ths mnhmhs ths listas

	return(0);
}
/* Handler tou shmatos SIGUSR1, to stelnei se oles tis 
 diergasies pou xeirizetai to programma */
static void sigusr1_handler(int signum) {
	process_node_t *curr;
	
	//An h lista adeia, epistrepse */
	if (process_list->next == process_list) {
		return;
	}
	// Apostolh tou SIGUSR1 se oses diergasies 
	for (curr = process_list->next ; curr != process_list ; curr = curr->next) {
		if (curr->status != DELETE) {
			kill(curr->pid, SIGUSR1);
		}
	}
}
/* Handler twn shmatwn termatismou, thetei ena flag
 gia na enhmerwsei to ypoloipo programma */
static void termsig_handler(int signum) {
	termsig_flag = TRUE; //Yphrxe shma termatismou
}

/* Handler tou SIGCHLD, shmadeuei gia diagrafh sthn lista ton
 kombo pou periexei tis  plhrofories gia thn diergasia pou
 termatisthke kai odhghse sthn apostolh tou SIGCHLD. Ean
 auth htan h trexousa, ginetai enallagh gia na mhn perasei
 xronos xwris na ekteleitai kapoia diergasia */
static void sigchld_handler(int signum) {
	pid_t reaped_pid;
	process_node_t *reaped_process;
	sigset_t new_mask;

	//Therismos tou zombie me thn waitpid
	reaped_pid = waitpid(-1, NULL, 0);

	/* Anazhthsh ths diergasias pou den termatisthke sthn lista */
	/* H process_list_src den xrhsimopoiei synarthseis pou 
	 den anagrafontai sto man -s7 signal, ara thn thewrw asfalh gia xrhsh
	 se handler */
	reaped_process = process_list_src(SRC_PID, reaped_pid, NULL);
	// An den yparxei, synaibei kati periergo (opws na stalei apo allh diergasia me kill to SIGCHLD) 
	if (!reaped_process) {
		return; 
	}

	/* An h diergasia pou termatisthke prohgoumenws etrexe, tha prepei
	 na ginei enallagh, wste na mhn spatalhthei xronos ekteleshs */
	if (reaped_process->status == RUNNING) {

		/* Xemplokarisma tou SIGALRM gia na ginei raise, ara kai h enallagh
		 (mplokaristhke sthn set_signal_handlers) */
		sigemptyset(&new_mask);
		sigaddset(&new_mask, SIGALRM);
		sigprocmask(SIG_UNBLOCK, &new_mask, NULL);

		//Pragmatopoihsh ths enallaghs kanontas raise to SIGALRM
		raise(SIGALRM);

		//Mploakrei xana to SIGALRM
		sigprocmask(SIG_BLOCK, &new_mask, NULL);
	}

	//Shmadema tou kombou gia diagrafh
	reaped_process->status = DELETE;

	//Efoson to flag ginetai TRUE, dhlwnetai oti yparxei mia diergasia pros diagrafh
	del_flag = TRUE;
}

/* Handler tou SIGALRM, kanei thn enallagh ths trexousas diergasias */
static void sigalrm_handler(int signum) {
	process_node_t *running, *next;

	//An h lista einai adeia, na mhn kanei tipota
	if (process_list->next == process_list) {
		alarm(EXECUTION_INTERVAL);
		return;
	}

	/* An o prohgoumenos kai epomenos ths kefalhs einai o idios kombos alla oxi h kefalh, 
	 yparxei mono enas kombos, opote synexisei na trexei o idios */
	if (process_list->next == process_list->prev) {
		alarm(EXECUTION_INTERVAL);
		return;
	}
	//Anazhthsh gia thn trexousa diergasia
	running = process_list_src(SRC_RUNNING, 0, NULL);

	/* O monos logos na mhn brethei me mh adeia lista, 
	 einai ean yparxoun diergasies pou den exoun diagrafei */
	if (!running) {
		alarm(EXECUTION_INTERVAL);
		return;
	}
	
	//Anazhthsh gia thn epomenh diergasia pou mporei na trexei 
	next = process_list_src(SRC_WAITING, 0, running);

	//An den yparxei epomenh
	if (!next) {
		alarm(EXECUTION_INTERVAL);
		return; //Synexizei na trexei h idia
	}

	//Alliws ginetai h enallagh, stamatwntas thn trexousa kai xekinwntas thn epomenh
	running->status = WAITING;
	kill(running->pid, SIGSTOP);
	next->status = RUNNING;
	kill(next->pid, SIGCONT);

	//Tithetai xana to xypnhthri
	alarm(EXECUTION_INTERVAL);
}

/* Arxikopoihsh ths kyklikhs listas twn diergasiwn */
int process_list_init(void) {
	//Desmeush tou termatikou kombou
	process_list = malloc(sizeof(process_node_t));
	if (!process_list) {
		fprintf(stderr, "%s:%d Error at malloc", __FILE__, __LINE__);
		return(-1);
	}
	//Oi deiktes tou termatikou, arxikopoiountai me to na deixnoun se auto
	process_list->next = process_list->prev = process_list;
	process_list->argv = NULL; //gia na mhn ginei free pouthena

	return(0);
}

/* Prosthikh neou kombou sthn lista */
int process_list_add(pid_t pid, char *argv[], int argc) {
	process_node_t *new, *running;

	//Dhmiourgia tou neou kombou
	new = malloc(sizeof(process_node_t));
	if (!new) {
		fprintf(stderr, "%s:%d Error at malloc", __FILE__, __LINE__);
		return(-1);
	}
	//Eisagwgh twn stoixeiwn sto new
	new->pid = pid;
	new->argv = argv;
	new->argc = argc;

	/* An den trexei kamia diergasia, h kainourgia diergasia-kombos mpainei ws trexousa,
	 alliws se anamonh */
	running = process_list_src(SRC_RUNNING, 0, NULL);
	if (running == NULL) {
		new->status = RUNNING;
	}
	else {
		new->status = WAITING;
	}

	/* Efoson einai kyklikh h lista to na ton prosthesw sto telos, einai to idio me
	 to na ton prosthesw pros ta "prev" tou termatikou kombou */
	new->next = process_list;
	new->prev = process_list->prev;
	process_list->prev->next = new;
	process_list->prev = new;

	return(0);
}

/* Anazhthsh kombou sthn lista, me bash kapoio xarakthrhstiko */
process_node_t *process_list_src(srcmode_t mode, pid_t pid, process_node_t *running) {
	process_node_t *curr;

	//Analoga to mode
	switch(mode) {
		//Anazthsh me bash to pid ths diergasias
		case SRC_PID:
			for (curr = process_list->next ; curr != process_list ; curr = curr->next) {
				//An den exei shmadeutei gia diagrafei kai exei to pid pou anazhtietai
				if (curr->status != DELETE && curr->pid == pid) {
					break;
				}
			}
			//An katalhxei sto termatiko, den brethike
			if (curr == process_list) {
				return(NULL);
			}
			break;
			//Anazththsh ths trexousas diergasias
		case SRC_RUNNING:
			for (curr = process_list->next ; curr != process_list && curr->status != RUNNING ; curr = curr->next);

			//An katalhxei sto termatiko, den brethike
			if (curr == process_list) {
				return(NULL);
			}
			break;
			//Anazhthsh ths epomenhs pros ta "next" ths trexousas pou brisketai se anamonh
		case SRC_WAITING:
			for (curr = running->next ; curr != running ; curr = curr->next) {
				//Na agnoei to termatiko kombo
				if (curr != process_list && curr->status == WAITING) {
					break;
				}
			}
			//An katalhxe sthn trexousa, den mprethike
			if (curr == running) {
				return(NULL);
			}
			break;
	}

	//Epistrofh deikth ston kombo pou brethike
	return(curr);
}

/* Elegxos ths listas gia kombous shmademenous gia diagrafh, kai diagrafh autwn */
void process_list_clean(void) {
	process_node_t *curr, *next;

	for (curr = process_list->next ; curr != process_list ; curr = next) {
		next = curr->next;
		//Ean enas kombos exei shmadeutei gia diagrafh 
		if (curr->status == DELETE) {
			//Na ginei h diagrafh tou, kai twn orismatwn pou periexei
			curr->prev->next = curr->next;
			curr->next->prev = curr->prev;
			if (curr->argv != NULL) {
				free_argv(curr->argv, curr->argc);
			}
			free(curr);
		}
	}
}

/* Apodesmeush ths kyklikhs listas diergasiwn */
void process_list_destroy(void) {
	process_node_t *curr, *next;

	//Apodesmeush twn orismatwn kai kathe kombou
	for (curr = process_list->next ; curr != process_list ; curr = next) {
		next = curr->next;
		if (curr->argv != NULL) {
			free_argv(curr->argv, curr->argc);
		}
		free(curr);
	}
	//Apodesmeush tou termatikou
	free(process_list);
}

/* Diasxish ths listas kai ektypwsh twn diergasiwn pou elegxei to programma,
 prosthetwntas (R) sthn grammh ths trexousas diergasias */
int process_list_print(void) {
	process_node_t *curr;
	int k, argc, find_flag = FALSE;
	sigset_t print_mask;

	/* Mplokarisma twn:
		a. SIGALRM: Gia na apofeugthei to endexomeno na
			ginei enallagh sta misa ths ektypwshs kai
			na emfanistoun dyo diergasies ws trexouses.
		b. SIGCHLD: Pio poly authaireth sxediastikh epilogh,
			na ektypwnontai oses diergasies htan energes
			otan klhthike h synarthsh
	*/
	print_mask = build_sigset(2, SIGALRM, SIGCHLD);
	sigprocmask(SIG_BLOCK, &print_mask, NULL);

	//Ektypwsh se epanalipsh twn diergasiwn
	if (process_list != NULL) {
		//Diasxish ths listas, mexri to termatiko kombo (kyklikh)
		for (curr = process_list->next ; curr != process_list ; curr = curr->next) {
			if (curr->status != DELETE) {
				printf("\tPID: %d, NAME: %s, ARGS: (", curr->pid, curr->argv[0]);
				argc = curr->argc; //Anathesh gia miwsh tyxaiwn prospelasewn
				if (argc != 0) {
					for (k = 0 ; k < argc ; k++) {
						if (k == argc-1) {
							printf("%s)", curr->argv[k]);
						}
						else {
							printf("%s, ", curr->argv[k]);
						}
					}
					//Brethike toulaxiston mia diergasia
					find_flag = TRUE;
				}
				//An h trexousa typwnetai to "(R)"
				if (curr->status == RUNNING) {
					puts(" (R)");
				}
				else {
					putchar('\n');
				}
			}
		}
	}
	//An den brethikan diergasies
	if (!find_flag) {
		printf("No processes are currently running!\n");
	}
	//Xemplokarisma twn shmatwn
	sigprocmask(SIG_UNBLOCK, &print_mask, NULL);

	return(0);
}

/* Topothethsh olwn twn signal handler tou programmatos */
int set_signal_handlers(void) {
	sigset_t handler_mask;

	/* Mplokarontai ta shmata auta, dioti ypopsiastika pws
	 o enas handler tha dhmiourgouse problhma ston allon.
	 P.X. ean mesa ston handler ths SIGCHLD, meta ton elegxo
	 gia to an etrexe h diergasia pou termatisthke, alla prin
	 to shmadema gia diagrafh, mporei n aginei enallagh (afou ypothetika
	 den mplokara to SIGALRM), kai na thewrithei ws trexousa h diergasia
	 pou termatisthke, spatalontas 20 deuterolepta pou tha mporousan 
	 na afierwthoun sthn ektelesh allhs diergasias h xeirotera, na meinei
	 xwris trexousa diergasia h lista, enw yparxoun polles se anamonh 
	 (an prolabei na diagrafei o kombos) */

	handler_mask = build_sigset(3, SIGCHLD, SIGALRM, SIGUSR1);

	/* Gia na synexizoun meta ton xeirismo 
	 ta system-calls, alla kai synarthseis pou ylopoiuntai
	 me auta, opws p.x. h fgets mpainei ws flag to SA_RESTART*/

	//Gia to sigusr1:
	my_sigaction(SIGUSR1, SA_RESTART, sigusr1_handler, &handler_mask);

	//Gia to sigchld:
	/* Mpainei to flag SA_NOCLDSTOP wste na mhn energopoihtai
	 o handler kathe fora pou dexetai SIGSTOP/SIGCONT kapoios kata
	 ton xronoprogrammatismo */

	my_sigaction(SIGCHLD, SA_RESTART | SA_NOCLDSTOP, sigchld_handler, &handler_mask);

	//Gia to sigalrm:
	my_sigaction(SIGALRM, SA_RESTART, sigalrm_handler, &handler_mask);

	/* Gia ta shmata termatismou, den thelw na xanaxekinoun ta system calls, alla 
	 na diakoptwntai, wste p.x. meta to ^C na mhn mplokarei h ektelesh sto input */
	my_sigaction(SIGINT, 0, termsig_handler, NULL);
	my_sigaction(SIGHUP, 0, termsig_handler, NULL);
	my_sigaction(SIGTERM, 0, termsig_handler, NULL);

	return(0);
}

/* Synarthsh-wrapper gia na apofygw twn epanalambanomeno kwdika tou gemismatos twn structs */
int my_sigaction(int signum, int flags, void (*handler)(int), sigset_t *mask) {
	struct sigaction disposition = {{0}};

	//Gemisma tou struct sigaction me ta flags, ton handler kai thn maska
	disposition.sa_flags = flags;
	if (mask != NULL) {
		disposition.sa_mask = *mask;
	}
	disposition.sa_handler = handler;

	//Allagh ths antimetwpishs tou shmatos
	if (sigaction(signum, &disposition, NULL) < 0) {
		perror("Error at my_sigaction");
		return(-1);
	}

	return(0);
}

/* Synarthsh gia thn kataskeuh sigset_t, me shamta analoga thn periptwsh */
sigset_t build_sigset(int set_num, ...) {
	va_list set_sigs;
	sigset_t sigset;
	int curr_sig;

	//Arxikopoihsh ths listas metablhtwn orismatwn va_list
	va_start(set_sigs, set_num);

	//Arxikopoihsh tou synolou sto keno
	sigemptyset(&sigset);

	//Gemisma tou synolou
	for (int k = 0 ; k < set_num ; k++) {
		curr_sig = va_arg(set_sigs, int);
		sigaddset(&sigset, curr_sig);
	}

	va_end(set_sigs);

	return(sigset);
}

/* Analysh entolhs pou epestrepse o xrhsths, kathorismos tou typou ths entolhs, 
 twn dedomenwn sta opoia auth prepei na drasei (p.x. exec sto onoma tou ektelesinou kai 
 sta orismata), kai ths syntaktikhs orthoththas */
int parse_command(char *line, command_t *command_ptr, pid_t *pid_ptr, int *argc_ptr, char ***argv_ptr) {
	char *ptr;
	int check;

	//H entoles pou dinei o xrhsths diaxwrizontai me keno, ara to xrhsimopoiw ws delimiter

	//Prwth lexh einai to eidos ths entolhs
	ptr = strtok(line, " ");
	if (!ptr) {
		//To na dwsei mono space einai akyrh entolh
		fprintf(stderr, "Invalid command: No command was given!\n");
		return(1);
	}
	//Mesw deikth, epistrefei kwdiko gia to eidos ths entolhs
	if (!strcmp(ptr, "exec")) {
		*command_ptr = EXEC;
	}
	else if (!strcmp(ptr, "sig")) {
		*command_ptr = SIG;
	}
	else if (!strcmp(ptr, "term")) {
		*command_ptr = TERM;
	}
	else if (!strcmp(ptr, "list")) {
		*command_ptr = LIST;
	}
	else if (!strcmp(ptr, "quit")) {
		*command_ptr = QUIT;
	}
	else {
		fprintf(stderr, "Invalid command: Command \"%s\" is not recognized!\n", ptr);
		return(1); //Akyrh entolh, o xrhsths edwse akyro eidos
	}

	//Analoga to eidos ths entolhs, exagontai na katallhla dedomena apo thn grammh pou dwthike
	switch(*command_ptr) {
		case EXEC:
			//Syllegei ta orismata kai to onoma tou ektelesimoy (orisma 0)
			check = obtain_argv(argv_ptr, argc_ptr);

			//An yphrxe moiraio sfalma
			if (check < 0) {
				return(-1);
			}
			//An den dwthike onoma ektelesimou
			else if (check > 0) {
				fprintf(stderr, "Invalid command: No executable name was given!\n");
				return(1);
			}
			break;
		case SIG:
		case TERM:
			//Syllegei to anagnwristiko ths diergasias stoxou
			if (obtain_pid(pid_ptr) > 0) {
				//Akyrh entolh
				return(1);
			}
			break;
		default:
			break;
	}

	return(0);
}

/* Apomonwsh twn orismatwn apo grammh pou edwse o xrhsths */
int obtain_argv(char ***argv_ptr, int *argc_ptr) {
	char **argv = NULL, **temp;
	int argc = 0; //Afou sthn arxh den exoun syllexthei orismata
	char *ptr;

	do {
		/* H strtok kaleitai me NULL, efoson exei klhthei hdh me 
		 ton delimiter ' ' apo thn parse_command */
		ptr = strtok(NULL, " ");
		//An den dwsei kan onoma (argv[0]), h entolh einai akyrh
		if (!ptr && !argv) { 
			return(1);
		}
		//An apla den yparxoun alla orismata
		else if (!ptr) {
			//Termatizetai to array me NULL wste na leitourghsei swsta h execv
			argv[argc] = NULL;
			break;
		}
		//Afou brethike ena akomh orisma
		temp = realloc(argv, (argc+2)*sizeof(char*));
		//An yphrxe sfalma
		if (!temp) {
			//Den ta kanei free h realloc an apotyxei, ara ginontai edw mesa
			free_argv(argv, argc);
			fprintf(stderr, "%s:%d: Error at realloc\n", __FILE__, __LINE__);
			return(-1);
		}
		//Ananewsh ths dieuthinshs tou pinaka
		argv = temp;
		//Auxhsh tou arithmou twn orismatwn
		argc++;
		//Antigrafh tou orismatos ston pinaka
		argv[argc-1] = strdup(ptr);
		//An yparxei sfalma
		if (!argv[argc-1]) {
			free_argv(argv, argc);
			fprintf(stderr, "%s:%d: Error at strdup\n", __FILE__, __LINE__);
			return(-1);
		}
	} while(1);

	//Epistrefontai o pinakas kai o arithmos orismatwn mes deiktwn
	*argc_ptr = argc;
	*argv_ptr = argv;
	
	return(0);
}

/* Apomonwsh tou anagnwristikou diergasias pou edwse o xrhsths */
int obtain_pid(pid_t *pid_ptr) {
	char *ptr;
	pid_t pid;

	ptr = strtok(NULL, " ");
	if (!ptr) {
		fprintf(stderr, "Invalid command: No pid was given!\n");
		return(1); //Den dwthike pid, ara kathos entolh
	}
	pid = atoi(ptr);
	//An to atoi() edwse 0, edwse term <alpharithimtiko> poy einai lathos
	if (pid == 0) {
		fprintf(stderr, "Invalid command: The target pid can only be a positive number!\n");
		return(1);
	}

	*pid_ptr = pid;

	return(0);
}

/* Apodesmeush dynamikou pinaka orismatwn */
void free_argv(char **argv, int argc) {
	//Apeleutherwsh twn orismatwn tou pinaka argv
	for (int k = 0 ; k < argc ; k++) {
		free(argv[k]);
	}
	//Apodesmeush tou idiou tou pinaka
	if (argv != NULL) {
		free(argv);
	}
}

/* Dhmiourgia diergasias, kai ektelesh ektelesimou me onoma kai
 orismata pou dwthikan apo to xrhsth */
int start_process(char **process_argv, int process_argc) {
	pid_t child_pid;
	process_node_t *running;
	sigset_t execute_mask;

	/* Allagh ths antimetwpishsh tou SIGTERM sthn default. O logos pou auto einai
	  anagaio, einai oti ston xrono pou mia diergasia einai stamathmenh alla den exei
	  kanei exec, den termatizetai apo to SIGTERM, axrhsteuontas thn entolh term */
	my_sigaction(SIGTERM, 0, SIG_DFL, NULL);

	/* Mplokarisma tou SIGCHLD gia na apofegthei to endexomeno na ektelesthei h diergasia
	 alla na termatisei prowra, prin topotheththei sthn lista, na klhthei h waitpid apo ton
	 handler tou SIGCHLD, na mhn brethei sthn lista, kai etsi na mhn diagrafei pote */
	execute_mask = build_sigset(1, SIGCHLD);
	sigprocmask(SIG_BLOCK, &execute_mask, NULL);

	//Dhmiourgia diergasias paidiou
	child_pid = fork();
	if (child_pid < 0) {
		perror("fork");
		return(-1);
	}
	//Gia to paidi
	else if (child_pid == 0) {
		//Anairountai tis allages sthn maska mplokarismatos gia to paidi, alliws den dexetai to SIGCHLD
		sigprocmask(SIG_UNBLOCK, &execute_mask, NULL);
		if (execv(process_argv[0], process_argv) < 0) {
			perror("execv");
			exit(1);
		}
	}

	//Gia ton gonio

	/* Ean den trexei kapoia diergasia, to paidi ekteleitai kanonika, alliws
	 dexetai to SIGSTOP kai stamata, mexri na erthei h seira tou na ektelestei */
	running = process_list_src(SRC_RUNNING, 0, NULL);
	if (running != NULL) {
		kill(child_pid, SIGSTOP);
	}

	//Topothethsh sthn lista me tis diergasies 
	if (process_list_add(child_pid, process_argv, process_argc) < 0) {
		kill(child_pid, SIGKILL); //Afou ta pragmata asxhma to skotwnei
		return(-1);
	}

	//Xemplokarisma tou SIGCHLD
	sigprocmask(SIG_UNBLOCK, &execute_mask, NULL);

	//Epanafora tou handler tou SIGTERM
	my_sigaction(SIGTERM, 0, termsig_handler, NULL);

	return(0);
}

/* Diasxish ths listas, kai apostolh tou SIGKILL se ola ta paidia */
void kill_all_children() {
	process_node_t *curr;

	//An h lista periexei mono to termatiko kombo (adeia dhladh)
	if (process_list->next == process_list) {
		return;
	}
	/* Apostolh tou SIGKILL se oles tis diergasies 
	(o kombos na mhn exei shmadeutei gia diagrafh) */
	for (curr = process_list->next ; curr != process_list ; curr = curr->next) {
		if (curr->status != DELETE) {
			kill(curr->pid, SIGKILL);
		}
	}
}
