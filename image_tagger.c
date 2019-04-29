/*
 * image_tagger.c
 * author: Dongwei Wei
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
static const int MAX_KEYWORD_LEN = 20;
static const int MAX_NUM_KEYWORDS_TO_GUESS = 20;

// function prototype
int InitialiseServerSocket(const char* readableIP, const int portNumber);
int showIntroPage(int n, char* buff, int sockfd);
int showStartPage(int n, char* buff, int sockfd, char* username);
int showFirstTurnPage(int n, char* buff, int sockfd);
int showGameoverPage(int n, char* buff, int sockfd);

// represents the types of method
typedef enum
{
    GET,
    POST,
    UNKNOWN
} METHOD;

struct Player
{
    int ID;
    char* name;
    bool hasRegistered;
    bool isPlaying;
    bool isGuessing;
    //char wordsList[MAX_NUM_KEYWORDS_TO_GUESS][MAX_KEYWORD_LEN] = '\0';

};

struct Player* createNewPlayer()
{
  struct Player* newPlayer = malloc(sizeof(struct Player));
  newPlayer->ID = -1;
  newPlayer->name = NULL;
  newPlayer->hasRegistered = false;
  newPlayer->isPlaying = false;
  newPlayer->isGuessing = false;
  return newPlayer;
}

static bool handleHttpRequest(int sockfd, struct Player* currentPlayer)
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
            if(!currentPlayer->hasRegistered)
            {
                showIntroPage(n, buff, sockfd);
            }
            // if the player has registered and click the 'start' button
            else if(currentPlayer->hasRegistered)
            {
                currentPlayer->isPlaying = true;
                showFirstTurnPage(n, buff, sockfd);
            }


        }

        else if (method == POST)
        {
            // if the player hasn't registered
            if(!currentPlayer->hasRegistered)
            {
                // locate the username, it is safe to do so in this sample code, but usually the result is expected to be
                // copied to another buffer using strcpy or strncpy to ensure that it will not be overwritten.
                char * username = strstr(buff, "user=") + 5;
                //strcpy(currentPlayer->name, username);
                //printf("username = %s\n", currentPlayer->name);
                showStartPage(n, buff, sockfd, username);
                currentPlayer->hasRegistered = true;
            }

            // if the player has registered and click the 'quit' button
            else if(currentPlayer->hasRegistered)
            {
                currentPlayer->isPlaying = false;
                showGameoverPage(n, buff, sockfd);
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
  long added_length = username_length + 2;
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


int main(int argc, char * argv[])
{
    const char* readableIP;
    int portNumber;
    int sockfd;
    struct Player* currentPlayer = createNewPlayer();

    // check command format
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s ip port\n", argv[0]);
      return 0;
    }

    readableIP = argv[1];
    portNumber = atoi(argv[2]);

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
                else if (!handleHttpRequest(i, currentPlayer))
                {
                    close(i);
                    FD_CLR(i, &masterfds);
                }
            }
    }

    return 0;
}
