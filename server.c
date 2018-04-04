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
#include "encrypt.h"
 // #include <openssl/rsa.h>
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
                   struct chatList *chat_list, char *ip);
void send_packet(int fd, int type, char *src, char *dst, int len, int msg_id, 
                 char *data);
void sendClientList(int fd, char *dst, struct clientList *client_list);
void sendChatList(int fd, char *dst, struct chatList *chat_list);
int make_socket (uint16_t port);
bool read_from_client (int fd, struct packet *p);
void geoChat(int fd, char *client, struct clientList *client_list, struct chatList *chat_list);
void print_packet(struct packet *p);


int main (int argc, char* argv[]) {
    int sock, i, port, size;
    fd_set active_fd_set, read_fd_set;
    struct sockaddr_in clientname;
    int numClients = 0, numChats = 0;
    int len = sizeof(struct sockaddr);


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

                        // struct sockaddr_in* addr = (struct sockaddr_in*)&clientname;
                        // struct in_addr ipAddr = addr->sin_addr;

                        // char client_ip[INET_ADDRSTRLEN];
                        // inet_ntop(AF_INET, &ipAddr, client_ip, INET_ADDRSTRLEN);
                        struct sockaddr_in foo;

                        if (getpeername(i, (struct sockaddr *)&foo, &len) < 0){
                            printf("ERROR: can't get IP\n");
                            continue;
                        } 

                        printf("client IP: %s\n", inet_ntoa(foo.sin_addr));

                        if (!handle_packet(i, &p, client_list, chat_list, inet_ntoa(foo.sin_addr))) {
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
                   struct chatList *chat_list, char *ip){

    print_packet(p);
    printClients(client_list);
    printChats(chat_list);

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
            addClient(p->src, fd, p->data, ip, client_list);
            send_packet(fd, LOGIN_ACK, "Server", p->src, 0, 0, "");

            geoChat(fd, p->src, client_list, chat_list);

        }
    } 

    else if (p->type == LOGIN){
        printf("Handling LOGIN from %s\n", p->src);

        if (!hasClient(p->src, client_list)) {
            printf("Error: client does not exist\n");
            send_packet(fd, LOGIN_FAIL, "Server", p->src, 0, 0, "");
        } 

        else if (!logInClient(p->src, p->data, ip, client_list)) {
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

                geoChat(fd, p->src, client_list, chat_list);

            }
        }
    }

    // should this have an ack??
    else if (p->type == LOGOUT){
        printf("Handling LOGOUT from %s\n", p->src);

        // if client exists and is logged in, send
        if (!isLoggedIn(p->src, client_list)){
            printf("Error: client %s already logged out\n", p->src);
        } else if (fd != getfd(p->src, client_list)) {
            printf("Error: fd for client %s does not match\n", p->src);
            return false;
        } else {
            logOutClient(p->src, client_list);
            char *country_chatid = getClientCountry(p->src, client_list);
            char *city_chatid = getClientCity(p->src, client_list);
            removeUserFromGeoChat(p->src, country_chatid, chat_list);
            removeUserFromGeoChat(p->src, city_chatid, chat_list);
        }

    }

    else if (p->type == CLIENT_LIST_REQ){
        printf("Handling CLIENT_LIST_REQ from %s\n", p->src);

        // if client exists and is logged in, send
        if (!isLoggedIn(p->src, client_list)){
            printf("Error: client %s is not logged in\n", p->src);
        } 

        else if (fd != getfd(p->src, client_list)) {
            printf("Error: fd for client %s does not match\n", p->src);
            return false;
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

        else if (fd != getfd(p->src, client_list)) {
            printf("Error: fd for client %s does not match\n", p->src);
            return false;
        } 

        else { 
            sendChatList(fd, p->src, chat_list);
        }

        // what should the chat list look like?
        // sendChatList("Error")

    }
    
    else if (p->type == MSG){
        char *chatId = p->dst;
        printf("Handling MSG from %s to chat %s\n", p->src, p->dst);
        struct chat *c = getChat(chatId, chat_list);

        // source must be logged in
        if (!isLoggedIn(p->src, client_list)){
            printf("Error: client %s is not logged in\n", p->src);
            char error[60];
            sprintf(error, "Client %s not logged in", p->src);
            send_packet(fd, MSG_ERROR, "Server", p->src, strlen(error) + 1, p->msg_id, error);
        }

        // fd must match
        else if (fd != getfd(p->src, client_list)) {
            printf("Error: fd for client %s does not match\n", p->src);
            return false;
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

                if (strcmp(p->src, c->members[j]) == 0){
                    continue;
                }

                // send if logged in
                if (isLoggedIn(c->members[j], client_list)){
                    int clientfd = getfd(c->members[j], client_list);
                    send_packet(clientfd, MSG, p->src, p->dst, p->len, 
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
                    memcpy(pck.dst, p->dst, strlen(p->dst) + 1);
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
            char *error = "Client must request at least one user to chat with";
            send_packet(fd, CHAT_FAIL, "Server", p->src, strlen(error) + 1, 0, error);
            return true;
        }

        char *members[numClients];
        memset(members, 0, 20 * numClients);
        int i = 1;

        int cursor = 0;
        char* buf = p->data;

        members[0] = p->src;

        while (cursor < p->len)
        {
            // sprintf(&members[i], "%s\n", buf);
            printf("%s\n", buf);
            members[i] = buf;
            cursor += strlen(buf) + 1;
            buf += strlen(buf) + 1;
            i+= 1;
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

        // fd must match
        else if (fd != getfd(p->src, client_list)) {
            printf("Error: fd for client %s does not match\n", p->src);
            return false;
        } 


        // valid chat -- forward to all clients
        else {

            char *id = addChat(numClients, members, NULL, chat_list);
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

        char *chatId = p->data;
        struct chat *ch = getChat(chatId, chat_list);

        if (ch == NULL){
            printf("Error: requested chat %s does not exist\n", chatId);
        }

        // fd must match
        else if (fd != getfd(p->src, client_list)) {
            printf("Error: fd for client %s does not match\n", p->src);
            return false;
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

        char *chatId = p->data;
        struct chat *ch = getChat(chatId, chat_list);

        if (ch == NULL){
            printf("Error: requested chat does not exist\n");
        }

        // fd must match
        else if (fd != getfd(p->src, client_list)) {
            printf("Error: fd for client %s does not match\n", p->src);
            return false;
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

        // fd must match
        else if (fd != getfd(p->src, client_list)) {
            printf("Error: fd for client %s does not match\n", p->src);
            return false;
        } 

        else {
            // Currently, all chats with the src as a member are deleted
            deleteChatsWithMember(p->src, chat_list);
            removeClient(p->src, client_list);
            send_packet(fd, DELETE_ACK, "Server", p->src, 0, 0, "");
        }
    } 

    // Disconnect ---> behaves like logout
    else if (p->type == 0) {
        logOutByFD(fd, client_list);
        return false;
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
                 char *data)
{
    encrypted_write(fd, type, src, dst, len, msg_id, data);
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

void sendChatList(int fd, char *dst, struct chatList *chat_list) {
    int len = getChatListLen(dst, chat_list);

    struct header h;
    h.type = htons(CHAT_LIST);

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

    writeChatList(fd, dst, chat_list);

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
    return encrypted_read(fd, p);
}

void geoChat(int fd, char *client, struct clientList *client_list, struct chatList *chat_list){
    // TODO here add to city + country chats + send chat reqs
    char *country = getClientCountry(client, client_list);
    char *city = getClientCity(client, client_list);

    if (strcmp(country, "\"\"") != 0){
        char *country_chatid = addUserToGeoChat(client, country, chat_list);
        send_packet(fd, CHAT_REQ, country_chatid, client, 
                strlen(country_chatid) + 1, 0, country_chatid);
    }

    if (strcmp(city, "\"\"") != 0){
        char *city_chatid = addUserToGeoChat(client, city, chat_list);
        send_packet(fd, CHAT_REQ, city_chatid, client, 
                strlen(city_chatid) + 1, 0, city_chatid);
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
