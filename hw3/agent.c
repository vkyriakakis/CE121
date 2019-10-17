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

#define BYE -1 //O agent stelnei auto ton kwdiko sto server otan anaxwrei mesw EXIT
#define FULL -2 /*O server stelnei auto ton kwdiko ston agent, otan den 
                 mporei na exyphrethsei allous praktores */


void print_help();


int agent_sock_init(char *server_path);
int exchange_communication_info(int age_sock, int *shmid, int *semid, int *flight_num);
int agent_loop(int age_sock, flight_t *flight_array, int flight_num, int semid);
int find_flights(flight_t *flight_array, int flight_num, flight_t flight_info, int semid);
int parse_command(char *command, char *option, flight_t *flight_info);
int reserve_flight(int age_sock, flight_t *flight_array, int flight_num, int semid, flight_t reservation_info);
int insert_next_word(char *buffer, int max_len);

int main(int argc, char *argv[]) {
	/*
		shmid: Shared memory id (to stelnei o server)
		semid: Id omadas shnatoforwn
		flight_num: Arithmos pthsewn sthn koinoxrhsth mnhmh
	*/
	int shmid, semid, flight_num; 
	//Socket tou praktora (me auto epikoinwnei me ton server)
	int age_sock; 
	//Brisketai se tmhma koinoxrhsths mnhmhs
	flight_t *flight_array;

	/* An sta orismata, yparxei meta to onoma tou ektelesimou to
	"--help", typwnetai mhnyma bohtheias pros to xrhsth. */
	if (argc > 1) {
		if (!strcmp(argv[1], "--help")) {
			print_help();
			return(0);
		}
	}
	/*An den dwthike "--jelp" kai ta orismata einai ligotera apo 2
	  (onoma praktora, kai monopati tou listener socket tou server) */
	if (argc < 2) { 
		printf("[ERROR]: Invalid number of arguements! Run with --help for more info.\n");
		return(1);
	}

	//Dhmiourgia tou socket, kai syndesh me ton server
	age_sock = agent_sock_init(argv[1]);
	if (age_sock < 0) {
		return(1);
	}
	
	/* Antallagh plhroforiwn epikoinwnias, pairnei apo ton server ta stoixeia
	   gia thn shared memory kai thn omada shmatoforwn */
	if (exchange_communication_info(age_sock, &shmid, &semid, &flight_num) < 0) {
		my_close(age_sock, __FILE__, __LINE__);
		return(1);
	}

	//Efoson exei to anagnwristiko ths koinoxrhsths, kanei attach to tmhma
	flight_array = my_shmat(shmid, NULL, 0, __FILE__, __LINE__);
	if (flight_array == SHM_FAIL) {
		my_close(age_sock, __FILE__, __LINE__);
		return(1);
	}

	//Mpainei sto agent loop
	if (agent_loop(age_sock, flight_array, flight_num, semid) < 0) {
		my_close(age_sock, __FILE__, __LINE__);
		my_shmdt(flight_array, __FILE__, __LINE__);
		return(1);
	}

	//Kanei detach to shared memory segment
	if (my_shmdt(flight_array, __FILE__, __LINE__) < 0) {
		my_close(age_sock, __FILE__, __LINE__);
		return(1);
	}

	//Klieinei to agent socket
	if (my_close(age_sock, __FILE__, __LINE__) < 0){
		return(1);
	}

	return(0);
}

/* print_help():
> Perigrafei ta orismata tou programmatos sto xrhsths
*/

void print_help() {
	printf("P2 Homework 3: Flight manager agent program\n\n");
	printf("Arguements: <program_name> <server socket path>\n");
}

/* exchange_communication_info():
> Antallasei plhrofories epikoinwnias me ton server, o agent stelnei
to pid tou, enw o server ta aparaiththa stoixeia IPC

> Orismata:
 1) age_sock: To socket tou agent, exei ginei syndesh me ton server
 2) shmid: Pointer ste metablhth apothikeushs tou anagnwristikou ths koin. mnhmhs
 3) semid: Pointer sto anagnwristiko ths omadas shmatoforwn
 4) flight_num: Pointer ston arithmos twn pthsewn sthn shared memory
> Epistrofes:

 Mesw return:
  > 0 se periptwsh epityxias
  > -1 se periptwsh apotyxias
 Mesw deikth:
  Ta semid, shmid, kai o arithmos twn pthsewn
*/

int exchange_communication_info(int age_sock, int *shmid_ptr, int *semid_ptr, int *flight_num_ptr) {
	pid_t self_pid;

	//Send agent pid to server
	self_pid = getpid();
	if (my_write(age_sock, (char*)&self_pid, sizeof(pid_t), __FILE__, __LINE__) < 0) {
		return(-1);
	}

	//Paralambanei to shared memory id apo server
	if (my_read(age_sock, (char*)shmid_ptr, sizeof(int), __FILE__, __LINE__) < 0) {
		return(-1);
	}

	/* An o server den mporei na diaxeiristei allous praktores, stelnei to mhnyma FULL,
	anti gia to shmid, (to FULL orizetai sthn arxh twn server.c, agent.c) */
	if (*shmid_ptr == FULL) {
		printf("[ERROR]: The maximum number of agents has been reached! Please try again later.\n");
		return(-1);
	}

	//Paralabh tou arithmou pthsewn apo ton server
	if (my_read(age_sock, (char*)flight_num_ptr, sizeof(int), __FILE__, __LINE__) < 0) {
		return(-1);
	}

	/* Paralambanei to id ths omadas shmatoforwn */
	if (my_read(age_sock, (char*)semid_ptr, sizeof(int), __FILE__, __LINE__) < 0) {
		return(-1);
	}

	return(0);
}

/* agent_loop():
> Einai ypeuthinh gia thn allhlepidrash tou xrhsth me to programma.
 Typwnei to menu kai dexetai entoles apo ton xrhsth. Akomh 
 parakolouthei an o server einai se leitourgia, kai an oxi 
 enhmerwnei ton xrhsth kai termatizei.

> Orismata:
 age_sock: To socket tou praktora
 flight_array: O pinakas twn pthsewn
 flight_num: To megethos tou pinaka twn pthsewn
 semid: To anagnwristiko ths omadas shmatoforwn

> Epistrofes:
 Mesw return:
  > 0 se periptwsh epityxias
  > -1 se periptwsh apotyxias sys-call
*/

int agent_loop(int age_sock, flight_t *flight_array, int flight_num, int semid) {
	char command[MAX_LINE]; //Entolh tou xrhsth
	char option = 0; //Leitourgia pou epilexthke
	flight_t flight_info; //Plhrofories pthshs (apo xrhsth)
	int bye_message = BYE; //Apothikeuetai to mhnyma apoxwrishs BYE
	fd_set r_master_set, r_set; //Set diabasmatos gia thn select
	int nfds = age_sock+1; //megistos fd + 1
	int check; //metablhth elegxou
	char temp;

	//Arxikopoihsh synolou ths select
	FD_ZERO(&r_master_set);
	FD_SET(age_sock, &r_master_set);
	FD_SET(STDIN_FILENO, &r_master_set);

	do {
		//Arxikopoiw to proswrino set ws to master
		r_set = r_master_set;

		printf("~~~~~~~~~~~~ COMMANDS ~~~~~~~~~~~~\n");
		printf("\tFIND SRC DEST NUM\n");
		printf("\tRESERVE SRC DEST AIRLINE NUM\n");
		printf("\tEXIT\n");

		/* Select gia polyplexia tou STDIN (input xrhsth) kai tou socket,
          (parakolouthei an o server ekleise, ara kai to socket tou server) */
		if (my_select(nfds, &r_set, NULL, NULL, NULL, __FILE__, __LINE__) < 0) {
			return(-1);
		}

		//An o xrhsths edwse input
		if (FD_ISSET(STDIN_FILENO, &r_set)) {
			fgets(command, MAX_LINE, stdin);
			command[strlen(command)-1] = '\0'; //afairese to '\n'

			//An h entolh einai egkyrh
			if (!parse_command(command, &option, &flight_info)) {
				switch(option) {
					case 'f':
						//Entolh FIND SRC DEST NUM
						if (find_flights(flight_array, flight_num, flight_info, semid) < 0) {
							return(-1);
						}
						break;
					case 'r':
						//RESERVE SRC DEST AIRLINE NUM
						if (reserve_flight(age_sock, flight_array, flight_num, semid, flight_info) < 0) {
							return(-1); 
						}
						break;
					case 'e':
						//EXIT
						// Stelnei BYE gia na eidopoihsei ton server gia thn anaxwrish tou praktora
						if (my_write(age_sock, (char*)&bye_message, sizeof(int), __FILE__, __LINE__) < 0) {
							return(-1); 
						}
						break;
				}
			}
			//Alliws an oxi egkyrh
			else {
				printf("[ERROR]: Incorrect command format!\n");
			}
		}
		//An ekleise h syndesh apo to server, autos periexetai sto read set ths select
		if (FD_ISSET(age_sock, &r_set)) {
			//kanw ena mikro diabasma me thn temp
			check = my_read(age_sock, &temp, 1, __FILE__, __LINE__); 
			if (check < 0) {
				return(-1);
			}
			//An edwse END_INPUT, o server exei kleisei
			else if (check == END_INPUT) {
				printf("[ERROR]: Server disconnected! Now exiting...\n");
				break;
			}
		}
	} while (option != 'e'); //Trexe to loop mexri o xrhsths na dwsei EXIT

	return(0);
}

/* find_flights():
> Psaxnei ston pinaka twn pthsewn (sth shared memory) gia
 pthseis pou plhroun sygekrimena krithria, kai tis ektypwnei.
 Oso psaxnei, me thn xrhsh shmatoforou, empodizei allh
 prospelash (diabasma/grapsimo) sthn koinoxrhsth
 mnhmh.

> Orismata:
 flight_array: O pinakas twn pthsewn
 flight_num: To megethos tou pinaka
 flight_info: Ta krithria pou prepei na plhroun oi pthseis
 semid: To anagnwristiko ths omadas shamtoforwn

> Epistrofes:
 Mesw return:
  > To fd tou socket tou praktora se periptwsh epityxias
  > -1 se periptwsh apotyxias
*/

int find_flights(flight_t *flight_array, int flight_num, flight_t flight_info, int semid) {
	int k;
	//To found_flag ginetai 1 an brethei toulaxiston mia pthsh
	int found_flag = 0;

	//Katebazei thn timh tou shmatoforou kata 1
	if (sem_down(semid, 0, 1, __FILE__, __LINE__) < 0) { 
		return(-1);
	}

	for (k = 0 ; k < flight_num ; k++) {
		//An plhrountai oi 3 proypotheseis
		if (!strcmp(flight_info.src, flight_array[k].src) && !strcmp(flight_info.dest, flight_array[k].dest) &&
			(flight_info.seats <= flight_array[k].seats)) {

			//Typwse thn pthsh
			printf("[FLIGHT]: ");
			printf("SRC: %s, ", flight_array[k].src);
			printf("DEST: %s, ", flight_array[k].dest);
			printf("AIRLINE: %s, ", flight_array[k].airline);
			printf("NUM: %d, ", flight_array[k].seats);
			printf("STOPS: %d\n", flight_array[k].stops);

			//Ypswse to flag
			found_flag = 1;
		}
	}
	//An den brethikan pthseis
	if (!found_flag) {
		printf("[FAILURE]: No flights fitting the specified criteria were found!\n");
	}

	//Anebazei thn timh tou shmatoforou
	if (sem_up(semid, 0, 1, __FILE__, __LINE__) < 0) {
		return(-1);
	}

	return(0);
}

/* reserve_flight():
> Prospathei na kleisei kapoio arithmo eishthiriwn
 ths pthshs me stoixeia pou eina apothikeumena
 sthn domh reservation_info

> Orismata:
 age_sock: To socket tou praktora
 flight_array: O pinakas twn pthsewn (shm)
 flight_num: To megethos tou pinaka
 semid: To anagnwristiko ths omadas shamtoforwn
 reservation_info: Ta stoixeia ths krathshs
 	(aerdromio anaxwrishs, afixhs, aeroporikh etaria
 	 zhtoumenos arithmos eishthriwn)

> Epistrofes:
 Mesw return:
  > 0 se periptwsh epityxias
  > -1 se periptwsh apotyxias
*/

int reserve_flight(int age_sock, flight_t *flight_array, int flight_num, int semid, flight_t reservation_info) {
	int k;

	//Katebazei ton shmatoforo kata 1
	if (sem_down(semid, 0, 1, __FILE__, __LINE__) < 0) {
		return(-1);
	}
	for (k = 0 ; k < flight_num ; k++) {
		//An vrethei h pthsh pou plhrei tis prodiagrafes sto flight_info
		if (!strcmp(flight_array[k].airline, reservation_info.airline) &&
			!strcmp(flight_array[k].src, reservation_info.src) &&
			!strcmp(flight_array[k].dest, reservation_info.dest)) {

			//Kai an oi yparxouses theseis einai perissoteres h ises me tis zhtoymenes
			if (flight_array[k].seats >= reservation_info.seats) { 
				flight_array[k].seats -= reservation_info.seats; 

				//Stelnetai ston server o arithmos twn eishthriwn ths krathshs
				if (my_write(age_sock, (char*)&(reservation_info.seats), sizeof(int), __FILE__, __LINE__) < 0) {
					sem_up(semid, 0, 1, __FILE__, __LINE__); //Gia na apofygw deadlocks, to anebazw feugontas
					return(-1);
				}
				printf("[SUCCESS]: The reservation was successful!\n");
			}
			else { //Den yparxoun arketes theseis
				printf("[FAILURE]: The specified flight does not have enough free seats!\n");
			}
			break;
		}
	}
	if (k == flight_num) {
		//Den vrethike kapoia pthsh me auta ta stoixeia (aerodromia kai etairia)
		printf("[FAILURE]: The specified flight does not exist!\n");
	}
	//Anebazei ton shmatoforo kata 1
	if (sem_up(semid, 0, 1, __FILE__, __LINE__) < 0) {
		return(-1);
	}

	return(0);
}

/* parse_command():
> Xwrizei mia entolh pou dinei o xrhstsh se tokens,
 kai epistrefei thn leitourgia kai ta stoixeia
 pthshs pou periexontai se auth kanontas parsing

> Orismata:
 command: H entolh pros parsing
 option_ptr: Plhroforia gia thn leitourgia poy epelexe o xrhsths 
  ('f' ~ FIND, 'r' ~ RESERVE, 'e' ~ EXIT)
 flight_info_ptr: Deikths se domh sthn opoia paketarontai stoixeia pthshs
  pou edwse o xrhsths

> Epistrofes:
 Mesw return:
  > 0 se periptwsh swsths entolhs
  > 1 se periptwsh mh apodekths entolhs
 Mesw deikth:
  > Plhroforia gia thn leitourgia mesw tou option_ptr
  > Plhrofories pthshs mesw tou flight_info_ptr
*/

int parse_command(char *command, char *option_ptr, flight_t *flight_info_ptr) {
	char *word;

	//Xrhsh strtok gia ton xwrismo ths entolhs se tokens

	/* Morfh egkyrhs entolhs:
	 FIND SRC DEST NUM |
	 RESERVE SRC DEST AIRLINE NUM |
	 EXIT
	*/

	//H prwth lexh mias entolhs prepei na einai FIND/RESERVE/EXIT
	word = strtok(command, " ");
	if (word == NULL) {
		return(1);
	}
	//An h lexh antistoixei se entolh, metaferei exw to eidos ths
	if (!strcmp(word, "FIND")) {
		*option_ptr = 'f';
	}
	else if (!strcmp(word, "RESERVE")) {
		*option_ptr = 'r';
	}
	else if (!strcmp(word, "EXIT")) {
		*option_ptr = 'e';
		return(0); //edw teleiwse gia to EXIT
	}
	//Alliws h entolh einai lathos, kai epistrefei
	else {
		return(1);
	}

	//H deuterh lexh einai to aerodromio anaxwrishs
	if (insert_next_word(flight_info_ptr->src, MAX_AIRPORT-1)) {
		//An den yparxei, h exei lathos mhkos, h entolh einai lathos
		return(1);
	}
	//Trith lexh einai to aerodromio afixhs
	if (insert_next_word(flight_info_ptr->dest, MAX_AIRPORT-1)) {
		//An den yparxei, h exei lathos mhkos, h entolh einai lathos
		return(1);
	}
	//An reserve h leitourgia exei epipleon lexh, to AIRLINE
	if (*option_ptr == 'r') { 
		if (insert_next_word(flight_info_ptr->airline, MAX_AIRLINE-1)) {
			//An den yparxei, h exei lathos mhkos, h entolh einai lathos
			return(1);
		}
	}
	//Kai ta 2, RESERVE kai FIND, teliwnoun me ton arithmo thwn eishthriwn
	word = strtok(NULL, " ");
	if (word == NULL) {
		//H entolh einai lathos
		return(1);
	}
	//arithmos thesewn apothikeuetai se akeraio
	flight_info_ptr->seats = atoi(word); 

	return(0); //Epityxes parsing, swsto command
}

/* insert_next_word():
> Bohthitikh synarthsh ths parse_command, elegxei
an h epomenh lexh yparxei, kai exei katallhlo mhkos,
kai an nai thn epistrefei.

> Orismata:
 buffer: Buffer gia thn apothikeush ths lexhs
 max_len: To megisto mhkos ths lexhs

> Epistrofes:
 Mesw return:
  > 0 se periptwsh epityxias
  > 1 an h lexh den yparxei h den exei to apaitoumeno mhkos
*/

int insert_next_word(char *buffer, int max_len) {
	char *word;

	word = strtok(NULL, " ");
	if (word == NULL) {
		return(1);
	}
	if (strlen(word) > max_len){
		return(1);
	}
	strncpy(buffer, word, max_len);

	return(0);
}

/* agent_sock_init():
> Arxikopoiei to socket tou praktora, dhmiourgontas kai 
syndeontas to me ton server.
> Orismata:
 server_path: To monopati sto file system tou listener 
  socket tou server

> Epistrofes:
 Mesw return:
  > To fd tou socket tou praktora se periptwsh epityxias
  > -1 se periptwsh apotyxias
*/

int agent_sock_init(char *server_path){
	int age_sock; //Socket tou praktora
	struct sockaddr_un server_addr; //Dieuthynsh tou server

	//Dhmiourgia tou socket
	age_sock = my_socket(AF_UNIX, SOCK_STREAM, 0, __FILE__, __LINE__);
	if (age_sock < 0) {
		return(-1);
	}

	//Symplhrwsh ths dieuthynshs tou server
	bzero((char*)&server_addr, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX;
	strncpy(server_addr.sun_path, server_path, 108);

	//Syndesh me ton server
	if (my_connect(age_sock, (struct sockaddr*)&server_addr, sizeof(server_addr), __FILE__, __LINE__) < 0) {
		my_close(age_sock, __FILE__, __LINE__);
		return(-1);
	}

	return(age_sock);
}
