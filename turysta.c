#include <stdio.h>
#include "myfun.h"


int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("ERROR: BAD ARGUMENTS - Usage: %s <VIP_FLAG> <CHILD_AGE_1> ... <CHILD_AGE_N>\n", argv[0]);
        exit(1);
    }

    // Parse the VIP flag
    int vip_flag = atoi(argv[1]);
    if (vip_flag != 0 && vip_flag != 1) {
        printf("ERROR: VIP flag must be 0 (non-VIP) or 1 (VIP).\n");
        exit(1);
    }

    // Parse children ages
    int children_count = argc - 2;
    if (children_count > X1-1) {
        printf("ERROR: Number of children exceeds the maximum allowed (%d).\n", M);
        exit(1);
    }
    int below_five=0;
    int children_ages[X1] = {0};  // Array to hold children ages
    for (int i = 0; i < children_count; i++) {
        children_ages[i] = atoi(argv[i + 2]);
        if (children_ages[i] <= 0) {
            printf("ERROR: Invalid age for child %d: %s. Must be a positive integer.\n", i + 1, argv[i + 2]);
            exit(1);
        }
        if(children_ages[i]<=5)
        {
            below_five++;
        }
    }
    

    int mypid=getpid();
    printf("Tourist PID: %d, VIP: %d, Number of children: %d\n", mypid, vip_flag, children_count);
    for (int i = 0; i < children_count; i++) {
        printf("\tChild %d age: %d\n", i + 1, children_ages[i]);
    }
    printf("\tBELOWFIVE:%d\n", below_five);
    

    CheckoutData* checkoutdata=checkoutSetupShm();

    enterqueue(checkoutdata, 1, children_count);
    

    struct message msg;  // Local message struct
    msg.value=0;

    // Receive message from the queue
    if (msgrcv(checkoutdata->msqid, &msg, sizeof(pid_t)+sizeof(int), mypid, 0) == -1) 
    {
        perror("msgrcv");
        exit(1);
    }

    int mygroup = msg.value;
    pid_t myprzew=checkoutdata->groups[mygroup][0];
    
    printf("    TURYSTA PID: %d   grupa: %d   przewodnik: %d\n", mypid, mygroup, myprzew);

    TourData* tourdata=tourSetupShm();
    

    while(checkoutdata->group_active[mygroup]!=1);

    if (msgrcv(checkoutdata->msqid, &msg, sizeof(pid_t)+sizeof(int), mypid, 0) == -1) 
    {
        perror("msgrcv");
        exit(1);
    }
    int trasa=msg.value;
if(trasa==1)
{

    most(tourdata, mypid, trasa, mygroup, children_count);

    msg.mtype=checkoutdata->groups[mygroup][0];
    if (msgsnd(checkoutdata->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
        perror("msgsnd - przewodnik notifying group arrival");
        exit(1);
    }

    if (msgrcv(checkoutdata->msqid, &msg, sizeof(pid_t)+sizeof(int), mypid, 0) == -1) 
    {
        perror("msgrcv");
        exit(1);
    }


    wieza(tourdata, mypid, mygroup, children_count);

    msg.mtype=checkoutdata->groups[mygroup][0];
    if (msgsnd(checkoutdata->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
        perror("msgsnd - przewodnik notifying group arrival");
        exit(1);
    }

    if (msgrcv(checkoutdata->msqid, &msg, sizeof(pid_t)+sizeof(int), mypid, 0) == -1) 
    {
        perror("msgrcv");
        exit(1);
    }

    prom(tourdata, mypid, mygroup, children_count, trasa);

    msg.mtype=checkoutdata->groups[mygroup][0];
    if (msgsnd(checkoutdata->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
        perror("msgsnd - przewodnik notifying group arrival");
        exit(1);
    }

    enterqueue(checkoutdata, 2, children_count);
}
else if(trasa==2)
{
    prom(tourdata, mypid, mygroup, children_count, trasa);
    

    msg.mtype=checkoutdata->groups[mygroup][0];
    if (msgsnd(checkoutdata->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
        perror("msgsnd - przewodnik notifying group arrival");
        exit(1);
    }

    if (msgrcv(checkoutdata->msqid, &msg, sizeof(pid_t)+sizeof(int), mypid, 0) == -1) 
    {
        perror("msgrcv");
        exit(1);
    }

    wieza(tourdata, mypid, mygroup, children_count);

    msg.mtype=checkoutdata->groups[mygroup][0];
    if (msgsnd(checkoutdata->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
        perror("msgsnd - przewodnik notifying group arrival");
        exit(1);
    }

    if (msgrcv(checkoutdata->msqid, &msg, sizeof(pid_t)+sizeof(int), mypid, 0) == -1) 
    {
        perror("msgrcv");
        exit(1);
    }

    most(tourdata, mypid, trasa, mygroup, children_count);

    msg.mtype=checkoutdata->groups[mygroup][0];
    if (msgsnd(checkoutdata->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
        perror("msgsnd - przewodnik notifying group arrival");
        exit(1);
    }

    enterqueue(checkoutdata, 2, children_count);
}
    

    printf("Koniec turysta: %d\n", mypid);

    return 0;
}