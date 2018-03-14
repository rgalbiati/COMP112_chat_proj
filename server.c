#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include "chatList.h"
#include "clientList.h"

const int BUFSIZE = 512;
const int DATA_SIZE = 400;
const int MAX_CHATS = 100;
const int MAX_CLIENTS = 100;
const static int USER_LEN = 20;
const static int PASS_LEN = 30;

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

struct __attribute__((__packed__)) header {
    unsigned short type;
    char src[20];
    char dst[20];
    unsigned int len;
    unsigned int msg_id;
};

struct __attribute__((__packed__)) packet {
    unsigned short type;
    char src[20];
    char dst[20];
    unsigned int len;
    unsigned int msg_id;
    char data[400];
};

bool handle_packet(int fd, struct packet *p, struct clientList *client_list, 
                   struct chatList *chat_list);
void send_packet(int fd, int type, char *src, char *dst, int len, int msg_id, 
                 char *data);
int make_socket (uint16_t port);
bool read_from_client (int fd, struct packet *p);
void print_packet(struct packet *p);


int main (int argc, char* argv[]) {
    // extern int make_socket (uint16_t port);
    int sock, i, port, size;
    fd_set active_fd_set, read_fd_set;
    struct sockaddr_in clientname;
    // struct chat *chat_list[MAX_CHATS];
    int numClients = 0, numChats = 0;

    if (argc != 2) {
        printf("Please provide a single port number.\n");
        return EXIT_FAILURE;
    }

    port = atoi(argv[1]);

    /* Create the socket and set it up to accept connections. */
    sock = make_socket (port);
    if (listen (sock, 1) < 0) {
        printf("Error on listen\n");
        exit (EXIT_FAILURE);
    }

    printf("Listening on port: %d\n", port);

    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (sock, &active_fd_set);

    struct clientList *client_list = newClientList();
    struct chatList *chat_list = newChatList();
    // Initialize chat array
    // for (int i = 0; i < MAX_CHATS; i++) {
    //     chat_list[i] = malloc(sizeof(struct chat));
    //     chat_list[i]->id = -1;
    //     chat_list[i]->public = false;
    //     chat_list[i]->numMembers = 0;
    // }

    // bool temp = true;

    while (1) {
        // if (temp) {
        //     handle_packet(NULL, client_list, &numClients, chat_list, &numChats);
        //     temp = !temp;
        // }

        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select (FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            printf("Error on select\n");
            exit (EXIT_FAILURE);
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i) {
            if (FD_ISSET (i, &read_fd_set)) {
                /* Connection request on original socket. */
                if (i == sock) {
                    int new;
                    size = sizeof (clientname);
                    new = accept (sock, (struct sockaddr *) &clientname, &size);
                    if (new < 0) {
                        printf("Error on accept\n");
                        exit (EXIT_FAILURE);
                    }
                    fprintf(stderr, "Server: connect from host %s, port %hd.\n",
                            inet_ntoa (clientname.sin_addr),
                            ntohs (clientname.sin_port));
                    FD_SET (new, &active_fd_set);
                } 
                
                /* Data arriving on an already-connected socket. */
                else {
                    struct packet p;
                    if (read_from_client(i, &p) <= 0) {
                        close (i);
                        FD_CLR (i, &active_fd_set);
                    }
                    else {
                        if (!handle_packet(i, &p, client_list, chat_list)) {
                            close(i);
                            FD_CLR(i, &active_fd_set);
                        }
                    }
                }
            }
        }
    }
}

bool handle_packet(int fd, struct packet *p, struct clientList *client_list, 
                   struct chatList *chat_list){
    // client_test(client_list, numClients);
    // return
    print_packet(p);
    printClients(client_list);

    if (p->type == NEW_USER){
        printf("Handling NEW_USER from %s:\n", p->src);

        // check if client already exists 
        if (hasClient(p->src, client_list)){
            printf("Error: client %s exists\n", p->src);
            send_packet(fd, LOGIN_FAIL, "Server", p->src, 0, 0, "");
        } 

        // probably should change this
        else if (numClients(client_list) >= MAX_CLIENTS) {
            printf("Error: out of space\n");
            send_packet(fd, LOGIN_FAIL, "Server", p->src, 0, 0, "");
        }

        // valid client, add & send ack
        else {
            addClient(p->src, fd, p->data, client_list);
            send_packet(fd, LOGIN_ACK, "Server", p->src, 0, 0, "");
        }
    } 

    else if (p->type == LOGIN){
        printf("Handling LOGIN from %s:\n", p->src);

        if (!hasClient(p->src, client_list)) {
            printf("Error: client does not exist\n");
            send_packet(fd, LOGIN_FAIL, "Server", p->src, 0, 0, "");
        } 

        else if (!validLogin(p->src, p->data, client_list)) {
            printf("Error: invalid login\n");
            send_packet(fd, LOGIN_FAIL, "Server", p->src, 0, 0, "");
        }
        else {
            logInClient(p->src, client_list);
            send_packet(fd, LOGIN_ACK, "Server", p->src, 0, 0, "");
        }

    }

    // should this have an ack??
    else if (p->type == LOGOUT){
        printf("Handling LOGOUT from %s:\n", p->src);

        // if client exists and is logged in, send

    }

    else if (p->type == CLIENT_LIST_REQ){
        printf("Handling CLIENT_LIST_REQ from %s:\n", p->src);

        // if client exists and is logged in, send

    }

    else if (p->type == CHAT_LIST_REQ){
        printf("Handling CHAT_LIST_REQ from %s:\n", p->src);

        // if client exists and is logged in, get the correct list & send

    }
    
    else if (p->type == MSG){
        printf("Handling MSG from %s:\n", p->src);

        // conditions:
        //      src client and chat id exist
        //      src is logged in
       
        // actions:     
        //      forward from src to all chat members
        //      send ack to src


    }
    
    else if (p->type == CREATE_CHAT){
        printf("Handling CREATE_CHAT from %s:\n", p->src);

        // conditions:
        //      src and all dests exist
        //      src is logged in
        // action:
        //      fwd chat request to all dests

    }
    
    else if (p->type == CHAT_ACCEPT){
        printf("Handling CHAT_ACCEPT from %s:\n", p->src);

        // conditions:
        //      src exists and chat id exists
        //      src is member of chat
        // action:
        //      update chat to have another member validated
        //      if all members have now validated, send CHAT_ACK


    }
    
    else if (p->type == CHAT_REJECT){
        printf("Handling CHAT_REJECT from %s:\n", p->src);

        // conditions:
        //      src exists and chat id exists
        //      src is member of chat
        // action:
        //      send CHAT_REJ to all members

    }
    
    else if (p->type == DELETE_USER){
        printf("Handling DELETE_USER from %s:\n", p->src);

        // conditions:
        //      * make sure user is logged in and fd matches


        // actions:
        //      * delete user
        //      * update chats to reflect
        //      * send delete ack

        // removeClient(p->src, client_list, numClients);

    

        send_packet(fd, DELETE_ACK, "Server", p->src, 0, 0, "");
    } 

    // Disconnect ---> currently deletes client but maybe should behave like logout?
    else if (p->type == 0) {
        return false;
    }

    // Unknown message type
    else {
        printf("ERROR: Invalid message type from: %s\n", p->src);
        // TODO - HANDLE
        return false;
    }
    return true;
}

void send_packet(int fd, int type, char *src, char *dst, int len, int msg_id, 
                 char *data){
    struct header p;
    p.type = htons(type);
    memset(p.src, 0, 20);
    memset(p.dst, 0, 20);
    memcpy(p.src, src, strlen(src) + 1);
    memcpy(p.dst, dst, strlen(dst) + 1);
    p.len = htonl(len);
    p.msg_id = htonl(msg_id);

    // Write header
    write(fd, (char *) &p, sizeof(p));

    // Write data
    if (len > 0) {
        write(fd, data, strlen(data) + 1);
    }
}
 
// returns file descriptor of new socket
int make_socket (uint16_t port) {
    int sock;
    struct sockaddr_in name;

    /* Create the socket. */
    sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        printf("Error creating socket\n");
        exit (EXIT_FAILURE);
    }

    /* Give the socket a name. */
    name.sin_family = AF_INET;
    name.sin_port = htons (port);
    name.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (sock, (struct sockaddr *) &name, sizeof (name)) < 0) {
        printf("Error on bind\n");
        exit (EXIT_FAILURE);
    }

  return sock;
}

bool read_from_client (int fd, struct packet *p) {
    char header_buf[sizeof(struct header)];
    memset(header_buf, 0, sizeof(struct header));
     
    int numBytes = recv(fd, header_buf, sizeof(struct header), 0);

    if (numBytes < 0) {
        printf("Error reading from client\n");
        return false;
    }

    else {
        /* Data read. */
        // fprintf (stderr, "Server: got message: `%s'\n", buffer);
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
            char data[DATA_SIZE];
            n = recv(fd, data, DATA_SIZE, 0);

            if (n < 0) {
                printf("Error reading packet data from client %s\n", p->src);
                return false;
            }
            memcpy(p->data, data, DATA_SIZE + 1);
        }

        return true;
    }
}

void print_packet(struct packet *p) {
    printf("Packet:\n");
    printf("* type: %d\n", p->type);
    printf("* src: %s\n", p->src);
    printf("* dst: %s\n", p->dst);
    printf("* len: %d\n", p->len);
    printf("* msg_id: %d\n", p->msg_id);

    if (p->len > 0) {
        printf("* data:\n%s\n", p->data);
    }

}
