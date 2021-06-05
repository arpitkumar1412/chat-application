#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#define MAX 200
#define SERVER 1L
#define TRUE 1
#define FALSE !TRUE

struct payload {
    char    buffer[MAX];
    char    header[20];
    long    pid;
    long    client_id;
    long    gid;
    time_t  msg_time;
};

typedef struct {
    long mtype;
    struct payload payload;
}MESSAGE;


int mid_client, mid_server;
struct msqid_ds buf;
key_t key1,key2;
MESSAGE msg;
int client_id = -1;

int main(){
    //Creating a message queue
    key1 = ftok("server.c", 'z');
    if((mid_server = msgget(key1, IPC_CREAT | 0660))<0){             //server receives here
        printf("Error Creating Message Queue\n");
        exit(-1);
    }

    //Creating a message queue
    key2 = ftok("client.c", 'z');
    if((mid_client = msgget(key2, IPC_CREAT | 0660))<0){             //new client receives here
        printf("Error Creating Message Queue\n");
        exit(-1);
    }
    printf("server q and client q %d %d\n", mid_server, mid_client);

    msg.mtype = 1;   //set the msg type
    FILE *groups;
    printf("type :new / id: ");     //get the id allotted if registered previously or type :new for a new user
    fgets(msg.payload.header, MAX, stdin);

    if(strcmp(msg.payload.header, ":new\n")==0){    //send the new msg to the server on server queue
        strcpy(msg.payload.header, ":new");
        msg.payload.client_id = (long)getpid();
        if(msgsnd(mid_server, &msg, sizeof(msg.payload), 0)==-1){
            perror("msgsnd");
	    exit(-1);
	}
        if(msgrcv(mid_client, &msg, sizeof(msg.payload), SERVER, 0)<0){
	    perror("msgrcv");
	    exit(-1);
        }
        printf("%s\n", msg.payload.header);     //get the new client id on common client q.
        client_id = msg.payload.client_id;
    }else{
        client_id = atoi(msg.payload.header);
    }
    printf("client id = %d\n", client_id);
    char input[20];
    sprintf(input, "%d", client_id);
    strcat(input, ".txt");
    groups = fopen(input, "a+");    //open a file where clients groups will be stored

    //Creating a message queue
    key1 = ftok(input, 'z');
    if((mid_client = msgget(key1, IPC_CREAT | 0660))<0){             //client private message q, it receives here
        printf("Error Creating Message Queue\n");
        exit(-1);
    }
    printf("private msg q = %d\n", mid_client);

    //--------------taking action input

    while(1){
    printf("1:list groups\n2:new group\n3:join group\n4:send message to group\n5: send msg to personal\n6: read msg\n-1: exit\nenter input: ");
    int act;
    scanf("%d", &act);

    switch(act){
        case -1:
            exit(1);

        case 1:
            strcpy(msg.payload.header, ":list");
            msg.payload.client_id = mid_client;   //adds the client msg q id here
            if(msgsnd(mid_server, (struct MESSAGE*)&msg, sizeof(msg.payload), 0)==-1){
                perror("msgsnd");
    	        exit(-1);
            }
            printf("message sent\n");
            if(msgrcv(mid_client, &msg, sizeof(msg.payload), SERVER, 0)<0){
	        perror("msgrcv");
	        exit(-1);
            }
            printf("groups available till - %ld\n", msg.payload.gid-1);
            break;

        case 2:
            strcpy(msg.payload.header, ":new group");            //add the newly alloted group to the file for future use
            msg.payload.client_id = mid_client;
            if(msgsnd(mid_server, (struct MESSAGE*)&msg, sizeof(msg.payload), 0)==-1){
                perror("msgsnd");
    	        exit(-1);
            }
            printf("message sent\n");
            if(msgrcv(mid_client, &msg, sizeof(msg.payload), SERVER, 0)<0){
	        perror("msgrcv");
	        exit(-1);
            }
            printf("%s\n", msg.payload.header);
            printf("group alloted = %ld\n", msg.payload.gid);
            fprintf(groups, "%ld\n", msg.payload.gid);
            break;

        case 3:
            strcpy(msg.payload.header, ":join group");
            msg.payload.client_id = mid_client;
            printf("enter the group id: ");
            scanf("%ld", &msg.payload.gid);
            if(msgsnd(mid_server, (struct MESSAGE*)&msg, sizeof(msg.payload), 0)==-1){
                perror("msgsnd");
    	        exit(-1);
            }
            if(msgrcv(mid_client, &msg, sizeof(msg.payload), SERVER, 0)<0){
	        perror("msgrcv");
	        exit(-1);
            }
            printf("%s\n", msg.payload.header);
            fprintf(groups, "%ld\n", msg.payload.gid);    //add the group to the file
            break;

        case 4:
            strcpy(msg.payload.header, ":group msg");
            printf("enter the group id: ");
            scanf("%ld", &msg.payload.gid);
            msg.payload.client_id = mid_client;
            printf("type :over for exiting out of loop\n");
            while(1){
                printf("enter the msg: ");
                scanf("%s", msg.payload.buffer);
                if(strcmp(msg.payload.buffer, ":over")==0)
                    break;
                msg.payload.msg_time = time(NULL);   //add the msg with time
                if(msgsnd(mid_server, (struct MESSAGE*)&msg, sizeof(msg.payload), 0)==-1){
                    perror("msgsnd");
    	            exit(-1);
                }
            }
            break;

        case 5:
            strcpy(msg.payload.header, ":personal msg");
            printf("enter the personal id: ");   //this should be the personal msg queue, not the personal id
            scanf("%ld", &msg.payload.pid);
            msg.payload.client_id = mid_client;
            printf("type :over for exiting out of loop\n");
            while(1){
                printf("enter the msg: ");
                scanf("%s", msg.payload.buffer);
                if(strcmp(msg.payload.buffer, ":over")==0)
                    break;
                msg.payload.msg_time = time(NULL);
                if(msgsnd(msg.payload.pid, (struct MESSAGE*)&msg, sizeof(msg.payload), 0)==-1){
                    perror("msgsnd");
    	            exit(-1);
                }
            }
            break;

        case 6:
            msgctl(mid_client, IPC_STAT, &buf);    //reading the msg
            int num_messages = buf.msg_qnum;
            while(num_messages--){
                if(msgrcv(mid_client, &msg, sizeof(msg.payload), SERVER, 0)<0){
    	            perror("msgrcv");
    	            exit(-1);
                }
                if(time(NULL)-msg.payload.msg_time >= 7){    //if greater than 30s, msg is redacted
                    printf("message redacted\n");
                }
                else if(strcmp(msg.payload.header,":group msg")==0){
                    printf("group - %ld, name - %ld\n%s\n", msg.payload.gid, msg.payload.client_id, msg.payload.buffer);
                }
                else if(strcmp(msg.payload.header,":personal msg")==0){
                    printf("name - %ld\n%s\n", msg.payload.client_id, msg.payload.buffer);
                }

            }
    }
    }
    fclose(groups);
    return (EXIT_SUCCESS);
}
