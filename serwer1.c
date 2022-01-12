#include <sys/socket.h>
#include <arpa/inet.h> 
#include <unistd.h>    
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <ctype.h>
#define PORT 4455
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100


static _Atomic unsigned int client_count = 0;
static int cid = 10;

typedef struct 
{
    struct sockaddr_in address;
    int sockfd;
    char name[64];
    int cid;
    int roomId;
    int owner;
    int exit;
}clientData;

typedef struct
{
    clientData* chatRoomClients;
    int id;
    int numberOfClients;

}chatRoom;

chatRoom* chatRooms;
int sizeChatRooms = -1;
clientData *clients;
int sizeClients;
char avaiableRooms[1000]; 

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

clientData* removeFromRoom2(int uid, int roomID){
    pthread_mutex_lock(&clients_mutex);
    int numberOfClients = chatRooms[roomID].numberOfClients;
    clientData* newChatRoomClients =(clientData*)malloc((numberOfClients-1)*sizeof(clientData));
    int k = 0;
	for(int i=0; i < numberOfClients; i++){
			if(chatRooms[roomID].chatRoomClients[i].cid == uid){
                continue;
			}
            newChatRoomClients[k] = chatRooms[roomID].chatRoomClients[i];
            k++;
		
	}
    chatRooms[roomID].numberOfClients--;
	pthread_mutex_unlock(&clients_mutex);
    return newChatRoomClients;
}


void send_message(char *s, int roomID){
    pthread_mutex_lock(&clients_mutex);
    int numberOfClients = chatRooms[roomID].numberOfClients;
	for(int i=0; i<numberOfClients; ++i){
		if(&chatRooms[roomID].chatRoomClients[i]){
            clientData client = chatRooms[roomID].chatRoomClients[i];
            int sockFD = client.sockfd;
				if(write(sockFD, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

int kickClientByName(char* name, int roomID)
{
    int numberOfClients = chatRooms[roomID].numberOfClients;
	for(int i=0; i<numberOfClients; ++i){
		if(strcmp(chatRooms[roomID].chatRoomClients[i].name, name) == 0){
            chatRooms[roomID].chatRoomClients[i].exit = 1;
            char* kick_messg = "kicked";
            printf("Imie: %s, exit: %d\n",  chatRooms[roomID].chatRoomClients[i].name, chatRooms[roomID].chatRoomClients[i].exit);
            if(write(chatRooms[roomID].chatRoomClients[i].sockfd,kick_messg, strlen(kick_messg)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
			}
            return 1;
        }
	}
    return 0;
}



void * handleClient(void *client_arg)
{
    char buffer[BUFFER_SIZE];
    char buffer_out[BUFFER_SIZE];
    char name[64];
    int leave_flag = 0;
    char roomIDStr[3];
    client_count++;
    clientData* client = (clientData*) client_arg;
    memset(buffer, '\0', sizeof(buffer));
    memset(buffer_out, '\0', sizeof(buffer_out));
    
    recv(client->sockfd, client->name,64,0);
    recv(client->sockfd,buffer,1024,0);

    pthread_mutex_lock(&clients_mutex);
    if (strcmp(buffer,"new")==0)
    {   
        sizeChatRooms ++;
        
        strcpy(roomIDStr, " ");
        strcpy(avaiableRooms, roomIDStr);
        chatRooms  = realloc(chatRooms,(sizeChatRooms+1)*sizeof(chatRoom));
        chatRooms[sizeChatRooms].id = sizeChatRooms;
        chatRooms[sizeChatRooms].numberOfClients = 1;
        int clientNumber = 0;
        chatRooms[sizeChatRooms].chatRoomClients = calloc(chatRooms[sizeChatRooms].numberOfClients,sizeof(clientData)*chatRooms[sizeChatRooms].numberOfClients);
        chatRooms[sizeChatRooms].chatRoomClients[clientNumber] = *client;
        client->roomId = sizeChatRooms;
        client->owner =1;
    }
    else if (atoi(buffer)<= sizeChatRooms)
    {
        int roomID = atoi(buffer);
        client->roomId = roomID;
        chatRooms[roomID].numberOfClients++;
        int clientNumber  = chatRooms[roomID].numberOfClients;
        chatRooms[roomID].chatRoomClients = realloc(chatRooms[roomID].chatRoomClients,clientNumber*sizeof(clientData));
        chatRooms[roomID].chatRoomClients[clientNumber-1] = *client;
        client->owner = 0;
    }
    else{
        close(client->sockfd);
        chatRooms[client->roomId].chatRoomClients =  removeFromRoom2(client->cid, client->roomId);
        client = NULL;
        free(client);
        pthread_detach(pthread_self());
        pthread_exit(NULL); 
        return NULL;
    }
    pthread_mutex_unlock(&clients_mutex);
    bzero(buffer, BUFFER_SIZE);

    sprintf(roomIDStr, "%d", client->roomId);
    strcat(buffer, client->name);
    
    
    strcat(buffer , " has joined room: ");
    strcat(buffer, roomIDStr);
    send_message(buffer, client->roomId);
    bzero(buffer, BUFFER_SIZE);
    client->exit = 0;

    while (1)
    {
        if(client->exit)
        {
            break;
        }
        int recive = recv(client->sockfd, buffer, 1024, 0);
        if (recive == 0)
        {
            perror("ERROR: Client diconected");
			break;
        }
        // printf("%s, %d\n", client->name, client->exit);
        if (strcmp(buffer,"exit")==0)
        {
            bzero(buffer, BUFFER_SIZE);
            strcat(buffer, client->name);
            strcat(buffer, " has left");
            send_message(buffer,client->roomId);
            bzero(buffer,BUFFER_SIZE);
            client->exit = 1;
        }
        else if (strcmp(buffer, "kick") == 0 && client->owner == 1)
        {
            bzero(buffer, BUFFER_SIZE);
            char* kick_messg = "Who you want to kick: ";
            if(write(client->sockfd,kick_messg, strlen(kick_messg)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
			}
            recv(client->sockfd, buffer, 1024,0);
            int kicked = kickClientByName(buffer, client->roomId);
            strcat(buffer_out,buffer);
            bzero(buffer, BUFFER_SIZE);
            sleep(1);
            if (kicked)
            {
                strcat(buffer_out, " has been kicked!");
            }
            else
            {
                strcat(buffer_out, " - no player has this name!");
            }
            send_message(buffer_out, client->roomId);
            bzero(buffer_out, BUFFER_SIZE);
        }
        else{
            strcat(buffer_out, client->name);
            strcat(buffer_out, " >> ");
            strcat(buffer_out, buffer);
            printf("%s %s\n", client->name, buffer_out);
            send_message(buffer_out,client->roomId);
            bzero(buffer_out, BUFFER_SIZE);
            bzero(buffer, BUFFER_SIZE);
        }
        if(client->exit)
        {
            break;
        }
    }

    close(client->sockfd);
    chatRooms[client->roomId].chatRoomClients =  removeFromRoom2(client->cid, client->roomId);
    client = NULL;
    free(client);
    pthread_detach(pthread_self());
    pthread_exit(NULL); 
    return NULL;

}

int main(){

    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    pthread_t tid;
    int id = 0;
    chatRooms = (chatRoom *)malloc(0*sizeof(chatRoom));

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 5); 
    printf("LISTENING Port Number: %d\n", PORT);
    
    while (1){
        int create_result = 0;
        clientData* client = (clientData *)malloc(sizeof(clientData));
        client->sockfd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);

        client->address = client_addr;
        client->cid = cid++;
        printf("NEW CONNECTION\n");
        if(client->sockfd<0)
        {
            fprintf(stderr, "ERROR CREATING CONNECTIOIN");
        }
        int id = client->sockfd;
        create_result = pthread_create(&tid, NULL, &handleClient, (void *)client);
        if (create_result)
        {
            printf("Error handling a client\n");
            exit(-1);
        }

    }
    printf("Connection closed\n");
    free(chatRooms);
    return 0;
}