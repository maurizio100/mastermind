/**
   @file client.c
   @author Maurizio Rinder 0828852
   @date 20.04.2012
**/
/*****************************************
Includes and Constants
*****************************************/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdint.h>
#include<unistd.h>
#include<stdarg.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<limits.h>
#include<errno.h>

#define PARITY_ERR_BIT (6)
#define GAME_LOST_ERR_BIT (7)
#define EXIT_PARITY_ERROR (2)
#define EXIT_GAME_LOST (3)
#define EXIT_MULTIPLE_ERRORS (4)

#define MAXROUNDS (35)
#define SLOTS (5)
#define SENDBYTES (2)
#define READBYTES (1)
#define MSGSIZE (15)
/****************************************
Makros
*****************************************/
#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))
/****************************************
Type Definitions
****************************************/
struct opts{
  char *portno;
  char *serverhost;
};

/*static enum { beige, darkblue, green, orange, red, black, violet, white }; for later */
/*************************************
Global variables
*************************************/
/* program name */
static const char *prgname = "client";
/* filedescriptor for clientsocket */
static int sockfd = -1;
/* addrinfo struct */
static struct addrinfo *ai;
/****************************************
Function prototypes
****************************************/
/**
 * @brief checks the arguments given to the program
 * @param argc The argument counter
 * @param argv The arguments given to the program
 * @param options parced arguments are stored in this structure
 */
static void parse_args(int argc, char **argv, struct opts *options);
/**
 * @brief creates a socket connection
 * @param options that were given to the program as arguments
 * @return the socket filedescriptor
 */
static int connect_to_socket( const struct opts *options );
/**
 * @brief creates a tipp for the mastermind server
 * @param puts the color tipp in the colstring
 * @param number of slots available on the mastermind board
 */
static void create_colorstring(char *colstring, int slots);
static uint16_t create_message( const char *colstring, int slots );
static void free_ressources(void);

/****************************************
Error routines
****************************************/
/**
 * @brief prints usage of the program
 */
static void usage(void);
/**
 * @brief terminates the program on program error
 * @param errmessage An errormessage that says what went wrong
 */ 
static void bailout(int errnumber, const char *errmessage);
/**
 * @brief prints error messages
 * @param errmessage
 */
static void printerror(const char* errmessage);
/**************************************
Functionpointer and operations
**************************************/

/**************************************
Main Procedure
**************************************/
int main(int argc, char **argv){
  /* variables */
  struct opts options;
  int round = 0;
  /*  ssize_t ret = 0; */

  char colstring[SLOTS];
  uint16_t codedstring;
  static uint8_t buffer[SENDBYTES];

  /* arguments check */
  (void) parse_args(argc, argv, &options);
  sockfd = connect_to_socket( &options );

  /* game start */
  for(round = 1; round <= MAXROUNDS; ++round){
  /* -- create colorstring */
    create_colorstring(&colstring[0], SLOTS);
  /* -- prepare for sending (parity) */
    codedstring = create_message( &colstring[0], SLOTS);
    buffer[0] = (uint8_t)codedstring & 0xff;
    buffer[1] = (uint8_t)((codedstring >> 8) & 0xff);
  /* -- send the string */
    write( sockfd, &buffer[0], SENDBYTES );
  /* -- evaluation of answer*/
  /*  ret = read(sockfd, &buffer[0], READBYTES ); */

  }
  /**********clearing and exit***************/
  free_ressources();
  exit(EXIT_SUCCESS);
}
/************************************
Function definitions
 ***********************************/
static void parse_args(int argc, char **argv, struct opts *options){

  char *srvname;
  char *srvport;
  char *endptr;
  long int port;

  if( argc != 3 ){ (void)usage(); }

  srvname = argv[1];
  srvport = argv[2];

  errno = 0;
  /*TODO check servername for correctness*/
  options->serverhost = srvname;
  port = strtol(srvport, &endptr, 10);
  if(( errno == ERANGE && (port == LONG_MAX || port == LONG_MIN))
     || (errno != 0 && port == 0)){
    bailout(EXIT_FAILURE, "strtol");
  }

  if(endptr == srvport){
    bailout(EXIT_FAILURE, "No digits were found");
  }

  if(*endptr != '\0'){
    bailout(EXIT_FAILURE, "Further characters after <server-port>");
  }

  if(port < 1 || port > 65535){
    bailout(EXIT_FAILURE, "Not a valid port number. Use (1-65535)");
  }

  options->portno = srvport;
}

static int connect_to_socket(const struct opts *options){
  int descriptor = -1;
  struct addrinfo hints;
  const char* host = options->serverhost; 
  const char* port = options->portno;
  errno = 0;
  /* init addrinfo */
  hints.ai_flags = 0;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_addrlen = 0;
  hints.ai_addr = NULL;
  hints.ai_canonname = NULL;
  hints.ai_next = NULL;

  if((getaddrinfo(host, port, &hints, &ai)) != 0){
    (void)bailout(EXIT_FAILURE, "getaddrinfo");
  } 

  if(ai == NULL){
    (void)bailout(EXIT_FAILURE, "Could not resolve host.");
  }
 
  if((descriptor = socket( ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0){
    (void)bailout(EXIT_FAILURE, "Socket creation failed");
  }

  if(connect(descriptor, ai->ai_addr, ai->ai_addrlen) < 0){
    (void) close(descriptor);
    bailout(EXIT_FAILURE, "Connection failed");
  }
  
  freeaddrinfo(ai);
  ai = 0;
  return descriptor;
}
static void create_colorstring(char *colstring, int slots){
  int i;
  for(i = 0; i < slots; ++i){
    colstring[i] = 'w';
  }
  /* more to come here */
}
static uint16_t create_message(const char *colstring, int slots){
  uint16_t msg = 0;
  uint8_t paritycalc = 0;
  enum color{ beige = 0, darkblue, green, orange, red, black, violet, white };

  int i = 0;
  for( i = 0; i < SLOTS; ++i ){
    uint8_t color;
    switch( colstring[i] ){
    case 'b': color = beige; break;
    case 'd': color = darkblue; break;
    case 'g': color = green; break;
    case 'o': color = orange; break;
    case 'r': color = red; break;
    case 's': color = black; break;
    case 'v': color = violet; break;
    case 'w': color = white; break;
    default: bailout(EXIT_FAILURE, "Bad color in tipp-sequence");
    }
    color &= 0x7;
    paritycalc ^= color ^ (color >> 1) ^ (color >> 2);
    msg |= (color << (i * 3));
  }
  paritycalc &= 0x1;
  msg |= (paritycalc << 15);
  return msg;

} 
/************************************
Error routines definitions
 ***********************************/
static void usage(void){
  (void)fprintf(stderr,"Usage: %s <serverhostname> <server-port>\n",prgname);
  bailout(EXIT_FAILURE, (const char*) 0);
}

static void bailout(int errnumber, const char *errmessage){
  
  if(errmessage != (const char*) 0){ printerror(errmessage); }

  free_ressources();
  exit(errnumber);
}

static void printerror(const char *errmessage){
  if( errno != 0 ){
    (void) fprintf(stderr, "%s: %s - %s\n", prgname, errmessage, strerror(errno));
  }else{
    (void) fprintf(stderr, "%s: %s\n", prgname, errmessage);
  }
}
static void free_ressources(void){

  if( ai != 0 ){
    (void)freeaddrinfo(ai);
    ai = NULL;
  }   

}
