server: encryption.c server.c clientList.c chatList.c 
	gcc -g encryption.c server.c clientList.c chatList.c -o server -lnsl -lssl -lcrypto

chat-test: chatList.c 
	gcc -g chatList.c -o chat-test -lnsl

client-list-test: chatList.c 
	gcc -g clientList.c -o client-list-test -lnsl

client: encryption.c client.c clientList.c chatList.c
	gcc -g encryption.c client.c clientList.c chatList.c -o client -lnsl -lssl -lcrypto
