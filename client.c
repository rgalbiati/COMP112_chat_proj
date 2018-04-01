#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdbool.h>

#include "chatList.h"
#include "clientList.h"


//TODO Client list only prints partially 
//TODO clean up client interface
//TODO make code more readable 
const static int USER_LEN = 20;
const static int PASS_LEN = 30;
const int BUFSIZE = 512;
const int DATA_SIZE = 400;

const int NEW_USER = 1;
const int LOGIN = 2;
const int LOGOUT = 3;
const int LOGIN_ACK = 4; 
const int LOGIN_FAIL = 5;
const int CLIENT_LIST_REQ = 6;
const int CHAT_LIST_REQ = 7; 
const int CLIENT_LIST = 8;
const int CHAT_LIST = 9;
const int MSG = 10;
const int CREATE_CHAT = 11;
const int CHAT_ACK = 12;
const int CHAT_FAIL = 13;
const int CHAT_ACCEPT = 14;
const int CHAT_REJECT = 15;
const int DELETE_USER = 16;
const int DELETE_ACK = 17;
const int MSG_ERROR = 18;
const int CHAT_REQ = 19;

struct __attribute__((__packed__)) header {
    unsigned short type;
    char src[20];
    char dst[20];
    unsigned int len;
    unsigned int msg_id;
};

void error(const char *msg);
char* create_new_user(int sockfd, char* username);
char* log_in(int sockfd, char* username);
void send_packet(int fd, int type, char *src, char *dst, int len, int msg_id, 
                 char *data);
bool read_from_server (int sockfd, struct packet *p);
char* welcome_user (int sockfd, char* username);
char* try_again(char* (f) (int, char*), int sockfd, char* error, char* username);
void print_usage ();


void print_clients (struct packet packet)
{
    int cursor = 0;
    char* buf = packet.data;

    while (cursor < packet.len)
    {
        printf("%s", buf);
        cursor += strlen(buf) + 1;
        buf += strlen(buf) + 1;
    }
}
void error(const char *msg)
{
    perror(msg);
    exit(0);
}

char* try_again(char* (f) (int, char*), int sockfd, char* error, char* username) 
{
    char input[4];

    printf("%s. Would you like to try again?[y/n]\n", error);
    while(1) {
        bzero(input, 4);
        fgets(input, 4 ,stdin);
        
        if (strncmp(input, "Y", 1) == 0 || strncmp(input, "y", 1) == 0) {
            return f(sockfd, username);
        }
        else if (strncmp(input, "N", 1) != 0 && strncmp(input, "n", 1) != 0) {
            printf("Invalid input. Would you like to try again?[y/n]\n");
        }
        else {
            return NULL;
        }
    }
}

char* create_new_user(int sockfd, char* username)
{
    char* password;
    bool valid = false;

    printf("Please enter a username. NOTE: must be less than %d charactors): ", USER_LEN);
    while (!valid) {
        bzero(username,256);
        fgets(username,256,stdin);

        if (strlen(username) > USER_LEN) {
            printf("Username too long. Try again. NOTE: must be less than %d charactors): ", USER_LEN);
        }
        else 
            valid = true;
    }

    valid = false;
    printf("Please enter a password. NOTE: must be less than %d charactors): ", PASS_LEN);
    while (!valid) {
        password = getpass("");

        if (strlen(password) > PASS_LEN) {
            printf("Password too long. Try again. NOTE: must be less than %d charactors): ", PASS_LEN);
        }
        else 
            valid = true;
    }

    send_packet(sockfd, NEW_USER, username, "Server", strlen(password), 0, password);

    struct packet p;

    read_from_server(sockfd, &p);

    if (p.type == LOGIN_ACK) {
        return username;
    }

    else 
        return try_again(create_new_user, sockfd, "Unable to create new user", username);

}

char* log_in(int sockfd, char* username) 
{
    char* password;
    char input[4];

    printf("Please enter your username : ");
    bzero(username,256);
    fgets(username,256,stdin);

    password = getpass("Password : ");

    send_packet(sockfd, LOGIN, username, "Server", strlen(password), 0, password);

    struct packet p;

    read_from_server(sockfd, &p);

    if (p.type == LOGIN_ACK) {
        return username;
    }
    else {
        return try_again(log_in, sockfd, "Invalid username or password", username);
    }

}

void logout(int sockfd) 
{
    printf("Goodbye\n");
    close(sockfd);
}

void send_packet(int sockfd, int type, char *src, char *dst, int len, int msg_id, 
                 char *data)
{
    char buffer[256];
    int n;
    struct header p;
    p.type = htons(type);
    memset(p.src, 0, 20);
    memset(p.dst, 0, 20);
    memcpy(p.src, src, strlen(src) + 1);
    memcpy(p.dst, dst, strlen(dst) + 1);
    p.len = htonl(len);
    p.msg_id = htonl(msg_id);

    // Write header
    write(sockfd, (char *) &p, sizeof(p));

    // Write data
    if (len > 0) {
        write(sockfd, data, strlen(data) + 1);
    }
}

bool read_from_server (int sockfd, struct packet *p) {
    char header_buf[sizeof(struct header)];
    memset(header_buf, 0, sizeof(struct header));
     
    int numBytes = read(sockfd, header_buf, sizeof(struct header));

    if (numBytes < 0) {
        printf("Error reading from client\n");
        return false;
    }

    else {
        /* Data read. */
        struct header *h = (struct header *)header_buf;
        p->type = ntohs(h->type);
        memcpy(p->src, h->src, strlen(h->src) + 1);
        memcpy(p->dst, h->dst, strlen(h->dst) + 1);
        p->len = ntohl(h->len);
        p->msg_id = ntohl(h->msg_id);

        if (p->len == 0){
            char *data = "";
            memcpy(p->data, data, strlen(data) + 1);
        } else {
            int n = 0;
            char data[p->len];
            n = read(sockfd, data, p->len);

            if (n < 0) {
                printf("Error reading packet data from client %s\n", p->src);
                return false;
            }
            memcpy(p->data, data, p->len + 1);
        }

        return true;
    }
}

char* welcome_user (int sockfd, char* username)
{
    bool done = false;
    char buffer[256];

    printf("Welcome to the multi-user chat.\n");

    while (!done) {
        printf("Enter ’N’ to create a new user or ‘L’ to log in as an existing user.\n");
        bzero(buffer,256);
        fgets(buffer,256,stdin);
        if (strncmp(buffer, "N", 1) == 0 || strncmp(buffer, "n", 1) == 0) {
            username = create_new_user(sockfd, username);
            done = true;
        }
        else if (strncmp(buffer, "L", 1) == 0 || strncmp(buffer, "l", 1) == 0) {
            username = log_in(sockfd, username);            
            done = true;
        }
        else {
            printf("Invalid input.\n");
        }
    } 
    
    return username;
}

void print_usage ()
{
    //pass
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    bool done = false;
    char username [USER_LEN];

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"Multi-user Chat\n\nusage : %s hostname port\n\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");
    
    char * client_username = welcome_user(sockfd, username);
    if (client_username != NULL) {
        while(!done) {
            printf("What would you like to do?\n");
            bzero(buffer,256);
            fgets(buffer,256,stdin);
            if (strncmp("List Clients", buffer, 12) == 0) {
                send_packet(sockfd, CLIENT_LIST_REQ, client_username, "Server", 0, 0, "");
                struct packet p;
                read_from_server(sockfd, &p);
                print_clients(p);
            }
            else if (strncmp("Logout", buffer, 6) == 0) {
                done = true;
                logout(sockfd);
            }
            else {
                printf("Invalid input\n");
                print_usage();
           }
        }  
    }
    else {
        //TODO Make logout timeout?
        //TODO ask if user wants to try again...
        logout(sockfd);
    }
    return 0;
}