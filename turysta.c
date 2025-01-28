#include <stdio.h>
#include "myfun.h"


volatile bool stopThreads = false;
volatile bool in_prom = false;      
volatile bool in_most = false;      
volatile bool in_wieza = false;     

CheckoutData* checkoutdata=NULL;
TourData* tourdata=NULL;
int children_count;
pthread_t child_threads[X1];

// Thread function that does nothing until signaled to exit
void *idleThread(void *arg) {
    int *child_id = (int *)arg;

    printf("Dziecko %d zaczyna trase z rodzicem.\n", *child_id);

    while (!stopThreads) {
        sleep(1); // Simulate idling (can be adjusted as needed)
    }

    pthread_exit(NULL);
}

void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        // if(in_wieza)
        // {
        //     printf(ANSI_COLOR_WHITE "Turysta schodzi z wiezy\n" ANSI_COLOR_RESET);
        //     sem_post(&tourdata->wiezaSpots);
        // }
        

    } else if (sig == SIGUSR2) {
        printf(ANSI_COLOR_WHITE "Turysta kieruje sie do kas\n" ANSI_COLOR_RESET);
        if (!in_prom && !in_most && !in_wieza) 
        {
            enterqueue(checkoutdata, 2, children_count);
            // Signal threads to stop
            stopThreads = true;

            // Wait for child threads to terminate
            for (int i = 0; i < children_count; i++) {
                pthread_join(child_threads[i], NULL);
            }


            checkoutCleanup(checkoutdata);
            tourCleanup(tourdata);

        } else {
            printf("Turysta w trakcie zwiedzania ignoruje sygnal.\n");
        }

        // Add your custom logic for SIGUSR2 here if needed
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("ERROR: BAD ARGUMENTS - Usage: %s <VIP_FLAG> <CHILD_AGE_1> ... <CHILD_AGE_N>\n", argv[0]);
        exit(1);
    }

    // Parse the VIP flag
    int vip_flag = atoi(argv[1]);
    if (vip_flag != 0 && vip_flag != 1) {
        printf("ERROR: VIP = 0 (non-VIP) or 1 (VIP).\n");
        exit(1);
    }

    //signal(SIGUSR1, signal_handler);  // Handle SIGUSR1
    signal(SIGUSR2, signal_handler);  // Handle SIGUSR2

    // Parse children ages
    children_count = argc - 2;
    if (children_count > X1-1) {
        printf("ERROR: Liczba dzieci zby wysoka (%d).\n", X1-1);
        exit(1);
    }
    int below_five=0;
    int children_ages[X1] = {0};  // Array to hold children ages
    for (int i = 0; i < children_count; i++) {
        children_ages[i] = atoi(argv[i + 2]);
        if (children_ages[i] <= 0 || children_ages[i] > 15) {
            printf("ERROR: zly wiek dla dziecka %d: %s.\n", i + 1, argv[i + 2]);
            exit(1);
        }
        if(children_ages[i]<=5)
        {
            below_five++;
        }
    }


    

    int mypid=getpid();
    printf("Tourist PID: %d, VIP: %d, Dzieci: %d\n", mypid, vip_flag, children_count);

    
    

    checkoutdata=checkoutSetupShm();

    enterqueue(checkoutdata, 1, children_count);


    int child_ids[X1];            // Array for child IDs

    // Create threads for children
    for (int i = 0; i < children_count; i++) {
        child_ids[i] = i + 1;
        if (pthread_create(&child_threads[i], NULL, idleThread, &child_ids[i]) != 0) {
            perror("Nie udalo sie utworzyc watku dziecka");
            return 1;
        }
    }
    

    struct message msg;  // Local message struct

    // Receive message from the queue
    if (msgrcv(checkoutdata->msqid, &msg, sizeof(pid_t)+sizeof(int), mypid, 0) == -1) 
    {
        perror("msgrcv");
        exit(1);
    }

    int mygroup = msg.value;

    // If the park is closing and there was no group
    if(mygroup == -1)
    {
        // Signal threads to stop
        stopThreads = true;

        // Wait for child threads to terminate
        for (int i = 0; i < children_count; i++) {
            pthread_join(child_threads[i], NULL);
        }


        checkoutCleanup(checkoutdata);
        

        //printf("Koniec turysta: %d\n", mypid);

        return 0;
    };



    pid_t myprzew=checkoutdata->groups[mygroup][0];
    
    printf("    TURYSTA PID: %d   grupa: %d   przewodnik: %d\n", mypid, mygroup, myprzew);

    tourdata=tourSetupShm();
    

    while(checkoutdata->group_active[mygroup]!=1);

    if (msgrcv(checkoutdata->msqid, &msg, sizeof(pid_t)+sizeof(int), mypid, 0) == -1) 
    {
        perror("msgrcv");
        exit(1);
    }
    int trasa=msg.value;
if(trasa==1)
{
    in_most=true;
    most(tourdata, mypid, trasa, mygroup, children_count);
    in_most=false;

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

    if(below_five==0)
    {
        in_wieza=true;
        wieza(tourdata, mypid, mygroup, children_count);
        in_wieza=false;
    }
    

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

    in_prom=true;
    prom(tourdata, mypid, mygroup, children_count, trasa);
    in_prom=false;

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

    enterqueue(checkoutdata, 2, children_count);
}
else if(trasa==2)
{
    in_prom=true;
    prom(tourdata, mypid, mygroup, children_count, trasa);
    in_prom=false;

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
    
    if(below_five>0)
    {
        in_wieza=true;
        wieza(tourdata, mypid, mygroup, children_count);
        in_wieza=false;
    }
    

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

    in_most=true;
    most(tourdata, mypid, trasa, mygroup, children_count);
    in_most=false;

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

    enterqueue(checkoutdata, 2, children_count);
}

    // Signal threads to stop
    stopThreads = true;

    // Wait for child threads to terminate
    for (int i = 0; i < children_count; i++) {
        pthread_join(child_threads[i], NULL);
    }


    checkoutCleanup(checkoutdata);
    tourCleanup(tourdata);
    

    //printf("Koniec turysta: %d\n", mypid);

    return 0;
}