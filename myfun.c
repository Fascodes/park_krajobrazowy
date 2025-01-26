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

    key_t mskey = ftok("/tmp", 'K');
    data->msqid = msgget(mskey, IPC_CREAT | 0666);

    if (data->initialized!=1)
    {
        data->enter_turn=0;
        data->enter_head=0; 
        data->enter_tail=0; // Head and tail for entering queue
        data->exit_head=0;
        data->exit_tail=0;   // Head and tail for exiting queue
        data->enter_turn=1;
        data->initialized=1;
            
        for(int i=0;i<P;i++)
        {
            data->group_counts[i]=0;
            data->group_children[i]=0;
        };


        // Initialize semaphores
        sem_init(&data->mutex, 1, 1);  
        sem_init(&data->enter_sem, 1, 0);  
        sem_init(&data->exit_sem, 1, 0);  
        sem_init(&data->working, 1, 0);  
        // for (int i = 0; i < P; i++) {
        //     sem_init(&data->group_ready[i], 1, 0);  // Group ready semaphores
        // }
        printf("CHECKOUT DATA Init finished by: %d\n",getpid());
    }
    


    return data;

    // ADD CLEANUP LATER
    // shmdt(data); 
    // shmctl(shmID, IPC_RMID, NULL); 

}


TourData *tourSetupShm(){
    // Generate a key using ftok() based on a file path and project ID
    key_t key = ftok("/tmp", 'T');  // Use a file and project identifier
    if (key == -1) {
        perror("ftok - tour");
        exit(1);
    }

    // Create or get the shared memory segment with shmget using the generated key
    int shmID = shmget(key, sizeof(TourData), IPC_CREAT | 0666);
    if (shmID == -1) {
        perror("shmget - tour");
        exit(1);
    }

    // Attach the shared memory to the process's address space
    TourData *data = (TourData *)shmat(shmID, NULL, 0);
    if (data == (TourData *)-1) {
        perror("shmat - tour");
        exit(1);
    }

    key_t mskey = ftok("/tmp", 'P');
    data->msqid = msgget(mskey, IPC_CREAT | 0666);


    if(data->initialized!=1)
    {
        // Initialize semaphores
        sem_init(&data->mostSpots1, 1, X1);
        sem_init(&data->mostSpots2, 1, X1);  
        sem_init(&data->wiezaSpots, 1, X2);  
        sem_init(&data->promSpots1, 1, X3);  
        sem_init(&data->promSpots2, 1, X3);  

        
        data->mostCounter1=0;
        data->mostCounter2=0;
        data->mostSide=1;
        data->prom=2; 
        data->initialized=1;
        data->mostWaiting1=0;
	    data->mostWaiting2=0;
        data->promSide=1;
        printf("TOUR DATA Init finished by: %d\n",getpid());
    }

    return data;
}


void enterqueue(CheckoutData* data,int qNumber, int children)
{
    //printf("ENTERQ, %d  ARG:%d\t", getpid(),tourist);
    sem_wait(&data->mutex);  // Lock access to shared resources
    struct message msg;
    
    msg.pid=getpid();
    msg.value=children;
    if(qNumber==1)
    {
        // data->enter_queue[data->enter_tail]=tourist;
        // data->enter_tail=(data->enter_tail+1)%N;
        msg.mtype=KASA1;
        sem_post(&data->enter_sem);
        
        if (msgsnd(data->msqid, &msg, sizeof(pid_t)+sizeof(int), 0) == -1) 
        {
            perror("msgsnd");
            exit(1);
        }
        //printf("ENTER SEM POSTED\n"); 

    }
    else if(qNumber==2)
    {
        // data->exit_queue[data->exit_tail]=tourist;
        // data->exit_tail=(data->exit_tail+1)%N;
        msg.mtype=KASA2;
        sem_post(&data->exit_sem);
        if (msgsnd(data->msqid, &msg, sizeof(pid_t)+sizeof(int), 0) == -1) 
        {
            perror("msgsnd");
            exit(1);
        }
        
    }
    sem_post(&data->mutex);  // Unlock access to shared resources

}

int leavequeue(CheckoutData* data,int qNumber)
{
    //printf("LEAVEQ %d qNumber=%d\n", getpid(), qNumber);
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
    //printf("CLIENT: %d\n",clientID);
    return clientID;
}

int checkgroups(int people, CheckoutData* data)
{
    int group=-1;
    //printf("CHECKING GROUPS\n");
    for(int i=0; i<P;i++)
    {
        if(data->group_active[i]==0 && data->group_counts[i] + data->group_children[i] + people <= M+1) 
        {
            group=i;
            break;
        };
    };

    return group;
}


void processClients(CheckoutData* data) // kasjer function
{
    time_t current_time = time(NULL); // Starting time
    time_t Tk = current_time + 90;    // Ending time after 60 seconds
    struct message msg;
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

                // Receive clients details
                if (msgrcv(data->msqid, &msg, sizeof(pid_t) + sizeof(int), KASA1, 0) == -1) 
                {
                    perror("msgrcv - kasjer receiving client details");
                    exit(1);
                }

                int people = msg.value + 1;
                int clientID=msg.pid;


                // Check for available group
                int group = checkgroups(people, data);
                while (group == -1 && current_time < Tk)
                {
                    printf("No available group for client %d. Waiting for group availability.\n", clientID);
                    sem_post(&data->mutex); // Unlock while waiting
                    sleep(1);
                    sem_wait(&data->mutex); // Reacquire lock
                    group = checkgroups(people, data);
                    current_time = time(NULL);
                }

                if(group!=-1)
                {
                    data->groups[group][data->group_counts[group]]=clientID;
                    data->group_counts[group]++;
                    for(int i=0;i<people-1;i++)
                    {
                        data->group_children[group]++;
                    }
                    struct message msg;
                    msg.mtype=clientID;
                    msg.value=group;
                    if (msgsnd(data->msqid, &msg, sizeof(pid_t)+sizeof(int), 0) == -1) 
                    {
                        perror("msgsnd");
                        exit(1);
                    }
                }
                else
                {
                    printf("There were no available groups for %d\n", clientID);
                }
            }
            data->enter_turn = 0;
        }
        else
        {
            if (sem_trywait(&data->exit_sem) == 0) 
            {
                if (msgrcv(data->msqid, &msg, sizeof(pid_t) + sizeof(int), KASA2, 0) == -1) 
                {
                    perror("msgrcv - kasjer receiving client details");
                    exit(1);
                }
                int clientID = msg.pid; // Process exiting queue
                printf("Processed exiting client %d\n", clientID);
            }
            data->enter_turn = 1;
        }

        sem_post(&data->mutex); // Unlock shared resource
        sleep(1);
    }
}


void przewodnikWaiting(CheckoutData* checkoutdata, int nr)
{
    //sem_wait(&checkoutdata->mutex);
    checkoutdata->group_active[nr]=0;
    checkoutdata->groups[nr][0]=getpid();
    checkoutdata->group_counts[nr]=1;
    //sem_post(&checkoutdata->mutex);

    //struct message msg;  // Local message struct

    // Wait here for full group or assign time limit
    printf("\t  PRZEWODNIK %d CZEKA NA GRUPE\n", getpid());
    while(checkoutdata->group_counts[nr]+checkoutdata->group_children[nr]<M+1);
    printf("\t\tPRZEWODNIK %d ZACZYNA TRASE z %d osobami %d dziecmi\n",getpid(),checkoutdata->group_counts[nr]-1, checkoutdata->group_children[nr]);
    checkoutdata->group_active[nr]=1;
}

void waitingForGroup(CheckoutData* checkoutdata, int mygroup) {
    struct message msg;
    int messagesReceived = 0;

    printf("Przewodnik %d is waiting for group %d to finish activities.\n", getpid(), mygroup);

    // Wait for all tourists in the group to notify the przewodnik
    while (messagesReceived < checkoutdata->group_counts[mygroup]-1) {
        // Wait for a message from a turysta in the group
        if (msgrcv(checkoutdata->msqid, &msg, sizeof(pid_t) + sizeof(int), getpid(), 0) == -1) {
            perror("msgrcv - przewodnik waiting for group");
            exit(1);
        }
        messagesReceived++;
        //printf("\t\tMESSAGES RECEIVED: %d\n", messagesReceived);
        //printf("\t\tGROUP COUNT %d: %d\n",mygroup, checkoutdata->group_counts[mygroup]);
    }

    printf("Przewodnik %d has received all messages from group %d. Simulating travel to destination...\n", getpid(), mygroup);

    // Simulate travel with a delay
    sleep(rand() % 5 + 1); // Replace 3 with the desired delay in seconds

    // Notify the group that they have arrived
    // Group-specific message type for arrival notification

    for(int i=0;i<checkoutdata->group_counts[mygroup]-1;i++)
    {
        msg.mtype = checkoutdata->groups[mygroup][i+1];
        if (msgsnd(checkoutdata->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
        perror("msgsnd - przewodnik notifying group arrival");
        exit(1);
    }
    }

    

    printf("Przewodnik %d notified group %d of their arrival.\n", getpid(), mygroup);
}



void most(TourData* data, int tourist_id, int trasa, int group_id, int children_count) 
{
    

    int mostTime = rand() % 5 + 1;; // Default crossing time for the spot


    if(trasa==1)
    {
        for (int i = 0; i < children_count+1; i++) 
        {
            sem_wait(&data->mostSpots1);
            data->mostWaiting1++;
        };
        //printf("\t\tDATAMOSTSIDE %d\n", data->mostSide);
        if(data->mostWaiting2==0 && data->mostSide!=1)
        {
            data->mostSide=1;
        };
        //printf("\t\tDATAMOSTSIDE %d\n", data->mostSide);
        
        while(data->mostSide!=1 && data->mostCounter2>0);
        for (int i = 0; i < children_count+1; i++) 
        {
            data->mostCounter1++;
        };
        printf("Tourist %d from group %d is crossing the bridge.\n",tourist_id, group_id);
        sleep(mostTime);
        printf("Tourist %d from group %d finished crossing the bridge.\n", tourist_id, group_id);        
        if(data->mostWaiting2>0 && data->mostSide!=2)
        {
            data->mostSide=2;
        };
        for (int i = 0; i < children_count+1; i++) 
        {
            data->mostCounter1--;
            data->mostWaiting1--;
            sem_post(&data->mostSpots1);
        };
        //printf("\t\tDATAMOSTSIDE %d\n", data->mostSide);
        
    }
    else if(trasa==2)
    {
        for (int i = 0; i < children_count+1; i++) 
        {
            sem_wait(&data->mostSpots1);
            data->mostWaiting2++;
        };
        if(data->mostWaiting1==0 && data->mostSide!=2)
        {
            data->mostSide=2;
        };
        while(data->mostSide!=2 && data->mostCounter1>0);
        for (int i = 0; i < children_count+1; i++) 
        {
            data->mostCounter2++;
        };
        printf("Tourist %d from group %d is crossing the bridge.\n",tourist_id, group_id);
        sleep(mostTime);
        printf("Tourist %d from group %d finished crossing the bridge.\n", tourist_id, group_id);
        if(data->mostWaiting1>0 && data->mostSide!=1)
        {
            data->mostSide=1;
        };
        for (int i = 0; i < children_count+1; i++) 
        {
            data->mostCounter2--;
            data->mostWaiting2--;
            sem_post(&data->mostSpots2);
        };
    }

    return;
}


// Tower climbing function
void wieza(TourData* data, int tourist_id, int group_id, int children_count) 
{
    sem_wait(&data->wiezaSpots);  // Semaphore for wieza spot
    for (int i = 0; i < children_count; i++) 
    {
        sem_wait(&data->wiezaSpots);
    }

    srand(time(NULL));  // Seed the random number generator
    int wiezaTime = rand() % 5 + 1;  // Random time between 1 and 5

    // Simulate wieza activity
    printf("Turysta %d z grupy %d spedza czas przy wiezy\n", tourist_id, group_id);
    sleep(wiezaTime);
    printf("Turysta %d z grupy %d zakonczyl pobyt przy wiezy\n", tourist_id, group_id);

    // Release the semaphore
    sem_post(&data->wiezaSpots);
    for (int i = 0; i < children_count; i++) {
            sem_post(&data->wiezaSpots);
        }
    return;
}


// Ferry riding function
void prom(TourData* data, int tourist_id, int group_id, int children_count, int trasa)
{
    int ferry_time=PROM;
    if(trasa==1)
    {
        for(int i=1;i<children_count+1;i++)
        {
            sem_wait(&data->promSpots1);
            
        }
        while(data->promSide!=1)
        {
            if (sem_trywait(&data->promSpots2) == 0) {
                // Someone is waiting on the other side, release and keep waiting
                sem_post(&data->promSpots2);
                continue;
            }
            // No one is waiting on the other side, summon the ferry
            printf("Tourist %d from group %d is summoning the ferry to side 1.\n", tourist_id, group_id);
            data->promSide = 1;
        };
        printf("Tourist %d from group %d is riding the ferry.\n",tourist_id, group_id);
        sleep(ferry_time);
        printf("Tourist %d from group %d finished riding the ferry.\n", tourist_id, group_id);
        if(data->promSide==1)
        {
            data->promSide=2;
        }
        
        for(int i=1;i<children_count+1;i++)
        {
            sem_post(&data->promSpots1);
        }

    }

    else if(trasa==2)
    {
        for(int i=1;i<children_count+1;i++)
        {
            sem_wait(&data->promSpots2);
        }
        while(data->promSide!=2)
        {
            if (sem_trywait(&data->promSpots1) == 0) {
                // Someone is waiting on the other side, release and keep waiting
                sem_post(&data->promSpots1);
                continue;
            }
            // No one is waiting on the other side, summon the ferry
            printf("Tourist %d from group %d is summoning the ferry to side 2.\n", tourist_id, group_id);
            data->promSide = 2;
        };
        printf("Tourist %d from group %d is riding the ferry.\n",tourist_id, group_id);
        sleep(ferry_time);
        printf("Tourist %d from group %d finished riding the ferry.\n", tourist_id, group_id);
        if(data->promSide==2)
        {
            data->promSide=1;
        }
        
        for(int i=1;i<children_count+1;i++)
        {
            sem_post(&data->promSpots2);
        }
    }
     // Release the ferry spot
}