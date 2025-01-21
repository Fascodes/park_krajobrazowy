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
        sem_init(&data->promSpots, 1, X3);  

        
        data->mostCounter=0;
        data->mostSide=1;
        data->prom=2; 
        data->initialized=1;
        printf("TOUR DATA Init finished by: %d\n",getpid());
    }

    return data;
}


void enterqueue(CheckoutData* data,int qNumber, int tourist)
{
    //printf("ENTERQ, %d  ARG:%d\t", getpid(),tourist);
    sem_wait(&data->mutex);  // Lock access to shared resources
    if(qNumber==1)
    {
        data->enter_queue[data->enter_tail]=tourist;
        data->enter_tail=(data->enter_tail+1)%N;
        sem_post(&data->enter_sem);
        //printf("ENTER SEM POSTED\n"); 

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
        if(data->group_active[i]==0 && data->group_counts[i] +people <= M+1) 
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
    time_t Tk = current_time + 15;    // Ending time after 60 seconds
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
                int clientID = leavequeue(data, 1); // Process entering kasa
                if(clientID==-1)
                {
                    fprintf(stderr, "Nie mozna znalezc klienta w kolejce");
                    data->enter_turn = 0;
                    continue;
                }

                // Send acknowledgment to the client
                msg.mtype = clientID;
                msg.pid = getpid(); // Cashier's PID
                msg.value = 0; // Message value indicating acknowledgment
                if (msgsnd(data->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) 
                {
                    perror("msgsnd - kasjer to turysta");
                    exit(1);
                }

                // Receive clients details
                if (msgrcv(data->msqid, &msg, sizeof(pid_t) + sizeof(int), clientID, 0) == -1) 
                {
                    perror("msgrcv - kasjer receiving client details");
                    exit(1);
                }

                int is_parent = msg.value; // Check if the client is a parent
                int children_count = 0;

                if (is_parent)
                {
                    // Receive the number of children
                    if (msgrcv(data->msqid, &msg, sizeof(pid_t) + sizeof(int), clientID, 0) == -1) 
                    {
                        perror("msgrcv - kasjer receiving children count");
                        exit(1);
                    }
                    children_count = msg.value;
                    printf("Processing parent client %d with %d children.\n", clientID, children_count);
                }

                int total_size = 1 + children_count; // Parent + children

                // Check for available group
                int group = checkgroups(total_size, data);
                while (group == -1 && current_time < Tk)
                {
                    printf("No available group for client %d. Waiting for group availability.\n", clientID);
                    sem_post(&data->mutex); // Unlock while waiting
                    sleep(1);
                    sem_wait(&data->mutex); // Reacquire lock
                    group = checkgroups(total_size, data);
                    current_time = time(NULL);
                }

                if (group != -1)
                {
                    printf("Assigning client %d and family to group %d.\n", clientID, group);

                    // Wait for individual IDs from the parent and children
                    for (int i = 0; i < total_size; i++)
                    {
                        if (msgrcv(data->msqid, &msg, sizeof(pid_t) + sizeof(int), getpid(), 0) == -1)
                        {
                            perror("msgrcv - kasjer waiting for IDs");
                            exit(1);
                        }
                        int personID = msg.value; // ID from the parent or child
                        data->groups[group][data->group_counts[group]] = personID;
                        data->group_counts[group]++;

                        printf("Added person %d to group %d.\n", personID, group);
                    }

                    // Notify the parent about the assigned group
                    msg.mtype = clientID;
                    msg.value = group;
                    if (msgsnd(data->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) 
                    {
                        perror("msgsnd - kasjer to parent");
                        exit(1);
                    }

                    printf("Assigned client %d and their family to group %d.\n", clientID, group);
                }
                else
                {
                    printf("No groups available for client %d and their family.\n", clientID);
                }
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


void przewodnikWaiting(CheckoutData* checkoutdata, int nr)
{
    sem_wait(&checkoutdata->mutex);
    checkoutdata->group_active[nr]=0;
    checkoutdata->groups[nr][0]=getpid();
    checkoutdata->group_counts[nr]=1;
    sem_post(&checkoutdata->mutex);

    struct message msg;  // Local message struct

    // Wait here for full group or assign time limit
    int counter=0;
    printf("\t  PRZEWODNIK %d CZEKA NA GRUPE\n", getpid());
    while(counter<M)
    {
        if (msgrcv(checkoutdata->msqid, &msg, sizeof(pid_t)+sizeof(int), getpid(), 0) == -1) 
        {
            perror("msgrcv - przewodnik");
            exit(1);
        }
        printf("Przewodnik received a message from turysta      %d\n", msg.pid);
        counter++;
    };
    printf("\t\tPRZEWODNIK %d ZACZYNA TRASE z %d osobami\n",getpid(),counter);
}


void most(TourData* data, int tourist_id, int trasa, int group_id, int is_child, int is_parent, int children_count, pthread_t* children_tids) 
{

    sem_wait(&data->mostSpots);  // Every process waits for their turn

    int mostTime = 1; // Default crossing time for the spot

    if (is_parent) {
        printf("Parent tourist %d from group %d is waiting for %d children to take their spots on the bridge.\n",tourist_id, group_id, children_count);

        // Wait for messages from all children
        for (int i = 0; i < children_count; i++) {
            struct message msg;
            if (msgrcv(data->msqid, &msg, sizeof(pid_t) + sizeof(int), getpid(), 0) == -1) {
                perror("msgrcv - parent from child");
                exit(1);
            }
            printf("Parent received message from child for group %d.\n", group_id);
        }

        printf("All children of parent tourist %d from group %d have taken their spots on the bridge.\n",
               tourist_id, group_id);

        mostTime = 3; // Set family crossing time for the spot

        // Send the crossing time to each child
        for (int i = 0; i < children_count; i++) {
            struct message msg;
            msg.mtype = children_tids[i];  // Use each child thread's TID
            msg.pid = 0;
            msg.data = mostTime;  // Include the time
            if (msgsnd(data->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
                perror("msgsnd - parent to child");
                exit(1);
            }
            printf("Parent sent crossing time to child %lu in group %d.\n", (unsigned long)children_tids[i], group_id);
        }
        
    } else if (is_child) {
        // Send readiness message to the parent
        struct message msg;  // Set mtype to the parent's PID
        msg.mtype = getppid();
        msg.pid = 0;
        if (msgsnd(data->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
            perror("msgsnd - child to parent");
            exit(1);
        }
        printf("Child tourist %d from group %d sent readiness message.\n", tourist_id, group_id);

        // Receive the crossing time from the parent
        if (msgrcv(data->msqid, &msg, sizeof(pid_t) + sizeof(int), pthread_self(), 0) == -1) {
            perror("msgrcv - child from parent");
            exit(1);
        }
        mostTime = msg.data;  // Use the received crossing time

    } 
    // Cross the bridge
    if(trasa==1)
    {
        if(data->mostSide==1)
        {
            printf("Tourist %d from group %d is crossing the bridge. %s\n",tourist_id, group_id, is_child ? "Child" : "Adult");
            data->mostCounter++;
            sleep(mostTime);  // Use the calculated family time for consistency
            data->mostCounter--;
            printf("Tourist %d from group %d finished crossing the bridge.\n", tourist_id, group_id);
        }
    }
    else if(trasa==2)
    {
        if(data->mostSide==2)
        {
            printf("Tourist %d from group %d is crossing the bridge. %s\n",tourist_id, group_id, is_child ? "Child" : "Adult");
            data->mostCounter++;
            sleep(mostTime);  // Use the calculated family time for consistency
            data->mostCounter--;
            printf("Tourist %d from group %d finished crossing the bridge.\n", tourist_id, group_id);
        }
    }
    

    sem_post(&data->mostSpots);
    return;


}


// Tower climbing function
void wieza(TourData* data, int tourist_id, int group_id, int is_child, int is_below_five, int is_parent, int children_count, pthread_t* children_tids) 
{
sem_wait(&data->wiezaSpots);  // Semaphore for wieza spot

    srand(time(NULL));  // Seed the random number generator
    int wiezaTime = rand() % 5 + 1;  // Random time between 1 and 5

    if (is_parent) {
        if (children_count <= 0 || children_tids == NULL) {
            fprintf(stderr, "Error: Rodzic turysta %d nie ma waznych informacji o dzieciach\n", tourist_id);
            sem_post(&data->wiezaSpots);  // Release semaphore to avoid blocking
            return;
        }
        int skipAttraction = 0;

        printf("Rodzic turysta %d z grupy %d koordynuje %d dzieci przy wiezy\n",
               tourist_id, group_id, children_count);

        // Wait for readiness messages from all children
        for (int i = 0; i < children_count; i++) {
            struct message msg;
            if (msgrcv(data->msqid, &msg, sizeof(pid_t) + sizeof(int), getpid(), 0) == -1) {
                perror("msgrcv - parent from child");
                exit(1);
            }
            if (msg.value == 1) {
                skipAttraction = 1;
                wiezaTime = 0;
            }
            printf("Rodzic otrzymal wiadomość od dziecka z grupy %d\n", group_id);
        }

        printf("Wszystkie dzieci rodzica turysty %d z grupy %d sa gotowe przy wiezy\n",
               tourist_id, group_id);

        // Send the wieza time to each child
        for (int i = 0; i < children_count; i++) {
            struct message msg;
            msg.mtype = children_tids[i];  // Use each child's TID
            msg.pid = 0;
            msg.value = wiezaTime;  // Include the time
            if (msgsnd(data->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
                perror("msgsnd - parent to child");
                exit(1);
            }

            printf("Rodzic wyslal czas wiezy do dziecka %lu z grupy %d\n", (unsigned long)children_tids[i], group_id);
        }
        if (skipAttraction) {
            printf("Rodzic %d i jego dzieci omijaja wieze\n", tourist_id);
            sem_post(&data->wiezaSpots);
            return;
        };
    } else if (is_child) {
        // Send readiness message to the parent
        struct message msg;
        msg.mtype = getppid();  // Send to the parent's PID
        msg.pid = 0;
        msg.value = is_below_five;
        if (msgsnd(data->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
            perror("msgsnd - child to parent");
            exit(1);
        }
        printf("Dziecko turysta %d z grupy %d wyslalo wiadomosc gotowosci\n", tourist_id, group_id);

        // Receive the wieza time from the parent
        if (msgrcv(data->msqid, &msg, sizeof(pid_t) + sizeof(int), pthread_self(), 0) == -1) {
            perror("msgrcv - child from parent");
            exit(1);
        }

        if ((wiezaTime = msg.value) == 0) {
            printf("Dziecko turysta %d z grupy %d omija wieze razem z rodzina\n", tourist_id, group_id);
            sem_post(&data->wiezaSpots);
            return;
        };  // Use the received wieza time
    }

    // Simulate wieza activity
    printf("Turysta %d z grupy %d spedza czas przy wiezy %s\n", tourist_id, group_id, is_child ? "Dziecko" : "Dorosly");
    sleep(wiezaTime);
    printf("Turysta %d z grupy %d zakonczyl pobyt przy wiezy\n", tourist_id, group_id);

    // Release the semaphore
    sem_post(&data->wiezaSpots);
}


// Ferry riding function
void prom(TourData* data, int tourist_id, int group_id, int is_child, int is_parent, int children_count)
{
    // If the parent, wait for all children to acquire the semaphore first
    if (is_parent) {
        printf("Parent tourist %d from group %d is waiting for %d children to take their spots on the ferry.\n",
               tourist_id, group_id, children_count);

        for (int i = 0; i < children_count; i++) {
            sem_wait(&data->promSpots); // Wait for each child
        }

        printf("All children of parent tourist %d from group %d have taken their spots on the ferry.\n",
               tourist_id, group_id);
    }

    // Each process (parent or child) waits for a spot
    sem_wait(&data->promSpots);

    printf("Tourist %d from group %d is riding the ferry. %s\n",
           tourist_id, group_id, is_child ? "Child" : "Adult");

    // Simulate ferry ride time, with 50% longer time for children
    int ferry_time = is_child ? 5 : 3;
    sleep(ferry_time);

    printf("Tourist %d from group %d finished riding the ferry.\n", tourist_id, group_id);

    sem_post(&data->promSpots); // Release the ferry spot
}