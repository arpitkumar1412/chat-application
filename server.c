#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
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


int groups[MAX][MAX];       //2-d array of groups and their members
int count[MAX];            //keeps count of member of each group
int group_count = 0;       //count of current groups alloted, group_count-1 = group id to be alloted
struct msqid_ds buf;
MESSAGE msg;
int mid_server, mid_client;    //the initial server and client msg q
key_t key1, key2;
int client_id = 0;    //this allots the client id

void sighandler(int sig){
    if (msgctl (mid_server, IPC_RMID, NULL) == -1) {
        perror ("client: msgctl");
        exit (1);
    }
    FILE *datab;
    datab = fopen("server.txt", "w+");
    fprintf(datab, "%d\n", client_id);
    fprintf(datab, "%d\n", group_count);
    for(int i = 0; i<group_count; ++i){
        fprintf(datab, "%d ", count[i]);
    }
    fprintf(datab, "\n");
    for(int i = 0; i<group_count; ++i){
        for(int j = 0; j<count[i]; ++j){
            fprintf(datab, "%d ", groups[i][j]);
        }
        fprintf(datab, "\n");
    }
    fclose(datab);
    exit(1);
}

int main(){
    printf("press Ctrl-C to close the server\n");
    signal(SIGINT, sighandler);
    //Creating a message queue
    FILE *datab;    //database of groups and members
    datab = fopen("server.txt", "w+");
    if(fscanf(datab, "%d", &client_id)!=0){    //fills the groups array whenever it starts
        fscanf(datab, "%d", &group_count);
        for(int i = 0; i<group_count; ++i){
            fscanf(datab, "%d", &count[i]);
        }
        for(int i = 0; i<group_count; ++i){
            for(int j = 0; j<count[i]; ++j){
                fscanf(datab, "%d", &groups[i][j]);
            }
        }
    }
    fclose(datab);
    key1 = ftok("server.c", 'z');
    msg.mtype = 1;
    if((mid_server = msgget(key1, IPC_CREAT | 0660))<0){             //server receives here for all requests to server
        printf("Error Creating Message Queue\n");
        exit(-1);
    }

    //Creating a message queue
    key2 = ftok("client.c", 'z');
    if((mid_client = msgget(key2, IPC_CREAT | 0660))<0){             //new client receives here/ server sends here
        printf("Error Creating Message Queue\n");
        exit(-1);
    }
    printf("server q and client q %d %d\n", mid_server, mid_client);

    while(1){
    //Receiving message from client, throws and error if input is invalid
        if(msgrcv(mid_server, &msg, sizeof(msg.payload), SERVER, 0)<0){
	    perror("msgrcv");
	    exit(-1);
        }
    //Server displays received message
        printf("Server receives: %s\n", msg.payload.header);

        if(strcmp(msg.payload.header, ":new")==0){      //register a new user, client_id here is process id, send it on the common client q
            printf("id alloted to %ld\n", msg.payload.client_id);
            strcpy(msg.payload.header, ":done");
            msg.payload.client_id = client_id++;
            if(msgsnd(mid_client, (struct MESSAGE*)&msg, sizeof(msg.payload), 0)==-1){
	        perror("msgsnd");
	    	exit(-1);
            }
        }


        else if(strcmp(msg.payload.header, ":new group")==0){           //msg buffer contains the client msg q, form a new group
            mid_client = msg.payload.client_id;
            printf("creating new group for %d\n", mid_client);
            groups[group_count][0] = mid_client;                //creates a new group and add the client to it
            ++count[group_count++];                             //increasing the count of group_counts and count of alloted group

            strcpy(msg.payload.header, ":done, group id in msg");
            msg.payload.gid = group_count-1;
            if(msgsnd(mid_client, (struct MESSAGE*)&msg, sizeof(msg.payload), 0)==-1){
	        perror("msgsnd");
	    	exit(-1);
            }
            printf("group id - %d\n", group_count-1);
        }


        else if(strcmp(msg.payload.header, ":join group")==0){         //msg buffer contains the client msg q and gid contains the group id
            int group_id = msg.payload.gid;
            mid_client = msg.payload.client_id;
            if(group_id>=group_count){
                strcpy(msg.payload.header, "illegal request");
                strcpy(msg.payload.buffer, "group not present");
                if(msgsnd(mid_client, (struct MESSAGE*)&msg, sizeof(msg.payload), 0)==-1){
	            perror("msgsnd");
	    	    exit(-1);
                }
            }else{
                groups[group_id][count[group_id]++] = mid_client;           //adding the client to requested group and incresing the count of the  group
                strcpy(msg.payload.header, ":group joined");
                if(msgsnd(mid_client, (struct MESSAGE*)&msg, sizeof(msg.payload), 0)==-1){
	            perror("msgsnd");
	    	    exit(-1);
                }
                printf("group - %ld joined by %ld\n", msg.payload.gid, msg.payload.client_id);
            }
        }
        else if(strcmp(msg.payload.header, ":list")==0){                  //group_count contains the number till which group is alloted
            printf("request for list of groups from %ld\n", msg.payload.client_id);
            msg.payload.gid = group_count;
            mid_client = msg.payload.client_id;
            if(msgsnd(mid_client, (struct MESSAGE*)&msg, sizeof(msg.payload), 0)==-1){
	        perror("msgsnd");
	        exit(-1);
            }
        }
        else if(strcmp(msg.payload.header, ":group msg")==0){
            for(int i = 0; i<count[msg.payload.gid]; ++i){     //loop for going through each member of the group
                mid_client = groups[msg.payload.gid][i];       //get the msg q id
                strcpy(msg.payload.header, ":group msg");
                if(msgsnd(mid_client, (struct MESSAGE*)&msg, sizeof(msg.payload), 0)==-1){
	            perror("msgsnd");
	            exit(-1);
                }
            }
        }
    }
    if (msgctl (mid_server, IPC_RMID, NULL) == -1) {
        perror ("client: msgctl");
        exit (1);
    }
    return (EXIT_SUCCESS);
}
