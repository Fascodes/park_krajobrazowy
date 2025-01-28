#include "myfun.h"


/**
 * Tworzy i inicjuje segment pamieci wspoldzielonej dla danych kasowych.
 *
 * return Wskaznik na zainicjalizowane dane typu CheckoutData.
 *
 * Plik "/tmp" istnieje i dostepny jest identyfikator projektu 'C'.
 * Segment pamieci wspoldzielonej zostal poprawnie utworzony i zainicjowany.
 */
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
    data->connected++;
    if (data->initialized!=1)
    {
        data->maxTurysci=N;
        data->enter_turn=1;
        data->initialized=1;
        data->parkClosed=0;
        data->turCounter=0;
            
        for(int i=0;i<P;i++)
        {
            data->group_counts[i]=0;
            data->group_children[i]=0;
        };


        // Initialize semaphores
        sem_init(&data->mutex, 1, 1);  
        sem_init(&data->enter_sem, 1, 0);  
        sem_init(&data->exit_sem, 1, 0);  
        sem_init(&data->checkoutspace, 1, K);
        sem_init(&data->cleanupMutex, 1, 1);

        printf("CHECKOUT DATA Init finished by: %d\n",getpid());
    }
    


    return data;


}

/**
 * Tworzy i inicjuje segment pamieci wspoldzielonej dla danych zwiazanych z atrakcjami turystycznymi.
 *
 * Wskaznik na zainicjalizowane dane typu TourData.
 *
 * Plik "/tmp" istnieje i dostepny jest identyfikator projektu 'T'.
 * Segment pamieci wspoldzielonej zostal poprawnie utworzony i zainicjowany.
 */
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

    data->connected++;


    if(data->initialized!=1)
    {

        // Initialize semaphores
        sem_init(&data->mostSpots1, 1, X1);
        sem_init(&data->mostSpots2, 1, X1);  
        sem_init(&data->wiezaSpots, 1, X2);  
        sem_init(&data->promSpots1, 1, X3);  
        sem_init(&data->promSpots2, 1, X3);
        sem_init(&data->promMutex, 1, 1);
        sem_init(&data->cleanupMutex, 1, 1);

        
        data->mostCounter1=0;
        data->mostCounter2=0;
        data->mostSide=1;
        data->prom=0; 
        data->promCounter1=0;
        data->promCounter2=0;
        data->promWaiting1=0;
        data->promWaiting2=0;
        data->initialized=1;
        data->mostWaiting1=0;
	    data->mostWaiting2=0;
        data->promSide=1;
        printf("TOUR DATA Init finished by: %d\n",getpid());
    }

    return data;
}

/**
 * Usuwa segment pamieci wspoldzielonej oraz kolejki komunikatow zwiazane z danymi kasowymi.
 *
 * param data Wskaznik na strukture CheckoutData do oczyszczenia.
 *
 * Wskaznik data wskazuje na poprawnie zainicjalizowana pamiec wspoldzielona.
 * Pamiec wspoldzielona oraz kolejki komunikatow zostaly usuniete, jesli brak podlaczonych procesow.
 */
void checkoutCleanup(CheckoutData* data)
{
    sem_wait(&data->cleanupMutex);
    data->connected--;
    if(data->connected==0)
    {
        // Remove the message queue
        if (msgctl(data->msqid, IPC_RMID, NULL) == -1) {
            perror("msgctl - checkout cleanup");
        }

        if (shmdt(data) == -1) {
            perror("shmdt - checkout cleanup");
        }

        // Generate the key for shared memory
        key_t key = ftok("/tmp", 'C');
        if (key == -1) {
            perror("ftok - checkout cleanup");
            return;
        }

        // Get the shared memory ID
        int shmID = shmget(key, sizeof(CheckoutData), 0666);
        if (shmID == -1) {
            perror("shmget - checkout cleanup");
            return;
        }

        // Remove shared memory
        if (shmctl(shmID, IPC_RMID, NULL) == -1) {
            perror("shmctl - checkout cleanup");
        }

        printf("CheckoutData cleanup PID: %d\n", getpid());
    }
    else
    {
        sem_post(&data->cleanupMutex);
    }
}


/**
 * Usuwa segment pamieci wspoldzielonej oraz kolejki komunikatow zwiazane z danymi turystycznymi.
 *
 * param data Wskaznik na strukture TourData do oczyszczenia.
 *
 * Wskaznik data wskazuje na poprawnie zainicjalizowana pamiec wspoldzielona.
 * Pamiec wspoldzielona oraz kolejki komunikatow zostaly usuniete, jesli brak podlaczonych procesow.
 */
void tourCleanup(TourData* data)
{
    sem_wait(&data->cleanupMutex);
    data->connected--;

    if(data->connected==0)
    {
        // Remove the message queue
        if (msgctl(data->msqid, IPC_RMID, NULL) == -1) {
            perror("msgctl - tour cleanup");
        }

        // Detach shared memory
        if (shmdt(data) == -1) {
            perror("shmdt - tour cleanup");
        }

        // Generate the key for shared memory
        key_t key = ftok("/tmp", 'T');
        if (key == -1) {
            perror("ftok - tour cleanup");
            return;
        }

        // Get the shared memory ID
        int shmID = shmget(key, sizeof(TourData), 0666);
        if (shmID == -1) {
            perror("shmget - tour cleanup");
            return;
        }
        

        // Remove shared memory
        if (shmctl(shmID, IPC_RMID, NULL) == -1) {
            perror("shmctl - tour cleanup");
        }


        printf("TourData cleanup PID: %d\n", getpid());
    }
    else
    {
        sem_post(&data->cleanupMutex);
    }
}

/**
 * Obsluguje dodawanie klientow do odpowiedniej kolejki kasowej.
 *
 * param data Wskaznik na strukture CheckoutData.
 * param qNumber Numer kolejki (1 dla wchodzacej, 2 dla wychodzacej).
 * param children Liczba dzieci dolaczajacych do klienta.
 *
 * Wskaznik data jest poprawny, a qNumber nalezy do {1, 2}.
 * Klient zostal dodany do odpowiedniej kolejki.
 */
void enterqueue(CheckoutData* data,int qNumber, int children)
{
    
    struct message msg;
    
    msg.pid=getpid();
    msg.value=children;
    if(qNumber==1)
    {

        sem_wait(&data->checkoutspace);
        sem_wait(&data->mutex);
        data->turCounter++;
        if(data->parkClosed || data->turCounter>=data->maxTurysci)
        {
            sem_post(&data->checkoutspace);
            sem_post(&data->mutex);
            checkoutCleanup(data);
            exit(0);
        }
        sem_post(&data->mutex);

        msg.mtype=KASA1;
        sem_post(&data->enter_sem);
        
        if (msgsnd(data->msqid, &msg, sizeof(pid_t)+sizeof(int), 0) == -1) 
        {
            perror("msgsnd");
            exit(1);
        }

    }
    else if(qNumber==2)
    {
        //sem_wait(&data->mutex);  // Lock access to shared resources

        msg.mtype=KASA2;
        sem_post(&data->exit_sem);
        if (msgsnd(data->msqid, &msg, sizeof(pid_t)+sizeof(int), 0) == -1) 
        {
            perror("msgsnd");
            exit(1);
        }
        //sem_post(&data->mutex);  // Unlock access to shared resources
    }
    

}

/**
 * Sprawdza, czy istnieje dostepna grupa dla nowo przybylych turystow.
 *
 * param people Liczba osob do dolaczenia.
 * param data Wskaznik na strukture CheckoutData.
 * return Indeks dostepnej grupy lub -1, jesli brak miejsca.
 *
 */
int checkgroups(int people, CheckoutData* data)
{
    int group=-1;

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

/**
 * Obsluguje przetwarzanie klientow w kolejkach kasowych.
 *
 * param data Wskaznik na strukture CheckoutData.
 *
 */
void processClients(CheckoutData* data) 
{

    struct message msg;

    while (!data->parkClosed)
    {


        sem_wait(&data->mutex);

        if (data->enter_turn)
        {
            if (sem_trywait(&data->enter_sem) == 0) 
            {
                // Receive client details
                if (msgrcv(data->msqid, &msg, sizeof(pid_t) + sizeof(int), KASA1, 0) == -1) 
                {
                    perror(ANSI_COLOR_RED "msgrcv - kasjer odbierajacy szczegoly klienta" ANSI_COLOR_RESET);
                    exit(1);
                }

                int people = msg.value + 1;
                int clientID = msg.pid;

                // Check for available group
                int group = checkgroups(people, data);
                while (group == -1 && !(data->parkClosed))
                {
                    
                    sem_post(&data->mutex); // Unlock while waiting
                    //sleep(1);
                    sem_wait(&data->mutex); // Reacquire lock
                    group = checkgroups(people, data);
                }

                if (group != -1)
                {
                    data->groups[group][data->group_counts[group]] = clientID;
                    data->group_counts[group]++;
                    data->processedCounter++;
                    for (int i = 0; i < people - 1; i++)
                    {
                        data->group_children[group]++;
                    }
                    
                    msg.mtype = clientID;
                    msg.value = group;
                    if (msgsnd(data->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) 
                    {
                        perror(ANSI_COLOR_RED "msgsnd - blad podczas wysylania informacji o grupie" ANSI_COLOR_RESET);
                        exit(1);
                    }
                    sem_post(&data->checkoutspace);
                }
                else
                {
                    printf(ANSI_COLOR_BLUE "Nie znaleziono dostepnej grupy dla klienta %d.\n" ANSI_COLOR_RESET, clientID);
                    msg.mtype=clientID;
                    msg.value=-1;
                    if (msgsnd(data->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) 
                    {
                        perror(ANSI_COLOR_RED "msgsnd - blad podczas wysylania informacji o grupie" ANSI_COLOR_RESET);
                        exit(1);
                    }
                    
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
                    perror(ANSI_COLOR_RED "msgrcv - kasjer odbierajacy szczegoly klienta" ANSI_COLOR_RESET);
                    exit(1);
                }
                if(data->processedCounter>0)
                {
                    data->processedCounter--;
                }
                
                int clientID = msg.pid; // Process exiting queue
                printf(ANSI_COLOR_GREEN "Przetworzono wychodzacego klienta %d\n" ANSI_COLOR_RESET, clientID);
            }
            data->enter_turn = 1;
        }

        sem_post(&data->mutex); // Unlock shared resource
        //sleep(1);
    }
    
    printf(ANSI_COLOR_RED "Limit czasu osiagniety. Kasy czekaja na klientow konczacych trasy.\n" ANSI_COLOR_RESET);
    sem_post(&data->checkoutspace);
    while(data->processedCounter>0)
    {
        sem_wait(&data->mutex);
        if (sem_trywait(&data->exit_sem) == 0) 
            {
                if (msgrcv(data->msqid, &msg, sizeof(pid_t) + sizeof(int), KASA2, 0) == -1) 
                {
                    perror(ANSI_COLOR_RED "msgrcv - kasjer odbierajacy szczegoly klienta" ANSI_COLOR_RESET);
                    exit(1);
                }
                if(data->processedCounter>0)
                {
                    data->processedCounter--;
                }
                
                int clientID = msg.pid; // Process exiting queue
                printf(ANSI_COLOR_GREEN "Przetworzono wychodzacego klienta %d\n" ANSI_COLOR_RESET, clientID);
            }
        sem_post(&data->mutex);
    }
    printf(ANSI_COLOR_RED "Kasjer %d konczy prace.\n" ANSI_COLOR_RESET, getpid());
}

/**
 * Oczekuje na uzupelnienie grupy turystycznej lub uplyniecie limitu czasu.
 *
 * param checkoutdata Wskaznik na dane kasowe.
 * param nr Numer grupy.
 * param Tk Limit czasu oczekiwania.
 * param tourdata Wskaznik na dane turystyczne.
 *
 */
void przewodnikWaiting(CheckoutData* checkoutdata, int nr, time_t Tk, TourData* tourdata)
{
    

    //struct message msg;  // Local message struct


    printf(ANSI_COLOR_WHITE "\tPRZEWODNIK %d CZEKA NA GRUPE\n" ANSI_COLOR_RESET, getpid());
    time_t current_time=time(NULL);
    while((checkoutdata->group_counts[nr]+checkoutdata->group_children[nr]<M+1) && current_time<Tk)
    {
        sleep(1);
        current_time=time(NULL);
    };
    if(checkoutdata->parkClosed && checkoutdata->group_counts[nr]==1)
    {
        printf(ANSI_COLOR_RED "PRZEWODNIK %d KONCZY PRACE\n" ANSI_COLOR_RESET, getpid());
        tourCleanup(tourdata);
        checkoutCleanup(checkoutdata);
        exit(0);
    }
}


/**
 * Czeka na zakonczanie aktywnosci przez grupe turystyczna.
 *
 * param checkoutdata Wskaznik na dane kasowe.
 * param mygroup Numer grupy turystycznej.
 *
 */
void waitingForGroup(CheckoutData* checkoutdata, int mygroup) {
    struct message msg;
    int messagesReceived = 0;

    printf(ANSI_COLOR_WHITE "Przewodnik %d czeka az grupa %d skonczy aktywnosc.\n" ANSI_COLOR_RESET, getpid(), mygroup);

    // Wait for all tourists in the group to notify the przewodnik
    while (messagesReceived < checkoutdata->group_counts[mygroup]-1) {
        // Wait for a message from a turysta in the group
        if (msgrcv(checkoutdata->msqid, &msg, sizeof(pid_t) + sizeof(int), getpid(), 0) == -1) {
            perror("msgrcv - przewodnik waiting for group");
            exit(1);
        }
        messagesReceived++;

    }

    printf(ANSI_COLOR_WHITE "Przewodnik %d otrzymal wszystkie wiadomosci od grupy %d. Rozpoczyna droge do kolejnej atrakcji.\n" ANSI_COLOR_RESET, getpid(), mygroup);

    // Simulate travel with a delay
    srand((getpid()));
    sleep((int)((rand() % 8 + 1) * (checkoutdata->group_children[mygroup] > 0 ? 1.5 : 1))); 


    for(int i=0;i<checkoutdata->group_counts[mygroup]-1;i++)
    {
        msg.mtype = checkoutdata->groups[mygroup][i+1];
        if (msgsnd(checkoutdata->msqid, &msg, sizeof(pid_t) + sizeof(int), 0) == -1) {
        perror("msgsnd - przewodnik notifying group arrival");
        exit(1);
    }
    }

    

    printf(ANSI_COLOR_WHITE "Przewodnik %d poinformowal grupe %d o dotarciu na miejsce.\n" ANSI_COLOR_RESET, getpid(), mygroup);
}




/**
 * Obsluguje przejscie turysty przez most.
 *
 * param data Wskaznik na dane turystyczne.
 * param tourist_id ID turysty.
 * param trasa Numer trasy.
 * param group_id ID grupy.
 * param children_count Liczba dzieci dolaczajacych do turysty.
 *
 */
void most(TourData* data, int tourist_id, int trasa, int group_id, int children_count) 
{
    srand((getpid()));
    int mostTime = rand() % 5 + 1; // Domyslny czas przejscia przez most

    if (trasa == 1)
    {
        for (int i = 0; i < children_count + 1; i++) 
        {
            sem_wait(&data->mostSpots1);
            data->mostWaiting1++;
        }

        if (data->mostWaiting2 == 0 && data->mostSide != 1)
        {
            data->mostSide = 1;
        }

        while (data->mostSide != 1 && data->mostCounter2 > 0);

        for (int i = 0; i < children_count + 1; i++) 
        {
            data->mostCounter1++;
        }

        printf(ANSI_COLOR_BLUE "Turysta %d z grupy %d przechodzi przez most.\n" ANSI_COLOR_RESET, tourist_id, group_id);
        sleep(mostTime);
        printf(ANSI_COLOR_BLUE "Turysta %d z grupy %d zakonczyl przejscie przez most.\n" ANSI_COLOR_RESET, tourist_id, group_id);

        if (data->mostWaiting2 > 0 && data->mostSide != 2)
        {
            data->mostSide = 2;
        }

        for (int i = 0; i < children_count + 1; i++) 
        {
            data->mostCounter1--;
            data->mostWaiting1--;
            sem_post(&data->mostSpots1);
        }
    }
    else if (trasa == 2)
    {
        for (int i = 0; i < children_count + 1; i++) 
        {
            sem_wait(&data->mostSpots2);
            data->mostWaiting2++;
        }

        if (data->mostWaiting1 == 0 && data->mostSide != 2)
        {
            data->mostSide = 2;
        }

        while (data->mostSide != 2 && data->mostCounter1 > 0);

        for (int i = 0; i < children_count + 1; i++) 
        {
            data->mostCounter2++;
        }

        printf(ANSI_COLOR_BLUE "Turysta %d z grupy %d przechodzi przez most.\n" ANSI_COLOR_RESET, tourist_id, group_id);
        sleep(mostTime);
        printf(ANSI_COLOR_BLUE "Turysta %d z grupy %d zakonczyl przejscie przez most.\n" ANSI_COLOR_RESET, tourist_id, group_id);

        if (data->mostWaiting1 > 0 && data->mostSide != 1)
        {
            data->mostSide = 1;
        }

        for (int i = 0; i < children_count + 1; i++) 
        {
            data->mostCounter2--;
            data->mostWaiting2--;
            sem_post(&data->mostSpots2);
        }
    }

    return;
}


/**
 * Obsluguje wejscie turysty na wieze.
 *
 * param data Wskaznik na dane turystyczne.
 * param tourist_id ID turysty.
 * param group_id ID grupy.
 * param children_count Liczba dzieci dolaczajacych do turysty.
 *
 */
void wieza(TourData* data, int tourist_id, int group_id, int children_count) 
{
    sem_wait(&data->wiezaSpots);  // Semaphore for wieza spot
    for (int i = 0; i < children_count; i++) 
    {
        sem_wait(&data->wiezaSpots);
    }

    srand((getpid()));
    int wiezaTime = rand() % 5 + 1;  // Random time between 1 and 5

    // Simulate wieza activity
    printf(ANSI_COLOR_YELLOW "Turysta %d z grupy %d spedza czas przy wiezy\n" ANSI_COLOR_RESET, tourist_id, group_id);
    sleep(wiezaTime);
    printf(ANSI_COLOR_GREEN "Turysta %d z grupy %d zakonczyl pobyt przy wiezy\n" ANSI_COLOR_RESET, tourist_id, group_id);

    for (int i = 0; i < children_count; i++) 
    {
        sem_post(&data->wiezaSpots);
    }
    // Release the semaphore
    sem_post(&data->wiezaSpots);
    return;
}

/**
 * Obsluguje przeplywanie promem.
 *
 * param data Wskaznik na dane turystyczne.
 * param tourist_id ID turysty.
 * param trasa Numer trasy.
 * param group_id ID grupy.
 * param children_count Liczba dzieci dolaczajacych do turysty.
 *
 */
void prom(TourData* data, int tourist_id, int group_id, int children_count, int trasa)
{
    if(trasa==1)
    {
        
        for(int i=0;i<children_count+1;i++)
        {
            sem_wait(&data->promSpots1);
        }
        data->promWaiting1++;
        sem_wait(&data->promMutex);
        if(data->promSide!=1 && data->promWaiting2==0 && data->promCounter2==0)
        {
            printf(ANSI_COLOR_YELLOW "Turysta %d z grupy %d wzywa prom na strone 1.\n" ANSI_COLOR_RESET, tourist_id, group_id);
            data->promSide=1;
            sleep(PROM);
        }
        sem_post(&data->promMutex);
        
        for(int i=0;i<children_count+1;i++)
        {
            data->promCounter1++;
        }
        data->promWaiting1--;
        while(data->promWaiting1>0 && data->promCounter1<X3);

        printf(ANSI_COLOR_MAGENTA "Turysta %d z grupy %d plynie promem.\n" ANSI_COLOR_RESET, tourist_id, group_id);

        data->prom = 1;

        sleep(PROM);

        printf(ANSI_COLOR_MAGENTA "Turysta %d z grupy %d zakonczyl plyniecie promem.\n" ANSI_COLOR_RESET, tourist_id, group_id);

        data->prom=0;

        data->promSide=2;

        for(int i=0;i<children_count+1;i++)
        {
            data->promCounter2--;
            sem_post(&data->promSpots1);
        }
    }
    else
    {
        
        for(int i=0;i<children_count+1;i++)
        {
            sem_wait(&data->promSpots2);
        }
        data->promWaiting2++;
        sem_wait(&data->promMutex);
        if(data->promSide!=2 && data->promWaiting1==0 && data->promCounter1==0)
        {
            printf(ANSI_COLOR_YELLOW "Turysta %d z grupy %d wzywa prom na strone 1.\n" ANSI_COLOR_RESET, tourist_id, group_id);
            data->promSide=2;
            sleep(PROM);
        }
        sem_post(&data->promMutex);
        
        for(int i=0;i<children_count+1;i++)
        {
            data->promCounter2++;
        }
        data->promWaiting2--;
        while(data->promWaiting2>0 && data->promCounter2<X3);

        printf(ANSI_COLOR_MAGENTA "Turysta %d z grupy %d plynie promem.\n" ANSI_COLOR_RESET, tourist_id, group_id);

        data->prom = 1;

        sleep(PROM);

        printf(ANSI_COLOR_MAGENTA "Turysta %d z grupy %d zakonczyl plyniecie promem.\n" ANSI_COLOR_RESET, tourist_id, group_id);

        data->prom=0;

        data->promSide=1;

        for(int i=0;i<children_count+1;i++)
        {
            data->promCounter2--;
            sem_post(&data->promSpots2);
        }
    }
}