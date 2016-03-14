#include <stdio.h> /* stderr, stdout */
#include <errno.h> /* errno.h - system error numbers */
#include <stdlib.h> /* memory allocation, process control etc. */
#include <string.h> /* strncpy function etc. */
#include <unistd.h> /* fork, pipe and I/O primitives (read, write, close, etc.) */
#include <sys/types.h> /* historical (BSD) implementations */
#include <sys/socket.h> /* AF_INET IPv4 Internet protocols */
#include <netinet/in.h> /* in_addr structure, htons() function */
#include <arpa/inet.h> /* inet_addr() */
#include <netdb.h>
#include <signal.h> /* For zombies */
#include <regex.h> /* For REGEX */
#include <sys/wait.h>
#include <locale.h>

#define tofind "[^A-Za-z0-9_]" /* Non alphabetical characters */
#define MAX_CHARACTERS 256
#define MAX_USERNAME 12
#define MAX_USERS 1024
#define MIN_REQUIRED 2
#define MAX_PORT 65536
#define MIN_PORT 1
#define TRUE 1

typedef struct rec {
  char receive[MAX_CHARACTERS];
  char transmit[MAX_CHARACTERS];
  char username[MAX_CHARACTERS];
  char users[MAX_USERS][MAX_USERNAME];
}PERMANENT;

/* get sockaddr, IPv4 or IPv6: */
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  else {
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
  }
}

#define handle_error(msg)				\
  do {perror(msg); exit(EXIT_FAILURE);} while (0)

/* Usage Instructions void because we do not have any return */
int help(void) {
  printf("Usage: server.c [<arg0>] [<arg1]\n");
  printf("\t: a string program name <arg0>\n");
  printf("\t: a string with the IP and PORT sample (127.0.0.1:5000) <arg1>\n");
  return (1);
}

void chomp(const char *s) {
  char *p;
  while (NULL != s && NULL != (p = strrchr(s, '\n'))){
    if(p) *p = '\0';
  }
} /* chomp */

void SigCatcher(int n) {
  wait3(NULL,WNOHANG,NULL);
}

/* Function declaration */
char *export (int tr_sock , char *ch_tr);

char *export (int tr_sock , char *ch_tr) {
  char* newline = "\n";
  strcat(ch_tr,newline);

  /* strlen + 1 for the null terminating character */
  if ( send(tr_sock,ch_tr,strlen(ch_tr)+1,0) < 0 )
    handle_error("ERROR writing to socket");
  chomp(ch_tr);
  return ch_tr;
}

/* function declaration */
char *receive (int rcv_sock);

char *receive (int rcv_sock) {
  PERMANENT *ptr_receive;

  ptr_receive = malloc (sizeof(PERMANENT));

  if (ptr_receive == NULL) {
    printf("Out of memmory!\nExit!\n");
    exit(0);
  }

  memset( (*ptr_receive).receive, '\0' , sizeof( (*ptr_receive).receive ) );

  if ((recv(rcv_sock, (*ptr_receive).receive , sizeof((*ptr_receive).receive), 0)) <= 0)
    handle_error("ERROR reading from socket");

  chomp((*ptr_receive).receive);

  return (*ptr_receive).receive;
}

int main(int argc, char **argv) {

  if (!setlocale(LC_CTYPE, "C.UTF-8")) {
    fprintf(stderr, "Can't set the specified locale! "
	    "Check LANG, LC_CTYPE, LC_ALL.\n");
    return 1;
  }

  fd_set master;    // master file descriptor list
  fd_set read_fds;  // temp file descriptor list for select()
  int fdmax;        // maximum file descriptor number

  int server_sck;     // listening socket descriptor
  int newfd;        // newly accept()ed socket descriptor

  struct sockaddr_storage remoteaddr; // client address
  socklen_t addrlen;

  regex_t regex;

  char * final_transmit;
  char * final_receive;
  char * first_transmit;
  char * pch;
  char * error = "ERROR";

  //int yes = 1;        // for setsockopt() SO_REUSEADDR, below
  int fd_socket, j, rv, reti;

  struct addrinfo hints, *ai, *p;

  FD_ZERO(&master);    // clear the master and temp sets
  FD_ZERO(&read_fds);

  /* Zombie porblem solved! */
  signal(SIGCHLD,SigCatcher);

  if ( argc < MIN_REQUIRED ) {
    printf ("Please follow the instructions: not less than %i argument inputs\n",MIN_REQUIRED);
    return help();
  }
  else if ( argc > MIN_REQUIRED ) {
    printf ("Please follow the instructions: not more than %i argument inputs\n",MIN_REQUIRED);
    return help();
  }
  else {
    PERMANENT *ptr_record;
    ptr_record = malloc (sizeof(PERMANENT));

    if (ptr_record == NULL) {
      printf("Out of memmory!\nExit!\n");
      exit(0);
    }

    char qolon = ':';

    char *quotPtr = strchr(argv[1] , qolon);

    if (quotPtr == NULL) {
      printf("\nPlease enter the qolon character ':' between the IP and PORT, e.g. (127.0.0.1:5000)\n\n");
      exit(0);
    }

    if (strlen(argv[1]) < 11 ) {
      printf("ERROR, make sure you have not forgot to enter the port number!\n");
      exit(0);
    }

    char IP[10]; /* IP = 9 digits 1 extra for \0 null terminating character string */
    char PORT[6]; /* Port = 5 digits MAX 1 extra for \0 null terminating character string */

    memset(IP , '\0' , sizeof(IP));
    memset(PORT , '\0' , sizeof(PORT));

    /* Alternative Solution */
    /* char *IP = strtok(argv[1] , ":");
       char *PORT = strtok(NULL , ":"); */

    strcpy(IP, strtok(argv[1], ":"));
    strcpy(PORT, strtok(NULL, ":"));

    /* Convert String to long int atoi() is deprecated with base 10 */
    long int PORT_COMPARE = strtol(PORT , (char **)NULL , 10);

    //long int IP_INT = strtol(IP , (char **)NULL , 10);

    /* Max Port Numbers 65536 */
    if ((PORT_COMPARE > MAX_PORT) || (PORT_COMPARE < MIN_PORT)) {
      printf("Valid PORT number is between the values %i - %i!\nPlease try again.\n",MIN_PORT,MAX_PORT);
      printf("You entered: %li\n",PORT_COMPARE);
      exit(0);
    }

    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    /* Because of AI_PASSIVE */
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
      fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
      exit(1);
    }

    for(p = ai; p != NULL; p = p->ai_next) {
      server_sck = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
      if (server_sck < 0) {
	continue;
      }

      /* Enable this option in case that we want to reuse the same socket, in our case we want them to be unique */
      //setsockopt(server_sck, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

      if (bind(server_sck, p->ai_addr, p->ai_addrlen) < 0) {
	close(server_sck);
	continue;
      }

      break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
      fprintf(stderr, "selectserver: failed to bind\n");
      exit(2);
    }

    freeaddrinfo(ai); // all done with this

    // listen
    if (listen(server_sck, 10) == -1) {
      perror("listen");
      exit(3);
    }

    // add the listener to the master set
    FD_SET(server_sck, &master);

    // keep track of the biggest file descriptor
    fdmax = server_sck; // so far, it's this one

    printf("\nServer is waiting for clients on IP: %s and PORT: %s\n",IP,PORT);

    /* Compile regular expression */
    reti = regcomp(&regex, tofind, REG_EXTENDED);
    if( reti ){ fprintf(stderr, "Could not compile regex\n"); exit(1); }

    /* Infinite loop TRUE = 1 */
    while (TRUE) {
      read_fds = master; // copy it
      if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
	perror("select");
	exit(4);
      }

      /* Check all existing ports one by one if there are any data to collect */
      for(fd_socket = 0; fd_socket <= fdmax; fd_socket++) {
	/* We check if the socket is registered if not register as newfd */
	if (FD_ISSET(fd_socket, &read_fds)) {
	  if (fd_socket == server_sck) {
	    /* Get the address length */
	    addrlen = sizeof remoteaddr;

	    /* Accept new sockets and register */
	    newfd = accept(server_sck,(struct sockaddr *)&remoteaddr,&addrlen);

	    /* Error conctrol */
	    if (newfd == -1) {
	      perror("accept");
	    }
	    else {
	      /* Register new socket to the registered sockets */
	      FD_SET(newfd, &master);

	      /* In case that the socket number assigned by the
		 server exceed the number 5000 set the new fdmax */
	      if (newfd > fdmax) {
		fdmax = newfd;
	      }

	      printf("selectserver: new connection from %s on socket %d\n",
		     inet_ntop(remoteaddr.ss_family,get_in_addr((struct sockaddr*)&remoteaddr),
			       IP, INET6_ADDRSTRLEN),newfd);
	    }

	    /* fist send */
	    memset((*ptr_record).transmit, '\0' , sizeof((*ptr_record).transmit));
	    strncpy((*ptr_record).transmit,"Hello version\0",sizeof((*ptr_record).transmit));
	    //printf("This is the message to send: %s\n",(*ptr_record).transmit);
	    first_transmit = export(newfd,(*ptr_record).transmit);
	    printf("First export: %s\n",first_transmit);
	  }
	  else { /* Registered socket check read data */
	    final_receive = NULL;
	    final_receive = receive(fd_socket);
	    if (strlen(final_receive) > MAX_CHARACTERS) {
	      printf("Final receive more than MAX_CHARACTERS!\n");
	      memset((*ptr_record).transmit , '\0' , sizeof((*ptr_record).transmit));
	      strncpy((*ptr_record).transmit,"You have send a huge string!\n",sizeof((*ptr_record).transmit));
	      final_transmit = export(fd_socket,(*ptr_record).transmit);
	      memset((*ptr_record).transmit , '\0' , sizeof((*ptr_record).transmit));
	    }
	    else {
	      if ((strncmp( final_receive , "NICK" , 4 )) == 0) {
		pch = strtok(final_receive," ");
		while (pch != NULL) {
		  //printf("PCH: %s\n",pch);
		  memset((*ptr_record).username , '\0' , sizeof((*ptr_record).username));
		  strncpy((*ptr_record).username , pch , sizeof((*ptr_record).username));
		  pch = strtok (NULL, " ");
		} /* End of While(pch != NULL) */

		reti = regexec(&regex , (*ptr_record).username , 0 , NULL , 0);

		if (strlen((*ptr_record).username) > MAX_USERNAME ) {
		  memset((*ptr_record).transmit , '\0' , sizeof((*ptr_record).transmit));
		  strncpy((*ptr_record).transmit , error , sizeof((*ptr_record).transmit));
		  strcat((*ptr_record).transmit, " The choosen nickname: ");
		  strcat((*ptr_record).transmit, "'");
		  strcat((*ptr_record).transmit, (*ptr_record).username);
		  strcat((*ptr_record).transmit, "'");
		  strcat((*ptr_record).transmit , " is too long please limit the size to ");
		  sprintf((*ptr_record).receive, "%i", MAX_USERNAME);
		  strcat((*ptr_record).transmit, (*ptr_record).receive);
		  strcat((*ptr_record).transmit, " characters!");
		  final_transmit = export(fd_socket,(*ptr_record).transmit);
		  printf("Long nickname: %s\n",final_transmit);
		  close(fd_socket); /* Release socket */
		  FD_CLR(fd_socket, &master); /* Final step remove socket from the list */
		} /* End of if long nickname */
		else if (!reti) {
		  memset((*ptr_record).transmit , '\0' , sizeof((*ptr_record).transmit));
		  strncpy((*ptr_record).transmit , error , sizeof((*ptr_record).transmit));
		  strcat((*ptr_record).transmit, " The choosen nickname: ");
		  strcat((*ptr_record).transmit, "'");
		  strcat((*ptr_record).transmit, (*ptr_record).username);
		  strcat((*ptr_record).transmit, "'");
		  strcat((*ptr_record).transmit , " contains non alphabetical characters!");
		  final_transmit = export(fd_socket,(*ptr_record).transmit);
		  printf("Last ReGeX nickname: %s\n",final_transmit);
		  close(fd_socket); /* Release socket */
		  FD_CLR(fd_socket, &master); /* Final step remove socket from the list */
		} /* End of else if (regex conditions) */
		else {
		  strncpy((*ptr_record).users[fd_socket] , (*ptr_record).username , strlen((*ptr_record).username)+1);
		  memset((*ptr_record).transmit , '\0' , sizeof((*ptr_record).transmit));
		  strncpy((*ptr_record).transmit, "OK" , sizeof((*ptr_record).transmit));
		  final_transmit = export(fd_socket,(*ptr_record).transmit);
		  for (j = 0; j <= fdmax; j++) { /* Loop through all sockets */
		    if (FD_ISSET(j, &master)) { /* Detect the socket(s) if they are registered on the list then forward */
		      printf("User: %s possition: %i\n",(*ptr_record).users[j],j);
		    } // if loop
		  } // For looping
		} /* End of NICK OK */
	      } /* End of if string equal NICK */
	      else if ((strncmp( final_receive , "MSG" , 3 )) == 0) {
		memset((*ptr_record).receive , '\0' , sizeof((*ptr_record).receive));
		strcpy((*ptr_record).receive , &final_receive[1]);
		strcpy((*ptr_record).receive , &final_receive[2]);
		strcpy((*ptr_record).receive , &final_receive[3]);
		if (((strstr(final_receive,"exit")) || (strstr(final_receive,"quit"))) != 0) {
		  printf("selectserver: socket %d hung up\n", fd_socket); /* If client terminates, release socket */
		  close(fd_socket); /* Release socket */
		  FD_CLR(fd_socket, &master); /* Final step remove socket from the list */
		  for (j = 0; j <= fdmax; j++) { /* Loop through all sockets */
		    if (FD_ISSET(j, &master)) { /* Detect the socket(s) if they are registered on the list then forward */
		      printf("User: %s possition: %i\n",(*ptr_record).users[j],j);
		    } // if loop
		  } // For looping
		  memset((*ptr_record).transmit , '\0' , sizeof((*ptr_record).transmit));
		  memset((*ptr_record).receive , '\0' , sizeof((*ptr_record).receive));
		}
		final_receive = NULL;
		for (j = 0; j <= fdmax; j++) { /* Loop through all sockets */
		  if (FD_ISSET(j, &master)) { /* Detect the socket(s) if they are registered on the list then forward */
		    if (j != server_sck && j != fd_socket) { /* Forward the message to all except our self and the server */
		      memset((*ptr_record).transmit , '\0' , sizeof((*ptr_record).transmit));
		      strncpy((*ptr_record).transmit, "MSG ", sizeof((*ptr_record).transmit));
		      strcat((*ptr_record).transmit , (*ptr_record).users[fd_socket]);
		      strncat((*ptr_record).transmit, (*ptr_record).receive , strlen((*ptr_record).receive));
		      final_transmit = NULL;
		      final_transmit = export(j,(*ptr_record).transmit);
		      memset((*ptr_record).transmit , '\0' , sizeof((*ptr_record).transmit));
		    } /* Check if the fd is not the Server or the one that came from */
		  } /* End check if fd is part of the list on the Server */
		} /* End of for all sockets loop */
	      }
	      else { /* Else there is an error while receiving */
		perror("Unkown Error rcv");
	      }
	    } /* End of strlen in the boundaries less than MAX_CHARACTERS */
	  } // END handle data from client
	} // END got new incoming connection
      } // END looping through file descriptors
    } /* END while TRUE */

    /* Release pointer receive() function */
    free ( ptr_record->receive );
    free ( ptr_record->transmit );
    free ( ptr_record->username );
    free ( ptr_record->users );
    free( ptr_record );
  } /* End of else argc () */
  return 0;
}
