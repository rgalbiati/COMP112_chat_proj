#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "chatList.h"

const static int MAX_CHATS = 100;
const static int USER_LEN = 20;
const static int GEO_LEN = 40;
const static int VALID_STATUS = 1;
const static int PENDING_STATUS = 0;
const static int ID_LEN = 40;

struct chatList *newChatList() {
	struct chatList *c = malloc(sizeof(struct chatList));
	c->numChats = 0;
	c->maxChats = MAX_CHATS;

	for (int i = 0; i < MAX_CHATS; i++) {
		c->chats[i] = malloc(sizeof(struct chat));
		memset(c->chats[i]->id, 0, ID_LEN);
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

bool existsChat(char *chat_id, struct chatList *c){
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++) {
		if (strcmp(c->chats[i]->id, chat_id) == 0) {
			return true;
		}
	}

	return false;
}

// returns id
char *addChat(int numMembers, char **members, char *id, struct chatList *c) {
	if (c->numChats == c->maxChats || numMembers > 5) {
		return "";
	}
	int chatIndex = c->numChats;
	for (int i = 0; i < numMembers; i++) {
		memcpy(c->chats[chatIndex]->members[i], members[i], strlen(members[i]) + 1);
		c->chats[chatIndex]->memberStatus[i] = PENDING_STATUS;
	}
	c->chats[chatIndex]->numMembers = numMembers;
	memset(c->chats[chatIndex]->id, 0, ID_LEN);

	if (id == NULL){
		sprintf(c->chats[chatIndex]->id, "%d", chatIndex);
	} else {
		memcpy(c->chats[chatIndex]->id, id, strlen(id) + 1);
	}
	

	c->chats[chatIndex]->chatStatus = PENDING_STATUS;

	c->numChats += 1;
	return c->chats[chatIndex]->id;
}

bool removeChat(char *chat_id, struct chatList *c) {
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++) {
		if (strcmp(c->chats[i]->id, chat_id) == 0) {
			// move last element into slot i to save space
			if (i != numChats - 1) {
				memset(c->chats[i]->id, 0, ID_LEN);
				memcpy(c->chats[i]->id, c->chats[numChats - 1]->id, ID_LEN);
				c->chats[i]->chatStatus = c->chats[numChats - 1]->chatStatus;
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

struct chat *getChat(char *chat_id, struct chatList *c) {
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++) {
		if (strcmp(c->chats[i]->id, chat_id) == 0) {
			return c->chats[i];
		}
	}
	return NULL;
}

int getChatListLen(char *client, struct chatList *c){
	int len = 0;
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++){
		if (isMemberChat(client, c->chats[i])){
			struct chat *ch = c->chats[i];
			len += strlen(ch->id) + 2;
			int numMembers = ch->numMembers;
			for (int j = 0; j < numMembers; j++){
				len += strlen(ch->members[j])+ 1;
			}
			len += 1;
		}
	}
	return len;
}

void writeChatList(int fd, char *client, struct chatList *c){
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++){
		if (isMemberChat(client, c->chats[i])){
			struct chat *ch = c->chats[i];
			write(fd, ch->id, strlen(ch->id) + 1);
			
			int numMembers = ch->numMembers;
			for (int j = 0; j < numMembers; j++){
				write(fd, ch->members[j], strlen(ch->members[j]) + 1);
			}
			write(fd, "\n", 1);
		}
	}
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
int memberAccept(char *id, char *client, struct chatList *c) {
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++){
		if (strcmp(c->chats[i]->id, id) == 0){
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
	
		printf("%d) id: %s, %d members, ", i, c->chats[i]->id, numMembers);

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

bool addUserToChat(char *client, char *chat_id, struct chatList *c){
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++){
		if (strcmp(chat_id, c->chats[i]->id) == 0){
			struct chat *ch = c->chats[i];
			if (ch->numMembers == 5){
				return false;
			} 
			int numMembers = ch->numMembers;

			memcpy(ch->members[numMembers], client, strlen(client) + 1);
			ch->memberStatus[numMembers] = PENDING_STATUS;

			ch->numMembers += 1;
			ch->chatStatus = PENDING_STATUS;
			return true;
		}
	}
	return false;
}

bool removeUserFromChat(char *client, char *chat_id, struct chatList *c){
	int numChats = c->numChats;
	for (int i = 0; i < numChats; i++){
		if (strcmp(chat_id, c->chats[i]->id) == 0){
			struct chat *ch = c->chats[i];
			int numMembers = ch->numMembers;

			for (int j = 0; j < numMembers; j++){
				// found client to remove
				if (strcmp(client, ch->members[j]) == 0){
					if (j != numMembers - 1) {
						memset(ch->members[j], 0, USER_LEN);
						memcpy(ch->members[j], ch->members[numMembers - 1], USER_LEN);

						ch->memberStatus[j] = ch->memberStatus[numMembers - 1]; 
	        		}
	        		ch->numMembers -= 1;
	        		bool chatReady = true;
	        		for (int k = 0; k < numMembers - 1; k++){
	        			chatReady = chatReady && ch->memberStatus[k];
	        		}
	        		if (chatReady){
	        			ch->chatStatus = VALID_STATUS;
	        		}
					return true;
				}		 
			}			
		}
	}
	return false;
}

char *addUserToGeoChat(char *client, char *location, struct chatList *c){
	char *test = malloc(ID_LEN);
	char *num_geo_str = malloc(7);
	int num_geo = 0;

	memset(num_geo_str, 0, 7);
	sprintf(num_geo_str, "_%d", num_geo);

	memset(test, 0, ID_LEN);
	strcpy(test, location);
	strcat(test, num_geo_str);
	struct chat *ch = getChat(test, c);

	while (ch != NULL){
	
		if (numMembersChat(ch) < 5) {
			addUserToChat(client, test, c);
			return test;
		}		

		num_geo += 1;
		memset(num_geo_str, 0, 7);
		sprintf(num_geo_str, "_%d", num_geo);

		memset(test, 0, ID_LEN);
		strcpy(test, location);
		strcat(test, num_geo_str);

		ch = getChat(test, c);
	}

	char *members[] = {client};
	addChat(1, members, test, c);
	return test;
}

void removeUserFromGeoChat(char *client, char *location, struct chatList *c){
	char *test = malloc(ID_LEN);
	char *num_geo_str = malloc(7);
	int num_geo = 0;

	memset(num_geo_str, 0, 7);
	sprintf(num_geo_str, "_%d", num_geo);

	memset(test, 0, ID_LEN);
	strcpy(test, location);
	strcat(test, num_geo_str);

	struct chat *ch = getChat(test, c);
	while (ch != NULL){
	
		if (isMemberChat(client, ch)) {
			removeUserFromChat(client, test, c);
			return;
		}		

		num_geo += 1;
		memset(num_geo_str, 0, 7);
		sprintf(num_geo_str, "_%d", num_geo);


		memset(test, 0, ID_LEN);
		strcpy(test, location);
		strcat(test, num_geo_str);

		ch = getChat(test, c);
	}
}


// int main(){
// 	printf("RUNNING CHAT TEST\n");

// 	struct chatList *c = newChatList();
// 	// printChats(c);

// 	char *members[] = {"Deanna", "Claire", "Caro"};
// 	char *members2[] = {"Caro", "Grace"};
// 	char *members3[] = {"Cupcakes", "Grace", "Spice"};

// 	// char *chat1_id = addChat(3, members, "Somerville_0", c);
// 	// char *chat2_id = addChat(2, members2, "Somerville_1", c);
// 	// // char *chat3_id = addChat(3, members3, c);
// 	// printChats(c);

// 	// memberAccept(chat1_id, "Deanna", c);
// 	// memberAccept(chat1_id, "Claire", c);
// 	// memberAccept(chat1_id, "Caro", c);
// 	// printChats(c);

// 	// addUserToChat("Sarah", chat1_id, c);
// 	// printChats(c);

// 	// addUserToChat("Emily", chat1_id, c);
// 	// printChats(c);

// 	// removeUserFromChat("Sarah", chat1_id, c);
// 	// printChats(c);

// 	// removeUserFromChat("Emily", chat1_id, c);
// 	// printChats(c);

// 	addUserToGeoChat("Deanna", "Somerville", c);
// 	printChats(c);
// 	addUserToGeoChat("Emily", "Somerville", c);
// 	addUserToGeoChat("Claire", "Somerville", c);
// 	addUserToGeoChat("Caroline", "Somerville", c);
// 	addUserToGeoChat("Sarah", "Somerville", c);


// 	char *geochat_id = addUserToGeoChat("Joseph", "Somerville", c);
// 	addUserToGeoChat("Paul", "Russia", c);
// 	addUserToGeoChat("Dmitri", "Russia", c);
// 	addUserToGeoChat("Sammy", "Somerville", c);
// 	printChats(c);

// 	removeUserFromGeoChat("Emily", "Somerville", c);
// 	printChats(c);

// 	addUserToGeoChat("Sophie", "Somerville", c);
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
// 	// struct chat *ch = getChat(chat2_id, c);

// 	// memberAccept(chat2_id, "Caro", c);
// 	// // printChats(c);
// 	// memberAccept(chat2_id, "Grace", c);
// 	// // printChats(c);

// 	// if (getChatStatus(ch) != VALID_STATUS){
// 	// 	printf("FAILED get chat status\n");
// 	// 	return -1;
// 	// }

// 	// deleteChatsWithMember("Grace", c);
// 	// printChats(c);

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



