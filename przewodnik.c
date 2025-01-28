#include <stdio.h>
#include "myfun.h"

CheckoutData* checkoutdata=NULL;
TourData* tourdata=NULL;
volatile bool in_prom = false;      
volatile bool in_most = false;      
volatile bool in_wieza = false;  
int nr;


void signal_handler(int sig) {
    if (sig == SIGUSR1) {
        // if(in_wieza)
        // {
        //     printf(ANSI_COLOR_WHITE "Turysta schodzi z wiezy\n" ANSI_COLOR_RESET);
        //     sem_post(&tourdata->wiezaSpots);
        // }
        

    } else if (sig == SIGUSR2) {
        printf(ANSI_COLOR_WHITE "Przewodnik wysyla sygnaly do grupy\n" ANSI_COLOR_RESET);
        if (!in_prom && !in_most && !in_wieza) 
        {
            for(int i=1; i<checkoutdata->group_counts[nr];i++)
            {
                kill((pid_t) checkoutdata->groups[nr][i], SIGUSR2);
            }
            checkoutdata->groups[nr][0]=getpid();
            checkoutdata->group_counts[nr]=1;
            checkoutdata->group_children[nr]=0;

        } else {
            printf(ANSI_COLOR_WHITE "Grupa jest w trakcie zwiedzania obiektu, sygnal nie otrzymany.\n" ANSI_COLOR_RESET);
        }

        // Add your custom logic for SIGUSR2 here if needed
    }
}



int main(int argc, char* argv[])
{
    if(argc!=2)
    {
        printf("ERROR BAD ARGUMENTS - przewodnik");
        exit(1);
    }
    nr=atoi(argv[1]);
    printf("PRZEWODNIK PID: %d grupa: %d\n", getpid(), nr);
    
    struct message msg;

    signal(SIGUSR2, signal_handler);  // Handle SIGUSR2
    checkoutdata=checkoutSetupShm();
    tourdata=tourSetupShm();


    while(!(checkoutdata->parkClosed))
    {
        sem_wait(&checkoutdata->mutex);
        checkoutdata->group_active[nr]=0;
        checkoutdata->groups[nr][0]=getpid();
        checkoutdata->group_counts[nr]=1;
        checkoutdata->group_children[nr]=0;
        sem_post(&checkoutdata->mutex);
        while(!(checkoutdata->group_counts[nr]>1))
        {
            przewodnikWaiting(checkoutdata, nr, (time(NULL)+15), tourdata);
        };
        sem_wait(&checkoutdata->mutex);
        checkoutdata->group_active[nr]=1;
        sem_post(&checkoutdata->mutex);
        printf(ANSI_COLOR_WHITE "\t\tPRZEWODNIK %d ZACZYNA TRASE z %d osobami %d dziecmi\n" ANSI_COLOR_RESET,getpid(),checkoutdata->group_counts[nr]-1, checkoutdata->group_children[nr]);
        

        sleep((int)((rand() % 5 + 1) * (checkoutdata->group_children[nr] > 0 ? 1.5 : 1))); 

        // Notify the group that they have arrived
        srand(time(NULL) ^ (getpid()));
        int trasa=((rand()%20+1)%2)+1;

        for(int i=0;i<checkoutdata->group_counts[nr]-1;i++)
        {
            msg.value=trasa;
            msg.mtype = checkoutdata->groups[nr][i+1];
            if (msgsnd(checkoutdata->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
            perror("msgsnd - przewodnik notifying group arrival");
            exit(1);
        }
        }

        if(trasa==1)
        {
            in_most=true;
            most(tourdata, getpid(), trasa, nr, 0);
            in_most=false;

            waitingForGroup(checkoutdata, nr);
            
            //wieza
            in_wieza=false;
            waitingForGroup(checkoutdata, nr);

            in_prom=true;
            prom(tourdata, getpid(), nr, 0, trasa);
            in_prom=false;

            waitingForGroup(checkoutdata, nr);
        }
        else if(trasa==2)
        {
            in_prom=true;
            prom(tourdata, getpid(), nr, 0, trasa);
            in_prom=false;

            waitingForGroup(checkoutdata, nr);
            //wieza
            in_wieza=false;
            waitingForGroup(checkoutdata, nr);

            in_most=true;
            most(tourdata, getpid(), trasa, nr, 0);
            in_most=false;

            waitingForGroup(checkoutdata, nr);
        }
    }
    
    
    printf(ANSI_COLOR_RED "PRZEWODNIK %d KONCZY PRACE\n" ANSI_COLOR_RESET, getpid());
    checkoutCleanup(checkoutdata);
    tourCleanup(tourdata);
    
    //TourData* tourdata=tourSetupShm();
    return 0;
}