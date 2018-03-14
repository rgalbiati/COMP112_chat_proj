#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "clientList.h"

const static int USER_LEN = 20;
const static int PASS_LEN = 30;
const static int MAX_CLIENTS = 100;

struct clientList *newClientList() {
    struct clientList *c = malloc(sizeof(struct clientList));
    c->numClients = 0;
    c->maxClients = MAX_CLIENTS;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        c->clients[i] = malloc(sizeof(struct client));
        memset(c->clients[i]->id, 0, USER_LEN);
        memset(c->clients[i]->pw, 0, PASS_LEN);
        c->clients[i]->fd = -1;
        c->clients[i]->logged_in = false;
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
    memset(c->clients[c->numClients]->pw, 0, PASS_LEN);
    memcpy(c->clients[c->numClients]->id, id, strlen(id) + 1);
    memcpy(c->clients[c->numClients]->pw, pw, strlen(pw) + 1);
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
            char *old_id = c->clients[numClients - 1]->id;
            memcpy(c->clients[i]->id, old_id, strlen(old_id) + 1);
            c->clients[i]->fd = c->clients[numClients - 1]->fd;
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

// THIS NEEDS TO BE ENCRYPTED
bool validLogin(char *id, char *pw, struct clientList *c) {
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++) {
        if (strcmp(c->clients[i]->id, id) == 0) {
            return (strcmp(c->clients[i]->pw, pw) == 0);
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

void logInClient(char *id, struct clientList *c) {
    int numClients = c->numClients;
    for (int i = 0; i < numClients; i++) {
        if (strcmp(c->clients[i]->id, id) == 0) {
            c->clients[i]->logged_in = true;
        }
    }
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
         printf("%d) %s : %s : %d\n", i, c->clients[i]->id, c->clients[i]->pw, c->clients[i]->fd);
    }
}


// FOR TESTING
/*
int main() {
    printf("RUNNING CLIENTLIST TEST\n");
    struct clientList *c = newClientList();
    addClient("moi", 1, "secret", c);
    addClient("meeee", 2, "password", c);
    addClient("deanna", 3, "db", c);
    printClients(c);

    if (!hasClient("moi", c)){
        printf("Failed hasClient 'moi'\n");
    }
    if (hasClient("notInList", c)){
        printf("Failed hasClient 'notInList'\n");
    }

    if (getfd("moi", c) != 1 || getfd("meeee", c) != 2 || getfd("deanna", c) != 3){
        printf("failed getfd()\n");
    }

    // remove front
    removeClient("moi", c);
    printClients(c);

    // remove back & add
    removeClient("meeee", c);
    printClients(c);


    printf("PASSED TESTS\n");

    freeClientList(c);
} */