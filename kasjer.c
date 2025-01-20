#include <stdio.h>
#include "myfun.h"




int main()
{
    CheckoutData* data=checkoutSetupShm();

    printf("KASJER PID: %d\n", getpid());
    
    processClients(data);
    sem_wait(&data->mutex);
    sem_post(&data->working);

    return 0;
}