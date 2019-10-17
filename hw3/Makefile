CC = gcc
CFLAGS = -g -Wall

all: server agent

server: server.o hw3_wrappers.o
	${CC} ${CFLAGS} server.o hw3_wrappers.o -o $@

agent: agent.o hw3_wrappers.o
	${CC} ${CFLAGS} agent.o hw3_wrappers.o -o $@

server.o: server.c hw3_wrappers.h hw3_types.h
	${CC} ${CFLAGS} server.c -c

agent.o: agent.c hw3_wrappers.h hw3_types.h
	${CC} ${CFLAGS} agent.c -c

hw3_wrappers.o: hw3_wrappers.c hw3_wrappers.h
	${CC} ${CFLAGS} hw3_wrappers.c -c

.PHONY: clean

clean:  #remove all object files
	rm *.o 

	
