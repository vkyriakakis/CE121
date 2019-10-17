#ifndef TYP_GUARD

	#define TYP_GUARD 2018

	#define MAX_AIRLINE 4
	#define MAX_AIRPORT 5

	//Domh pou periexei oles tis plhrofories gia mia pthsh
	typedef struct {
		//Kwdikos aeropwrikhs etairias
		char airline[MAX_AIRLINE];

		//Kwdikos aerodromiou anaxwrishs
		char src[MAX_AIRPORT];

		//Kwdikos aerdromiou afixhs
		char dest[MAX_AIRPORT];

		//Arithmos stasewn
		int stops;

		//Arithmos diathesimwn thesewn
		int seats;
	} flight_t;

	//Domh pou periexei tis plhrofories gia enan praktora

	typedef struct {
		//Process id tou praktora
		pid_t pid;

		//Theseis pou exei krathsei o praktoras
		int seats;

		//Socket epikoinwnias me ton praktora
		int socket;
	} agent_t;
#endif
