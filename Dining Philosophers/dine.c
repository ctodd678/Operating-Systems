#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>

#define MAX_THREADS 50

void initSemaphores(sem_t *m, sem_t **s, int n);
void *philosopher (void *);
void pickup(int);
void putdown(int);
void test(int);
void eat();
void think();
int checkDone();
void exitProgram();

//DINING PHILOSOPHERS IN C (Connor Todd)

pthread_mutex_t mutex;
sem_t **semaphores;
int philosophers[1000];
int philState[1000]; //* 0 for thinking, 1 for hungry, 2 for eating, -1 for done
int philEatCount[1000];
int nPhils;

int main(int argc, char *argv[]) {

    int i;
    if(argc > 3) {
        printf("ERROR! Too many arguments entered.\n");
        printf("Example args: './dine 3 2'\n");
        printf("Exiting...\n");
        exit(1);
    }

    int nPhilosophers = atoi(argv[1]);
    int nEat = atoi(argv[2]);

    if(nPhilosophers == 0 || nEat == 0 || nPhilosophers < 2) {
        printf("ERROR! Incorrect number of philosphers or number of eats entered.\n");
        printf("Example args: './dine 3 2'\n");
        printf("Exiting...\n");
        exit(1);
    }

    pthread_t t[nPhilosophers];
    nPhils = nPhilosophers;

    srand(time(NULL));


    pthread_mutex_init(&mutex, NULL);

    semaphores = malloc(sizeof(sem_t*) * nPhilosophers);

    for(i = 0; i < nPhilosophers; i++) {
        semaphores[i] = malloc(sizeof(sem_t));
        sem_init(semaphores[i], 0, 1);
    }

    //initSemaphores(&mutex, semaphores, nPhilosophers);


    for(i = 0; i < nPhilosophers; i++) {
        philosophers[i] = i;
        philState[i] = 0;
        philEatCount[i] = nEat;
    }

    for(i = 0; i < nPhilosophers; i++) {
        pthread_create(&t[i], NULL, philosopher, &philosophers[i]);
        printf("Philosopher %d thinking\n", i + 1);
    }


    for(i = 0; i < nPhilosophers; i++) {
        pthread_join(t[i], NULL);
    }

    printf("\n");
    return 0;
}


//*FUNCTIONS

void initSemaphores(sem_t *m, sem_t **s, int n) {

    int i = 0;

    sem_init(m, 0, 1);

    s = malloc(sizeof(sem_t*) * n);

    for(i = 0; i < n; i++) {
        s[i] = malloc(sizeof(sem_t));
        sem_init(s[i], 0, 0);
    }
}

void *philosopher (void *i) {

    while(1) {
        int *tI = i;

        think();
        pickup(*tI);
        eat();
        putdown(*tI);
    }
}

void pickup(int i) {
    

    pthread_mutex_lock(&mutex);   //*GRAB MUTEX LOCK

    philState[i] = 1;

    //printf("Philosopher %d hungry\n" , i + 1);

    test(i);
    
    pthread_mutex_unlock(&mutex);   //*RELEASE MUTEX LOCK

    sem_wait(semaphores[i]);    //*MAKE CURRENT SEMAPHORE WAIT
    
    //sleep(1);
}

void putdown(int i) {
    
    pthread_mutex_lock(&mutex);

    philState[i] = 0;

    printf("Philosopher %d thinking\n" , i + 1);

    test((i + 4) % nPhils);
    test((i + 1) % nPhils);

    pthread_mutex_unlock(&mutex);
}

void test(int i) {
    if(philEatCount[i] <= 0) {
        philState[i] = -1;
        //printf("Philosopher %d DONE\n", i + 1); 
        sem_destroy(semaphores[i]);
        if(i == nPhils)
            i--;
        else
            i++;
    }

    if(checkDone() == 1) {
        exitProgram();
    }
        
    //printf("philEatCount: %d\n", philEatCount[i]);

    if (philState[i] == 1 
        && philState[(i + 4) % nPhils] != 2 
        && philState[(i + 1) % nPhils] != 2) { 
        
        philState[i] = 2; 
        philEatCount[i]--;

        eat();
        printf("Philosopher %d eating\n", i + 1); 

        sem_post(semaphores[i]); 
    } 
}

int checkDone() {

    for(int i = 0; i < nPhils; i++)
        if(philEatCount[i] > 0)
            return -1;

    return 1;
}

void exitProgram() {
    //*FREE EVERYTHING AND EXIT
    printf("Exiting...\n");

    pthread_mutex_destroy(&mutex);

    exit(1);
}

void eat() {
    sleep(rand() % 3);
}

void think() {
    sleep(rand() % 3);
}
