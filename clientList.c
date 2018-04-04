#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include "clientList.h"

const static int USER_LEN = 20;
const static int PASS_LEN = 30;
const static int SALT_LEN = 16;
const static int LOC_LEN = 40;
const static int MAX_CLIENTS = 100;

char *getCountry(char *data);
char *getCity(char *data);
char *getGeoData(char *ip);

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
        memset(c->clients[i]->country, 0, LOC_LEN);
        memset(c->clients[i]->city, 0, LOC_LEN);
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

bool addClient(char *id, int fd, char *pw, char *ip, struct clientList *c){
    if (c->numClients == c->maxClients){
        return false;
    }

    if (strlen(ip) > 15) {
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

    memset(c->clients[c->numClients]->ip, 0, 16);
    memcpy(c->clients[c->numClients]->ip, ip, strlen(ip) + 1);

    char *data = getGeoData(ip);
    char *country = getCountry(data);
    char *city = getCity(data);
    
    memset(c->clients[c->numClients]->country, 0, LOC_LEN);
    memcpy(c->clients[c->numClients]->country, country, strlen(country) + 1);
 
    memset(c->clients[c->numClients]->city, 0, LOC_LEN);
    memcpy(c->clients[c->numClients]->city, city, strlen(city) + 1);

    c->clients[c->numClients]->fd = fd;
    c->clients[c->numClients]->logged_in = true;

    c->numClients += 1;
    return true;
}

char *getCountry(char *data){
    char *tmp = malloc(strlen(data) + 1);
    memcpy(tmp, data, strlen(data) + 1);
    int i = 0;
    char *token = strtok(tmp, ",");
    while(token){
        // printf("%s\n", token);
        if (token[i] == '\"' && token[i + 1] == 'c' && token[i+2] == 'o' &&
            token[i+3] == 'u' && token[i+4] == 'n' && token[i+5] == 't' && 
            token[i+6] == 'r' && token[i+7] == 'y' && token[i+8] == '_' && 
            token[i+9] == 'n' && token[i+10] == 'a' && token[i+11] == 'm' && 
            token[i+12] == 'e'){
            char *entry = token;
            token = strtok(entry, ":");
            token = strtok(NULL, ",");

            // char *city = malloc(strlen(token) + 1);
            // printf("found country: %s\n", token);
            char *country = malloc(LOC_LEN);
            memcpy(country, token, strlen(token) + 1);
            free(tmp);
            return country;
        }
        token = strtok(NULL, ",");
    }
}

char *getCity(char *data){
    char *tmp = malloc(strlen(data) + 1);
    memcpy(tmp, data, strlen(data) + 1);

    int i = 0;
    char *token = strtok(tmp, ",");
    while(token){
        // printf("%s\n", token);
        if (token[i] == '\"' && token[i + 1] == 'c' && token[i+2] == 'i' &&
            token[i+3] == 't' && token[i+4] == 'y') {
            char *entry = token;
            token = strtok(entry, ":");
            token = strtok(NULL, ",");

            // char *city = malloc(strlen(token) + 1);
            // printf("found city: %s\n", token);
            char *city = malloc(LOC_LEN);
            memcpy(city, token, strlen(token) + 1);
            free(tmp);
            return city;
        }
        token = strtok(NULL, ",");
    }
}


char *getGeoData(char *ip){
    char *url = "freegeoip.net";
    int portno = 80, n, BIG_BUF = 1024;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0){
        perror("Error opening socket to freegeoip.net\n.");
        return false;
    }

    server = gethostbyname("freegeoip.net");
    if (server == NULL){
        perror("No such host.\n");
        return false;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        perror("Error connecting to freegeoip.net\n");
        return false;
    }

    char *header1 = "GET /json/";
    char *header2 = " HTTP/1.0\r\nHost: freegeoip.net\r\n\r\n";
    int msg_size = strlen(header1) + strlen(ip) + strlen(header2) + 1;
    char *msg = malloc(msg_size);
    memset(msg, 0, msg_size);

    strcat(msg, header1);
    strcat(msg, ip);
    strcat(msg, header2);

    n = write(sockfd, msg, strlen(msg));

    if (n < 0) {
        perror("Error writing to freegeoip.net\n");
        return false;
    }

    // printf("post write\n");

    char buffer[BIG_BUF];
    memset(buffer, 0, BIG_BUF);

    n = read(sockfd, buffer, BIG_BUF);
    if (n < 0) {
        printf("Error reading from freegeoip.net\n");
        return false;
    }
    // printf("BUFFER:\n%s\n", buffer);

    int i = 0;

    // GET CITY
    char *token = NULL;
    token = strtok(buffer, "\n\n");
    while(token){
        // printf("TOKEN\n%s\n", token);
        if (token[0] == '{') break;
        token = strtok(NULL, "\n\n");
    }
    int len = strlen(token) + 1;
    char *json = malloc(len);
    memcpy(json, token, len);
    return json;
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
bool logInClient(char *id, char *pw, char *ip, struct clientList *c) {
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

                memset(c->clients[i]->ip, 0, sizeof(c->clients[i]->ip));
                memcpy(c->clients[i]->ip, ip, strlen(ip) + 1);


                char *data = getGeoData(ip);
                char *country = getCountry(data);
                char *city = getCity(data);
                
                memset(c->clients[c->numClients]->country, 0, LOC_LEN);
                memcpy(c->clients[c->numClients]->country, country, strlen(country) + 1);
             
                memset(c->clients[c->numClients]->city, 0, LOC_LEN);
                memcpy(c->clients[c->numClients]->city, city, strlen(city) + 1);

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
        printf("%d) %s : %d : %s : %s : %s : ", i, 
                c->clients[i]->id, c->clients[i]->fd, c->clients[i]->ip, 
                c->clients[i]->city, c->clients[i]->country);
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

// char *cityClientList(char *id, char *city, struct clientList *c){
//     int numClients = c->numClients, cityMatches = 0;
//     char *cityClientList = malloc(LOC_LEN * 4);
//     memset(cityClientList, 0, LOC_LEN * 4);
//     int bytes = 0;

//     for (int i = 0; i < numClients; i++){
//         // found a match (not self)
//         if (strcmp(c->clients[i]->city, city) == 0 && strcmp(c->clients[i]->id, id) != 0){
            
//         }
//     }

//     return cityClientList;
// }


int mailboxSize(struct client *c){
    return c->numMessages;
}

void emptyMailbox(struct client *c)
{
    cl->numMessages = 0;
}

// FOR TESTING

// int main() {
//     printf("RUNNING CLIENTLIST TEST\n");
//     struct clientList *c = newClientList();
//     // char *country = malloc(sizeof(40));
//     // char *city = malloc(sizeof(40));
//     // memset(country, 0, 40);
//     // memset(city, 0, 40);
//     char *data = getGeoData("208.80.152.201");
    
//     char *country = getCountry(data);
//     printf("Country: %s\n", country);

//     // printf("%s\n", data);
//     char *city = getCity(data);
//     printf("City: %s\n", city);

//     free(city);
//     free(country);
//     free(data);
//     // printf("Country: %s, City: %s\n", country, city);
//     // addClient("moi", 1, "secret", c);
//     // addClient("meeee", 2, "password", c);
//     // addClient("deanna", 3, "db", c);
//     // printClients(c);

//     // if (!hasClient("moi", c)){
//     //     printf("Failed hasClient 'moi'\n");
//     // }
//     // if (hasClient("notInList", c)){
//     //     printf("Failed hasClient 'notInList'\n");
//     // }

//     // if (getfd("moi", c) != 1 || getfd("meeee", c) != 2 || getfd("deanna", c) != 3){
//     //     printf("failed getfd()\n");

//     // }

//     // if (!isLoggedIn("deanna", c) || (!isLoggedIn("moi", c))){
//     //     printf("Failed log in test");
//     //     return -1;
//     // }

//     // logOutClient("deanna", c);

//     // if (isLoggedIn("deanna", c) || !isLoggedIn("moi", c)){
//     //     printf("Failed log out test");
//     //     return -1;

//     // }
//     // printClients(c);

//     // logInClient("deanna", "fakenews", c);

//     // if (isLoggedIn("deanna", c)){
//     //     printf("BAD\n");
//     //     return -1;
//     // }

//     // logInClient("deanna", "db", c);

//     // if (!isLoggedIn("deanna", c)){
//     //     printf("should be logged in\n");
//     //     return -1;
//     // }


//     // printClients(c);


//     // // // remove front
//     // // removeClient("moi", c);
//     // // printClients(c);

//     // // // remove back & add
//     // // removeClient("meeee", c);
//     // // printClients(c);


//     // printf("PASSED TESTS\n");

//     freeClientList(c);
// } 