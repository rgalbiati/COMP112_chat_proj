#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdbool.h>
#include <poll.h>


#include "chatList.h"
#include "clientList.h"

#define STDIN 0


//TODO Client list only prints partially 
//TODO clean up client interface
//TODO make code more readable 
#define USER_LEN 20
const static int PASS_LEN = 30;
const int BUFSIZE = 512;
const int DATA_SIZE = 400;

const int NEW_USER = 1;         // Write --DONE
const int LOGIN = 2;            // Write --DONE
const int LOGOUT = 3;           // W --DONE
const int LOGIN_ACK = 4;        // R --DONE
const int LOGIN_FAIL = 5;       // R --DONE
const int CLIENT_LIST_REQ = 6;  // W --DONE
const int CHAT_LIST_REQ = 7;    // W
const int CLIENT_LIST = 8;      // R --DONE
const int CHAT_LIST = 9;        // R
const int MSG = 10;             // W --DONEish
const int CREATE_CHAT = 11;     // W --DONE
const int CHAT_ACK = 12;        // R --DONE
const int CHAT_FAIL = 13;       // R --DONE
const int CHAT_ACCEPT = 14;     // W --DONE
const int CHAT_REJECT = 15;     // W --DONE
const int DELETE_USER = 16;     // W --DONE
const int DELETE_ACK = 17;      // R --DONEish
const int MSG_ERROR = 18;       // R 
const int CHAT_REQ = 19;        // W --DONEish

char USERNAME[USER_LEN];

struct __attribute__((__packed__)) header {
    unsigned short type;
    char src[20];
    char dst[20];
    unsigned int len;
    unsigned int msg_id;
};

void error(const char *msg);
bool create_new_user(int sockfd);
bool login(int sockfd);
void send_packet(int fd, int type, char *src, char *dst, int len, int msg_id, 
                 char *data);
bool read_from_server (int sockfd, struct packet *p);
bool welcome_user (int sockfd);
bool try_again(bool (f) (int), int sockfd, char* error);
void print_usage ();


void print_clients (struct packet * packet)
{
    int cursor = 0;
    char* buf = packet->data;

    while (cursor < packet->len)
    {
        printf("%s\n", buf);
        cursor += strlen(buf) + 1;
        buf += strlen(buf) + 1;
    }
}

void print_chats (struct packet *packet)
{
    int cursor = 0;
    char* buf = packet->data;

    printf("%s\n", buf);

    // while (cursor < packet.len)
    // {
    //     printf("%s", buf);
    //     cursor += strlen(buf) + 1;
    //     buf += strlen(buf) + 1;
    // }
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

bool try_again(bool (f) (int), int sockfd, char* error) 
{
    char input[4];

    printf("%s. Would you like to try again?[y/n]\n", error);
    while(1) {
        bzero(input, 4);
        fgets(input, 4 ,stdin);
        
        if (strncmp(input, "Y", 1) == 0 || strncmp(input, "y", 1) == 0) {
            return f(sockfd);
        }
        else if (strncmp(input, "N", 1) != 0 && strncmp(input, "n", 1) != 0) {
            printf("Invalid input. Would you like to try again?[y/n]\n");
        }
        else {
            return false;
        }
    }
}

char* sanitize_input(char* word)
{
    size_t ln = strlen(word)-1;
    if (word[ln] == '\n')
    word[ln] = '\0';
    return word;
}

bool create_new_user(int sockfd)
{
    char* password;
    bool valid = false;
    char username[256];

    printf("Enter a username : ", USER_LEN);
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
    printf("Enter a password : ", PASS_LEN);
    while (!valid) {
        password = getpass("");

        if (strlen(password) > PASS_LEN) {
            printf("Password too long. Try again. NOTE: must be less than %d charactors): ", PASS_LEN);
        }
        else 
            valid = true;
    }

    memcpy(USERNAME, sanitize_input(username), strlen(username));

    send_packet(sockfd, NEW_USER, USERNAME, "Server", strlen(password), 0, password);

    struct packet p;

    read_from_server(sockfd, &p);

    if (p.type == LOGIN_ACK) {
        return true;
    }

    else 
        if (try_again(create_new_user, sockfd, "Unable to create new user") == false) {
            printf("Would you like to login as an existing user? [y/n]\n");
            while(1) {
                char input[4];
                bzero(input, 4);
                fgets(input, 4 ,stdin);
        
                if (strncmp(input, "Y", 1) == 0 || strncmp(input, "y", 1) == 0) {
                    return login(sockfd);
                }
                else if (strncmp(input, "N", 1) != 0 && strncmp(input, "n", 1) != 0) {
                    printf("Invalid input. [y/n]\n");
                }
                else
                    return false;
            }
        }
        return true;
}

bool login(int sockfd) 
{
    char* password;
    char input[4];
    char username[256];

    printf("Username : ");
    bzero(username,256);
    fgets(username,256,stdin);

    password = getpass("Password : ");

    memcpy(USERNAME, sanitize_input(username), strlen(username));

    send_packet(sockfd, LOGIN, USERNAME, "Server", strlen(password), 0, password);

    struct packet p;

    read_from_server(sockfd, &p);

    if (p.type == LOGIN_ACK) {
        return true;
    }
    else {
        if (try_again(login, sockfd, "Invalid username or password") == false) {
            printf("Would you like to create a new user? [y/n]\n");
            while(1) {
                bzero(input, 4);
                fgets(input, 4 ,stdin);
        
                if (strncmp(input, "Y", 1) == 0 || strncmp(input, "y", 1) == 0) {
                    return create_new_user(sockfd);
                }
                else if (strncmp(input, "N", 1) != 0 && strncmp(input, "n", 1) != 0) {
                    printf("Invalid input. [y/n]\n");
                }
                else
                    return false;
            }
        }
        return true;
    }
}

void logout(int sockfd) 
{
    send_packet(sockfd, LOGOUT, USERNAME, "Server", 0, 0, "");
    printf("Goodbye\n");
    close(sockfd);
}

void send_packet(int sockfd, int type, char *src, char *dst, int len, int msg_id, 
                 char *data)
{
    printf("s_type %d\n", type);
    printf("s_src %s\n", src);
    printf("s_dst %s\n", dst);
    printf("s_len %d\n", len);
    printf("s_data %s\n", data);

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

bool welcome_user (int sockfd)
{
    char buffer[4];

    printf("Welcome to the multi-user chat.\n");

    while (1) {
        printf("Enter ’N’ to create a new user or ‘L’ to log in as an existing user.\n");
        bzero(buffer,4);
        fgets(buffer,4,stdin);
        if (strncmp(buffer, "N", 1) == 0 || strncmp(buffer, "n", 1) == 0) {
            return create_new_user(sockfd);
        }
        else if (strncmp(buffer, "L", 1) == 0 || strncmp(buffer, "l", 1) == 0) {
            return login(sockfd);            
        }
        else {
            printf("Invalid input.\n");
        }
    } 
}

void print_usage ()
{
    //pass
}

void send_chat_req(int sockfd)
{
    bool valid = false;
    int num_clients;
    char username[USER_LEN];
    int c;
    // char clients[num_clients][USER_LEN + 1]; is declared below 
    // then create header?
    // then send clients one by one?
    while (!valid) {
        printf("How many users will you include in your chat?\n");
        int result = scanf("%d", &num_clients);

        if (result == EOF) {
            printf("Invalid input\n");
        }
        else if (result == 0) {
            while (fgetc(stdin) != '\n');
            printf("Invalid input\n");
        }
        else if (num_clients > 4) {
            printf("The number of clients must be < 5. Try again\n");
        }
        else 
            valid = true;

    }

    char clients[num_clients][USER_LEN];

    printf("Enter the usernames to be included in the chat seperated by an enter\n");
    while ( (c = getchar()) != '\n' && c != EOF );
    for (int i = 0; i < num_clients; i++) {
        memset(username,0,sizeof(username));
        fgets(username,USER_LEN,stdin);
        char* new_username = sanitize_input(username);
        strcpy(clients[i], new_username);
    }

    int len = 0;
    for (int i = 0; i < num_clients; i++) {
        len += strlen(clients[i]) + 1;
    }

    struct header h;
    h.type = htons(CREATE_CHAT);
    memset(h.src, 0, 20);
    memset(h.dst, 0, 20);
    memcpy(h.src, USERNAME, strlen(USERNAME) + 1);
    memcpy(h.dst, "Server", strlen("Server") + 1);
    h.len = htonl(len);
    h.msg_id = htonl(0);
    write(sockfd, (char *) &h, sizeof(h));

    for (int i = 0; i < num_clients; i++){
        write(sockfd, clients[i], strlen(clients[i]) + 1);
    }
}

void send_message (int sockfd) 
{
    char buffer [256];

    printf("Chat ID : ");
    bzero(buffer,256);
    fgets(buffer,256,stdin);
    char* id = buffer;
    id = sanitize_input(id);
    printf("New Message : ");
    bzero(buffer,256);
    fgets(buffer,256,stdin);

    // not sure what to do with message id...
    send_packet(sockfd, MSG, USERNAME, id, strlen(buffer) + 1, 
                            1, buffer);
}


void respond_to_request (int sockfd, struct packet *p)
{
    printf("You have recieved a chat request from %s." \
        "  Would you like to accept the request? [y/n]\n", p->src);
                    
    bool valid = false;
    int accept;
    while (!valid) {
        char input [4];
        bzero(input, 4);
        fgets(input, 4 ,stdin);
        
        if (strncmp(input, "Y", 1) == 0 || strncmp(input, "y", 1) == 0) {
            accept = CHAT_ACCEPT;
            valid = true;
        }
        else if (strncmp(input, "N", 1) != 0 && strncmp(input, "n", 1) != 0) {
            printf("Invalid input. Would you like to accept the request? [y/n]\n");
        }
        else {
            accept = CHAT_REJECT;
            valid = true;
        }
    }
    send_packet(sockfd, accept, USERNAME, "Server", strlen(p->data) + 1, 0, p->data);
}

void read_message(struct packet *p)
{
    printf("%s : %s : ", p->src, p->dst);
    printf("%s\n", p->data);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n, ret;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    bool done = false;
    // char username [USER_LEN];
    fd_set writefds;
    fd_set readfds;
    struct pollfd fd;
    fd.fd = sockfd; // socket handler 
    fd.events = POLLIN;
    struct packet p;

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
    
    

    if (welcome_user(sockfd)) {
        printf("Welcome to the chat server. What would you like to do?\n");

        while(!done){
            FD_ZERO(&readfds);
            FD_ZERO(&readfds);

            FD_SET(STDIN, &readfds);
            FD_SET(sockfd, &readfds);

            select(sockfd+1, &readfds, &writefds, NULL, NULL);

            if(FD_ISSET(sockfd, &readfds) != 0)
            {
                read_from_server(sockfd, &p);
                printf("r_type %d\n", p.type);
                printf("r_src %s\n", p.src);
                printf("r_dst %s\n", p.dst);
                printf("r_len %d\n", p.len);
                printf("r_data %s\n", p.data);
                if (p.type == 19) {
                    respond_to_request(sockfd, &p);
                }
                else if (p.type == MSG) {
                    read_message(&p);
                }
                else if (p.type == CHAT_ACK) {
                    printf("You have been added to chat %s\n", p.data);
                }
                else if (p.type == CHAT_FAIL) {
                    printf("Failed to create chat\n");
                }
                else if (p.type == MSG_ERROR) {
                    printf("Falied to send message %d\n", p.msg_id);
                }
            }
            else if (FD_ISSET(STDIN, &readfds))
            {
                bzero(buffer,256);
                fgets(buffer,256,stdin);
                if (strncmp("List Clients", buffer, 12) == 0) {
                    send_packet(sockfd, CLIENT_LIST_REQ, USERNAME, "Server", 0, 0, "");
                    struct packet p;
                    read_from_server(sockfd, &p);
                    print_clients(&p);
                }
                else if (strncmp("List Chats", buffer, 10) == 0) {
                    send_packet(sockfd, CHAT_LIST_REQ, USERNAME, "Server", 0, 0, "");
                    struct packet p;
                    read_from_server(sockfd, &p);
                    print_chats(&p);
                }
                else if (strncmp("Create Chat", buffer, 11) == 0) {
                    send_chat_req(sockfd);
                }
                else if (strncmp("Send Message", buffer, 12) == 0) {
                    send_message(sockfd);
                }
                else if (strncmp("Logout", buffer, 6) == 0) {
                    done = true;
                    logout(sockfd);
                }
                else if (strncmp("Delete Client", buffer, 13) == 0) {
                    done = true;
                    send_packet(sockfd, DELETE_USER, USERNAME, "Server", 0, 0, "");
                    struct packet p;
                    read_from_server(sockfd, &p);
                    // if (p.type == 17)
                    printf("User %s has been deleted.\n", USERNAME);
                    close(sockfd);
                }
                else {
                    printf("Invalid input\n");
                    print_usage();
               }
            }
        }
    }
    else {
        printf("Goodbye\n");
        close(sockfd);
    }


    // else {
    //     //TODO Make logout timeout?
    //     logout(sockfd, client_username);
    // }
    return 0;
}