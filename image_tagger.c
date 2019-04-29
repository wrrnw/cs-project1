/*
 * image_tagger.c
 * Deived from lab6 http-server.c
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// constants
#define MAX_KEYWORD_LEN 20
#define MAX_NUM_KEYWORDS_TO_GUESS 20
static char const * const HTTP_200_FORMAT = "HTTP/1.1 200 OK\r\n\
Content-Type: text/html\r\n\
Content-Length: %ld\r\n\r\n";
static char const * const HTTP_400 = "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_400_LENGTH = 47;
static char const * const HTTP_404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
static int const HTTP_404_LENGTH = 45;
static char const* const INTRO_PAGE = "./html/1_intro.html";
static char const* const START_PAGE = "./html/2_start.html";
static char const* const FIRST_TURN_PAGE = "./html/3_first_turn.html";
static char const* const ACCEPTED_PAGE = "./html/4_acceptedhtml";
static char const* const DISCARDED_PAGE = "./thml/5_discarded.html";
static char const* const ENDGAME_PAGE = "./html/6_endgame.html";
static char const* const GAMEOVER_PAGE = "./html/7_gameover.html";


// function prototype
void initialisePlayersInfo();
int InitialiseServerSocket(const char* readableIP, const int portNumber);
int showIntroPage(int n, char* buff, int sockfd);
int showStartPage(int n, char* buff, int sockfd, char* username);
int showFirstTurnPage(int n, char* buff, int sockfd);
int showGameoverPage(int n, char* buff, int sockfd);
int showDiscardedPage(int n, char* buff, int sockfd);
int showAcceptedPage(int n, char* buff, int sockfd);
int showEndgamePage(int n, char* buff, int sockfd);
bool checkFinish(int sockfd, char* wordTyped);

// represents the types of method
typedef enum
{
    GET,
    POST,
    UNKNOWN
} METHOD;


// set some values for player one and player two
bool playerOneActive, playerTwoActive;
int playerOneSocket, playerTwoSocket;
char playerOneWordList[MAX_NUM_KEYWORDS_TO_GUESS][MAX_KEYWORD_LEN];
char playerTwoWordList[MAX_NUM_KEYWORDS_TO_GUESS][MAX_KEYWORD_LEN];
int playerOneGuessedNumber;
int playerTwoGuessedNumber;
bool finishGame;
bool quitGame;

// struct Player
// {
//     int ID;
//     char* name;
//     bool hasRegistered;
//     bool isPlaying;
//     bool isGuessing;
//     //char wordsList[MAX_NUM_KEYWORDS_TO_GUESS][MAX_KEYWORD_LEN] = '\0';
//
// };

// static struct Player* createNewPlayer()
// {
//   struct Player* newPlayer = malloc(sizeof(struct Player));
//   newPlayer->ID = -1;
//   newPlayer->name = NULL;
//   newPlayer->hasRegistered = false;
//   newPlayer->isPlaying = false;
//   newPlayer->isGuessing = false;
//   return newPlayer;
// }

static bool handleHttpRequest(int sockfd)
{
    // try to read the request
    char buff[2049];
    int n = read(sockfd, buff, 2049);
    if (n <= 0)
    {
        if (n < 0)
            perror("read");
        else
            printf("socket %d close the connection\n", sockfd);
        return false;
    }

    // if players are disconnected, reset the value of finishGame and quitGame
    if (playerOneSocket < 0 && playerTwoSocket < 0){
    	finishGame = false;
    	quitGame = false;
	   }

  	// Record the players' socket if a new player conncet to server
  	if (playerOneSocket < 0 && sockfd != playerTwoSocket)
    {
  		  playerOneSocket = sockfd;
  	} else if (playerTwoSocket < 0 && sockfd != playerOneSocket)
    {
  		  playerTwoSocket = sockfd;
  	} else if (!(playerOneSocket < 0) && !(playerTwoSocket < 0) && sockfd != playerOneSocket && sockfd != playerTwoSocket)
    {
        // too many player joined
        exit(EXIT_FAILURE);
    }

    // terminate the string
    buff[n] = 0;

    char * curr = buff;

    // parse the method
    METHOD method = UNKNOWN;
    if (strncmp(curr, "GET ", 4) == 0)
    {
        curr += 4;
        method = GET;
    }
    else if (strncmp(curr, "POST ", 5) == 0)
    {
        curr += 5;
        method = POST;
    }
    else if (write(sockfd, HTTP_400, HTTP_400_LENGTH) < 0)
    {
        perror("write");
        return false;
    }

    // sanitise the URI
    while (*curr == '.' || *curr == '/')
        ++curr;
    // assume the only valid request URIs are "/" and "?" but it can be modified to accept more files
    if (*curr == ' ' || *curr == '?')
    {

        if (method == GET)
        {
            // if the player hasn't registered
            // if(!currentPlayer->hasRegistered)
            // {
            //     showIntroPage(n, buff, sockfd);
            // }
            // if the player has registered and click the 'start' button
            if (strstr(buff, "start=Start") != NULL)
            {
                //currentPlayer->isPlaying = true;
                // modify the state of the player who are ready to play
                if (sockfd == playerOneSocket)
                {
                    playerOneActive = true;
                } else if (sockfd == playerTwoSocket)
                {
                    playerTwoActive = true;
                }
                showFirstTurnPage(n, buff, sockfd);

            } else {
                showIntroPage(n, buff, sockfd);
            }


        }

        else if (method == POST)
        {
            // if the player click the 'quit' button
            if (strstr(buff, "quit=") != NULL)
            {
                // reset the info of the player who quitted
                if (sockfd == playerOneSocket)
                {
                  playerOneSocket = -1;
                  playerOneActive = false;
                  quitGame = true;
                } else if (sockfd == playerTwoSocket)
                {
                  playerTwoSocket = -1;
                  playerTwoActive = false;
                  quitGame = true;
                }

                //currentPlayer->isPlaying = false;
                showGameoverPage(n, buff, sockfd);
            }

            // if the player type name and click "submit" button
            else if (strstr(buff, "user=") != NULL)
            {
                // locate the username, it is safe to do so in this sample code, but usually the result is expected to be
                // copied to another buffer using strcpy or strncpy to ensure that it will not be overwritten.
                char * username = strstr(buff, "user=") + 5;
                //strcpy(currentPlayer->name, username);
                //printf("username = %s\n", currentPlayer->name);
                showStartPage(n, buff, sockfd, username);
                //currentPlayer->hasRegistered = true;
            }

            // if the player click the "guess" button
            else if (strstr(buff, "guess") != NULL)
            {
               char* getKeyword = strstr(buff, "keyword=") + 8;

               // if one of players has quitted the game
               if (quitGame)
               {
                  showGameoverPage(n, buff, sockfd);
               }

               // if the game has finished
               if (finishGame)
               {
                  if (sockfd == playerOneSocket)
                  {
                    playerOneSocket = -1;
                    playerOneActive = false;
                    playerOneGuessedNumber = 0;
                  } else if (sockfd == playerTwoSocket)
                  {
                    playerTwoSocket = -1;
                    playerTwoActive = false;
                    playerTwoGuessedNumber = 0;
                  }
                  showEndgamePage(n, buff, sockfd);
               }

               // if both players are ready to play
               if (playerOneActive && playerTwoActive)
               {
                  // if game finished
                  if (checkFinish(sockfd, getKeyword)) {

                      if (sockfd == playerOneSocket) {
                          // mofidy the info of both player
                          playerOneSocket = -1;
                          playerOneActive = false;
                          playerOneGuessedNumber = 0;
                          playerTwoSocket = -1;
                          playerTwoActive = false;
                          playerTwoGuessedNumber = 0;

                      showEndgamePage(n, buff, sockfd);
                      }
                  }
                  // if game not finished yet
                  else
                  {
                      if (sockfd == playerOneSocket)
                      {
                          strcpy(playerOneWordList[playerOneGuessedNumber++], getKeyword);
                      } else if (sockfd == playerTwoSocket)
                      {
                          strcpy(playerOneWordList[playerTwoGuessedNumber++], getKeyword);
                      }
                      showAcceptedPage(n, buff, sockfd);
                  }
               }
               // if not both players are active
               else
               {
                  showDiscardedPage(n, buff, sockfd);
               }
            }


        }

        else
        {
            // never used, just for completeness
            fprintf(stderr, "No other methods supported\n");
        }
    }



    // send 404
    else if (write(sockfd, HTTP_404, HTTP_404_LENGTH) < 0)
    {
        perror("write");
        return false;
    }


    return true;
}




int main(int argc, char * argv[])
{
    const char* readableIP;
    int portNumber;
    int sockfd;
    // struct Player* currentPlayer = createNewPlayer();
    // struct Player* otherPlayer = createNewPlayer();
    // check command format
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s ip port\n", argv[0]);
      return 0;
    }

    readableIP = argv[1];
    portNumber = atoi(argv[2]);


    initialisePlayersInfo();

    sockfd = InitialiseServerSocket(readableIP, portNumber);

    // listen on the socket, support two connection with two client browser simultaneously
    listen(sockfd, 5);

    printf("image_tagger server is now running at IP: %s on port %d\n", readableIP, portNumber);

    // initialise an active file descriptors set
    fd_set masterfds;
    FD_ZERO(&masterfds);
    FD_SET(sockfd, &masterfds);
    // record the maximum socket number
    int maxfd = sockfd;

    // acheive persistent TCP connection
    while (1)
    {
        // monitor file descriptors
        fd_set readfds = masterfds;
        if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // loop all possible descriptor
        for (int i = 0; i <= maxfd; ++i)
            // determine if the current file descriptor is active
            if (FD_ISSET(i, &readfds))
            {
                // create new socket if there is new incoming connection request
                if (i == sockfd)
                {
                    struct sockaddr_in cliaddr;
                    socklen_t clilen = sizeof(cliaddr);
                    int newsockfd = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen);
                    if (newsockfd < 0)
                        perror("accept");
                    else
                    {
                        // add the socket to the set
                        FD_SET(newsockfd, &masterfds);
                        // update the maximum tracker
                        if (newsockfd > maxfd)
                            maxfd = newsockfd;
                        // print out the IP and the socket number
                        char ip[INET_ADDRSTRLEN];
                        printf(
                            "new connection from %s on socket %d\n",
                            // convert to human readable string
                            inet_ntop(cliaddr.sin_family, &cliaddr.sin_addr, ip, INET_ADDRSTRLEN),
                            newsockfd
                        );
                    }
                }
                // a request is sent from the client
                else if (!handleHttpRequest(i))
                {
                    close(i);
                    FD_CLR(i, &masterfds);
                }
            }
    }

    return 0;
}


/* Initialise players infomation
 */
void initialisePlayersInfo()
{
      playerOneActive = false;
      playerTwoActive = false;
  	  playerOneSocket = -1;
      playerTwoSocket = -1;
  	  playerOneGuessedNumber = 0;
      playerTwoGuessedNumber = 0;
     	finishGame = false;
  	  quitGame = false;
}


/* Show Intro Page on users' browser
 * Derived from lab-6 http-server.c
 */
int showIntroPage(int n, char* buff, int sockfd)
{
  // get the size of the file
  struct stat st;
  stat(INTRO_PAGE, &st);
  n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
  // send the header first
  if (write(sockfd, buff, n) < 0)
  {
      perror("write");
      return false;
  }
  // send the file
  int filefd = open(INTRO_PAGE, O_RDONLY);
  do
  {
      n = sendfile(sockfd, filefd, NULL, 2048);
  }
  while (n > 0);
  if (n < 0)
  {
      perror("sendfile");
      close(filefd);
      return false;
  }
  close(filefd);
  return 1;
}


/* Show Start Page on users' browser
 * Derived from lab-6 http-server.c
 */
int showStartPage(int n, char* buff, int sockfd, char* username)
{

  int username_length = strlen(username);
  // the length needs to include the ", " before the username
  long added_length = username_length + 9;
  // get the size of the file
  struct stat st;
  stat(START_PAGE, &st);
  // increase file size to accommodate the username
  long size = st.st_size + added_length;
  n = sprintf(buff, HTTP_200_FORMAT, size);
  // send the header first
  if (write(sockfd, buff, n) < 0)
  {
      perror("write");
      return false;
  }
  // read the content of the HTML file
  int filefd = open(START_PAGE, O_RDONLY);
  n = read(filefd, buff, 2048);
  if (n < 0)
  {
      perror("read");
      close(filefd);
      return false;
  }
  close(filefd);

  // move the trailing part backward
  int p1, p2;
  for (p1 = size - 1, p2 = p1 - added_length; p1 >= size - 25; --p1, --p2)
      buff[p1] = buff[p2];
  ++p2;
  // put the separator
  buff[p2++] = ',';
  buff[p2++] = ' ';
  // copy the username
  strncpy(buff + p2, username, username_length);
  if (write(sockfd, buff, size) < 0)
  {
      perror("write");
      return false;
  }

  return 1;

}



/* Show First Turn Page on users' browser
 * Derived from lab-6 http-server.c
 */
int showFirstTurnPage(int n, char* buff, int sockfd)
{
  // get the size of the file
  struct stat st;
  stat(FIRST_TURN_PAGE, &st);
  n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
  // send the header first
  if (write(sockfd, buff, n) < 0)
  {
      perror("write");
      return false;
  }
  // send the file
  int filefd = open(FIRST_TURN_PAGE, O_RDONLY);
  do
  {
      n = sendfile(sockfd, filefd, NULL, 2048);
  }
  while (n > 0);
  if (n < 0)
  {
      perror("sendfile");
      close(filefd);
      return false;
  }
  close(filefd);
  return 1;
}


/* Show Gameover Page on users' browser
 * Derived from lab-6 http-server.c
 */
int showGameoverPage(int n, char* buff, int sockfd)
{
  // get the size of the file
  struct stat st;
  stat(GAMEOVER_PAGE, &st);
  n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
  // send the header first
  if (write(sockfd, buff, n) < 0)
  {
      perror("write");
      return false;
  }
  // send the file
  int filefd = open(GAMEOVER_PAGE, O_RDONLY);
  do
  {
      n = sendfile(sockfd, filefd, NULL, 2048);
  }
  while (n > 0);
  if (n < 0)
  {
      perror("sendfile");
      close(filefd);
      return false;
  }
  close(filefd);
  return 1;
}


/* Show Discarded Page on users' browser
 * Derived from lab-6 http-server.c
 */
int showDiscardedPage(int n, char* buff, int sockfd)
{
  // get the size of the file
  struct stat st;
  stat(DISCARDED_PAGE, &st);
  n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
  // send the header first
  if (write(sockfd, buff, n) < 0)
  {
      perror("write");
      return false;
  }
  // send the file
  int filefd = open(DISCARDED_PAGE, O_RDONLY);
  do
  {
      n = sendfile(sockfd, filefd, NULL, 2048);
  }
  while (n > 0);
  if (n < 0)
  {
      perror("sendfile");
      close(filefd);
      return false;
  }
  close(filefd);
  return 1;
}



/* Show Endgame Page on users' browser
 * Derived from lab-6 http-server.c
 */
int showEndgamePage(int n, char* buff, int sockfd)
{
  // get the size of the file
  struct stat st;
  stat(ENDGAME_PAGE, &st);
  n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
  // send the header first
  if (write(sockfd, buff, n) < 0)
  {
      perror("write");
      return false;
  }
  // send the file
  int filefd = open(ENDGAME_PAGE, O_RDONLY);
  do
  {
      n = sendfile(sockfd, filefd, NULL, 2048);
  }
  while (n > 0);
  if (n < 0)
  {
      perror("sendfile");
      close(filefd);
      return false;
  }
  close(filefd);
  return 1;
}



/* Show Accepted Page on users' browser
 * Derived from lab-6 http-server.c
 */
int showAcceptedPage(int n, char* buff, int sockfd)
{
  // get the size of the file
  struct stat st;
  stat(ACCEPTED_PAGE, &st);
  n = sprintf(buff, HTTP_200_FORMAT, st.st_size);
  // send the header first
  if (write(sockfd, buff, n) < 0)
  {
      perror("write");
      return false;
  }
  // send the file
  int filefd = open(ACCEPTED_PAGE, O_RDONLY);
  do
  {
      n = sendfile(sockfd, filefd, NULL, 2048);
  }
  while (n > 0);
  if (n < 0)
  {
      perror("sendfile");
      close(filefd);
      return false;
  }
  close(filefd);
  return 1;
}

/* Initialise the server sockets
 * Derived from lab6 http-server.c
 */
int InitialiseServerSocket(const char* readableIP, const int portNumber)
{
  // create TCP socket which only accept IPv4
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
      perror("socket");
      exit(EXIT_FAILURE);
  }

  // reuse the socket if possible
  int const reuse = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0)
  {
      perror("setsockopt");
      exit(EXIT_FAILURE);
  }

  // create and initialise address we will listen on
  struct sockaddr_in serv_addr;
  bzero(&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  // ip address and port number are confugurable using command line
  serv_addr.sin_addr.s_addr = inet_addr(readableIP);
  serv_addr.sin_port = htons(portNumber);

  // bind address to socket
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
      perror("bind");
      exit(EXIT_FAILURE);
  }

  return sockfd;
}




// check if the game is finished by compare word to other player's word list
bool checkFinish(int sockfd, char* wordTyped) {
  if (playerOneGuessedNumber > 20 || playerTwoGuessedNumber > 20) {
    return true;
  }
	if (sockfd == playerOneSocket) {
		for (int i = 0; i < playerTwoGuessedNumber; i++){
			if (strcmp(wordTyped, playerTwoWordList[i]) == 0) {
				finishGame = true;
				return true;
			}
		}
		strcpy(playerOneWordList[playerOneGuessedNumber], wordTyped);
		playerOneGuessedNumber++;
	} else if (sockfd == playerTwoSocket) {
		for (int i = 0; i < playerOneGuessedNumber; i++){
			if (strcmp(wordTyped, playerOneWordList[i]) == 0) {
				finishGame = true;
				return true;
			}
		}
		strcpy(playerTwoWordList[playerTwoGuessedNumber], wordTyped);
		playerTwoGuessedNumber++;
	}
	return false;
}
