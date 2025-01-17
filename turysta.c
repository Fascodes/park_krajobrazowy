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
    //TourData* tourdata=tourSetupShm();

    sleep(8);
    printf("Koniec turysta: %d\n", mypid);

    return 0;
}