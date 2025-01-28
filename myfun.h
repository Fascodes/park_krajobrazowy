#include <stdio.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdbool.h>



// przewodnicy musza czekac az grupa, ktora jest w trakcie zapelniania sie wystartuje wycieczke, zanim zaczna wypelniac swoja grupe
// kasjerzy musza miec mozliwosc przyjmowac sygnal(informacje) kiedy obslugiwac ktora kolejke
// prom prawdopodobnie na dwoch semaforach


// typy wiadomosci dla przewodnikow, kasjerow ??
// #define PUSTY 1
// #define PELNY 2
// #define WOLNY 3
// #define ZAJETY 4

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_RESET "\x1b[0m"


#define N 350
#define VIP N/20
#define M 10
#define P 1
#define K 1
#define X1 7 // most
#define X2 15 // wieza
#define X3 12 // prom
#define KASA1 1
#define KASA2 2
#define PROM 10
#define PARK 70

// Shared memory structure
typedef struct {

    int enter_head, enter_tail; // Head and tail for entering queue
    int exit_head, exit_tail;   // Head and tail for exiting queue

	int groups[P][M+1]; // Groups assigned to przewodnik, na pozycji 0 pid przewodnika
    int group_counts[P];      // Count of clients in each group
	int group_children[P];
    int group_active[P];      // Status of each group (1 = active, 0 = inactive)
	
	int initialized;

    int enter_turn;  // Flag to alternate between queues (1 = enter, 0 = exit)

	int msqid;

	int processedCounter;

	bool parkClosed;

	int connected;

    sem_t mutex;     // Protects access to the shared data
    sem_t enter_sem; // Tracks the number of clients in the enter queue
    sem_t exit_sem;  // Tracks the number of clients in the exit queue
    //sem_t group_ready[P]; 
	sem_t checkoutspace;
	sem_t working;
	sem_t cleanupMutex;
	
} CheckoutData;

typedef struct {

	int connected;

	int prom; 
	int promCounter1;
	int promCounter2;
	int promWaiting1;
	int promWaiting2;
	int initialized;
	int msqid;
	int mostCounter1;
	int mostCounter2;
	int mostSide;
	int mostWaiting1;
	int mostWaiting2;
	int promSide;

	

	
	sem_t mostSpots1;
	sem_t mostSpots2;
	sem_t wiezaSpots;
	sem_t promSpots1;
	sem_t promSpots2;
	sem_t promMutex;
	sem_t cleanupMutex;

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

void processClients(CheckoutData* data); // kasjer function

void przewodnikWaiting(CheckoutData* checkoutdata, int nr, time_t Tk, TourData* tourdata);

void prom(TourData* data, int tourist_id, int group_id, int children_count, int trasa);

void wieza(TourData* data, int tourist_id, int group_id, int children_count);

void most(TourData* data, int tourist_id, int trasa, int group_id, int children_count) ;

void waitingForGroup(CheckoutData* checkoutdata, int mygroup);

void tourCleanup(TourData* data);

void checkoutCleanup(CheckoutData* data);




// void startTour(); // for przewodnik

// void endTour();
