#ifndef CLIENT_H
#define CLIENT_H

struct __attribute__((__packed__)) packet {
    unsigned short type;
    char src[20];
    char dst[20];
    unsigned int len;
    unsigned int msg_id;
    char data[400];
};

struct client {
    char id[20];
    char salt[16];
    char pw[46];            // this needs to be changed
    int fd;
    char ip[16];
    char country[40];
    char city[40];
    bool logged_in;
    int numMessages;
    struct packet mailbox[10];
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
bool addClient(char *id, int fd, char *pw, char *ip, struct clientList *c);
int getfd(char *id, struct clientList *c);
bool setfd(char *id, int fd, struct clientList *c);
bool logInClient(char *id, char *pw,  char *ip, struct clientList *c);
void printClients(struct clientList *c);
bool isLoggedIn(char *id, struct clientList *c);
void logOutClient(char *id, struct clientList *c);
int getClientListLen(struct clientList *c);
void writeClientList(int fd, struct clientList *c);
struct client *getClient(char *id, struct clientList *c);
void addPacketMailbox(char *id, struct packet p, struct clientList *c);
void logOutByFD(int fd, struct clientList *c);
char *getClientCountry(char *id, struct clientList *c);
char *getClientCity(char *id, struct clientList *c);

// client functions
int mailboxSize(struct client *c);

#endif