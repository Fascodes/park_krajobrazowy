#include <stdio.h>
#include "myfun.h"


int main(int argc, char *argv[])
{
    if(argc!=2)
    {
        printf("ERROR BAD ARGUMENTS FOR - turysta");
        exit(1);
    }
    int age=atoi(argv[1]);
    int mypid=getpid();
    printf("TURYSTA PID: %d wiek: %d\n", mypid, age);

    CheckoutData* checkoutdata=checkoutSetupShm();

    enterqueue(checkoutdata, 1, mypid);
    

    struct message msg;  // Local message struct

    // Receive message from the queue
    if (msgrcv(checkoutdata->msqid, &msg, sizeof(pid_t)+sizeof(int), getpid(), 0) == -1) 
    {
        perror("msgrcv");
        exit(1);
    }

    int mygroup = msg.value;
    pid_t myprzew=checkoutdata->groups[mygroup][0];
    
    printf("    TURYSTA PID: %d   grupa: %d   przewodnik: %d\n", mypid, mygroup, myprzew);
    msg.mtype=myprzew;
    msg.pid=mypid;
    msg.value=1;

    if (msgsnd(checkoutdata->msqid, &msg, sizeof(pid_t)+sizeof(int), 0) == -1) 
    {
        perror("msgsnd - turysta => pzewodnik mygroup");
        exit(1);
    }

    TourData* tourdata=tourSetupShm();

    printf("Koniec turysta: %d\n", mypid);

    return 0;
}