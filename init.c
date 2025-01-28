#include <stdio.h>
#include "myfun.h"
#include <time.h>



int intLength(int num);

int main()
{
    CheckoutData* data=checkoutSetupShm();
    // Allow the user to assign a value to data->max_turysci
    printf("Podaj max liczbe turystow w parku: ");
    int max_tourists;
    if (scanf("%d", &max_tourists) != 1 || max_tourists <= 0)
    {
        fprintf(stderr, "Nieprawidlowy input\n");
        exit(1);
    }

    sem_wait(&data->mutex);  
    data->maxTurysci = max_tourists;
    sem_post(&data->mutex);  

    for(int i=0;i<K;i++)
    {
        pid_t pid = fork();
        if(pid<0)
        {
            perror("FORK ERROR - init\n");
            exit(1);
        }
        else if(pid == 0) // child process
        {
            if(execl("./kasjer", "./kasjer", (char*) NULL)==-1)
            {
                perror("EXEC ERROR - kasjer\n");
                exit(1);
            } 
        }
    }
    sleep(2);


    int P_length=intLength(P);
    for(int i=0;i<P;i++)
    {
        pid_t pid = fork();
        
        if(pid<0)
        {
            perror("FORK ERROR - init\n");
            exit(1);
        }
        else if(pid == 0) // child process
        {
            char przewNR[P_length+2]; // Enough space for "50\0"
            snprintf(przewNR, sizeof(przewNR), "%d", i); // Convert age to string
            if(execl("./przewodnik", "./przewodnik", przewNR,(char*) NULL)==-1)
            {
                perror("EXEC ERROR - przewodnik\n");
                exit(1);
            } 
        }
    }
    sleep(2);
    


    // for(int i=0;i<40;i++)
    // {
    //     pid_t pid = fork();
    //     if(pid<0)
    //     {
    //         perror("FORK ERROR - init\n");
    //         exit(1);
    //     }
    //     else if(pid == 0) // child process
    //     {
    //         //int age = (rand() % 50) + 1; // Generate random age between 1 and 50
    //         int age = 0;
    //         char age_str[4]; // Enough space for "50\0"
    //         snprintf(age_str, sizeof(age_str), "%d", age); // Convert age to string
    //         if(execl("./turysta", "./turysta", age_str,(char*) NULL)==-1)
    //         {
    //             perror("EXEC ERROR - turysta\n");
    //             exit(1);
    //         }
            
    //     }
    //     sleep(1);
    // }

    time_t current_time = time(NULL); // Starting time
    time_t Tk=current_time+PARK;
    
    while(data->turCounter<data->maxTurysci && current_time < Tk)
    {
        pid_t pid = fork();
        if(pid<0)
        {
            perror("FORK ERROR - init\n");
            exit(1);
        }
        else if(pid == 0) // child process
        {
            //int age = (rand() % 50) + 1; // Generate random age between 1 and 50
            int age = 0;
            char age_str[4]; // Enough space for "50\0"
            snprintf(age_str, sizeof(age_str), "%d", age); // Convert age to string
            if(execl("./turysta", "./turysta", age_str,(char*) NULL)==-1)
            {
                perror("EXEC ERROR - turysta\n");
                exit(1);
            }
            
        }
        sleep(1);
        current_time = time(NULL);
    }



    
    while(current_time < Tk)
    {
        sleep(2);
        current_time = time(NULL);
    };
    sem_wait(&data->mutex);
    data->parkClosed=1;
    sem_post(&data->mutex);
    printf("\t\t\t\t\t\tINIT ROZLACZA SIE\n");
    checkoutCleanup(data);
}







int intLength(int num) {
    int length = 0;

    // Handle 0 as a special case
    if (num == 0) {
        return 1;
    }

    
    if (num < 0) {
        num = -num; 
    }

   
    while (num > 0) {
        num /= 10; 
        length++;
    }

    return length;
}