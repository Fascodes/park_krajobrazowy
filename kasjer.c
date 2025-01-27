#include <stdio.h>
#include "myfun.h"




int main()
{
    CheckoutData* data=checkoutSetupShm();

    printf("KASJER PID: %d\n", getpid());
    
    processClients(data);
    
    checkoutCleanup(data);

    return 0;
}