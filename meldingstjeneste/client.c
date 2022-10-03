#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <ctype.h>

#include <time.h>
#include "send_packet.h"

#define BUFSIZE 1400

struct Message{
    char* message;
    struct Client *to;
    int seqNum;
    struct Message* prevMessage;
    struct Message* nextMessage;
    int givUpCount;
};

struct Client{
    char* name;
    char* ipAdress;
    char* port;
    int isBlocked;
    int ignorenumber;
    struct sockaddr_in clientSock;
    struct Client* next;
};

struct Client *clientList = NULL;
struct Message *messageList = NULL;
struct sockaddr_in serverAddr, myAddr;
int stdTimeout = 1;
int messageFD;
static int seqNumber = -1;
char *myName;

//Sjekker om streng inneholder nummer
int isAnumber(char *str){
    char *start = str;
    while(*str){
        if(isdigit(*str) == 0){
            *str = *start;
            return 0;
        }
        ++str;
    }
    str = start;
    return 1;
}

//Gir ett høyere sekvensnummer for hvert kall
int get_sq_num(){
    seqNumber++;
    if(seqNumber == INT8_MAX){
        seqNumber = 0;
    }
    return seqNumber;
}
//Hentet fra cbra
void get_string_from_stdin(char buf[], int size){
    char c;
    
    fgets(buf, size, stdin);
    if(buf[strlen(buf)-1] == '\n'){
        buf[strlen(buf)-1]= 0;
    }
    else{
        //EOF end of file
        while((c= getchar()) != '\n' && c != EOF);
    }
}

//Frigjør meldingsliste
void free_msg_list(){
    struct Message* p = messageList;
    if(!p)return;
    struct Message* nextMsg;
    while(p->nextMessage){
        nextMsg = p->nextMessage;
        free(p->message);
        free(p);
        p = nextMsg;
    }
    free(p->message);
    free(p);
}

//Frigjør clientliste
void free_client_list(){
    struct Client* p = clientList;
    if(!p)return;
    struct Client* nextNode;
    while(p->next){
        nextNode = p->next;
        free(p->name);
        free(p->ipAdress);
        free(p->port);
        free(p);
        p = nextNode;
    }
    free(p->name);
    free(p->ipAdress);
    free(p->port);
    free(p);
}

//Kalles når programet skal avslutte for å figjøre minne
void fs_shutdown(){
    free(myName);
    free_client_list();
    free_msg_list();
    return;
}

//Errortest for malloc
void malloc_error(size_t *m){
    if(m == NULL){
        perror("Malloc: klarte ikke allokere minnet");
        fs_shutdown();
        exit(EXIT_FAILURE);
    }
}

//Errortest for inporterte metoder
void check_error(int rc, char *msg){
    if(rc==-1){
        perror(msg);
        fs_shutdown();
        exit(EXIT_FAILURE);
    }
}

//Oppretter lokal client og registrerer i liste
struct Client* put_client_in_list(char *name, char buf[]){

    //ACK
    strtok(buf, " ");
    //Nummer
    strtok(NULL, " ");
    //NICK
    strtok(NULL, " ");
    //name
    strtok(NULL, " ");
    //IP
    strtok(NULL, " ");
    char *ipAdress = strtok(NULL, " ");
    //PORT
    strtok(NULL, " ");
    char *port = strtok(NULL, " ");

    struct Client* newClient = malloc(sizeof(struct Client));
    malloc_error((size_t*)newClient);

    newClient->name = (char*) malloc(strlen(name)*sizeof(char)+1);
    malloc_error((size_t*)newClient->name);
    strcpy(newClient->name, name);

    newClient->ipAdress = (char*)malloc(strlen(ipAdress)*sizeof(char)+1);
    malloc_error((size_t*)newClient->ipAdress);
    strcpy(newClient->ipAdress, ipAdress);

    newClient->port = (char*)malloc(strlen(port)*sizeof(char)+1);
    malloc_error((size_t*)newClient->port);
    strcpy(newClient->port, port);

    newClient->isBlocked = 0;

    //Registrer bruker
    if(clientList == NULL){
        newClient->ignorenumber = 9;
        newClient->next = NULL;
        clientList = newClient;
    }
    else{
        struct Client *p = clientList;
        while(p->next  && strcmp(p->name, newClient->name)){
            p = p->next;
        }
        if(!strcmp(p->name, newClient->name)){
            strcpy(p->ipAdress, newClient->ipAdress);
            strcpy(p->port, newClient->port);
            p->ignorenumber = 9;
            free(newClient->name);
            free(newClient->port);
            free(newClient->ipAdress);
            free(newClient);
            return p;
        }
        else{
            p->next = newClient;
        }
    }
    return newClient;
}

//Sender melding
void send_message(char* message, struct sockaddr_in toAddr){
    int rc;
    rc = send_packet(messageFD, message, strlen(message), 0, (struct sockaddr*)&toAddr, sizeof(struct sockaddr_in));
    check_error(rc, "send packet");
}

//Spør server om client
struct Client* lookup_client(char *name){
    
    fd_set lookupSet;
    char buf[BUFSIZE];
    int rc;
    struct timeval timeout;

    int seqNum = get_sq_num();

    char message[50];
    sprintf(message, "PKT %d LOOKUP %s", seqNum, name);

    char notFoundMsg[50];
    sprintf(notFoundMsg, "ACK %d NOT FOUND", seqNum);

    timeout.tv_sec = stdTimeout;
    timeout.tv_usec = 0;

    int count = 3;
    char innMessage[50];
    //Loppen ignorerer input og inkomende meldinger under loopen
    while(count){
        send_message(message, serverAddr);


        FD_ZERO(&lookupSet);
        FD_SET(messageFD, &lookupSet);

        rc = select(FD_SETSIZE, &lookupSet, NULL,NULL,&timeout);
        check_error(rc, "select");

        if (FD_ISSET(messageFD, &lookupSet)){
            rc = read(messageFD, buf, BUFSIZE - 1);
            check_error(rc, "read");
            buf[rc] = 0;
            strcpy(innMessage, buf);


            if(!strcmp(notFoundMsg, buf)){
                return NULL;
            }

            char *code = strtok(buf, " ");
            int num = atoi(strtok(NULL, " "));
            if(!strcmp(code, "ACK")){
                if(num == seqNum){
                    //printf("\r%45s [SERVER]\n", innMessage);
                    return put_client_in_list(name, innMessage);
                }
            }   
        }

        else{
            count--;
            timeout.tv_sec = stdTimeout;
            timeout.tv_usec = 0;
        }
    }
    return NULL;
}

//Leter etter client i lokalt minne, om ikke klienten finnes
//gjør den oppslag hos serveren
struct Client* find_client(char *name){
    if(clientList == NULL){
        return lookup_client(name);
    }
    struct Client *p = clientList;
    while(p->next){
        if(!strcmp(p->name, name))break;
        p = p->next;
    }
    if(!strcmp(p->name, name)){
        return p;
    }

    return lookup_client(name);
}

int block_client(char *name){
    if(clientList == NULL){
        printf("Ingen å blokke\n");
        return 0;
    }

    struct Client *p = clientList;
    while(p->next){
        if(!strcmp(p->name, name))break;
        p = p->next;
    }
    if(!strcmp(p->name, name)){
        p->isBlocked = 1;
        printf("%s er blokkert\n", p->name);
        return 1;
    }
    return 0;
}

int unblock_client(char *name){
    if(clientList == NULL){
        printf("Ingen å blokke\n");
        return 0;
    }

    struct Client *p = clientList;
    while(p->next){
        if(!strcmp(p->name, name))break;
        p = p->next;
    }
    if(!strcmp(p->name, name)){
        p->isBlocked = 0;
        printf("%s er unblokkert\n", p->name);
        return 1;
    }
    return 0;
    
}

//Sjekker og behandler lett input
int check_if_valid(char buf[], int size){
    if(strlen(buf)<3)return 0;
    char bufcopy[size];
    strcpy(bufcopy, buf);
    
    char *command = strtok(bufcopy, " ");
    if(!command)return 0;
    if(strlen(command)<2)return 0;

    else if(!strcmp(command, "QUIT"))return 0;

    char *values = strtok(NULL, " ");
    if(!values) return 0;

    else if(!strcmp(command, "BLOCK")){
        block_client(values);
        return 0;
    }

    else if(!strcmp(command, "UNBLOCK")){
        unblock_client(values);
        return 0;
    }

    else if(command[0] == '@') return 1;

    return 0;

}

//Sender ACK respons
void send_response(struct Client *friend, int seqNum){
    int rc;
    struct sockaddr_in friendAddr;
    friendAddr.sin_family = AF_INET;
    friendAddr.sin_port = htons(atoi(friend->port));
    
    rc = inet_pton(AF_INET, friend->ipAdress, &friendAddr.sin_addr.s_addr);
    check_error(rc, "inet_pton");

    char response[50];
    sprintf(response, "ACK %d OK", seqNum);
    send_message(response, friendAddr);
    return;
}

//Prosesserer ACK melding i meldingsliste
void process_ack(char *num){
    
    if(!messageList){
        return;
    }
    struct Message *p = messageList;
    while(p->nextMessage && p->seqNum!=atoi(num)){
            p = p->nextMessage;
    }
    if(p->seqNum == atoi(num)){
        if(p->prevMessage){
            p->prevMessage->nextMessage = p->nextMessage;
        }
        else{
            messageList = p->nextMessage;
        }
        if(p->nextMessage){
            p->nextMessage->prevMessage = p->prevMessage;
        }
        free(p->message);
        free(p);
    }
    return;
}

//Prosesserer melding som kommer inn
void process_incoming_msg(char buf[]){
    char *code = strtok(buf, " ");
    char *num = strtok(NULL, " ");

    if(!strcmp(code, "ACK")){
        process_ack(num);
    }
    if(strcmp(code, "PKT"))return;

    
    //Melding kommer fra annen client og behandles
    char *FROM = strtok(NULL, " ");
    if(strcmp(FROM, "FROM"))return;

    char *fromNick = strtok(NULL, " ");

    char *TO = strtok(NULL, " ");
    if(strcmp(TO, "TO"))return;

    char *toNick = strtok(NULL, " ");
    if(strcmp(toNick, myName))return;
    
    char *MSG = strtok(NULL, " ");
    if(strcmp(MSG, "MSG"))return;

    char *message = strtok(NULL, "\n");

    struct Client *friend = find_client(fromNick);
    if(!friend)return;
    if(friend->isBlocked){
        send_response(friend, atoi(num));
        return;
    }
    if(friend->ignorenumber != atoi(num)){
        friend->ignorenumber = atoi(num);
        printf("%s: %s\n",friend->name, message);
    }
    
    send_response(friend, atoi(num));
    
    return;
}

//Kalles ved stdin, behandler input og oppretter melding i meldingsliste
void send_msg_to_friend(struct Client *friend, char *message){
    int rc;
    int seqNum = get_sq_num();
    struct sockaddr_in friendAddr;

    friendAddr.sin_family = AF_INET;
    friendAddr.sin_port = htons(atoi(friend->port));

    rc = inet_pton(AF_INET, friend->ipAdress, &friendAddr.sin_addr.s_addr);
    check_error(rc, "inet_pton");
    char response[BUFSIZE];
    sprintf(response, "PKT %d FROM %s TO %s MSG %s", seqNum, myName, friend->name, message);

    char exspectedResponse[30];
    sprintf(exspectedResponse, "ACK %d OK", seqNum);

    struct Message *newMessage = malloc(sizeof(struct Message));
    malloc_error((size_t*)newMessage);

    newMessage->message = malloc(sizeof(char)*((int)strlen(response))+1);
    malloc_error((size_t*)newMessage->message);

    newMessage->givUpCount = 3;
    strcpy(newMessage->message, response);
    newMessage->seqNum = seqNum;
    newMessage->to = friend;

    struct Message *p = messageList;

    if(!p){
        messageList = newMessage;
        newMessage->prevMessage = NULL;
        newMessage->nextMessage = NULL;
    }
    else{
        while(p->nextMessage){
            p = p->nextMessage;
        }
        p->nextMessage = newMessage;
        newMessage->prevMessage = p;
        newMessage->nextMessage = NULL;
    }
    send_message(response, friendAddr);
}

//Registrerer bruker skjer ved oppstart
int reg_client(int count){

    int seqNumber = get_sq_num();
    int rc;
    char incomingMessage[50];
    char message[50];
    sprintf(message, "PKT %d REG %s", seqNumber, myName);

    char exspectedResponse[50];
    sprintf(exspectedResponse, "ACK %d OK", seqNumber);

    struct timeval timeout;
    fd_set registrationSet;

    while(count){
        count--;
        send_message(message, serverAddr);
        
        //Venter på svar
        timeout.tv_sec = stdTimeout;
        timeout.tv_usec = 0;
        FD_ZERO(&registrationSet);
        FD_SET(messageFD, &registrationSet);

        rc = select(FD_SETSIZE, &registrationSet, NULL,NULL,&timeout);
        check_error(rc, "select");
    
        if (FD_ISSET(messageFD, &registrationSet)){
            rc = read(messageFD, incomingMessage, BUFSIZE - 1);
            check_error(rc, "read");
            incomingMessage[rc] = 0;
            if(!strcmp(exspectedResponse, incomingMessage)){
                //printf("\r%45s [SERVER]\n", incomingMessage);
                break;
            }
            else{
                printf("Kunne ikke registrere deg ):\n");
                fs_shutdown();
                return EXIT_SUCCESS;

            }
        }
        else if(count <= 0){
            printf("Timout for REG \n");
            fs_shutdown();
            return EXIT_SUCCESS;
        }
    }
    return 1;
}

int main(int argc, char const *argv[]){
    
    if (argc < 6){
        printf("Bruk: %s <nick> <adresse> <port> <timout> <tapssansynlighet>\n", argv[0]);
        return EXIT_SUCCESS;
    }
    if (strlen(argv[1])>20){
        printf("Navnet ditt: %s er for langt\n", argv[1]);
        return EXIT_SUCCESS;
    }
    time_t diff, start;
    int rc;
    fd_set fds;
    char buf[BUFSIZE];
    struct timeval timeout;
    stdTimeout = atoi(argv[4]);

    //myname er global og brukes i hele systemet
    myName = malloc(strlen(argv[1])+1);
    malloc_error((size_t*)myName);
    strcpy(myName, argv[1]);

    //Lager server socket
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[3]));
    rc = inet_pton(AF_INET, argv[2], &serverAddr.sin_addr.s_addr);
    check_error(rc, "inet_pton");

    messageFD = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(messageFD, "socket");
    
    //Lager client socket
    myAddr.sin_family = AF_INET;
    myAddr.sin_port = htons(0);
    myAddr.sin_addr.s_addr = INADDR_ANY;

    rc = bind(messageFD, (struct sockaddr *)&myAddr, sizeof(struct sockaddr_in));
    check_error(rc, "bind");

    //Setter tapssansynlighet
    if(isAnumber((char*)argv[5])){
        float precent = (atof(argv[5])/100.0);
        set_loss_probability(precent);
    }
    else{
        printf("Tapsansynlighet er ikke et nummer, bruk prosent\n");
        fs_shutdown();
        return EXIT_SUCCESS;
    }
    
    if(!reg_client(3)){
        return EXIT_SUCCESS;
    };

    printf("VELKOMMEN TIL CHATTEN!\n");
    buf[0] = 0;

    start = time(NULL);
    char regMessage[50];

    //Hoved loop, går så lenge ingen feil eller QUIT mottas
    while(strcmp(buf, "QUIT")){
        diff = time(NULL)-start;
        if(diff >=10){
            sprintf(regMessage, "PKT %d REG %s", get_sq_num(), myName);
            start = time(NULL);
            send_message(regMessage, serverAddr);
        }

        //Setter timout
        timeout.tv_sec = stdTimeout;
        timeout.tv_usec = 0;

        FD_ZERO(&fds);
        FD_SET(messageFD, &fds);
        FD_SET(STDIN_FILENO, &fds);

        
        fflush(stdout);
        
        rc = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
        check_error(rc, "select");
        
        //Sjekker om det har kommet meldinger til fildiscriptor
        if (FD_ISSET(messageFD, &fds)){
            rc = read(messageFD, buf, BUFSIZE - 1);
            check_error(rc, "read");
            buf[rc] = 0;
            process_incoming_msg(buf);
        }
        
        //Sjekker om det er input fra bruker
        if (FD_ISSET(STDIN_FILENO, &fds)){
            get_string_from_stdin(buf, BUFSIZE);
            if(!strcmp(buf,"QUIT")){
                break;
            }
            else if(!check_if_valid(buf, BUFSIZE)){
                buf[0] = 0;
                continue;
            }
            
            char *toNick = strtok(buf, " ");
            //Fjerner @
            toNick++;
            char *message = strtok(NULL, "\n");

            struct Client *friend = find_client(toNick);
            if(friend){
                send_msg_to_friend(friend, message);
            }
            else{
                printf("NICK %s NOT REGISTERD\n", toNick);
            }
            buf[0] = 0;
        }
        
        //Dersom timout, funksjonen går gjennom meldingsliste
        else{
            if(messageList != NULL){
                struct Message* freeMsg;
                struct Message* p = messageList;
                struct Client *c;
                
                while(p){
                    c = p->to;
                    //Sjekker om client har byttet informasjon hos server
                    if(p->givUpCount == 2){
                        p->to = lookup_client(c->name);
                    }

                    //Dersom det ikke er noe hell i oppslag hos server
                    if(p->to == NULL){
                        printf("NICK %s UNREACHABLE\n", c->name);
                        freeMsg = p;
                        if(p->prevMessage){
                            p->prevMessage = p->nextMessage;
                        }
                        else{
                            messageList = p->nextMessage;
                        }
                        if(p->nextMessage){
                            p->nextMessage = p->prevMessage;
                        }

                        p = p->nextMessage;
                        free(freeMsg->message);
                        free(freeMsg);
                    }
                    //Sender ny melding eller gir opp
                    else{
                        struct sockaddr_in friendAddr;
                        friendAddr.sin_family = AF_INET;
                        friendAddr.sin_port = htons(atoi(p->to->port));

                        rc = inet_pton(AF_INET, p->to->ipAdress, &friendAddr.sin_addr.s_addr);
                        check_error(rc, "inet_pton");

                        send_message(p->message, friendAddr);
                        p->givUpCount --;
                        if(p->givUpCount <= 0){
                            printf("NICK %s UNREACHABLE\n", p->to->name);
                            freeMsg = p;
                            messageList = p->nextMessage;
                            p = p->nextMessage;
                            free(freeMsg->message);
                            free(freeMsg);
                            break;
                        }
                        else{
                            p = p->nextMessage;
                        }
                    }
                }
            }
        }
    }
    fs_shutdown();
    close(messageFD);

    return EXIT_SUCCESS;

}