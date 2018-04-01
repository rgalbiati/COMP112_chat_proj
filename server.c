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
 #include <openssl/rsa.h>
// #include <openssl/ssl.h>

const int BUFSIZE = 512;
const int DATA_SIZE = 400;
const int MAX_CHATS = 100;
const int MAX_CLIENTS = 100;
const static int USER_LEN = 20;
const static int PASS_LEN = 30;
const static int VALID_STATUS = 1;
const static int PENDING_STATUS = 0;
const static int MAILBOX_SIZE = 10;

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

bool handle_packet(int fd, struct packet *p, struct clientList *client_list, 
                   struct chatList *chat_list);
void send_packet(int fd, int type, char *src, char *dst, int len, int msg_id, 
                 char *data);
void sendClientList(int fd, char *dst, struct clientList *client_list);
int make_socket (uint16_t port);
bool read_from_client (int fd, struct packet *p);
void print_packet(struct packet *p);


int main (int argc, char* argv[]) {
    int sock, i, port, size;
    fd_set active_fd_set, read_fd_set;
    struct sockaddr_in clientname;
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


    while (1) {
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


                    // TODO SEND SERVER PUBLIC KEY TO CLIENT HERE
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
    print_packet(p);
    printClients(client_list);

    if (p->type == NEW_USER){
        printf("Handling NEW_USER from %s\n", p->src);

        // check if client already exists 
        if (hasClient(p->src, client_list)){
            printf("Error: client %s exists\n", p->src);
            send_packet(fd, LOGIN_FAIL, "Server", p->src, 0, 0, "");
        } 

        // TODO probably should change this
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
        printf("Handling LOGIN from %s\n", p->src);

        if (!hasClient(p->src, client_list)) {
            printf("Error: client does not exist\n");
            send_packet(fd, LOGIN_FAIL, "Server", p->src, 0, 0, "");
        } 

        else if (!logInClient(p->src, p->data, client_list)) {
            printf("Error: invalid login\n");
            send_packet(fd, LOGIN_FAIL, "Server", p->src, 0, 0, "");
        }
        else {
            if (setfd(p->src, fd, client_list)) {
                send_packet(fd, LOGIN_ACK, "Server", p->src, 0, 0, "");

                // send any messages in mailbox
                struct client *cl = getClient(p->src, client_list);
                int numMessages = mailboxSize(cl);

                for (int i = 0; i < numMessages; i++){
                    struct packet p = cl->mailbox[i];
                    send_packet(fd, p.type, p.src, p.dst, p.len, p.msg_id, p.data);
                }

            }
        }
    }

    // should this have an ack??
    else if (p->type == LOGOUT){
        printf("Handling LOGOUT from %s\n", p->src);

        // if client exists and is logged in, send
        if (!isLoggedIn(p->src, client_list)){
            printf("Error: client %s already logged out\n", p->src);
        } else {
            logOutClient(p->src, client_list);
        }

    }

    else if (p->type == CLIENT_LIST_REQ){
        printf("Handling CLIENT_LIST_REQ from %s\n", p->src);

        // if client exists and is logged in, send
        if (!isLoggedIn(p->src, client_list)){
            printf("Error: client %s is not logged in\n", p->src);
        }

        else {
            sendClientList(fd, p->src, client_list);
        }
    }

    else if (p->type == CHAT_LIST_REQ){
        printf("Handling CHAT_LIST_REQ from %s\n", p->src);

        // if client exists and is logged in, get the correct list & send
        if (!isLoggedIn(p->src, client_list)){
            printf("Error: client %s is not logged in\n", p->src);
        }

        else {

        }

        // what should the chat list look like?
        // sendChatList("Error")

    }
    
    else if (p->type == MSG){
        int chatId = atoi(p->dst);
        printf("Handling MSG from %s to chat %d\n", p->src, p->dst);
        struct chat *c = getChat(chatId, chat_list);

        // source must be logged in
        if (!isLoggedIn(p->src, client_list)){
            printf("Error: client %s is not logged in\n", p->src);
            char error[60];
            sprintf(error, "Client %s not logged in", p->src);
            send_packet(fd, MSG_ERROR, "Server", p->src, strlen(error) + 1, p->msg_id, error);
        }

        // chat does not exist
        else if (c == NULL){
            printf("Error: chat %d not found\n", chatId);
            char error[60];
            sprintf(error, "Chat %s not logged in", chatId);
            send_packet(fd, MSG_ERROR, "Server", p->src, strlen(error) + 1, p->msg_id, error);
        }

        // not valid chat member
        else if (!isMemberChat(p->src, c)){
            printf("Error: %s is not a member of chat: %d\n", p->src, chatId);
            char error[60];
            sprintf(error, "Client %s is not a member of chat %s", p->src, chatId);
            send_packet(fd, MSG_ERROR, "Server", p->src, strlen(error) + 1, p->msg_id, error);
        }

        // chat must be open
        else if (getChatStatus(c) != VALID_STATUS){
            printf("Error: chat status is not valid\n");
            char *error = "Chat is not yet available";
            send_packet(fd, MSG_ERROR, "Server", p->src, strlen(error) + 1, p->msg_id, error);
        }

        // valid chat message
        else {
            int numMembers = numMembersChat(c);
            for (int j = 0; j < numMembers; j++){

                // send if logged in
                if (isLoggedIn(c->members[j], client_list)){
                    int clientfd = getfd(p->src, client_list);
                    send_packet(clientfd, MSG, p->src, c->members[j], p->len, 
                            p->msg_id, p->data);
                } 

                // store in mailbox for when user logs back in
                else {
                    if (mailboxSize(getClient(c->members[j], client_list)) == MAILBOX_SIZE){
                        printf("Error: client %s's mailbox is full\n", c->members[j]);
                        // TODO - better error handling here
                        return true;
                    }

                    struct packet pck;
                    pck.type = MSG;
                    memset(pck.src, 0, USER_LEN);
                    memcpy(pck.src, p->src, strlen(p->src) + 1);
                    memset(pck.dst, 0, USER_LEN);
                    memcpy(pck.dst, c->members[j], strlen(c->members[j]) + 1);
                    pck.len = p->len;
                    pck.msg_id = p->msg_id;
                    memset(pck.data, 0, 400);
                    memcpy(pck.data, p->data, p->len + 1);
                    addPacketMailbox(c->members[j], pck, client_list);
                }
                
            }
        }
    }
    
    else if (p->type == CREATE_CHAT){
        printf("Handling CREATE_CHAT from %s\n", p->src);

        // create list of dest clients
        int len = p->len;
        int numClients = 1;         // include src

        // get num clients in data
        for (int i = 0; i < len; i++){
            if (p->data[i] == '\0') {
                numClients += 1;
            }
        }

        if (numClients > 5) {
            printf("Error: Too many clients for chat\n");
            char *error = "Too many clients";
            send_packet(fd, CHAT_FAIL, "Server", p->src, strlen(error) + 1, 0, error);
            return true;
        } else if (numClients == 1){
            printf("Error: client must request at least one user to chat with\n");
            return true;
        }

        char *token;
        char *members[numClients];
        memset(members, 0, 20 * numClients);
   
        /* Get the first client name */
        token = strtok(p->data, "\0");
        members[0] = p->src;
        int i = 1;
   
        /* Get other client names */
        while (token != NULL) {
            members[i] = token;
            token = strtok(NULL, "\0");
            i += 1;
        }

        printf("Clients:\n");
        for (int j = 0; j < numClients; j++){
            printf("* %s\n", members[j]);
        }

        // all clients exist
        for (int i = 0; i < numClients; i++){
            printf("Checking for client %s\n", members[i]);
            if (!hasClient(members[i], client_list)) {
                printf("Error: client %s does not exist\n");
                char *error = "Client does not exist";
                send_packet(fd, CHAT_FAIL, "Server", p->src, strlen(error) + 1, 0, error);
                return true;
            }
        }
       
        // client must be logged in
        if (!isLoggedIn(p->src, client_list)) {
            printf("Error: client %s is not logged in\n");
            char *error = "Source not logged in";
            send_packet(fd, CHAT_FAIL, "Server", p->src, strlen(error), 0, error);
        }

        // valid chat -- forward to all clients
        else {
            // for now, all chats public
            int id = addChat(true, numClients, members, chat_list);
            memberAccept(id, p->src, chat_list);

            char id_str[12];
            sprintf(id_str, "%d", id);

            for (int i = 1; i < numClients; i++){
                if (isLoggedIn(members[i], client_list)){
                    int clientfd = getfd(members[i], client_list);
                    send_packet(clientfd, CHAT_REQ, p->src, members[i], 
                                strlen(id_str) + 1, 0, id_str);
                }
                    
                // store in mailbox
                else {

                    if (mailboxSize(getClient(members[i], client_list)) == MAILBOX_SIZE){
                        printf("Error: client %s's mailbox is full\n", members[i]);

                        char error[60];
                        sprintf(error, "Client %s's mailbox is full", members[i]);
                        send_packet(fd, CHAT_FAIL, "Server", p->src, 
                                    strlen(error) + 1, 0, error);
                        return true;
                    }
                    
                    struct packet pck;
                    pck.type = CHAT_REQ;
                    
                    memset(pck.src, 0, USER_LEN);
                    memcpy(pck.src, p->src, strlen(p->src) + 1);

                    memset(pck.dst, 0, USER_LEN);
                    memcpy(pck.dst, members[i], strlen(members[i]) + 1);

                    pck.len = strlen(id_str) + 1;
                    pck.msg_id = 0;
                    memset(pck.data, 0, 400);
                    memcpy(pck.data, id_str, strlen(id_str) + 1);
                    addPacketMailbox(members[i], pck, client_list);
                }
            }
        }
    }
    
    else if (p->type == CHAT_ACCEPT){
        printf("Handling CHAT_ACCEPT from %s\n", p->src);

        int chatId = atoi(p->data);
        struct chat *ch = getChat(chatId, chat_list);

        if (ch == NULL){
            printf("Error: requested chat does not exist\n");
        }

        else if (!isLoggedIn(p->src, client_list)){
            printf("Error: client %s is not logged in\n", p->src);
        } 

        else if (!isMemberChat(p->src, ch)) {
            printf("Error: client %s is not a member of requested chat\n");
        }

        // valid chat accept
        else {
            memberAccept(chatId, p->src, chat_list);

            // all members have validated chat, send ack to all members
            if (getChatStatus(ch) == VALID_STATUS){
                int numMembers = ch->numMembers;
                for (int j = 0; j < numMembers; j++){

                    if (isLoggedIn(ch->members[j], client_list)){
                        int clientfd = getfd(ch->members[j], client_list);
                        
                        send_packet(clientfd, CHAT_ACK, "Server", ch->members[j], 
                                    strlen(p->data) + 1, 0, p->data);
                    }
                    

                    // store in mailbox
                    else {

                        if (mailboxSize(getClient(ch->members[j], client_list)) == MAILBOX_SIZE){
                            printf("Error: client %s's mailbox is full\n", ch->members[j]);
                            char error[60];
                            sprintf(error, "Client %s's mailbox is full", ch->members[j]);
                            send_packet(fd, CHAT_FAIL, "Server", p->src, 
                                        strlen(error) + 1, 0, error);
                            return true;
                        }
                        struct packet pck;
                        pck.type = CHAT_ACK;
                        memset(pck.src, 0, USER_LEN);
                        memcpy(pck.src, p->src, strlen(p->src) + 1);
                        memset(pck.dst, 0, USER_LEN);
                        memcpy(pck.dst, ch->members[j], strlen(ch->members[j]) + 1);
                        pck.len = p->len;
                        pck.msg_id = 0;
                        memset(pck.data, 0, 400);
                        memcpy(pck.data, p->data, p->len + 1);
                        addPacketMailbox(ch->members[j], pck, client_list);
                    }
                }       
            }
        }
    }
    
    else if (p->type == CHAT_REJECT){
        printf("Handling CHAT_REJECT from %s\n", p->src);

        int chatId = atoi(p->data);
        struct chat *ch = getChat(chatId, chat_list);

        if (ch == NULL){
            printf("Error: requested chat does not exist\n");
        }

        else if (!isLoggedIn(p->src, client_list)){
            printf("Error: client %s is not logged in\n", p->src);
        } 

        else if (!isMemberChat(p->src, ch)) {
            printf("Error: client %s is not a member of requested chat\n");
        }

        // Valid chat reject
        else {
            // send reject to all requested members
            int numMembers = ch->numMembers;
            for (int j = 0; j < numMembers; j++){

                if (isLoggedIn(ch->members[j], client_list)){
                    int clientfd = getfd(ch->members[j], client_list);
                    send_packet(clientfd, CHAT_FAIL, "Server", ch->members[j], 
                                strlen(p->data) + 1, 0, p->data);
                }
                // else store in mailbox + send when user logs back in
                else {
                    if (mailboxSize(getClient(ch->members[j], client_list)) == MAILBOX_SIZE){
                        printf("Error: client %s's mailbox is full\n", ch->members[j]);
                        char error[60];
                        sprintf(error, "Client %s's mailbox is full", ch->members[j]);
                        send_packet(fd, CHAT_FAIL, "Server", p->src, 
                                    strlen(error) + 1, 0, error);
                        return true;
                    }
                    struct packet pck;
                    pck.type = CHAT_FAIL;
                    memset(pck.src, 0, USER_LEN);
                    memcpy(pck.src, p->src, strlen(p->src) + 1);
                    memset(pck.dst, 0, USER_LEN);
                    memcpy(pck.dst, ch->members[j], strlen(ch->members[j]) + 1);
                    pck.len = p->len;
                    pck.msg_id = 0;
                    memset(pck.data, 0, 400);
                    memcpy(pck.data, p->data, p->len + 1);
                    addPacketMailbox(ch->members[j], pck, client_list);
                }
            }

            // remove chat
            removeChat(chatId, chat_list);   
        }
    }
    
    else if (p->type == DELETE_USER){
        printf("Handling DELETE_USER from %s\n", p->src);

        if (!isLoggedIn(p->src, client_list)){
            printf("Error: client %s is not logged in\n");
        } 

        else {
            // Currently, all chats with the src as a member are deleted
            deleteChatsWithMember(p->src, chat_list);
            removeClient(p->src, client_list);
            send_packet(fd, DELETE_ACK, "Server", p->src, 0, 0, "");
        }

        send_packet(fd, DELETE_ACK, "Server", p->src, 0, 0, "");
    } 

    // Disconnect ---> currently deletes client but maybe should behave like logout?
    else if (p->type == 0) {
        logOutByFD(fd, client_list);
        // return false;
    }

    // Unknown message type -- remove client + delete from chats
    else {
        printf("ERROR: Invalid message type from: %s\n", p->src);
        removeClient(p->src, client_list);
        deleteChatsWithMember(p->src, chat_list);
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

void sendClientList(int fd, char *dst, struct clientList *client_list) {
    int len = getClientListLen(client_list);
    struct header h;
    h.type = htons(CLIENT_LIST);
    memset(h.src, 0, 20);
    memset(h.dst, 0, 20);
    memcpy(h.src, "Server", strlen("Server") + 1);
    memcpy(h.dst, dst, strlen(dst) + 1);
    h.len = htonl(len);
    h.msg_id = htonl(0);
    int n = write(fd, (char *) &h, sizeof(h));

    if (n < 0){
        printf("Error writing to %d\n", fd);
        return;
    }
    writeClientList(fd, client_list);
}
 
// Creates socket and returns new file descriptor
int make_socket (uint16_t port) {
    int sock;
    struct sockaddr_in name;

    sock = socket (PF_INET, SOCK_STREAM, 0);
    if (sock < 0){
        printf("Error creating socket\n");
        exit (EXIT_FAILURE);
    }

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
