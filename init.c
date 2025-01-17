#include <stdio.h>
#include "myfun.h"
#include <time.h>

#define K 10
#define P 10


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
    sleep(1);

    // for(int i=0;i<P;i++)
    // {
    //     pid_t pid = fork();
    //     if(pid<0)
    //     {
    //         perror("FORK ERROR - init\n");
    //         exit(1);
    //     }
    //     else if(pid == 0) // child process
    //     {
    //         if(execl("./przewodnik", "./przewodnik", (char*) NULL)==-1)
    //         {
    //             perror("EXEC ERROR - przewodnik\n");
    //             exit(1);
    //         } 
    //     }
    // }
    // sleep(1);

    for(int i=0;i<N;i++)
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
            
            int age = (rand() % 50) + 1; // Generate random age between 1 and 50
            char age_str[4]; // Enough space for "50\0"
            snprintf(age_str, sizeof(age_str), "%d", age); // Convert age to string
            if(execl("./turysta", "./turysta", age_str,(char*) NULL)==-1)
            {
                perror("EXEC ERROR - turysta\n");
                exit(1);
            }
            
        }
        sleep(1); 
    }

    //while() // tutaj semafor w przewodniku do zamkniecia parku

    key_t key = ftok("/tmp", 'C');  // Use a file and project identifier
    if (key == -1) {
        perror("cleanup");
        exit(1);
    }

    // Create or get the shared memory segment with shmget using the generated key
    int shmID = shmget(key, sizeof(CheckoutData), IPC_CREAT | 0666);
    if (shmID == -1) {
        perror("cleanup");
        exit(1);
    }

    // Attach the shared memory to the process's address space
    CheckoutData *data = (CheckoutData *)shmat(shmID, NULL, 0);
    if (data == (CheckoutData *)-1) {
        perror("cleanup");
        exit(1);
    }
    for(int i=0; i<K;i++)
    {
        sem_wait(&data->working); // Wait for each process to signal
        //sem_post(&data->mutex);
    }
    
    
    shmdt(data); // Detach memory
    shmctl(shmID, IPC_RMID, NULL); // Mark memory for deletion
    printf("CLEANUP DONE by init\n");
}