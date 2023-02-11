#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

#define CHARSIZE 1000
void makeInterogation(char *final, char *clauses, char *from, char* conditions){
    strcpy(final, "select ");
    final[strlen(final)-1]='\0';
    strcat(final, clauses);
    final[strlen(final)-1]='\0';
    strcat(final, " from ");
    final[strlen(final)-1]='\0';
    strcat(final, from);
    final[strlen(final)-1]='\0';
    if(conditions!=NULL){
        strcat(final, " ");
        final[strlen(final)-1]='\0';
        strcat(final, conditions);
        final[strlen(final)-1]='\0';
    }
}
//function to automate a 'SELECT * from..' interogation

int callback(void *NotUsed, int argc, char **argv, char **azColName)
{

    for (int i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
} //base function to print the arguments

int callback_login(char *notUsed, int argc, char **argv, char **columnNames)
{
    if(argc==1)
    {
        strcpy(notUsed, "true");
        return 1;
    }
    strcpy(notUsed, "false");
    return 0;
} //this checks if a user exists or not in the database



int callback_get_id(char* ans,int argc,char** argv,char**azColname)
{
    strcpy(ans, argv[0] ? argv[0] : "!");
    //printf("%s\n", ans);
    return 0;
} /*Callback to get the id of a certain line from a sql table*/

int callback_change(char* ans,int argc,char**argv,char**azColName)
{
    strcat(ans,":\nLimita viteza:");
    strcat(ans,argv[0] ? argv[0] : "NULL");
    strcat(ans,"KM/H\n\n");
    //printf("Rezultat in callback_change:\n%s\n\n",ans);
    return 0;
} /*Callback to get the speedlimit*/

int callback_events(char* ans,int argc,char**argv,char**azColname)
{
    for (int i = 0; i < argc; i++)
    {
        strcat(ans, azColname[i]);
        strcat(ans, " : ");
        strcat(ans,argv[i]);
        strcat(ans,"\n");
    }
    strcat(ans,"\n");
    return 0;
}/*Callback to get the events/places*/

int command_login(char *username){
    char sql[256];
    sleep(10);
    strcpy(sql,"SELECT user_name FROM users WHERE user_name='");
    strcat(sql,username);
    sql[strlen(sql)-1]='\0';
    strcat(sql,"';"); /* Prepare SQLite interogation for username search in database */
    sqlite3 *db;
    char* zErrMsg;

    if(0!=sqlite3_open("dateTrafic.db",&db))
    {
        printf("Error opening database: %s\n",sqlite3_errmsg(db));
        exit(-1);
    }

    int valid=0;
    int *p_valid=&valid;

    if(sqlite3_exec(db, sql, (int (*)(void *, int, char **, char **)) callback_login, p_valid, &zErrMsg) != SQLITE_OK)
    {
        sqlite3_free(zErrMsg);
    }
    if(valid==1)
    {
        return 1;
    }
    return 0;
}
/* API to check into the database if the user exists
 The function returns 1/0 based on the accuracy of the username in the database*/

void afisareRezultat(char *p){
    printf("\nRezultatul este: \n%s", p);
}
/* Function to automate the 'debug' procces */

void stateOfRc(int rc, char* zErrMsg, char* place){
    if( rc != SQLITE_OK ) {
        printf("Nu s-a accesat ok in RC din: %s\n", place);
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
    }
}
/*function to check if a sqlite3_exec() function worked */

void command_changeLocation(char *temp, int extra, char *respond){
    if(temp[0]=='S')
    {
        //strcpy(change, change+2);
        char *change=temp+2;
        //delete de newLine from street
        change[strlen(change)-1]='\0';
        sqlite3 *db;
        char *zErrMsg = 0;
        char aux[2048];
        int rc;
        char sql[2048];
        const char* data = "Callback function called";

        /* Open database */
        rc = sqlite3_open("dateTrafic.db", &db);

        if( rc ) {
          fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        } else {
          fprintf(stderr, "Opened database successfully\n");
        }
        
        /* Create SQL statement */
        strcpy(sql,"SELECT street_id from map where street_name='");
        strcat(sql, change);
        strcat(sql, "';");
        /*** streed id -----------------*/
        
        char street_id[10];
        /* Execute SQL statement*/
        rc = sqlite3_exec(db, sql, (int(*)(void*,int,char**,char**))callback_get_id, &street_id, &zErrMsg);
        stateOfRc(rc, zErrMsg, "street_id");
        strcpy(aux, street_id);
        /*** speed limit -----------------*/
        
        strcpy(sql,"SELECT speed_limit from speed WHERE street_id='");
        strcat(sql,street_id);
        strcpy(aux,change);
        strcat(sql,"';");
        
        //Prepare the sql for speed limit in the area
        rc=sqlite3_exec(db, sql, (int (*)(void *, int, char **, char **)) callback_change, &aux, &zErrMsg);
        stateOfRc(rc, zErrMsg, "speed limit");
        
        if(extra==1)
        {
            //events
            strcpy(sql,"Select event_title,event_type FROM events NATURAL JOIN map where street_id='");
            strcat(sql,street_id);
            strcat(aux,"Extra on this street:\n");
            strcat(sql,"';");
            rc=sqlite3_exec(db, sql, (int (*)(void *, int, char **, char **)) callback_events, &aux, &zErrMsg);
            stateOfRc(rc, zErrMsg, "events extra=1");
        }
        if(extra==2)
        {
            //neighbourdhood_id --------------------
            strcpy(sql,"Select neighbourhood_id FROM map WHERE street_id='");
            strcat(sql,street_id);
            strcat(aux,"Extra in the neighbourhood:\n");
            strcat(sql,"';");
            char neighbourhood_id[10];
            
            rc=sqlite3_exec(db, sql, (int (*)(void *, int, char **, char **)) callback_get_id, &neighbourhood_id, &zErrMsg);
            stateOfRc(rc, zErrMsg, "nid id in extra=2");
            
            
            strcat(aux, "Events happend in the neighbourhood:\n");
            printf("NID:%s\n",neighbourhood_id);
            strcpy(sql,"Select event_title,event_type FROM events NATURAL JOIN map where neighbourhood_id='");
            strcat(sql,neighbourhood_id);
            /*
            strcat(sql,"' AND street_id<>'");
            strcat(sql, street_id);
             */
            strcat(sql, "';");
            rc=sqlite3_exec(db, sql, (int (*)(void *, int, char **, char **)) callback_events, &aux, &zErrMsg);
            stateOfRc(rc, zErrMsg, "nid2 in extra=2");
            
            
            strcat(aux, "Additional info's that are valid in the neighbourhood.\n");
            //all details
            strcpy(sql,"SELECT location_name,location_type,description FROM interest_points NATURAL JOIN map where neighbourhood_id='");
            strcat(sql,neighbourhood_id);
            strcat(sql,"';");
            rc=sqlite3_exec(db, sql, (int (*)(void *, int, char **, char **)) callback_events, &aux, &zErrMsg);
            stateOfRc(rc, zErrMsg, "all details in extra=2");
            //delete the last newLine
            //aux[strlen(aux)-1]='\0';
        }
        /* Add extra informations based on preferences */
        strcpy(respond, aux);
        /*
        street_id[0]='\0';
        aux[0]='\0';
         */
    }
    else if(temp[0]=='N')
    {
        //respond=strdup("Neighbourhood change detected!\n");
        strcpy(respond, "Neighbourhood change detected!\n");
    }
    else{
        strcpy(respond, "commandReceived not foun.\n");
    }
}
/*API to interogate the database for the street that is shared with the variable temp.
 Variable 'extra' gives the amount of information shared by the database.
 Variable 'respond' returns the information to the client*/

int command_alert(char *commandReceived, char *respond){
    respond[0]='\0';
    char type[20];
    int i=9,j=0;
    char aux[CHARSIZE];
    while(commandReceived[i]!='S'||commandReceived[i+1]!=':')
    {
        type[j++]=commandReceived[i];
        i++;
    }
    type[j-1]='\0';
    i+=2;
    j=0;
    char street[50];
    while(commandReceived[i]!='\0')
    {
        street[j++]=commandReceived[i];
        i++;
    }
    street[j-1]='\0';
    char sql[256];
    strcpy(sql,"SELECT street_id FROM map WHERE street_name='");
    strcat(sql,street);
    strcat(sql,"';");
    char street_id[20];
    sqlite3 *db;
    char* error_message;
    int rc = sqlite3_open("dateTrafic.db", &db);

    if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    } else {
      fprintf(stderr, "Opened database successfully\n");
    }
    street_id[0]='\0';
    
    rc=sqlite3_exec(db, sql, (int (*)(void *, int, char **, char **)) callback_get_id, &street_id, &error_message);
    stateOfRc(rc, error_message, street_id);
    //afisareRezultat(street_id);
    /* Save street_id for future use based on street name given*/
    if(strlen(street_id)!=1 || street_id[0]<'1' || street_id[0]>'9'){
        strcpy(respond, "Not a valid combination street/type.\n");
        return -1;
    }
    
    strcpy(sql,"SELECT event_id FROM events WHERE event_type='");
    strcat(sql,type);
    strcat(sql,"' AND street_id='");
    strcat(sql,street_id);
    strcat(sql,"'");
    
    
    //char *p_found=malloc(10);
    char p_found[10];
    rc=sqlite3_exec(db, sql, (int (*)(void *, int, char **, char **)) callback_login, &p_found, &error_message);
    //afisareRezultat(p_found);
    if(strcmp(p_found,"true")==0)
    {
        strcpy(aux, "We already have this event recorded in our database!\n");
        strcpy(commandReceived, aux);
        //afisareRezultat(respond);
        return 1;
    }
    char event_id[10];
    rc=sqlite3_exec(db, "SELECT COUNT(*) FROM events;",
                    (int (*)(void *, int, char **, char **)) callback_get_id, &event_id, &error_message);
    stateOfRc(rc, error_message, "in alert, found=0, select * from events");
    
    strcpy(sql,"Select neighbourhood_id FROM map WHERE street_id='");
    strcat(sql,street_id);
    strcat(sql,"'");
    char neighbourhood_id[10];
    rc=sqlite3_exec(db, sql, (int (*)(void *, int, char **, char **)) callback_get_id, &neighbourhood_id, &error_message);
    stateOfRc(rc, error_message, "in alert, found=0, nbid from events\n");
    
    sprintf(event_id,"%d",1+atoi(event_id));
    /* Get a unique event_id (EVENT ID IS THE PRIMARY KEY FOR THE TABLE*/
    char insertion[256];
    /*
     insert into events values('8','Blocaj_DA','Blocaj','3',DA','1');
     */
    strcpy(insertion,"INSERT INTO events VALUES('");
    strcat(insertion,event_id);strcat(insertion,"','");
    strcat(insertion,type);strcat(insertion,"_");strcat(insertion,street);strcat(insertion,"','");
    strcat(insertion,type);
    strcat(insertion,"','3','");
    strcat(insertion,street_id);strcat(insertion,"','");
    strcat(insertion,neighbourhood_id);
    strcat(insertion,"');");

    rc=sqlite3_exec(db,insertion,NULL,NULL,&error_message);
    stateOfRc(rc, error_message, "in alert, dupa insert\n");
    /* Event is inserted into the DB*/

    respond = strdup("Thank you for submitting the alert!\n");
    strcpy(sql,"SELECT event_title,street_name FROM events NATURAL JOIN map WHERE event_id='");
    strcat(sql,event_id);
    strcat(sql,"'");
    
    strcpy(aux,"IMPORTANT ALERT:\n");
    rc=sqlite3_exec(db, sql, (int (*)(void *, int, char **, char **)) callback_events, &aux, &error_message);
    stateOfRc(rc, error_message, "alert, final select\n");
    /* Take the event from the database and prepare it for delivery to the clients */
    strcat(respond, aux);
    strcpy(commandReceived, respond);
    //afisareRezultat(respond);
    return 0;
    /* Iterate through all the other clients and send the alert to them */
}
/* API to create an alert. It takes the type and the street for which the alert is made, as
 "T:<Type> S:<Street>". If the alert is not present in the database, the function will automatically update the databse.
 Variable 'commandReceived' takes the arguments of the alert.
 Variable 'respond' is used as an output of the 'new' information that will be send to the client/s */

void command_search(char *commandReceived){
    char* searchinput=commandReceived+8;
    //take out the newline:
    searchinput[strlen(searchinput)-1]='\0';
    char sql[256];
    char aux[2048];
    sqlite3 *db;
    char* error_message;
    int rc = sqlite3_open("dateTrafic.db", &db);

    if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    } else {
      fprintf(stderr, "Opened database successfully\n");
    }
    if(searchinput[0]=='E')
    {
        char sql[256];
        strcpy(sql,"SELECT event_title,street_name FROM events NATURAL JOIN map WHERE event_title LIKE '%");
        strcat(sql,searchinput+2);
        strcat(sql,"%' OR event_type LIKE '%");
        strcat(sql,searchinput+2);
        strcat(sql,"%';");
        strcpy(aux,"Results found:\n");
        rc=sqlite3_exec(db, sql, (int (*)(void *, int, char **, char **)) callback_events, &aux, &error_message);
        stateOfRc(rc, error_message, "Search, primul select.\n");
        strcpy(commandReceived, aux);
    }
    else if(searchinput[0]=='P')
    {
        char sql[256];
        strcpy(sql,"SELECT location_name,street_name FROM interest_points NATURAL JOIN map WHERE location_name LIKE '%");
        strcat(sql,searchinput+2);
        strcat(sql,"%' OR location_type LIKE '%");
        strcat(sql,searchinput+2);
        strcat(sql,"%';");
        strcpy(aux,"Results found:\n");
        if(sqlite3_exec(db, sql, (int (*)(void *, int, char **, char **)) callback_events, &aux, &error_message) != SQLITE_OK)
        {
            printf("Error opening database: %s\n",sqlite3_errmsg(db));
            sqlite3_free(error_message);
        }
        //respond=strdup(aux);
        strcpy(commandReceived, aux);
    }
    else
    {
        //respond=strdup("Search syntax: Search: (<E:>|<P:>)<String>");
        strcpy(commandReceived, "Search syntax: Search: (<E:>|<P:>)<String>");
    }
}
/*API to search in the database. It takes a pattern for which it will look for in the database.
 */
