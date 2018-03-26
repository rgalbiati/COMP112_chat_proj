#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "chatList.h"

const static int MAX_CHATS = 100;
const static int USER_LEN = 20;
const static int VALID_STATUS = 1;
const static int PENDING_STATUS = 0;

// struct chat {
//     int id;
//     int chatStatus;			  // -1 - unset, 0 - valid, 1 - pending 
//     bool public;			  
//     int numMembers;			  
//     char members[5][20];      // up to 5 clients in a chat
//    	int memberStatus[5];	  // -1 - unset, 0 - valid, 1 - pending 
// };

struct chatList *newChatList() {
	struct chatList *c = malloc(sizeof(struct chatList));
	c->numChats = 0;
	c->maxChats = MAX_CHATS;

	for (int i = 0; i < MAX_CHATS; i++) {
		c->chats[i] = malloc(sizeof(struct chat));
		c->chats[i]->id = -1;
		c->chats[i]->public = false;
		c->chats[i]->numMembers = 0;
		c->chats[i]->chatStatus = -1;
		for (int j = 0; j < 5; j++) {
			memset(c->chats[i]->members[j], 0, USER_LEN);
			c->chats[i]->memberStatus[j] = -1;
		}
	}

	return c;
}

void freeChatList(struct chatList *c){
	int maxChats = c->maxChats;

    for (int i = 0; i < maxChats; i++) {
        free(c->chats[i]);
    }
    free(c);
}

int numMembersChat(struct chat *ch){
	if (ch == NULL){
		return 0;
	}
	return ch->numMembers;
}

int getChatStatus(struct chat *ch){
	if (ch == NULL){
		return -1;
	}

	return ch->chatStatus;
}

bool existsChat(int chat_id, struct chatList *c){
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++) {
		if (c->chats[i]->id == chat_id) {
			return true;
		}
	}

	return false;
}

// returns id
int addChat(bool isPublic, int numMembers, char **members, struct chatList *c) {
	if (c->numChats == c->maxChats || numMembers > 5) {
		return -1;
	}
	int chatIndex = c->numChats;
	for (int i = 0; i < numMembers; i++) {
		memcpy(c->chats[chatIndex]->members[i], members[i], strlen(members[i]) + 1);
		c->chats[chatIndex]->memberStatus[i] = PENDING_STATUS;
	}
	c->chats[chatIndex]->numMembers = numMembers;
	c->chats[chatIndex]->public = isPublic;
	c->chats[chatIndex]->id = chatIndex;
	c->chats[chatIndex]->chatStatus = PENDING_STATUS;

	c->numChats += 1;
	return chatIndex;
}

bool removeChat(int chat_id, struct chatList *c) {
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++) {
		if (c->chats[i]->id == chat_id) {
			// move last element into slot i to save space
			if (i != numChats - 1) {
				c->chats[i]->id = c->chats[numChats - 1]->id;
				c->chats[i]->chatStatus = c->chats[numChats - 1]->chatStatus;
				c->chats[i]->public = c->chats[numChats - 1]->public;
				c->chats[i]->numMembers = c->chats[numChats - 1]->numMembers;

				int nm = c->chats[numChats - 1]->numMembers;
				for (int j = 0; j < nm; j++) {
					char *mem = c->chats[numChats - 1]->members[j];
					memcpy(c->chats[i]->members[j], mem, strlen(mem) + 1);
					c->chats[i]->memberStatus[j] = c->chats[numChats - 1]->memberStatus[j];
				}
        	} 
        	c->numChats -= 1;
        	return true;

		}
	}
	return false;
}

struct chat *getChat(int chat_id, struct chatList *c) {
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++) {
		if (c->chats[i]->id == chat_id) {
			return c->chats[i];
		}
	}
	return NULL;
}


bool isMemberChat(char *client, struct chat *ch) {
	int numMembers = ch->numMembers;

	for (int j = 0; j < numMembers; j++) {
		if (strcmp(client, ch->members[j]) == 0) {
			return true;
		}
	}

	return false;
}


// updates member within chat & returns overall chat status
int memberAccept(int id, char *client, struct chatList *c) {
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++){
		if (c->chats[i]->id == id){
			int numMembers = c->chats[i]->numMembers; 

			// valid client
			for (int j = 0; j < numMembers; j++) {
				if (strcmp(c->chats[i]->members[j], client) == 0){
					c->chats[i]->memberStatus[j] = VALID_STATUS;

					// return overall chat status
					bool allValid = true;
					for (int k = 0; k < numMembers; k++) {
						allValid = allValid && c->chats[i]->memberStatus[k];
					}
					c->chats[i]->chatStatus = allValid;
					return allValid;
				}
			}
		}
	}

	// not found
	return -1;

}

void printChats(struct chatList *c) {
	printf("CHAT LIST:\n");
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++) {
		int numMembers = c->chats[i]->numMembers;
	
		printf("%d) id: %d, %d members, ", i, c->chats[i]->id, numMembers);
	
		
		if (c->chats[i]->public == true) {
			printf("public, ");
		} else {
			printf("private, ");
		}

		if (c->chats[i]->chatStatus == VALID_STATUS) {
			printf("status: valid");
		} else if (c->chats[i]->chatStatus == PENDING_STATUS){
			printf("status: pending");
		}

		printf("\n");

		for (int j = 0; j < numMembers; j++){
			printf(" -> %s - ", c->chats[i]->members[j], c);
			if (c->chats[i]->memberStatus[j] == VALID_STATUS) {
				printf("accepted\n");
			} else {
				printf("pending\n");
			}
		}
	}
}

void deleteChatsWithMember(char *member, struct chatList *c){
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++){
		struct chat *ch = c->chats[i];
		if (isMemberChat(member, ch)){
			removeChat(ch->id, c);
		}
	}

}

// int main(){
// 	printf("CHAT TEST\n");

// 	struct chatList *c = newChatList();
// 	// printChats(c);

// 	char *members[] = {"Deanna", "Claire", "Caro"};
// 	char *members2[] = {"Caro", "Grace"};
// 	char *members3[] = {"Cupcakes", "Grace", "Spice"};

// 	int chat1_id = addChat(true, 3, members, c);
// 	int chat2_id = addChat(false, 2, members2, c);
// 	int chat3_id = addChat(true, 3, members3, c);
// 	printChats(c);


// 	// isMember tests
// 	// if (!isMemberChat("Deanna", chat1_id, c)) {
// 	// 	printf("Failed isMember1 test\n");
// 	// 	return -1;

// 	// } 
// 	// if (isMemberChat("Deanna", chat2_id, c)){
// 	// 	printf("Failed isMember2 test\n");
// 	// 	return -1;
// 	// }
// 	struct chat *ch = getChat(chat2_id, c);

// 	memberAccept(chat2_id, "Caro", c);
// 	// printChats(c);
// 	memberAccept(chat2_id, "Grace", c);
// 	// printChats(c);

// 	if (getChatStatus(ch) != VALID_STATUS){
// 		printf("FAILED get chat status\n");
// 		return -1;
// 	}

// 	deleteChatsWithMember("Grace", c);
// 	printChats(c);

// 	// //remove tests
// 	// if (!removeChat(0, c)){
// 	// 	printf("Unable to remove chat 2\n");
// 	// 	return -1;
// 	// }
	

// 	// if (!removeChat(2, c)){
// 	// 	printf("Unable to remove chat 2\n");
// 	// 	return -1;
// 	// }

	
// 	// printChats(c);

// 	freeChatList(c);

// 	printf("PASSED!\n");
// 	return 0;
// }



