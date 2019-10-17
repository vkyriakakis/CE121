#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>
#include "hw3_wrappers.h"

/* my_open():
 * Wrappper tou sys-call open()
 * Epistrofh:
 * > Me return():
 *  fd > 0, se periptwsh epityxias
 * -1, se sfalma
*/

int my_open(char *name, int flags, mode_t mode, char *file, int line) {
	int fd;
	char message[MESSAGE_SIZE] = {'\0'};
	
	sprintf(message, "%s: Error in my_openclose at line %d", file, line);

	fd = open(name, flags, mode);
	if (fd < 0) {
		perror(message);
		return(-1);
	}

	return(fd);
}

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

/* my_unlink():
 * Wrappper tou sys-call unlink()
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias
 * -1, se sfalma
*/

int my_unlink(char *filepath, char *file, int line) {
	char message[MESSAGE_SIZE];

	sprintf(message, "%s: Error in my_unlink at line %d", file, line);

	if (unlink(filepath) < 0) {
		perror(message);
		return(-1);
	}

	return(0);
}

/* my_socket():
 * Wrappper tou sys-call socket()
 * Epistrofh:
 * > Me return():
 *  fd tou neou socket, se periptwsh epityxias
 * -1, se sfalma
*/

int my_socket(int domain, int type, int protocol, char *file, int line) {
	int sock;
	char message[MESSAGE_SIZE];

	sprintf(message, "%s: Error in my_socket at line %d", file, line);

	sock = socket(domain, type, protocol);
	if (sock < 0) {
		perror(message);
		return(-1);
	}

	return(sock);
}

/* my_bind():
 * Wrappper tou sys-call bind()
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias
 * -1, se sfalma
*/

int my_bind(int sock, struct sockaddr *addr, socklen_t addr_len, char *file, int line) {
	char message[MESSAGE_SIZE];

	sprintf(message, "%s: Error in my_bind at line %d", file, line);
	if (bind(sock, addr, addr_len) < 0) {
		perror(message);
		return(-1);
	}

	return(0);
}

/* my_listen():
 * Wrappper tou sys-call listen()
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias
 * -1, se sfalma
*/

int my_listen(int sock, int pending, char *file, int line) {
	char message[MESSAGE_SIZE];

	sprintf(message, "%s: Error in my_listen at line %d", file, line);
	if (listen(sock, pending) < 0) {
		perror(message);
		return(-1);
	}

	return(0);
}

/* my_accept():
 * Wrappper tou sys-call accept()
 * Epistrofh:
 * > Me return():
 *  neo stream socket, se periptwsh epityxias
 * -1, se sfalma
*/

int my_accept(int sock, struct sockaddr *addr, socklen_t *addr_len, char *file, int line) {
	char message[MESSAGE_SIZE];
	int new_sock;

	sprintf(message, "%s: Error in my_accept at line %d", file, line);
	
	new_sock = accept(sock, addr, addr_len);
	if (new_sock < 0) {
		perror(message);
		return(-1);
	}

	return(new_sock);
}

/* my_connect():
 * Wrappper tou sys-call connect()
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias
 * -1, se sfalma
*/

int my_connect(int sock, struct sockaddr *addr, socklen_t len, char *file, int line) {
	char message[MESSAGE_SIZE];

	sprintf(message, "%s: Error in my_connect at line %d", file, line);
	if (connect(sock, addr, len) < 0) {
		perror(message);
		return(-1);
	}

	return(0);
}

/* my_select():
 * Wrappper tou sys-call select()
 * Epistrofh:
 * > Me return():
 *  0, se periptwsh epityxias
 * -1, se sfalma
*/

int my_select(int nfds, fd_set *read_set, fd_set *write_set, fd_set *exception_set, struct timeval *timeout, char *file, int line) {
	char message[MESSAGE_SIZE];

	sprintf(message, "%s: Error in my_select at line %d", file, line);

	if (select(nfds, read_set, write_set, exception_set, timeout) < 0) {
		perror(message);
		return(-1);
	}
	return(0);
}

/* my_shmget():
 * Wrappper tou sys-call shmget()
 * Epistrofh:
 * > Me return():
 *  Anagnwristiko shared memory, se periptwsh epityxias
 * -1, se sfalma
*/

int my_shmget(key_t key, size_t size, int flags, char *file, int line) {
	int shmid;
	char message[MESSAGE_SIZE];

	sprintf(message, "%s: Error in my_shmget at line %d", file, line);
	
	shmid = shmget(key, size, flags);
	if (shmid < 0) {
		perror(message);
		return(-1);
	}

	return(shmid);
}

/* my_shmat():
 * Wrappper tou sys-call shmat()
 * Epistrofh:
 * > Me return():
 * Pointer sto xekinhma tou koinoxrhstou thmhmatos pou prosarththike
 * SHM_FAIL, se sfalma
*/

void *my_shmat(int shmid, void *start, int flags, char *file, int line) {
	char message[MESSAGE_SIZE];
	char *shm_ptr;

	sprintf(message, "%s: Error in my_shmat at line %d", file, line);
	
	shm_ptr = shmat(shmid, start, flags);
	if (shm_ptr == SHM_FAIL) {
		perror(message);
	}

	return(shm_ptr); //returns SHM_FAIL on fail
}

/* my_shmdt():
 * Wrappper tou sys-call shmdt()
 * Epistrofh:
 * > Me return():
 * 0, se epityxia
 * -1, se sfalma
*/

int my_shmdt(void *shm_ptr, char *file, int line) {
	char message[MESSAGE_SIZE];

	sprintf(message, "%s: Error in my_shmdt at line %d", file, line);

	if (shmdt(shm_ptr) < 0) {
		perror(message);
		return(-1);
	}

	return(0);
}

/* my_shmctl():
 * Wrappper tou sys-call shmctl()
 * Epistrofh:
 * > Me return():
 * 0, se epityxia
 * -1, se sfalma
*/

int my_shmctl(int shmid, int cmd, struct shmid_ds *info, char *file, int line) {
	char message[MESSAGE_SIZE];

	sprintf(message, "%s: Error in my_shmctl at line %d", file, line);
	if (shmctl(shmid, cmd, info) < 0) {
		perror(message);
		return(-1);
	}

	return(0);
}

//SEMAPHORE API:
/* O logos pou xrhsimopoieitai anti gia to API tou system V, einai
h apofygh epanalambanomenou kwdika stis anatheseis sta struct
sembuf */

/* sem_create():
 * Dhmiourgia kai arxikopoihsh omadas shmatoforwn.
 * Orismata:
   1) num: O arithmos twn shmatoforwn ths omadas
   2) init_vals: Pinakas me tis arxikes times twn shmatoforwn
   3) Ta file, line vohthane sthn anixneush lathwn
 * Epistrofh:
 * > Me return():
 * Anagnwrstiko semid, se epityxia
 * -1, se sfalma
*/

int sem_create(int num, short *init_vals, char *file, int line) {
	int semid;
	char message[MESSAGE_SIZE];

	sprintf(message, "%s: Error in sem_create at line %d", file, line);

	//Dhmiourgia omadas shmatoforwn me enan shmatoforo
	semid = semget(IPC_PRIVATE, num, S_IRWXU);
	if (semid < 0) {
		perror(message);
		return(-1);
	}

	//Arxikopoihsh tou shmatoforou me thn timh pou dwthike
	if (semctl(semid, 0, SETALL, init_vals) < 0) {
		perror(message);
		return(-1);
	}

	return(semid); //epistrofh anagnwristikou
}

/* sem_up():
 * Auxhsh ths timhs enos shmatofrou mias omadas shamtoforwn kata 1
 * Orismata:
   1) semid: To anagnwristiko ths omadas
   2) semnum: O arithmos tou shmatoforou pros auxhsh (>= 0)
   3) increment: Poso prosauxhshs
 * Epistrofh:
 * > Me return():
 *  0, se epityxia
 * -1, se sfalma
*/

int sem_up(int semid, int semnum, int increment, char *file, int line) {
	char message[MESSAGE_SIZE];
	struct sembuf operation;

	sprintf(message, "%s: Error in sem_up at line %d", file, line);

	operation.sem_num = semnum; 
	operation.sem_op = increment;
	operation.sem_flg = 0;

	if (semop(semid, &operation, 1) < 0) {
		perror(message);
		return(-1);
	}

	return(0);
}

/* sem_down():
 * Meiwsh ths timhs enos shmatofrou mias omadas shamtoforwn kata 1 (an hdh 0, mplokarei)
 * Orismata:
   1) semid: To anagnwristiko ths omadas
   2) semnum: O arithmos tou shmatoforou pros auxhsh (>= 0)
   3) decrement: Poso meiwshs
 * Epistrofh:
 * > Me return():
 *  0, se epityxia
 * -1, se sfalma
*/

int sem_down(int semid, int semnum, int decrement, char *file, int line) {
	char message[MESSAGE_SIZE];
	struct sembuf operation;

	sprintf(message, "%s: Error in sem_down at line %d", file, line);

	operation.sem_num = semnum; 
	operation.sem_op = -decrement;
	operation.sem_flg = 0;

	if (semop(semid, &operation, 1) < 0) {
		perror(message);
		return(-1);
	}

	return(0);
}

/* sem_remove():
 * Katastrofh omadas shmatoforwn.
 * Orismata:
   1) semid: Anagnwristiko ths omadas
 * Epistrofh:
 * > Me return():
 * 0, se epityxia
 * -1, se sfalma
*/
	
int sem_remove(int semid, char *file, int line) {
	char message[MESSAGE_SIZE];

	sprintf(message, "%s: Error in sem_remove at line %d", file, line);

	if (semctl(semid, 0, IPC_RMID, 0) < 0) {
		perror(message);
		return(-1);
	}

	return(0);
}



