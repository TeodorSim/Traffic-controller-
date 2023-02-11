//Simionescu Teodor, 2b5

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int port;
char buffer[200];

int main(int argc, char *argv[])
{
  int sd;
  struct sockaddr_in server;
  if (argc != 3)
  {
    printf("[CONNECT]: %s <server_adress> <port>\n", argv[0]);
    return -1;
  }
  port = atoi(argv[2]);
  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("[Client] Error creating socket!\n");
    exit(1);
  }
  
    //TCP connection
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = inet_addr(argv[1]);
  server.sin_port = htons(port);

  if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
  {
    perror("[Client] Not possible connecting to the server\n");
    exit(2);
  }

    //Input - managed by the parent.
    //Output - managed by the child
  pid_t pid;
  if((pid=fork())==-1)
  {
    perror("[Client] Error at fork\n");
    exit(1);
  }
  if(pid)//Parent
  {

    printf("[Client] Possible commands for server: \nAlert: T<typeOfEvent> S:<streetOfEvent>\n");
    printf("Search: <E>/<P>:<eventOrPattern>\nChange: <S>/<N>:<streetOrNeighbourd>");
    
    printf("[Client] Add a command for server:\n");
      fflush(stdout);
      //fflush(stdin);
    while (1)
    {
        //preparing the environment
      memset(buffer, 0, sizeof(buffer));
      fd_set readfds;
      FD_SET(0, &readfds);
      struct timeval tv; //to sent the request automatically
      tv.tv_sec = 15;
      tv.tv_usec = 0;
      fflush(stdout);
      int ready;
        
        //if there is no input from the stdin in 15 seconds, an automatic request will be send to the client
      if ((ready = select(1, &readfds, NULL, NULL, &tv)) < 0)
      {
         perror("[Client] Error at select.\n");
         return -1;
      }
      else if (ready)
        read(0, buffer, sizeof(buffer));
      else
        strcpy(buffer, "Change: S:Soseaua Nicolina\n"); /*Random street*/
        
        
      int length = 0;
      while (buffer[length] != '\0')
      {
        ++length;
      }
      char *street = malloc(length * sizeof(char));
      for (int i = 0; i < length; ++i)
      {
        street[i] = buffer[i];
      }
      if (write(sd, &length, sizeof(int)) <= 0)
      {
        perror("[Client] Can't write to server!\n");
        close(sd);
        exit(3);
      }
      if (write(sd, street, length) <= 0)
      {
        perror("[Client] Can't write to server!\n");
        close(sd);
        exit(4);
      }
      if(strncmp(street,"Quit",4)==0)
      {
        wait(NULL);
        close(sd);
        exit(0);
      }
  }
  }
  else//Child
  {
    while (1)
    {
      int answerSize;
      if (read(sd, &answerSize, sizeof(int)) < 0)
      {
        perror("[Client] Error at reading the size from server.\n");
        close(sd);
        exit(5);
      }
      char *answer = malloc((answerSize + 1) * sizeof(char));
      if (read(sd, answer, answerSize) < 0)
      {
        perror("[Client] Error at reading the string from server.\n");
        close(sd);
        exit(6);
      }
        printf("[Client]Answer from server:\n%s", answer);
      if(strncmp(answer,"Quit",4)==0)
      {
        close(sd);
        exit(0);
      }
      printf("[Client] Add a command for server:\n");
    } 
  }
}
