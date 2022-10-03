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

#include "send_packet.h"

#define BUFSIZE 325

struct Client{
    char* name;
    struct sockaddr_in clientSock;
    struct Client* next;
    int isLogggedInn;
};

struct Client *clientList = NULL;
int messageFD;

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

void get_string_from_stdin(char buf[], int size){
    char c;
    fgets(buf, size, stdin);
    if(buf[strlen(buf)-1] == '\n') buf[strlen(buf)-1]= 0;
    else
        //EOF end of file
        while((c= getchar()) != '\n' && c != EOF);

}

void fs_shutdown(){
    struct Client *p = clientList;
    if(!p)return;
    struct Client* nextNode;
    while(p->next){
        nextNode = p->next;
        free(p->name);
        free(p);
        p = nextNode;
    }
    free(p->name);
    free(p);
    return;
}

void malloc_error(size_t *m){
    if(m == NULL){
        printf("Malloc: klarte ikke allokere minnet");
        fs_shutdown();
        exit(1);
    }
}

void check_error(int rc, char *msg){
    if(rc==-1){
        perror(msg);
        fs_shutdown();
        exit(EXIT_FAILURE);
    }
}

int isAscii(char *word){
    char *start = word;
    while (*word){
        if(*word < 0 || *word > 126) return 0;
        word++;
    }
    word = start;
    return 1;
}

void check_client_list(){
    struct Client* p = clientList;
    if(!p)return;
    
    struct Client* prev = NULL;
    struct Client* freeNode = p;
    while(p){
        if(!p->isLogggedInn){
            printf("freeer: %s\n", p->name);
            freeNode = p;
            p = p->next;
            if(prev)prev->next = p;
            else clientList = p;
            free(freeNode->name);
            free(freeNode);
        }
        else{
            p->isLogggedInn = 0;
            prev = p;
            p = p->next;
        }
    }
}

int check_incoming_msg(char buf[], char *msgParts[]){
    char *token = strtok(buf, " ");

    int i = 0;
    while(token){
        msgParts[i++] = token;
        token = strtok(NULL, " ");
    }

    if(i < 2){
        msgParts[1] = "422";
        printf("Ugyldig komando\n");
        return 0;
    }

    if(!isAnumber(msgParts[1])){
        printf("Er ikke et nummer: %s\n", msgParts[1]);
        msgParts[1] = "422";
        //printf("Er ikke et nummer: %s\n", msgParts[1]);
        return 0;
    }

    if(i != 4){
        printf("Ugyldig komando\n");
        return 0;
    }

    if(strcmp((char*)msgParts[0], "PKT")){
        printf("Ugyldig komando\n");
        return 0;
    }


    if(strcmp(msgParts[2], "REG") && strcmp(msgParts[2], "LOOKUP")){
        printf("Feil komando\n");
        return 0;
    }

    return 1;
}

int check_incoming_msg2(char *msgParts[], struct sockaddr_in in, int ant){

    char response[50];
    int rc;


    if(ant < 2){
        sprintf(response, "ACK 422 WRONG FORMAT");
        rc = send_packet(messageFD, response, strlen(response), 0, (struct sockaddr*)&in, sizeof(struct sockaddr_in));
        check_error(rc, "send packet");
    }

    char *nummer = (char*)msgParts[1];

    if(ant != 4){
        sprintf(response, "ACK %s WRONG FORMAT", nummer);
    }

    else if(!isAscii(msgParts[3])){
        sprintf(response, "ACK %s WRONG NAME", nummer);
        return 0;
    }

    else if(strlen(msgParts[3])>20){
        sprintf(response, "ACK %s WRONG NAME", nummer);
        return 0;
    }


    else if(strcmp(msgParts[2], "REG") && strcmp(msgParts[2], "LOOKUP")){
        sprintf(response, "ACK %s WRONG NAME", nummer);

        return 0;
    }
    else{
        return 1;
    }

    rc = send_packet(messageFD, response, strlen(response), 0, (struct sockaddr*)&in, sizeof(struct sockaddr_in));
    check_error(rc, "send packet");

    return 0;
}

void read_msg(char buf[], struct sockaddr_in in){
    
    int rc;
    char response[200];

    char *msgParts[4];
    char *token = strtok(buf, " ");

    int i = 0;
    while(token && i<4){
        msgParts[i++] = token;
        token = strtok(NULL, " ");
    }
     
    if(!check_incoming_msg2(msgParts, in, i)){
        return;
    }

    char *nummer = (char*)msgParts[1];
    char *command = (char*)msgParts[2];
    char *value = (char*)msgParts[3];

    //Registrerer eller oppdaterer ny client
    if(!strcmp(command, "REG")){
        /*
        struct Client *p = clientList;
        while(p){
            if(!strcmp(p->name, value)){
                p->clientSock = in;
                p->isLogggedInn = 1;
                sprintf(response, "ACK %s OK", nummer);
                rc = send_packet(messageFD, response, strlen(response), 0, (struct sockaddr*)&in, sizeof(struct sockaddr_in));
                check_error(rc, "send packet");
                return;
            }
            p = p->next;
        }
        */
        
        struct Client* newClient = malloc(sizeof(struct Client));
        malloc_error((size_t*)newClient);
        newClient->clientSock = in;
        newClient->isLogggedInn = 1;

        newClient->name = malloc(strlen(value)*sizeof(char)+1);
        malloc_error((size_t*)newClient->name);
        strcpy(newClient->name, value);

        //Registrer bruker
        if(clientList == NULL){
            newClient->next = NULL;
            clientList = newClient;
        }
        else{
            struct Client *p = clientList;
            while(p->next  && strcmp(p->name, newClient->name)){
                p = p->next;
            }
            if(!strcmp(p->name, newClient->name)){
                p->clientSock = newClient->clientSock;
                p->isLogggedInn = 1;
                free(newClient->name);
                free(newClient);
            }
            else{
                p->next = newClient;
                newClient->next = NULL;
            }
        }
        
        
        sprintf(response, "ACK %s OK", nummer);
        rc = send_packet(messageFD, response, strlen(response), 0, (struct sockaddr*)&in, sizeof(struct sockaddr_in));
        check_error(rc, "send packet");
        
        /*
        struct Client *p = clientList;

        
        printf("Liste: [");
        while(p){
            printf(" %s ", p->name);
            p = p->next;
        }
        printf("]\n");
        */

        return;
    }

    else if(!strcmp(command, "LOOKUP")){
        if(clientList == NULL){
            return;
        }

        struct Client *p = clientList;
        while(p->next && strcmp(p->name,value)){
            p = p->next;
        }
        

        if(!strcmp(p->name,value)){
            char addr_buf[INET_ADDRSTRLEN];

            inet_ntop(AF_INET, &in.sin_addr, addr_buf, sizeof(addr_buf));
            sprintf(response, "ACK %s NICK %s IP %s PORT %d", nummer, value, addr_buf, htons(p->clientSock.sin_port));
            rc = send_packet(messageFD, response, strlen(response), 0, (struct sockaddr*)&in, sizeof(struct sockaddr_in));

            check_error(rc, "send packet");
        }
        else{
            sprintf(response, "ACK %s NOT FOUND", nummer);
            rc = send_packet(messageFD, response, strlen(response), 0, (struct sockaddr*)&in, sizeof(struct sockaddr_in));
            check_error(rc, "send packet");
        }
        response[0] = 0;
        return;
    }
}

int main(int argc, char const *argv[]){
    struct timeval timeout;
    int rc;
    fd_set fds;
    struct sockaddr_in in_addr, thisAddr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    char buf[BUFSIZE];

    if (argc < 3){
        printf("Bruk: %s <server port> <tapsprosent> \n", argv[0]);
        return EXIT_SUCCESS;
    }

    if(isAnumber((char*)argv[2])){
        float precent = (atof(argv[2])/100.0);
        set_loss_probability(precent);
    }
    else{
        printf("Tapsansynlighet er ikke et nummer, bruk prosent\n");
        fs_shutdown();
        return EXIT_SUCCESS;
    }

    messageFD = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(messageFD, "socket");

    thisAddr.sin_family = AF_INET;
    //antar at alt er ok, burde skjekke om det er gyldig port
    thisAddr.sin_port = htons(atoi(argv[1]));
    thisAddr.sin_addr.s_addr = INADDR_ANY;

    rc = bind(messageFD, (struct sockaddr *)&thisAddr, sizeof(struct sockaddr_in));
    check_error(rc, "bind");
    
    printf("Serveren har startet\n");
    buf[0] = 0;
    timeout.tv_sec = 30;
    timeout.tv_usec = 0;
    while(strcmp(buf, "QUIT")){

        FD_ZERO(&fds);
        //legger til
        FD_SET(messageFD, &fds);
        FD_SET(STDIN_FILENO, &fds);
        
        //rc = select(FD_SETSIZE, &fds, NULL,NULL,&timout);
        rc = select(FD_SETSIZE, &fds, NULL,NULL,&timeout);
        check_error(rc, "select");
        
        if (FD_ISSET(messageFD, &fds)){
            rc = recvfrom(messageFD, buf, BUFSIZE - 1, 0, (struct sockaddr*)&in_addr, &addr_len);
            check_error(rc, "read");
            buf[rc] = 0;
            read_msg(buf, in_addr);
        }

        else if (FD_ISSET(STDIN_FILENO, &fds)){
            get_string_from_stdin(buf, BUFSIZE);
        }

        else{
            check_client_list();
            timeout.tv_sec = 30;
            timeout.tv_usec = 0;
        }
    }

    fs_shutdown();
    close(messageFD);

    return EXIT_SUCCESS;

}