#ifndef CHAT_H
#define CHAT_H

struct chat {
    int id;
    int chatStatus;			  // -1 - unset, 0 - valid, 1 - pending 
    bool public;			  
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
int addChat(bool isPublic, int numMembers, char **members, struct chatList *c);
bool removeChat(int chat_id, struct chatList *c);
bool isMember(char *client, int chat_id, struct chatList *c);
int memberAccept(int id, char *client, struct chatList *c) ;


#endif