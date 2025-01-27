#include <stdio.h>
#include "myfun.h"


int main(int argc, char* argv[])
{
    if(argc!=2)
    {
        printf("ERROR BAD ARGUMENTS - przewodnik");
        exit(1);
    }
    int nr=atoi(argv[1]);
    printf("PRZEWODNIK PID: %d grupa: %d\n", getpid(), nr);
    
    struct message msg;

    CheckoutData* checkoutdata=checkoutSetupShm();
    TourData* tourdata=tourSetupShm();
    time_t Tk=time(NULL)+PARK;


    while(time(NULL)<Tk)
    {
        przewodnikWaiting(checkoutdata, nr, (time(NULL)+PARK/3));

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
            most(tourdata, getpid(), trasa, nr, 0);

            waitingForGroup(checkoutdata, nr);
            //wieza
            waitingForGroup(checkoutdata, nr);

            prom(tourdata, getpid(), nr, 0, trasa);

            waitingForGroup(checkoutdata, nr);
        }
        else if(trasa==2)
        {
            printf("TUTAJ\n");
            prom(tourdata, getpid(), nr, 0, trasa);

            waitingForGroup(checkoutdata, nr);
            //wieza
            waitingForGroup(checkoutdata, nr);

            most(tourdata, getpid(), trasa, nr, 0);

            waitingForGroup(checkoutdata, nr);
        }
    }
    
    

    checkoutCleanup(checkoutdata);
    tourCleanup(tourdata);
    
    //TourData* tourdata=tourSetupShm();
    return 0;
}