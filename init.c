#include <stdio.h>
#include "myfun.h"
#include <time.h>



int intLength(int num);

int main()
{

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
    //sleep(1);


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
    //sleep(1);
    time_t current_time = time(NULL); // Starting time
    time_t Tk = current_time + 50;    // Ending time after 60 seconds
    for(int i=0;i<N && current_time<Tk;i++)
    {
        pid_t pid = fork();
        if(pid<0)
        {
            perror("FORK ERROR - init\n");
            exit(1);
        }
        else if(pid == 0) // child process
        {
            srand(time(NULL));
            
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
        //sleep(1);
        current_time=time(NULL); 
    }
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