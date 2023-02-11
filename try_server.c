#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include <sqlite3.h>
#include "functii.h"

#define PORT 4444
#define CHARSIZE 1000

int ALERT=0;
int client_list[200];
int connected[200];
int counter=0;

typedef struct thData
{
    int idThread;
    int cl;
} thData;

void action(void *arg)
{
    int length, i = 0;
    struct thData tdL;
    /* doar pentru teste logged=1, in mod normal logged=0*/
    int logged=1;
    int level=0;
    tdL = *((struct thData *)arg);
    while (1)
    {
        //request of the client
        if (read(tdL.cl, &length, sizeof(int)) <= 0)
        {
            printf("[thread:%d]\n", tdL.idThread);
            printf("The client assigned for thread %d disconnected!\n", tdL.idThread);
            connected[tdL.idThread]=0;
            break;
        }
        char *commandReceived = malloc(length * sizeof(char));
        commandReceived[length]='\0';
        if (read(tdL.cl, commandReceived, length) <= 0)
        {
            printf("[thread:%d]\n", tdL.idThread);
            printf("The client assigned for thread %d disconnected!\n", tdL.idThread);
            connected[tdL.idThread]=0;
            break;
        }

        char *respond=malloc(500);
        if (strncmp(commandReceived,"Quit",4)==0)
        {
            respond=strdup("Quitting...\n");
            connected[tdL.idThread]=0;
        }

        else if(logged==0)
        {
        if (strncmp(commandReceived, "Login: ", 7) == 0)
        {
            char* username=commandReceived+7;
            logged=command_login(username);
            if(logged==1)
            {
                respond=strdup("Welcome, ");
                strcat(respond,username);
            }
            else
            {
                respond=strdup("User not recognized\n");
            }
        }
        /* Schema: "Login: <username>"*/
        /* API command_login(char*) connects to the database and performs a search in 'user' table in order to validate the user or not*/

        else
        {
            respond=strdup("You must be logged in to connect!\n");
        }
        }
        else if(logged==1)
        {
            if (strncmp(commandReceived, "Login: ", 7) == 0)
            {
                respond=strdup("You are already logged in!\n");
            }
            else if (strncmp(commandReceived, "Level: ", 7) == 0)
            {
                printf(commandReceived+7);
                if(commandReceived[7]=='1')
                {
                    level=1;
                    respond = strdup("Level package 1 set![Street level events]\n");
                }
                else if(commandReceived[7]=='2')
                {
                    level=2;
                    respond=strdup("Level packege 2 set![Neighbourhood level events]\n");
                }
                else
                {
                    level=0;
                    respond=strdup("Level options were removed!\n");
                }
            }
            /* Option to select the cantity of information received:
            Level 0- Speed information
            Level 1- Actions&Objectives based on the street
            level 2- Actions&Objectives based on the neighbourhood_id
            Schema: "Level: <level>" */
            
            else  if (strncmp(commandReceived, "Change: ", 8) == 0)
            {
                char* change=commandReceived+8;
                char *temp=malloc(sizeof(char)*2048);
                command_changeLocation(change, level, temp);
                respond=strdup(temp);
            }
            /* API command_change(char*, int, char*) questions the database in order to get the amount information selected previously. It can take a new street, or the actual street.
            Schema: "Change: S:<nameOfTheStreet>" - to find out details about a certain street or
                    "Change: N:<nameOfTheNeighbourdhood>" to find out details about a certain neighbourhood*/

            else if (strncmp(commandReceived, "Alert: ", 7) == 0)
            {
                //using 'commandReceived' as a returning object because temp is not working properly
                char *temp=malloc(sizeof(char)*CHARSIZE);
                int spread=command_alert(commandReceived, temp);
                respond=strdup(commandReceived);
                //if spread==0, the alert will go to all the users connected at this moment
                if(spread==0)
                {
                    //going through all the connected users
                    for(int i=0;i<counter;++i)
                    {
                        if(connected[i]!=0 && i!=tdL.idThread)
                        {
                            int l = strlen(respond);
                            if (write(client_list[i], &l, sizeof(int)) <= 0)
                            {
                                printf("[thread %d] ", tdL.idThread);
                                perror("[thread] Can't write respond length.\n");
                            }
                            if (write(client_list[i], respond, l) <= 0)
                            {
                                printf("[thread %d] ", tdL.idThread);
                                perror("[thread] Can't write message  to client.\n");
                            }
                        }
                    }
                    /* Iterate through all the other clients and send the alert to them */
                }
            }
            /* API commnad_allert(char*, char*) takes the alert and based on what exists in the database, returns different things. If the alert already exists, the it will inform only this user that it is a known problem. If the alert is new, all the users connected will be notified.
            Schema: "Alert: T:<nameOfTheType> S:<nameOfTheSreet>"
             ex: "Alert: T:Blocaj S:DA"*/
            
            else if(strncmp(commandReceived,"Search: ",8)==0)
            {
                char *temp=malloc(500);
                strcpy(temp, commandReceived);
                command_search(temp);
                strcpy(respond, temp);
                afisareRezultat(respond);
            }
            /*API command_search(char*) is interogating the database based on an event type/title or a place/location and returns the information to the client
            Schema: "Search: E:<patternToSearchFor>" - for the event table or
                    "Search: P:<patternToSearchFor>" - for the place tables
             ex: Search: E:Accident*/

        else
            respond = strdup("commandReceived not recognized!\n");
        }

        int aux = strlen(respond);
        if (write(tdL.cl, &aux, sizeof(int)) <= 0)
        {
            printf("[thread %d] ", tdL.idThread);
            perror("[thread] Error at writing message length.\n");
        }
        if (write(tdL.cl, respond, aux) <= 0)
        {
            printf("[thread %d] ", tdL.idThread);
            perror("[thread] Error at writing message to the client.\n");
        }
        else
            printf("[thread %d] Message sent:\n %sTotal length:%d\n", tdL.idThread, respond, aux);
        if(connected[tdL.idThread]==0)
        {
            printf("The client assigned for thread %d disconnected!\n", tdL.idThread);
            close(tdL.cl);
            break;
        }
    free(respond);
    for(int i=0;i<strlen(commandReceived);++i)
    {
        commandReceived[i]=0;
    }
    free(commandReceived);
    length=0;
    }
}

static void *treat(void *arg)
{
    struct thData tdL;
    tdL = *((struct thData *)arg);
    printf("[thread:%d] Waiting for client...\n", tdL.idThread);
    fflush(stdout);
    pthread_detach(pthread_self());
    action((struct thData *)arg);
    close((intptr_t)arg);
    return (NULL);
}


int main()
{

    struct sockaddr_in server;
    struct sockaddr_in from;
    int sd;
    int pid;
    pthread_t th[1024];
    int i = 0;
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("[Server] Can't create socket!\n");
        exit(1);
    }
    int on = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    bzero(&server, sizeof(server)); // Clean structures
    bzero(&from, sizeof(from)); //Clean structures

    //conection being set
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        perror("[Server] Error at bind!\n");
        exit(2);
    }
    if (listen(sd, 5) == -1)
    {
        perror("[Server] Error at listen.\n");
        exit(3);
    }
    while (1)
    {
        thData *td;
        int length = sizeof(from);
        printf("[server] Waiting at port %d\n", PORT);
        fflush(stdout);
        int client;
        if ((client = accept(sd, (struct sockaddr *)&from, &length)) < 0)
        {
            perror("[Server] Error at accept!\n");
            exit(4);
        }
        //creating the thread
        connected[counter]=1;
        client_list[counter++]=client;
        int idThread;
        int cl;
        td = (struct thData *)malloc(sizeof(struct thData));
        td->idThread = i++;
        td->cl = client;
        pthread_create(&th[i], NULL, &treat, td);
    }
}

