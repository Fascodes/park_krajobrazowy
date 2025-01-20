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
    
    

    CheckoutData* checkoutdata=checkoutSetupShm();
    //printf("Przewodnik changing values %d\n",getpid());
    przewodnikWaiting(checkoutdata, nr);


    sem_wait(&checkoutdata->mutex);
    sem_post(&checkoutdata->working);
    
    //TourData* tourdata=tourSetupShm();
    return 0;
}