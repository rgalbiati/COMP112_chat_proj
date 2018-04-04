#ifndef CHAT_H
#define CHAT_H

struct chat {
    char id[40];
    int chatStatus;			  // -1 - unset, 0 - valid, 1 - pending 
    // bool public;			  
    int numMembers;			  
    char members[5][20];      // up to 5 clients in a chat
   	int memberStatus[5];	  // -1 - unset, 0 - valid, 1 - pending 
};

struct chatList {
	int numChats;
	int maxChats;
	struct chat *chats[100];
};

// Chat List Functions
struct chatList *newChatList();
void freeChatList(struct chatList *c);
bool existsChat(char *chat_id, struct chatList *c);
char *addChat(int numMembers, char **members, char *id, struct chatList *c);
bool removeChat(char *chat_id, struct chatList *c);
struct chat *getChat(char *chat_id, struct chatList *c);
int memberAccept(char *id, char *client, struct chatList *c);
void deleteChatsWithMember(char *member, struct chatList *c);
void writeChatList(int fd, char *client, struct chatList *c);
int getChatListLen(char *client, struct chatList *c);
void printChats(struct chatList *c) ;
bool addUserToChat(char *client, char *chat_id, struct chatList *c);
bool removeUserFromChat(char *client, char *chat_id, struct chatList *c);
char *addUserToGeoChat(char *client, char *location, struct chatList *c);
void removeUserFromGeoChat(char *client, char *location, struct chatList *c);

// Chat functions
int numMembersChat(struct chat *ch);
int getChatStatus(struct chat *ch);
bool isMemberChat(char *client, struct chat *ch);
// bool isPublic(struct chat *ch);

#endif