
server: server.c clientList.c chatList.c
	gcc -g server.c clientList.c chatList.c -o server -lnsl

chat-test: chatList.c 
	gcc -g chatList.c -o chat-test -lnsl

client-list-test: chatList.c 
	gcc -g clientList.c -o client-list-test -lnsl

client: client.c clientList.c chatList.c
	gcc -g client.c clientList.c chatList.c -o client -lnsl
