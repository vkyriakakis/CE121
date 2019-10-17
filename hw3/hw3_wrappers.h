#ifndef HW3_GUARD

#define HW3_GUARD 2018

#define MESSAGE_SIZE 128
#define MAX_LINE 128 

//Stathera pu dhlwnei to telos tou input
#define END_INPUT 1

//Stathera gia to sfalma stis synarthseis pou mplekoub me shared memory
#define SHM_FAIL (void*)-1

//Protypa synarthsewm
int my_open(char *name, int flags, mode_t mode, char *file, int line);
int my_close(int fd, char *file, int line);
int my_write(int fd, char *buffer, off_t count, char *file, int line);
int my_read(int fd, char *buffer, off_t count, char *file, int line);
int my_unlink(char *filepath, char *file, int line);
int my_socket(int domain, int type, int protocol, char *file, int line);
int my_bind(int sock, struct sockaddr *addr, socklen_t addr_len, char *file, int line);
int my_listen(int sock, int pending, char *file, int line);
int my_accept(int sock, struct sockaddr *addr, socklen_t *addr_len, char *file, int line);
int my_connect(int sock, struct sockaddr *addr, socklen_t len, char *file, int line);
int my_select(int nfds, fd_set *read_set, fd_set *write_set, fd_set *exception_set, struct timeval *timeout, char *file, int line);
int my_shmget(key_t key, size_t size, int flags, char *file, int line);
void *my_shmat(int shmid, void *start, int flags, char *file, int line);
int my_shmdt(void *shm_ptr, char *file, int line);
int my_shmctl(int shmid, int cmd, struct shmid_ds *info, char *file, int line);


//Semaphore API
int sem_create(int num, short *init_vals, char *file, int line);
int sem_up(int semid, int semnum, int increment, char *file, int line);
int sem_down(int semid, int semnum, int decrement, char *file, int line);
int sem_remove(int semid, char *file, int line);

#endif
