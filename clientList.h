#ifndef CLIENT_H
#define CLIENT_H

struct client {
    char id[20];
    char pw[30];            // this needs to be changed
    int fd;
    bool logged_in;
    // message mailbox (<= 5 msgs to send when logged in), 
    // chat request mailbox (<= 5 reqs)
};

struct clientList {
	int numClients;
	int maxClients;
	struct client *clients[100];
};

// Client List Functions
struct clientList *newClientList();
void *freeClientList(struct clientList *c);
int numClients(struct clientList *c);
bool hasClient(char *id, struct clientList *c);
void removeClient(char *id, struct clientList *c);
bool addClient(char *id, int fd, char *pw, struct clientList *c);
int getfd(char *id, struct clientList *c);
bool validLogin(char *id, char *pw, struct clientList *c);
void printClients(struct clientList *c);
bool isLoggedIn(char *id, struct clientList *c);
void logInClient(char *id, struct clientList *c);
void logOutClient(char *id, struct clientList *c);

#endif