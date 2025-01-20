#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>


// przewodnicy musza czekac az grupa, ktora jest w trakcie zapelniania sie wystartuje wycieczke, zanim zaczna wypelniac swoja grupe
// kasjerzy musza miec mozliwosc przyjmowac sygnal(informacje) kiedy obslugiwac ktora kolejke
// prom prawdopodobnie na dwoch semaforach


// typy wiadomosci dla przewodnikow, kasjerow ??
// #define PUSTY 1
// #define PELNY 2
// #define WOLNY 3
// #define ZAJETY 4

#define N 30
#define VIP N/20
#define M 10
#define P 1
#define K 10
#define X1 7 // most
#define X2 15 // wieza
#define X3 12 // prom


// Shared memory structure
typedef struct {

	int enter_queue[N];  // Queue for entering clients
    int exit_queue[N];   // Queue for exiting clients
    int enter_head, enter_tail; // Head and tail for entering queue
    int exit_head, exit_tail;   // Head and tail for exiting queue

	int groups[P][M+1]; // Groups assigned to przewodnik, na pozycji 0 pid przewodnika
    int group_counts[P];      // Count of clients in each group
    int group_active[P];      // Status of each group (1 = active, 0 = inactive)
	
	int initialized;

    int enter_turn;  // Flag to alternate between queues (1 = enter, 0 = exit)

	int msqid;

    sem_t mutex;     // Protects access to the shared data
    sem_t enter_sem; // Tracks the number of clients in the enter queue
    sem_t exit_sem;  // Tracks the number of clients in the exit queue
    //sem_t group_ready[P]; 
	sem_t working;
	
} CheckoutData;

typedef struct {

	int most_queue1[N];
	int most_queue2[N];
	int wieza_queue[N];
	int prom_queue1[N];
	int prom_queue2[N];

	int most_group[X1];
	int wieza_group[X2];
	int prom_group[X3];


	sem_t mutexMost;
	sem_t mutexWieza;
	sem_t mutexProm;
    sem_t group_ready[P]; // Signal when a specific group is ready
} TourData;


struct message {
    long mtype;
    pid_t pid;
    int value;
};


CheckoutData *checkoutSetupShm();

TourData *tourSetupShm();

int leavequeue(CheckoutData* data,int qNumber);

void enterqueue(CheckoutData* data,int qNumber, int tourist);

int checkgroups(int people, CheckoutData* data);

// int whatgroup(int clientID, CheckoutData* data);

void processClients(CheckoutData* data); // kasjer function

void przewodnikWaiting(CheckoutData* checkoutdata, int nr);

// void waitingforgroup();

// void startTour(); // for przewodnik

// void endTour();

// void nextSpot(); // for przewodnik

// void wieza();

// void most();

// void prom();