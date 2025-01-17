#include "myfun.h"

CheckoutData *checkoutSetupShm()
{
    // Generate a key using ftok() based on a file path and project ID
    key_t key = ftok("/tmp", 'C');  // Use a file and project identifier
    if (key == -1) {
        perror("ftok - checkout");
        exit(1);
    }

    // Create or get the shared memory segment with shmget using the generated key
    int shmID = shmget(key, sizeof(CheckoutData), IPC_CREAT | 0666);
    if (shmID == -1) {
        perror("shmget - checkout");
        exit(1);
    }

    // Attach the shared memory to the process's address space
    CheckoutData *data = (CheckoutData *)shmat(shmID, NULL, 0);
    if (data == (CheckoutData *)-1) {
        perror("shmat - checkout");
        exit(1);
    }

    if (data->initialized!=1)
    {
        data->enter_turn=0;
        data->enter_head=0; 
        data->enter_tail=0; // Head and tail for entering queue
        data->exit_head=0;
        data->exit_tail=0;   // Head and tail for exiting queue
        data->enter_turn=1;
        data->initialized=1;

        // Initialize semaphores
        sem_init(&data->mutex, 1, 1);  
        sem_init(&data->enter_sem, 1, 0);  
        sem_init(&data->exit_sem, 1, 0);  
        sem_init(&data->working, 1, 0);  
        for (int i = 0; i < P; i++) {
            sem_init(&data->group_ready[i], 1, 0);  // Group ready semaphores
        }
        printf("Init finished by: %d\n",getpid());
    }
    


    return data;

    // ADD CLEANUP LATER
    // shmdt(data); 
    // shmctl(shmID, IPC_RMID, NULL); 

}


// TourData *tourSetupShm(){
//     // Generate a key using ftok() based on a file path and project ID
//     key_t key = ftok("/tmp", 'T');  // Use a file and project identifier
//     if (key == -1) {
//         perror("ftok - tour");
//         exit(1);
//     }

//     // Create or get the shared memory segment with shmget using the generated key
//     int shmID = shmget(key, sizeof(TourData), IPC_CREAT | 0666);
//     if (shmID == -1) {
//         perror("shmget - tour");
//         exit(1);
//     }

//     // Attach the shared memory to the process's address space
//     TourData *data = (TourData *)shmat(shmID, NULL, 0);
//     if (data == (TourData *)-1) {
//         perror("shmat - tour");
//         exit(1);
//     }

//     // Initialize semaphores
//     sem_init(&data->mutex, 1, 1);  // Mutex for shared memory access
//     for (int i = 0; i < P; i++) {
//         sem_init(&data->group_ready[i], 1, 0);  // Group ready semaphores
//     }

//     return data;
// }


void enterqueue(CheckoutData* data,int qNumber, int tourist)
{
    printf("ENTERQ, %d  ARG:%d\t", getpid(),tourist);
    sem_wait(&data->mutex);  // Lock access to shared resources
    if(qNumber==1)
    {
        data->enter_queue[data->enter_tail]=tourist;
        data->enter_tail=(data->enter_tail+1)%N;
        sem_post(&data->enter_sem);
        printf("ENTER SEM POSTED\n"); 

    }
    else if(qNumber==2)
    {
        data->exit_queue[data->exit_tail]=tourist;
        data->exit_tail=(data->exit_tail+1)%N;
        sem_post(&data->exit_sem);
        
    }
    sem_post(&data->mutex);  // Unlock access to shared resources

}

int leavequeue(CheckoutData* data,int qNumber)
{
    printf("LEAVEQ %d qNumber=%d\n", getpid(), qNumber);
    int clientID=-1;
    if(qNumber==1)
    {


        clientID=data->enter_queue[data->enter_head];
        data->enter_queue[data->enter_head]=0;
        data->enter_head=(data->enter_head+1)%N;


    }
    else if(qNumber==2)
    {
        
        clientID=data->exit_queue[data->exit_head];
        data->exit_queue[data->exit_head]=0;
        data->exit_head=(data->exit_head+1)%N;


    }
    printf("CLIENT: %d\n",clientID);
    return clientID;
}


void processClients(CheckoutData* data) // kasjer function
{
    time_t current_time = time(NULL); // Starting time
    time_t Tk = current_time + 10;    // Ending time after 60 seconds

    while (1)
    {
        current_time = time(NULL); // Get the current time

        // Check if the current time exceeds Tk
        if (current_time > Tk)
        {
            printf("Time limit reached. Stopping process.\n");
            break; // Exit the loop
        }

        sem_wait(&data->mutex);

        if (data->enter_turn)
        {
            if (sem_trywait(&data->enter_sem) == 0) 
            {
                int clientID = leavequeue(data, 1); // Process entering queue
                printf("Processed entering client %d\n", clientID);
            }
            data->enter_turn = 0;
        }
        else
        {
            if (sem_trywait(&data->exit_sem) == 0) 
            {
                int clientID = leavequeue(data, 2); // Process exiting queue
                printf("Processed exiting client %d\n", clientID);
            }
            data->enter_turn = 1;
        }

        sem_post(&data->mutex); // Unlock shared resource
        sleep(1);
    }
}