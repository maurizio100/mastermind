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

#define SENDBYTES (2)
#define READBYTES (1)

#define SLOTS (5)
#define COLORS (8)
/****************************************
Type Definitions
****************************************/
/* stores the connection information */
struct opts{
  char *portno;
  char *serverhost;
};
typedef enum{FALSE, TRUE} bool;
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
 * @brief converts the message to a sendable bitcode
 * @param colstring the chosen colors
 * @param slots the amount of colors 
 */
static void create_message( const char *colstring, uint8_t *buf, int slots );
/**
 * @brief evaluates bit 7 and 6 of server's answer
 * @param buf the buffer where read message is in
 * @return void because it exits program when an error occurs
 */
static void evaluate_errors( const uint8_t *buf );
/**
 * @brief frees all used ressources
 */
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
Main Procedure
**************************************/
int main(int argc, char **argv){
  /******** variables **********/
  struct opts options;
  ssize_t ret = 0;
  static uint8_t buffer[SENDBYTES];

  /* array that keeps all colors appeared in the code */
  static char clrstring[SLOTS];
  static const char colors[] = {'b','d','g','o','r','s','v','w'};

  /* flag array that says which pins are on right position */
  static bool fixed[] = {FALSE, FALSE, FALSE, FALSE, FALSE};
  static char guess[SLOTS];

  int clr = 0, round = 0, clrfound = 0;
  int i = 0, j = 0, clr_occ = 1;
  int reds = 0,  whites = 0, curr_red = 0, curr_white = 0 ;
  char clr_wrong = ' ';
  /******** initialization **********/
  /* arguments check */
  (void) parse_args(argc, argv, &options);
  /******socket*********************/
  if((sockfd = connect_to_socket( &options )) < 0){
    bailout(EXIT_FAILURE, "main No socket created");
  }
 
  /* game start */
  /** first pick up all colors available in the mastermind code **/
  for(clr=0; (clrfound < SLOTS) &&  (clr < COLORS); ++clr){
    round++;

    for(i = clrfound; i < SLOTS; ++i){
      clrstring[i] = colors[clr];
    }
    (void)create_message( &clrstring[0], &buffer[0], SLOTS);
    
    if( (ret = write( sockfd, &buffer[0], SENDBYTES)) < 0 ){
      bailout(EXIT_FAILURE, "main Could not write data");
    }
    if( (ret = read(sockfd, &buffer[0], READBYTES )) < 0 ){
      bailout(EXIT_FAILURE, "main Could not read data");
    }

    (void)evaluate_errors( &buffer[0] );
    
    reds = buffer[0] & 0x7;
    whites = (buffer[0] >> 3) & 0x7; 

    /* when feedback pins are in sum more than before a color is found */
    if((reds+whites) > (curr_red + curr_white)){
      clrfound += ((reds+whites)-(curr_red+curr_white));
    }else{ clr_wrong = colors[clr]; }
    
    curr_red = reds;
    curr_white = whites;

  }
  /** after that loop all colors are found **/
  curr_red = 0;
	if(clr_wrong == ' ' ){ clr_wrong = colors[COLORS-1]; }
  /** now lets see where those colors are placed in the code **/
  if(reds < 5 ){
    for( i = 0; i < SLOTS; ++i ){ guess[i] = clr_wrong; }

    for(i = 0; i < SLOTS; ++i){
      if( i != SLOTS -1 ){
	/* goes to the last equivalent color and also increments its occurence */
	if( clrstring[i] == clrstring[i+1] ){ ++clr_occ; continue; }
      }

      for( j = 0; clr_occ > 0; ++j ){
	if( fixed[j] == FALSE ){
	  round++;
	  guess[j] = clrstring[i];

	  (void)create_message( &guess[0], &buffer[0], SLOTS);
    
	  if( (ret = write( sockfd, &buffer[0], SENDBYTES)) < 0 ){
	    bailout(EXIT_FAILURE, "main Could not write data");
	  }
	  if( (ret = read(sockfd, &buffer[0], READBYTES )) < 0 ){
	    bailout(EXIT_FAILURE, "main Could not read data");
	  }
	  (void)evaluate_errors( &buffer[0] );
    
	  reds = buffer[0] & 0x7;
	  if( reds > curr_red  ){
	    /* with every correct placed color its amount occurence decreases */ 
	    --clr_occ;
	    fixed[j] = TRUE;
	    guess[j] = clrstring[i];
	    curr_red++;
	  }else{
	    guess[j] = clr_wrong;
	  }
	}
      }

      clr_occ = 1;
    }
  }
  /**********clearing and exit***************/
  printf("Runden: %d\n", round);
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
  options->serverhost = srvname;

  port = strtol(srvport, &endptr, 10);
  if(( errno == ERANGE && (port == LONG_MAX || port == LONG_MIN))
     || (errno != 0 && port == 0)){
    bailout(EXIT_FAILURE, "parse_args strtol");
  }

  if(endptr == srvport){
    bailout(EXIT_FAILURE, "parse_args No digits were found");
  }

  if(*endptr != '\0'){
    bailout(EXIT_FAILURE, "parse_args Further characters after <server-port>");
  }

  if(port < 1 || port > 65535){
    bailout(EXIT_FAILURE, "parse_args Not a valid port number. Use (1-65535)");
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
    (void)bailout(EXIT_FAILURE, "connect_to_socket getaddrinfo");
  } 

  if(ai == NULL){
    (void)bailout(EXIT_FAILURE, "connect_to_socket Could not resolve host.");
  }
 
  if((descriptor = socket( ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0){
    (void)bailout(EXIT_FAILURE, "connect_to_socket Socket creation failed");
  }

  if(connect(descriptor, ai->ai_addr, ai->ai_addrlen) < 0){
    (void) close(descriptor);
    bailout(EXIT_FAILURE, "connect_to_socket Connection failed");
  }
  
  freeaddrinfo(ai);
  ai = 0;
  return descriptor;
}

static void create_message(const char *colstring, uint8_t *buf, int slots){
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
  buf[0] = (uint8_t)msg & 0xff;
  buf[1] = (uint8_t)((msg >> 8) & 0xff);

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
static void evaluate_errors( const uint8_t *buf ){
    int parity = (buf[0] >> PARITY_ERR_BIT) & 0x1;
    int gamelost = (buf[0] >> GAME_LOST_ERR_BIT) & 0x1;

    if( parity > 0 && gamelost > 0 ){
      bailout(EXIT_MULTIPLE_ERRORS,"Parity error\nGame lost");
    }
    if( parity > 0 ){
      bailout(EXIT_PARITY_ERROR, "Parity error");
    }
    if( gamelost > 0 ){
      bailout(EXIT_GAME_LOST, "Game lost");
    }

}
static void free_ressources(void){

  if( ai != 0 ){
    (void)freeaddrinfo(ai);
    ai = NULL;
  }   

  if(sockfd > -1 ){
    if( (close(sockfd)) < 0 ){
      printerror("free_ressources Could not close socket");
    }
    sockfd = -1;
  }
}
