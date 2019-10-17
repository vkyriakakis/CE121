#include <stdio.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "hw3_wrappers.h"
#include "hw3_types.h"

#define BATCH 4096 //Gia tis realloc sto fortwma tou arxeiou
#define BYE -1 //O agent stelnei auto ton kwdiko sto server otan anaxwrei mesw EXIT
#define FULL -2 /*O server stelnei auto ton kwdiko ston agent, otan den 
                 mporei na exyphrethsei allous praktores */
#define MAX_PENDING 5 //Megistos arithmos aithmatwn syndeshs se ekkremothta

/* Paketarei thn plhroforia pou xrhsimopoiei h select gia na meiwsei
 ton arithmo twn orismatwn */
struct select_info {
	int nfds;
	fd_set r_set; 
	fd_set r_master_set;
};

void print_help();

//File Loading functions:
flight_t *shm_init(int *shmid, size_t size);
flight_t *load_flight_file(char *filename, int *shmid_ptr, int *flight_num_ptr);
void line_to_flight(char *line, flight_t *flight);
flight_t *fill_temp_buffer(flight_t *buffer, char *filename, int *flight_num);
int update_file(char *filename, flight_t *flights, int flight_num);

//Server - Agent interaction functions:
int create_listener_socket(char *socket_path);
int server_loop(int listen_fd, int max_agents, int shmid, int flight_num, int semid);
void print_active_agents(agent_t *agent_array, int max_agents);
int process_agent_messages(int listen_fd, agent_t *agent_array, int *agent_num, int max_agents, struct select_info *sel_nfo);
int process_connection_request(int listen_fd, agent_t *agent_array, int *agent_num, int max_agents, struct select_info *sel_nfo);
int send_ipc_info(int age_sock, int shmid, int flight_num, int semid);
int search_new_nfds(agent_t *agent_array, int max_agents, int listen_fd);

//Agent array management functions:
agent_t *agent_arr_init(int max_agents);
void agent_arr_insert(agent_t *agent_array, int max_agents, int age_sock, int age_pid);
int agent_arr_search(agent_t *agent_array, int max_agents, int age_sock);
void agent_arr_remove(agent_t *agent_array, int max_agents, int age_sock);
int agent_arr_clear(agent_t *agent_array, int max_agents);

int main(int argc, char *argv[]) {
	int flight_num; //arithmos twn pthsewn
	int shmid, listen_fd, semid; //Stoixeia IPC
	flight_t *flights; //pinakas pthsewn sto shared memory
	short val = 1; //Arxikh timh tou shmatoforou
	char fail_flag = 0; //Ypswnetai otan apotyxei kapoia synarthsh 
	int max_agents; //Megistos arithmos praktorwn

	//Arguement hanlding
	if (argc > 1) {
		//An dwthei to "--help" ws prwto orisma typwnetai mhnyma bohtheias
		if (!strcmp(argv[1], "--help")) {
			print_help();
			return(0);
		}
	}
	//Ean den zhththei bohtheia kai den dwthoun arketa orismata
	if (argc < 4) { 
		fprintf(stderr, "[ERROR]: Invalid number of arguements!");
		fprintf(stderr, "Run with --help for more info.\n");
		return(1);
	}

	//Metatroph tou prwtou orismatos se akeraio
	max_agents = atoi(argv[1]);

	//An den dwthei egkyros megistos arithmos praktorwn
	if (max_agents < 1) {
		fprintf(stderr, "[ERROR]: Invalid maximum number of agents!");
		fprintf(stderr, "Try a positive one next time,\n");
		return(1);
	}

	//Fortwsh arxeiou sthn koinoxrhsth mnhmh
	flights = load_flight_file(argv[2], &shmid, &flight_num);
	if (flights == SHM_FAIL) {
		return(1); //No resources, so no cleanup needed
	}

	/* Xrhsh goto gia thn apofygh xrhshs synarthshs
	 me megalh polyplokothta orismatwn (polla orismata,
	 flags) h thn epanalipsh kwdika, gia to katharisma
	 twn diaforwn resources. */

	//Dhmiourgia kai arxikopoihsh omadas shmatoforwn me thn sem_create()
	//O shmatoforos arxikopoietai se 1
	semid = sem_create(1, &val, __FILE__, __LINE__); 
	if (semid < 0) {
		fail_flag = 1; //Set fail flag
		goto RESOURCE_CLEANUP1;
	}

	//Dhmiourgia listener socket
	listen_fd = create_listener_socket(argv[3]); 
	if (listen_fd < 0) {
		fail_flag = 1;
		goto RESOURCE_CLEANUP2;
	}

	//Server loop 
	if (server_loop(listen_fd, max_agents, shmid, flight_num, semid) < 0) {
		fail_flag = 1;
		goto RESOURCE_CLEANUP3;
	}

	//Ananewsh arxeiou meta ton termatismo
	if (update_file(argv[2], flights, flight_num) < 0) {
		fail_flag = 1;
		goto RESOURCE_CLEANUP3;
	}

	RESOURCE_CLEANUP3:
	my_close(listen_fd, __FILE__, __LINE__);
	my_unlink(argv[3], __FILE__, __LINE__);
	
	RESOURCE_CLEANUP2:
	sem_remove(semid, __FILE__, __LINE__);
	
	RESOURCE_CLEANUP1:
	my_shmctl(shmid, IPC_RMID, NULL, __FILE__, __LINE__);
	my_shmdt(flights, __FILE__, __LINE__);
	
	//An yphtxe apotyxia
	if (fail_flag) {
		return(1);
	}

	return(0);
}

/* print_help():
> Perigrafei ta orismata tou programmatos sto xrhsths
*/
void print_help() {
	printf("P2 Homework 3: Flight manager server program\n\n");
	printf("Arguements: <program_name> <max agent no> <filght file> <socket path>\n");
}

//File Loading functions:

/* update_file():
> Ananewnei to arxeio pthsewn, me bash tis allages pou exoun
 katagrafei ston pinaka twn pthsewn (koin. mnhmh)

> Orismata:
  filename: Onoma tou arxeiou pthsewn
  flights: Pinakas pthsewn (sthn shm)
  flight_num: Arithmos pthsewn

> Epistrofes:
 Mesw return:
  > Se periptwsh epityxias to 0
  > Se periptwsh apotyxias to -1
*/

int update_file(char *filename, flight_t *flights, int flight_num) {
	FILE *flight_fp;
	int k;

	//Anoigma tou stream
	flight_fp = fopen(filename, "w");
	if (!flight_fp) {
		return(-1);
	}

	//Grapsimo twn periexomenwn tou pinaka flights sto arxeio
	for (k = 0 ; k < flight_num ; k++) {
		    fprintf(flight_fp, "%s ", flights[k].airline);
		    fprintf(flight_fp, "%s ", flights[k].src);
		    fprintf(flight_fp, "%s ", flights[k].dest);
		    fprintf(flight_fp, "%d ", flights[k].stops);
		    fprintf(flight_fp, "%d\n", flights[k].seats);
	}

	if (fclose(flight_fp) < 0) {
		return(-1);
	}

	return(0);
}

/* shm_init():
> Dhmiourgei tmhma koinoxrhsths mnhmhs kai to prosartei
 sthn eikonikh mnhmh ths diergasias tou server.

> Orismata:
  shmid_ptr: Pointer sto anagnwristiko ths koinoxrhsths mnhmhs
  size: To megethos tou tmhmatos koinoxrhsths mnhmhs

> Epistrofes:
 Mesw return:
  > Se periptwsh epityxias ton deikth sto xekinhma tou
  tmhmatos koinoxrhsths mnhmhs 
  > Se periptwsh apotyxias SHM_FAIL

 Mesw deiktwn:
  To anagnwristiko shmid ths koinoxrhsths mnhmhs
*/

flight_t *shm_init(int *shmid_ptr, size_t size) {
	flight_t *shm_ptr;

	//Dhmiourgia tmhmatos koinoxthsths mnhmhs
	*shmid_ptr = my_shmget(IPC_PRIVATE, size, S_IRWXU, __FILE__, __LINE__);
	if (*shmid_ptr < 0) {
		return(SHM_FAIL);
	}

	//Prosarthsh tmhmatos koinoxrhsths mnhmhs 
	shm_ptr = my_shmat(*shmid_ptr, NULL, 0, __FILE__, __LINE__);
	if (shm_ptr == SHM_FAIL) {
		my_shmctl(*shmid_ptr, IPC_RMID, NULL, __FILE__, __LINE__); //Afou den egine h douleia to katastrefw
		return(SHM_FAIL);
	}

	return(shm_ptr); //Epistrofh tou ptr sto shared memory
}

/* load_flight_file():
> Dhmiourgei perioxh koinoxrhsths mnhmhs kai fortwnei
 ta periexomena enos arxeiou pthsewn se auth, me apotelesma
 thn dhmiourgia enos pinaka pthsewn.

> Orismata:
  filename: To onoma tou arxeiou pthsewn
  shmid_ptr: Deikths sto anagnwristiko ths koinoxrhsths mnhmhs
  flight_num_ptr: Deikths ston arithmo twn pthsewn tou arxeiou

> Epistrofes:
 Mesw return:
  Se periptwsh epityxias ton deikths sto xekinhma tou tmhmatos 
  koinoxrhsths mnhmhs, enw se apotyxia gyrna SHM_FAIL

 Mesw deiktwn:
  To anagnwristiko tou tmhmatos koinoxrhsths mnhmhs, kai
  ton arithmo twn pthsewn pou fortwthikan se authn.
*/

flight_t *load_flight_file(char *filename, int *shmid_ptr, int *flight_num_ptr) {
	flight_t *flights, *load_array, *temp;

	//Allocate temporary buffer, statring at BATCH flights
	load_array = malloc(BATCH*sizeof(flight_t));
	if (load_array == NULL) {
		fprintf(stderr, "%s: Error at malloc at line %d\n", __FILE__, __LINE__);
		return(SHM_FAIL);
	}

	//Fortwnei to arxeio sto temp buffer
	temp = fill_temp_buffer(load_array, filename, flight_num_ptr);
	if (temp == NULL) {
		//to free exei ginei mesa sthn fill_temp_buffer
		return(SHM_FAIL);
	}
	load_array = temp; //an petyxe to thetw sto load_array

	//Dhmiourgia ths shm kai to shmid stelnetai ap'exw
	flights = shm_init(shmid_ptr, (*flight_num_ptr)*sizeof(flight_t));
	if (flights == SHM_FAIL) {
		free(load_array);
		return(SHM_FAIL);
	}

	//Antigrafh tou periexomenou tou arxikou pinaka sthn shared memory
	memcpy(flights, load_array, (*flight_num_ptr)*sizeof(flight_t)); 

	//Efoson egine h antigrafh, eleutheronetai to array fortwshs
	free(load_array); 

	return(flights);
}

/* fill_temp_buffer():
> Fortwnei to arxeio se ena dynamiko proswrino buffer, to opoio
 kai epistrefei.

> Orismata:
  load_buffer: To buffer sto opoio fortwnetai to arxeio
  filename: To onoma tou arxeiou twn pthsewn
  flight_num_ptr: Deikths ston arithmo twn pthsewn tou arxeiou

> Epistrofes:
 Mesw return:
  > Se periptwsh epityxias ton deikth sto xekinhma ths dynamikhs
  mnhmhs, sthn opoia fortwthikan oi pthseis 
  > Se periptwsh apotyxias NULL

 Mesw deiktwn:
  Ton arithmo twn pthsewn pou fortwhikan sth mnhmh
*/

flight_t *fill_temp_buffer(flight_t *load_buffer, char *filename, int *flight_num_ptr) {

	//Proswrinh metablhth gia ton elegxo sth realloc
	flight_t *temp; 

	//buffer pou apothikeuei mia grammh tou arxeiou
	char line[MAX_LINE]; 

	//Xrhsimopoieitai gia ton periorismo twn allocations
	int max_flights = BATCH; 

	//O pragmatikos arithmos twn pthsewn
	int flight_num = 0; 

	//File stream tou arxeiou pthsewn
	FILE *flight_fp; 

	//Anoigma tou stream
	flight_fp = fopen(filename, "r");
	if (flight_fp == NULL) {
		//To arxeio den yparxei
		if (errno == ENOENT) {
			fprintf(stderr, "[ERROR]: Filght file \"%s\" does not exist!\n", filename);
		}
		else {
			fprintf(stderr, "%s: Error at fopen at line %d!\n", __FILE__, __LINE__);
		}
		free(load_buffer); //To free ginetai edw oxi exw
		return(NULL);
	}

	//Loop fortwshs tou arxeiou
	do {
		errno = 0; //Th thetei 0, afou yparxei periptwsh na thn allaxoun alles synarthseis
		if (!fgets(line, MAX_LINE, flight_fp)) {
			if (errno != 0) { //Yphrxe ontws sfalma
				fclose(flight_fp);
				free(load_buffer);
				return(NULL);
			}
			//errno == 0 kai epistrofh NULL, ara EOF kai teleiwnei h loop
			else {
				break; 
			}
		}

		//Apothikeush ths plhroforias ths grammhs se struct flight_t
		line_to_flight(line, &load_buffer[flight_num]);

		//Megalwnei to megethos tou pinaka
		flight_num++;

		//An o arithmos twn pthsewn yperbei to megisto
		if (flight_num == max_flights) { 

			//Megalwnei o pinakas kata BATCH
			max_flights += BATCH;

			//Resize to buffer
			temp = realloc(load_buffer, max_flights*sizeof(flight_t)); 
			if (temp == NULL) {
				fprintf(stderr, "%s: Error at realloc at line %d\n", __FILE__, __LINE__);
				fclose(flight_fp);

				//To free ginetai mesa, afou exw se sfalma tha perastei NULL
				free(load_buffer); 
				return(NULL);
			}

			//Efoson to temp oxi null
			load_buffer = temp; 
		}
	} while(1);

	*flight_num_ptr = flight_num; //Pernaw ap'exw ton arithmo twn pthsewn

	if (fclose(flight_fp) < 0) {
		fprintf(stderr, "%s: Error at fclose at line %d!\n", __FILE__, __LINE__);
		free(load_buffer); //gia na mhn ginei exw me ptr epistrofh
		return(NULL);
	}

	//Epistrefw to buffer (tha allaxei thesh logw realloc), ap'exw
	return(load_buffer); 
}
	
/* line_to_flight():
> Xwrizei mia grammh se tokens, kai apo authn
 gemizei mia domh typou flight_t me plhrofories pthshs

> Orismata:
 line: H grammh me tis plhrofories pthshs
*/

void line_to_flight(char *line, flight_t *flight) {
	char *temp;

	//Xwrismos ths grammhs se tokens, kai symplhrwsh
	temp = strtok(line, " ");
	strcpy(flight->airline, temp);
	temp = strtok(NULL, " ");
	strcpy(flight->src, temp);
	temp = strtok(NULL, " ");
	strcpy(flight->dest, temp);
	temp = strtok(NULL, " ");
	flight->stops = atoi(temp);
	temp = strtok(NULL, " ");
	flight->seats = atoi(temp);
}

//Server - Agent interaction functions:

/* create_listener_socket():
> Dhmiourgei to socket, kai kalei tis katallhles
 leitourgies gia na to kanei listener.

> Orismata:
 filepath: To monopati sto file system tou socket 
  
> Epistrofes:
 Mesw return:
  > To fd tou listener socket se periptwsh epityxias
  > -1 se periptwsh apotyxias
*/

int create_listener_socket(char *socket_path) {
	int sock_fd;
	struct sockaddr_un server_addr;

	//Dhmiourgia tou socket
	sock_fd = my_socket(AF_UNIX, SOCK_STREAM, 0, __FILE__, __LINE__);
	if (sock_fd < 0) {
		return(-1);
	}

	//Symplhrwsh ths dieuthinshs tou server
	bzero((char*)&server_addr, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, socket_path, 108);

	//Kathista to socket entopisimo
	if (my_bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr), __FILE__, __LINE__) < 0) {\
		my_close(sock_fd, __FILE__, __LINE__);
		return(-1);
	}

	//Metatroph tou socket se listener
	if (my_listen(sock_fd, MAX_PENDING, __FILE__, __LINE__) < 0) {
		my_close(sock_fd, __FILE__, __LINE__);
		return(-1);
	}

	return(sock_fd);
}

/* server_loop():
> Enswmatwnei oles tis leitourgies pou prepei na ektelei o server,
 oso einai leitourgikos, dhladh, na apodexetai aithmata syndeshs, 
 na parakolouthei thn kathierwmenh eisodo tou gia "Q" h ^D, na
 katalabainei pote oi praktores aposyndeeontai, kai na ananewnei
 ta posa eisithriwn pou exoun agorasei. 

> Orismata:
  listen_fd: Fd tou listener socket
  max_agents: Megistos arithmos praktorwn
  shmid: Anagnwristiko koinoxrisths mnhmhs
  flight_num: Arithmos pthsewn sthn shm
  semid: Anagnwristiko omadas shmatoforwn

 > Epistrofes:
  Se periptwsh epityxias to 0, alliws to -1
*/

int server_loop(int listen_fd, int max_agents, int shmid, int flight_num, int semid) {
	int age_sock;
	char stdin_input[MESSAGE_SIZE] = {'\0'};
	struct select_info sel_nfo;
	int agent_num = 0;
	agent_t *agent_array;

	//Arxikopoihsh read master set kai nfds
	sel_nfo.nfds = listen_fd+1;
	FD_ZERO(&(sel_nfo.r_master_set));
	FD_SET(listen_fd, &(sel_nfo.r_master_set));
	FD_SET(STDIN_FILENO, &(sel_nfo.r_master_set));

	//Dhmiourgia pinaka praktorwn
	agent_array = agent_arr_init(max_agents);
	if (agent_array == NULL) {
		return(-1);
	}

	do {
		//Anthesh tou master set sto metablhto
		sel_nfo.r_set = sel_nfo.r_master_set;

		//Anamonh gia fds etoimous gia diabasma me thn select
		if (my_select(sel_nfo.nfds, &(sel_nfo.r_set), NULL, NULL, NULL, __FILE__, __LINE__) < 0) {
			return(-1);
		}
		//An o xrhsths grafei sto stdout
		if (FD_ISSET(STDIN_FILENO, &(sel_nfo.r_set))) { 
			//arxikopoihsh ths errno (mporei na exei metablhthei apo allh synarthsh)
			errno = 0; 
			if (!fgets(stdin_input, MESSAGE_SIZE, stdin)) { //An h fgets dwsei NULL
				if (errno != 0) { //me errno != 0 exw sfalma
					perror("server.c: Error at fgets, at line 190");
					agent_arr_clear(agent_array, max_agents);
					return(-1);
				}
				//alliws exw end of input (^D) kai o server bgainei apo to loop
				else { 
					break;
				}
			}
			//An dwthei "Q" pali o server bgainei apo to loop
			if (!strcmp("Q\n", stdin_input)) { 
					break;
			}
		}
		//An to listener socket einai etoimo gia diabasma yparxoun syndeseis se ekkremothta
		if (FD_ISSET(listen_fd, &(sel_nfo.r_set))) {

			//Epexergasia aithmatos syndeshs
			age_sock = process_connection_request(listen_fd, agent_array, &agent_num, max_agents, &sel_nfo);
			if (age_sock == -1) { //ERROR twn sys-calls
				agent_arr_clear(agent_array, max_agents);
				return(-1);
			}
			//An xwrses (den epistrafhke FULL)
			if (age_sock != FULL) { 
				//Apostolh twn plhroforiwn IPC
				if (send_ipc_info(age_sock, shmid, flight_num, semid) < 0) {
					agent_arr_clear(agent_array, max_agents);
					return(-1);
				}
			}
		}
		//Epexergasia twn etoimwn gia diabasma praktorwn
		if (process_agent_messages(listen_fd, agent_array, &agent_num, max_agents, &sel_nfo) < 0) {
			agent_arr_clear(agent_array, max_agents);
			return(-1);
		}
	} while (1);

	//Meta ton termatismo, ektypwsh twn energwn praktorwn
	print_active_agents(agent_array, max_agents);

	//Katharisma tou pinaka praktorwn
	if (agent_arr_clear(agent_array, max_agents) < 0) {
		return(-1);
	}

	return(0);
}

/* print_active_agents():
> Ektypwnei ta eishthria pou exoun krathsei oi energoi praktores
 (autoi tous opoious dextike o server), h ean den yparxoun energoi
 katallhlo mhnyma.

> Orismata:
  agent_array: Pinakas twn praktwrwn
  max_agents: Megistos arithmos praktorwn
*/
void print_active_agents(agent_t *agent_array, int max_agents) {
	int k;
	int active_flag = 0; //An yparxei toulaxiston enas energos ginetai 1

	printf("Active agent summary:\n");
	for (k = 0 ; k < max_agents ; k++) {
		if (agent_array[k].pid != 0) { //An h thesh tou pinaka den einai adeia (ara energos praktoras)
			printf("\tAgent with PID %d reserved %d seats in total!\n", agent_array[k].pid, agent_array[k].seats);
			active_flag = 1;
		}
	}
	if (!active_flag) {
		printf("No agents were active during shutdown!\n");
	}
}

/* process_agent_messages():
> Elegxei ola ta sockets twn praktorwn, gia to an einai
 etoima gia diabasma, kai an nai epexergazetai ta mhnymata tous.

> Orismata:
  listen_fd: To listener socket (gia na to agnoohsei)
  agent_array: Pinakas twn praktwrwn
  agent_num_ptr: Deikths ston arithmo twn praktorwn
  max_agents: Megistos arithmos praktorwn
  sel_nfo: Pointer se struct me plhroforia pou afora th select 
  (read set, nfds, master read set)

> Epistrofes:
   Mesw return:
	Se periptwsh apotyxias twn system calls, epistrefei -1
	An ola pane kala, epistrefei 0
*/

int process_agent_messages(int listen_fd, agent_t *agent_array, int *agent_num_ptr, int max_agents, struct select_info *sel_nfo) {
	int read_set_fd; //Metrhths

	//H o arithmos eishthriwn pou kratithike h BYE (an kanei quit o agent)
	int  agent_message; 

	//O deikths tou apostolea ston pinaka
	int sender_idx; 

	//Proswrinh apothikeush tou nfds gia exoikonomish tyxaiwn prospelasewn sth mnhmh
	int nfds = sel_nfo->nfds; 

	//Pali exoikonomish tyxaiwn prospelasewn
	fd_set r_set = sel_nfo->r_set;

	//Apothikeush elegxwn
	int check;

	/* Elegxei olous tous fd ektos twn kathierwmenwn exodwn/eisodwn kai tou listener fd
	 kai an einai etoimoi gia diabasma, dexetai kai epexergazetai ta mhnymata tous */
	for (read_set_fd = STDERR_FILENO+1 ; read_set_fd < nfds ; read_set_fd++) {

		//An o agent einai sto read_set, kai oxi o listener (kalyptetai allou), diabase mhnyma
		if (FD_ISSET(read_set_fd, &r_set) && read_set_fd != listen_fd) {

			//Afou etoimos diabazei to mhnyma
			check = my_read(read_set_fd, (char*)&agent_message, sizeof(int), __FILE__, __LINE__);
			if (check < 0) {
				return(-1);
			}

			/* An anti gia arithmo eishthriwn (> 0) esteile BYE, aposyndethike me EXIT,
			 enw an h my_read epestrepse END_INPUT, to socket tou client ekleise, ara
             aposyndethike logw sfalmatos (apotoma, xwris na steilei BYE) */
			if (check == END_INPUT || agent_message == BYE) {

				//Euresh tou praktora me bash to socket tou sto agent array
				sender_idx = agent_arr_search(agent_array, max_agents, read_set_fd);

				//An efyge apotoma
				if (check == END_INPUT) {
					printf("[QUIT]: Agent with PID %d disconnected due to client-side error after having reserved %d seats!\n",
						agent_array[sender_idx].pid, agent_array[sender_idx].seats);
				}

				//An efyge mesw EXIT
				else if (agent_message == BYE) {
					printf("[QUIT]: Agent with PID %d quit after having reserved %d seats!\n", 
						agent_array[sender_idx].pid, agent_array[sender_idx].seats);
				}
				//Kleisimo tou socket tou
				if (my_close(read_set_fd, __FILE__, __LINE__) < 0) {
					return(-1);
				}

				//Afairesh tou apo to array
				agent_arr_remove(agent_array, max_agents, read_set_fd);

				//Meiwsh arithmoy syndedemenwn praktorwn
				(*agent_num_ptr)--;

				//Afairesh tou apo to master set twn fd pros anamonh gia diabasma
				FD_CLR(read_set_fd, &(sel_nfo->r_master_set));

				//Briskei to neo nfds an autos pou efyge htan o megalyteros
				if (nfds == read_set_fd+1) { 
					//Mesw deikth paei ston exw kosmo
					sel_nfo->nfds = search_new_nfds(agent_array, max_agents, listen_fd);
				}
			}
			//Alliws an esteile agent_message > 0 (arithmos eishthriwn)
			else {
				//Auxhse to poso twn eishthriwn tou agent kata agent_message
				sender_idx = agent_arr_search(agent_array, max_agents, read_set_fd);
				agent_array[sender_idx].seats += agent_message;
				printf("[RESERVATION]: Agent with pid %d reserved %d seats!\n", 
					agent_array[sender_idx].pid, agent_message);
			}
		}
	}

	return(0);
}

/* process_connection_request():
> Apodexetai to aithma syndeshs enos praktora, kai ean mporei na 
 exyphrethsei epipleon praktores, paramenei syndedemenos, alliws
 ton eidopoiei gia thn elleipsh xwrou kai termatizei thn syndesh
 me auton.

> Orismata:
  listen_fd: To listener socket
  agent_array: Pinakas twn praktwrwn
  agent_num_ptr: Deikths ston arithmo twn praktorwn
  max_agents: Megistos arithmos praktorwn
  sel_nfo: Pointer se struct me plhroforia pou afora th select 
  (read set, nfds, master read set)

> Epistrofes:
   Mesw return:
	Se periptwsh apotyxias twn system calls, epistrefei -1
	Se periptwsh pou den yphrxe xwros gia ton praktora epistrefei FULL 
	An ola pane kala. epistrefei ton fd tou socket pou antistoixei ston neo praktora
*/

int process_connection_request(int listen_fd, agent_t *agent_array, int *agent_num_ptr, int max_agents, struct select_info *sel_nfo) {
	int age_sock; //Socket tou praktora
	pid_t age_pid; //Pid tou praktora
	int full_message = FULL; //Apothikeuei to mhnyma FULL

	//Apodoxh ths syndeshs kai dhmiourgia stream socket
	age_sock = my_accept(listen_fd, NULL, NULL, __FILE__, __LINE__);
	if (age_sock < 0) {
		return(-1);
	}
	//Paralabh tou pid tou praktora
	if (my_read(age_sock, (char*)&age_pid, sizeof(pid_t), __FILE__, __LINE__) < 0) {
		my_close(age_sock, __FILE__, __LINE__);
		return(-1);
	}

	//An o arithmos twn praktorwn mikroteros apo ton megisto
	if (*agent_num_ptr < max_agents) {
		//Eisagwgh tou praktora ston pinaka
		agent_arr_insert(agent_array, max_agents, age_sock, age_pid);

		//Eisagwgh tou socket tou sto master read set
		FD_SET(age_sock, &(sel_nfo->r_master_set));

		//An megalyteros apo ton megisto, allagh tou nfds
		if (age_sock >= sel_nfo->nfds) {
			sel_nfo->nfds = age_sock+1;
		}

		//Auxhsh tou arithmou twn praktorwn
		(*agent_num_ptr)++;

		//Ektypwsh mhnymatos otan syndethei
		printf("[CONNECTION]: Agent with PID %d connected!\n", age_pid);
	}
	/*An den yparxei xwros, stelnetai FULL wste na eidopoihthei o praktoras, 
	 kai na diakopsei thn syndesh */
	else {
		if (my_write(age_sock, (char*)&full_message, sizeof(int), __FILE__, __LINE__) < 0) {
			my_close(age_sock, __FILE__, __LINE__);
			return(-1);
		}
		if (my_close(age_sock, __FILE__, __LINE__) < 0) {
			return(-1);
		}

		//Ap'exw epistrefei FULL
		return(FULL);
	}

	return(age_sock); //Epistrefei to socket epikoinwnias me ton prakora
}

/* send_ipc_info():
> Stelnei ta aparaithta stoixeia epikoinwnias IPC
 (anagnwristika shared memory, omadas shmatoforwn,
  arithmos pthsewn sto shared memory) se kapoio
  agent mesw tou socket pou dinetai ws orisma.

> Orismata:
  age_sock: Socket mesw tou opoiou o server epikoinwnei me ton praktora
  shmid: Anagnwristiko koinoxrisths mnhmhs
  flight_num: Arithmos pthsewn
  semid: Anagnwwristiko omadas shmatoforwn

> Epistrofes:
  Epistrefei 0 se epityxia, kai -1 se apotyxia
*/

int send_ipc_info(int age_sock, int shmid, int flight_num, int semid) {
	//stelnei ston agent to shared memory id
	if (my_write(age_sock, (char*)&shmid, sizeof(int), __FILE__, __LINE__) < 0) { 
		return(-1);
	}
	//Stelnei kai ton arithmo twn pthsewn
	if (my_write(age_sock, (char*)&flight_num, sizeof(int), __FILE__, __LINE__) < 0) { 
		return(-1);
	}
	//Stelnei to id ths omadas shmatoforwn
	if (my_write(age_sock, (char*)&semid, sizeof(int), __FILE__, __LINE__) < 0) { 
		return(-1);
	}

	return(0);
}



//AGENT ARRAY HANDLING

/* Domh tou pinaka praktorwn:
 Monodiastatos pinakas megethous max_agents, apo domes typou agent_t.
 Arxikopoieitai wste ta pid twn praktorwn na einai 0, kai opoia thesh
 exei to pedio pid na einai 0, thewreitai adeia (ara kai shmademena
 gia overwrite). */

/* search_new_nfds():
> Anazhtei ton megalytero fd ston pinaka ton praktorwn kai ton
 epistrefei auxhmeno kata 1 (nfds = max_fd+1)

> Orismata:
  agent_array: Pinakas praktorwn
  max_agents: Megistos arithmos praktorwn
  listen_fd: Arxikopoiei to max_fd me to listen_fd

 > Epistrofes:
  To neo nfds (max_fd+1)
*/

int search_new_nfds(agent_t *agent_array, int max_agents, int listen_fd) {
	int k;
	int max_fd;

	max_fd = listen_fd;

	for (k = 0 ; k < max_agents ; k++) {
		if (agent_array[k].pid != 0) {
			if (agent_array[k].socket > max_fd) {
				max_fd = agent_array[k].socket;
			}
		}
	}

	return(max_fd+1); //epistrefei to neo nfds
}


//Agent array management functions:

/* agent_arr_init():
> Dhmiourgei (kai arxikopoiei se pid == 0) enan
 pinaka praktorwn megethous max agents

> Orismata:
  max_agents: Arithmos megistwn praktorwn

> Epistrofes:
  Mesw return, pointer ston pinaka praktorwn se
  epityxia, alliws NULL.
*/

agent_t *agent_arr_init(int max_agents) {
	agent_t *array;
	int k;

	array = malloc(sizeof(agent_t)*max_agents);
	if (array == NULL) {
		perror("server.c: agent_arr_init error at malloc");
		return(NULL);
	}

	for (k = 0 ; k < max_agents ; k++) {
		array[k].pid = 0; //adeies theseis
	}

	return(array);
}

/* agent_arr_insert():
> Eisagei enan praktora ston pinaka

> Orismata:
  agent_array: Pinakas praktorwn
  max_agents: Arithmos megistwn praktorwn
  age_sock: O fd tou socket, tou praktora pros eisagwgh
  age_pid: To pid tou praktora pros eisagwgh
*/

void agent_arr_insert(agent_t *agent_array, int max_agents, int age_sock, int age_pid) {
	int k;

	for (k = 0 ; k < max_agents ; k++) {
		//An pid == 0 einai adeia, ara topotheteitai ekei
		if (agent_array[k].pid == 0) {
			agent_array[k].socket = age_sock;
			agent_array[k].pid = age_pid;

			//Arxikopoiei ta eishthria tou praktora se 0
			agent_array[k].seats = 0;
			break;
		}
	}
}


/* agent_arr_search():
> Afairei enan praktora apo ton pinaka me bash
 ton fd tou socket me to opoio epikoinwnei mazi tou o server.

> Orismata:
  agent_array: Pinakas praktorwn
  max_agents: Arithmos megistwn praktorwn
  age_sock: O fd tou socket, tou praktora pros anazhthsh

> Epistrofes:
  Epistrefei ton deikth tou praktora an breathei
  alliws to -1.
*/

int agent_arr_search(agent_t *agent_array, int max_agents, int age_sock) {
	int j;

	for (j = 0 ; j < max_agents ; j++) {
		//Agnoa tis adeies theseis
		if (agent_array[j].pid != 0) {
			if (agent_array[j].socket == age_sock) {
				return(j); //Epistrofh tou deikth tou katallhlou praktora
			}
		}
	}

	return(-1); //if not found
}

/* agent_arr_remove():
> Afairei enan praktora apo ton pinaka me bash
 ton fd tou socket me to opoio epikoinwnei mazi tou o server.

> Orismata:
  agent_array: Pinakas praktorwn
  max_agents: Arithmos megistwn praktorwn
  age_sock: O fd tou socket, tou praktora pros afairesh
*/

void agent_arr_remove(agent_t *agent_array, int max_agents, int age_sock) {
	int j;

	for (j = 0 ; j < max_agents ; j++) {
		//Agnoa tis adeies theseis
		if (agent_array[j].pid != 0) {
			if (agent_array[j].socket == age_sock) {
				agent_array[j].pid = 0; //marked for overwrite
			}
		}
	}
}

/* agent_arr_clear():
> Eleutherwnei ton pinaka twn praktorwn, afou prwta kleisei ta sockets tous

> Orismata:
  agent_array: Pinakas praktorwn
  max_agents: Arithmos megistwn praktorwn

> Epistrofes:
 Mesw return:
  > Se periptwsh epityxias to 0
  > Se periptwsh apotyxias to -1
*/
int agent_arr_clear(agent_t *agent_array, int max_agents) {
	int k;

	for (k = 0 ; k < max_agents ; k++) {
		if (agent_array[k].pid != 0) { //Kleinoun ta socket sto array clear (SOS)
			if (my_close(agent_array[k].socket, __FILE__, __LINE__) < 0) {
				free(agent_array);
				return(-1);
			}
		}
	}
	free(agent_array);
	return(0);
}