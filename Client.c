#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h> /* killing forking process */
#include <sys/wait.h>
#include <locale.h>
#include <arpa/inet.h>

#define MAX_CHARACTERS 1024
#define MAX_USERNAME 20
#define MIN_REQUIRED 3
#define MAX_PORT 65536
#define MIN_PORT 1

/* Usage Instructions void because we do not have any return */
int help(void) {
  printf("Usage: Assingment_5_Client [<arg1>]\n");
  printf("\t: program name <arg0>\n");
  printf("\t: IP:PORT format sample (127.0.0.1:5000) <arg1>\n");
  printf("\t: client username sample (Thanos) <arg2>\n");
  return (1);
}

typedef struct rec {
  char username[MAX_USERNAME];
  char receive[MAX_CHARACTERS];
  char transmit[MAX_CHARACTERS];
  char temp[MAX_CHARACTERS];
}RECORD;

/* Void function does not return a value */
void error(const char *msg) {
  perror(msg);
  exit(0);
}

void chomp(const char *s) {
  char *p;
  while (NULL != s && NULL != (p = strrchr(s, '\n'))){
    if(p) *p = '\0';
  }
} /* chomp */

/* Function declaration */
char *export (int tr_sock , char *ch_tr);

char *export (int tr_sock , char *ch_tr) {

  char* newline = "\n";

  strcat(ch_tr,newline);

  //printf("From socket: '%i', text inside send function: %s",tr_sock,ch_tr);

  if ( send(tr_sock,ch_tr,strlen(ch_tr),0) < 0 )
    error("ERROR writing to socket");

  chomp(ch_tr);

  return ch_tr;

}

/* function declaration */
char *receive (int rcv_sock);

char *receive (int rcv_sock) {

  RECORD *ptr_receive;

  ptr_receive = (RECORD *) malloc (sizeof(RECORD));

  if (ptr_receive == NULL) {
    printf("Out of memmory!\nExit!\n");
    exit(0);
  }

  memset( (*ptr_receive).receive, '\0' , sizeof( (*ptr_receive).receive ) );

  if ((recv(rcv_sock, (*ptr_receive).receive , sizeof((*ptr_receive).receive) , 0)) <= 0)
    error("ERROR reading from socket");

  chomp((*ptr_receive).receive);
  
  return (*ptr_receive).receive;

}

int main(int argc, char **argv) {

  if (!setlocale(LC_CTYPE, "C.UTF-8")) {
    fprintf(stderr, "Can't set the specified locale!"
	    "Check LANG, LC_CTYPE, LC_ALL.\n");
    return 1;
  }

  RECORD *ptr_record;
  
  ptr_record = (RECORD *) malloc (sizeof(RECORD));

  if (ptr_record == NULL) {
    printf("Out of memmory!\nExit!\n");
    exit(0);
  }

  int client_sck,con;
  struct sockaddr_in cli_addr;
  struct hostent *client;

  if ( argc < MIN_REQUIRED ) {
    printf ("Please follow the instructions: not less than %i argument inputs\n",MIN_REQUIRED);
    return help();
  }
  else if ( argc > MIN_REQUIRED ) {
    printf ("Please follow the instructions: not more than %i argument inputs\n",MIN_REQUIRED);
    return help();
  }
  else {

    char qolon = ':';

    char *quotPtr = strchr(argv[1] , qolon);

    if (quotPtr == NULL) {
      printf("\nPlease enter the qolon character ':' between the IP and PORT, e.g. (127.0.0.1:5000)\n\n");
      exit(0);
    }

    char IP[16]; /* IP = 15 digits 1 extra for \0 null terminating character string */

    char PORT_STR[6]; /* Port = 5 digits MAX 1 extra for \0 null terminating character string */

    if (strlen(argv[1]) < 11 ) {
      printf("ERROR, make sure you have not forgot to enter the port number!\n");
      exit(0);
    }

    memset(IP , '\0' , sizeof(IP));
    memset(PORT_STR , '\0' , sizeof(PORT_STR));

    /* http://stackoverflow.com/questions/17085189/c-split-string-into-token-by-delimiter-and-save-as-variables */
    strcpy(IP, strtok(argv[1], ":"));
    strcpy(PORT_STR, strtok(NULL, ":"));

    /* Convert String to long int atoi() is deprecated with base 10 */
    long int PORT = strtol(PORT_STR , (char **)NULL , 10);

    /* Max Port Numbers 65536 */
    if ((PORT > MAX_PORT) || (PORT < MIN_PORT)) {
      printf("Valid PORT number is between the values %i - %i!\nPlease try again.\n",MIN_PORT,MAX_PORT);
      exit(0);
    }

    memset( (*ptr_record).username , '\0' , sizeof( (*ptr_record).username ) );

    strncpy( (*ptr_record).username , argv[2] , sizeof((*ptr_record).username) );

    if((client_sck=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))==-1) {
      printf("\nSocket problem");
      return 0;
    }

    client = gethostbyname(IP);
    if (client == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
    }

    printf("This is the IP: %s\n",IP);

    memset((char *) &cli_addr, '\0' , sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_port = htons(PORT);
    /* Take the local address where the script runs */
    //cli_addr.sin_addr.s_addr=htonl(IP);
    //memcpy((char *)client->h_addr, (char *)&cli_addr.sin_addr.s_addr, client->h_length);

    if( inet_pton( AF_INET , IP , &cli_addr.sin_addr ) <= 0 ) {
      printf("\n inet_pton error occured\n");
      return 1;
    }

    con=connect(client_sck,(struct sockaddr*)&cli_addr,sizeof(cli_addr));
    if(con==-1) {
      printf("\nConnection error\n");
      return 0;
    }

    char *text_trn;
    char *text_rcv;

    /* First Receive */
    text_rcv = receive(client_sck);
    
    printf("First Receive: %s\n",text_rcv);

    if (strcmp(text_rcv,"Hello 1\n")) {
      memset((*ptr_record).transmit , '\0' , sizeof((*ptr_record).transmit));
      strncpy((*ptr_record).transmit, "NICK \0" , sizeof((*ptr_record).transmit));
      strncat((*ptr_record).transmit,(*ptr_record).username,strlen((*ptr_record).username)+1);
      text_trn = export(client_sck,(*ptr_record).transmit);
      printf("First transmit: %s\n",text_trn);
      memset((*ptr_record).transmit , '\0' , sizeof((*ptr_record).transmit));
      text_rcv = receive(client_sck);
      memset((*ptr_record).receive , '\0' , sizeof((*ptr_record).receive));
      strncpy((*ptr_record).receive, text_rcv , strlen(text_rcv)+1);
      if (strncmp((*ptr_record).receive,"OK",2) == 0) {
	pid_t pid;
	pid = fork();
	if (pid == -1) {
	  perror("Error forking");
	  return -1;
	}
	else if (pid != 0) { /* Parent procees */
	  printf("Client '%s' enter your text here (Receive):\n",(*ptr_record).username);
      
	  while (((strcmp(text_rcv,"exit")) && (strcmp(text_rcv,"quit"))) != 0) {
	    text_rcv = receive(client_sck);
	    int i,blank;
	    blank = 0;
	    for (i=0; text_rcv[i] != '\0'; ++i) {
	      if ( text_rcv[i] == ' ' ) {
		++blank;
	      }
	    }

	    switch(blank) {
	    case 1 :
	      break;
	    default :
	      printf("%s\n",text_rcv);
	    }
	  }

	}
	else { /* Child processs */
	  
	  while (((strstr((*ptr_record).transmit,"exit")) || (strstr((*ptr_record).transmit,"quit"))) != 1) {
	    memset( (*ptr_record).transmit , '\0' , sizeof((*ptr_record).transmit) );
	    memset( (*ptr_record).temp , '\0' , sizeof((*ptr_record).temp) );
	    strncpy((*ptr_record).transmit, "MSG " ,sizeof((*ptr_record).transmit));
	    fgets( (*ptr_record).temp , sizeof((*ptr_record).temp) , stdin );
	    
	    if ((*ptr_record).temp[1] == '\0' ) {
	      printf("You are about to send an empty string, please type something!\n");
	    }
	    else {
	      strcat((*ptr_record).transmit,(*ptr_record).temp);
	      chomp((*ptr_record).transmit);
	      text_trn = export(client_sck,(*ptr_record).transmit);
	    }
	    
	    printf("Client '%s' enter your text here (Transmit):\n",(*ptr_record).username);

	  } /* End of while loop */

	  int status = -1;
	  waitpid(pid, &status, WEXITED);
	  kill(pid, SIGTERM);
	 
	} /* End of chicld FORK process */
      } /* End of if NICK is OK */
      else if (strncmp((*ptr_record).receive,"ERROR",5) == 0) {
	printf("%s\n",text_rcv);
	exit(0);
      } /* End of else did not receive ok for NICK */
    } /* End of if receive Hello version */
    else {
      printf("Client did not receive Hello version exit!\n");
      send(client_sck,"exit\0",5,0);
      exit(0);
    }
    
    /* release pointer not to run out of memory */
    free ( ptr_record->username );
    free ( ptr_record->receive );
    free ( ptr_record->transmit );
    free ( ptr_record->temp );
    free( ptr_record );

    free( ptr_record );
    /* Cose client socket */
    close(client_sck);
    return 0;
  }/* End of else argc() */
}/* End of main() */
