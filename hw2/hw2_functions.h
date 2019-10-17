#ifndef HW2_GUARD

#define HW2_GUARD 1

#define END_INPUT 1
#define BLOCK_SIZE 2048
#define MESSAGE_SIZE 128

int my_close(int fd, char *file, int line);
int my_write(int fd, char *buffer, off_t count, char *file, int line);
int my_read(int fd, char *buffer, off_t count, char *file, int line);
int my_dup(int fd, int id, char *file, int line);
pid_t my_fork(char *file, int line);
int my_pipe(int pipe_fd[2], char *file, int line);
int transfer_data(int source_fd, int destination_fd, off_t data_size);
#endif
