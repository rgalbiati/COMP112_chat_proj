
server: server.c clientList.c chatList.c
	gcc -g server.c clientList.c chatList.c -o server -lnsl
