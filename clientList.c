#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
// #include <sys/socket.h>
// #include <sys/types.h>
#include <unistd.h>
#include "clientList.h"

const static int USER_LEN = 20;
const static int PASS_LEN = 30;
const static int SALT_LEN = 16;
const static int MAX_CLIENTS = 100;

// generate random string of 16 chars
void createSalt(char *salt){
    for (int i = 0; i < SALT_LEN; i++){
        salt[i] = rand() % 128;
    }
}

struct clientList *newClientList() {
    struct clientList *c = malloc(sizeof(struct clientList));
    c->numClients = 0;
    c->maxClients = MAX_CLIENTS;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        c->clients[i] = malloc(sizeof(struct client));
        memset(c->clients[i]->id, 0, USER_LEN);
        memset(c->clients[i]->pw, 0, PASS_LEN + SALT_LEN);
        memset(c->clients[i]->salt, 0, SALT_LEN);
        c->clients[i]->fd = -1;
        c->clients[i]->logged_in = false;
        c->clients[i]->numMessages = 0;
        memset(c->clients[i]->mailbox, 0, sizeof(struct packet) * 10);
    }
    return c;
}

void *freeClientList(struct clientList *c){
    int maxClients = c->maxClients;

    for (int i = 0; i < maxClients; i++) {
        free(c->clients[i]);
    }
    free(c);
}

int numClients(struct clientList *c) {
    return c->numClients;
}

bool addClient(char *id, int fd, char *pw, struct clientList *c){
    if (c->numClients == c->maxClients){
        return false;
    }
    
    memset(c->clients[c->numClients]->id, 0, USER_LEN);
    memset(c->clients[c->numClients]->pw, 0, PASS_LEN + SALT_LEN);
    
    createSalt(c->clients[c->numClients]->salt);
    memcpy(c->clients[c->numClients]->id, id, strlen(id) + 1);

    // TODO PW IS SALTED BUT STILL NEEDS TO BE HASHED
    char salted_pass[SALT_LEN + PASS_LEN];
    memcpy(&salted_pass, c->clients[c->numClients]->salt, SALT_LEN);
    memcpy(&salted_pass[SALT_LEN], pw, strlen(pw) + 1);
    // printf("salted: %s\n", salted_pass);

    memcpy(c->clients[c->numClients]->pw, salted_pass, strlen(salted_pass) + 1);

    c->clients[c->numClients]->fd = fd;
    c->clients[c->numClients]->logged_in = true;
    c->numClients += 1;
    return true;
}

bool hasClient(char *id, struct clientList *c) {
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++) {
        if (strcmp(c->clients[i]->id, id) == 0) {
            return true;
        }
    }
    return false;
}


void removeClient(char *id, struct clientList *c) {
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++) {

        // save memory by moving last up to new open spot
        if (strcmp(c->clients[i]->id, id) == 0 && i != numClients - 1) {
            char *last_id = c->clients[numClients - 1]->id;
            char *last_salt = c->clients[numClients - 1]->salt;
            char *last_pass = c->clients[numClients - 1]->pw;

            memset(c->clients[i]->id, 0, USER_LEN);
            memset(c->clients[i]->salt, 0, SALT_LEN);
            memset(c->clients[i]->pw, 0, PASS_LEN + SALT_LEN);

            memcpy(c->clients[i]->id, last_id, strlen(last_id) + 1);
            memcpy(c->clients[i]->salt, last_salt, SALT_LEN);
            memcpy(c->clients[i]->pw, last_pass, strlen(last_pass) + 1);


            c->clients[i]->fd = c->clients[numClients - 1]->fd;
            c->clients[i]->logged_in = c->clients[numClients - 1]->logged_in;

            int numMessages = c->clients[numClients - 1]->numMessages;
            c->clients[i]->numMessages = numMessages;
            memcpy(c->clients[i]->mailbox, c->clients[numClients - 1]->mailbox, numMessages * sizeof(struct packet));

            break; 
        }
    }

    c->numClients -= 1;
}

int getfd(char *id, struct clientList *c) {
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++) {
        if (strcmp(c->clients[i]->id, id) == 0) {
            return c->clients[i]->fd;
        }
    }
    return -1;
}

bool setfd(char *id, int fd, struct clientList *c){
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++) {
        if (strcmp(c->clients[i]->id, id) == 0) {
            c->clients[i]->fd = fd;
            return true;
        }
    }
    return false;
}

// THIS NEEDS TO BE ENCRYPTED
bool logInClient(char *id, char *pw, struct clientList *c) {
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++) {

        if (strcmp(c->clients[i]->id, id) == 0){

            // salt & hash the test password
            char salted_pass[PASS_LEN + SALT_LEN];
            char *salt = c->clients[i]->salt;
            memcpy(&salted_pass, salt, SALT_LEN);
            memcpy(&salted_pass[SALT_LEN], pw, strlen(pw) + 1);

            // TODO password is hashed not salted

            if (strcmp(salted_pass, c->clients[i]->pw) == 0){
                c->clients[i]->logged_in = true;
                return true;
            }        
        }
    }
    return false;
}

bool isLoggedIn(char *id, struct clientList *c) {
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++) {
        if (strcmp(c->clients[i]->id, id) == 0) {
            return c->clients[i]->logged_in;
        }
    }
    return false;
}

void logOutClient(char *id, struct clientList *c) {
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++) {
        if (strcmp(c->clients[i]->id, id) == 0) {
            c->clients[i]->logged_in = false;
        }
    }
}

void printClients(struct clientList *c) {
    printf("CLIENT LIST:\n");
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++) {
        printf("%d) %s : %d : ", i, c->clients[i]->id, c->clients[i]->fd);
         // printf("%d) %s : %s : %d : ", i, c->clients[i]->id, c->clients[i]->pw, c->clients[i]->fd);
         if (c->clients[i]->logged_in){
            printf("online\n");
         } else {
            printf("offline\n");
         }
    }
}

int getClientListLen(struct clientList *c){
    int len = 0, numClients = c->numClients;
    for (int i = 0; i < numClients; i++){
        len += strlen(c->clients[i]->id) + 1;
    }
    return len;
}


void writeClientList(int fd, struct clientList *c) {
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++){
        write(fd, c->clients[i]->id, strlen(c->clients[i]->id) + 1);
    }
}

void logOutByFD(int fd, struct clientList *c){
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++){
        if (c->clients[i]->fd == fd){
            c->clients[i]->logged_in = false;
            return;
        }
    }
}

struct client *getClient(char *id, struct clientList *c){
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++){
        if (strcmp(id, c->clients[i]->id) == 0){
            return c->clients[i];
        }
    }
    return NULL;
}

void addPacketMailbox(char *id, struct packet p, struct clientList *c){
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++){
        if (strcmp(id, c->clients[i]->id) == 0){
            int numMessages = c->clients[i]->numMessages;
            c->clients[i]->mailbox[numMessages].type = p.type;
            memset(c->clients[i]->mailbox[numMessages].src, 0, USER_LEN);
            memcpy(c->clients[i]->mailbox[numMessages].src, p.src, strlen(p.src) + 1);
            memset(c->clients[i]->mailbox[numMessages].dst, 0, USER_LEN);
            memcpy(c->clients[i]->mailbox[numMessages].dst, p.dst, strlen(p.dst) + 1);
            c->clients[i]->mailbox[numMessages].len = p.len;
            c->clients[i]->mailbox[numMessages].msg_id = p.msg_id;

            memset(c->clients[i]->mailbox[numMessages].data, 0, 400);
            memcpy(c->clients[i]->mailbox[numMessages].data, p.data, p.len + 1);

            c->clients[i]->numMessages += 1;
            return;
        }
    }
}



int mailboxSize(struct client *c){
    return c->numMessages;
}

// FOR TESTING

// int main() {
//     printf("RUNNING CLIENTLIST TEST\n");
//     struct clientList *c = newClientList();
//     addClient("moi", 1, "secret", c);
//     addClient("meeee", 2, "password", c);
//     addClient("deanna", 3, "db", c);
//     printClients(c);

//     if (!hasClient("moi", c)){
//         printf("Failed hasClient 'moi'\n");
//     }
//     if (hasClient("notInList", c)){
//         printf("Failed hasClient 'notInList'\n");
//     }

//     if (getfd("moi", c) != 1 || getfd("meeee", c) != 2 || getfd("deanna", c) != 3){
//         printf("failed getfd()\n");

//     }

//     if (!isLoggedIn("deanna", c) || (!isLoggedIn("moi", c))){
//         printf("Failed log in test");
//         return -1;
//     }

//     logOutClient("deanna", c);

//     if (isLoggedIn("deanna", c) || !isLoggedIn("moi", c)){
//         printf("Failed log out test");
//         return -1;

//     }
//     printClients(c);

//     logInClient("deanna", "fakenews", c);

//     if (isLoggedIn("deanna", c)){
//         printf("BAD\n");
//         return -1;
//     }

//     logInClient("deanna", "db", c);

//     if (!isLoggedIn("deanna", c)){
//         printf("should be logged in\n");
//         return -1;
//     }


//     printClients(c);


//     // // remove front
//     // removeClient("moi", c);
//     // printClients(c);

//     // // remove back & add
//     // removeClient("meeee", c);
//     // printClients(c);


//     printf("PASSED TESTS\n");

//     freeClientList(c);
// } 