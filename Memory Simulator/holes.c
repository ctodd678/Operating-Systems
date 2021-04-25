#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define MAX_MEM 128
#define MAX_P 27    //*26 LETTERS IN ALPHABET, MAX NUMBER OF PROCESSES

typedef struct Process {
    char id;
    int size;
    int swaps;
    int timeInMem;
}Process;

void        createProcessQueue      (FILE *, Process *, int *);
void        printProcess            (Process *);
int         findSwapIndex           (Process *m, int, char);
void        insertProcess           (Process *, Process, int);
void        removeProcess           (Process *, Process, int);
void        removeProcessFromPos    (Process *arr, int pos, int size);
int         loadProcess             (Process, char *, int);
void        unloadProcess           (Process, char *);
int         fitInMemory             (char *, int, int);
int         fitInMemoryBest         (char *, int, int);
int         fitInMemoryNext         (char *, int, int, int);
int         fitInMemoryWorst        (char *, int, int);
int         countHoles              (char *);

//Memory Simulator (Connor Todd)

int main(int argc, char *argv[]) {

    FILE *fp;
    int mode = 2; //* 0 for first (default), 1 for best, 2 for next, 3 for worst
    int nProcesses = 0;
    int totalMem = 128;
    int currentMem = 128;
    int loadedMem = 0;
    int i = 0;
    int holes = 1; 
    int loads = 0;
    int loadedProcs = 0;
    int pSize = 0;                  //*SIZE OF PROCESS (QUEUE) ARRAY
    int memSize = 0;             //*SIZE OF MEM ARRAY
    int totalSwaps = 0;
    int lastFit = 0;
    double memUsage = 0;
    double cumulativeMem = 0;
    double averageHoles = 0;
    double averageProcs = 0;

    //*INPUT ERROR HANDLING
    if(argc > 3) {
        printf("ERROR! Too many arguments entered!\n");
        printf("Example args: './holes testfile.txt first'\n");
        printf("Exiting...\n");
        exit(1);
    }
    else if(argc < 3) {
        printf("ERROR! Not enough arguments entered!\n");
        printf("Example args: './holes testfile.txt first'\n");
        printf("Exiting...\n");
        exit(1);
    }

    //*CHECK MODE
    if(strcmp(argv[argc - 1], "first") == 0) {
        mode = 0;
    }
    else if(strcmp(argv[argc - 1], "best") == 0) {
        mode = 1;
    }
    else if(strcmp(argv[argc - 1], "next") == 0) {
        mode = 2;
    }
    else if(strcmp(argv[argc - 1], "worst") == 0) {
        mode = 3;
    }
    else {
        mode = 0;
    }

    //*CHECK IF VALID FILE & OPEN
    if((fp = fopen(argv[1], "r")) == NULL) {
        printf("ERROR! Invalid file entered!\n");
        printf("Please try again with a valid file name.\n");
        printf("Exiting...\n");
        exit(1);
    }

    fp = fopen("testfile.txt", "r");

    //*MEMORY ARRAY, PROCESS QUEUE AND ALLOCATED
    char mem[MAX_MEM + 1];
    Process p[MAX_P];
    Process memory[MAX_P];

    for(i = 0; i < MAX_MEM; i++)
        mem[i] = '-';
    
    mem[MAX_MEM] = '\0';

    for(i = 0; i < MAX_P; i++) {
        memory[i].id = '\0';
        memory[i].size = 0;
    }

    createProcessQueue(fp, p, &pSize);
    nProcesses = pSize - 1;

    //*START ALLOCATION
    char swapId[MAX_P] = {'\0'};
    
    if(mode == 0)
        printf("\t\t\t~~~FIRST FIT ALLOCATION~~~\n");
    else if(mode == 1) 
        printf("\t\t\t~~~BEST FIT ALLOCATION~~~\n");
    else if(mode == 2) 
        printf("\t\t\t~~~NEXT FIT ALLOCATION~~~\n");
    else if(mode == 3) 
        printf("\t\t\t~~~WORST FIT ALLOCATION~~~\n");

    int z = 0;
    int l = 0;
    
    while(1) {

        //*ALLOCATE PROCESS IN MEMORY
        l = 0;

        if(loads >= (nProcesses) * 3) {
            break;
        }

        //*CHECK IF BIG ENOUGH HOLE
        if(mode == 0) {         //*FIRST FIT
            l = fitInMemory(mem, p[0].size, holes);
        }
        else if(mode == 1) {    //*BEST FIT
            l = fitInMemoryBest(mem, p[0].size, holes);
        }
        else if(mode == 2) {    //*NEXT FIT
            l = fitInMemoryNext(mem, p[0].size, holes, lastFit);
            if(l == -1) {
                l = fitInMemoryNext(mem, p[0].size, holes, 0);
            }
        }
        else if(mode == 3) {     //*WORST FIT
            l = fitInMemoryWorst(mem, p[0].size, holes);
        }
        if(l != -1) {
            lastFit = l;

            loadProcess(p[0], mem, l); //load process into memory array
            insertProcess(memory, p[0], memSize + 1); //add process to memory queue

            currentMem = currentMem - p[0].size;
            loadedMem = loadedMem + p[0].size;

            pSize--;
            memSize++;

            swapId[loadedProcs] = p[0].id;

            loadedProcs++;
            loads++;
            averageProcs = averageProcs + loadedProcs;
            memUsage = ((double)loadedMem / 128) * 100;
            cumulativeMem = cumulativeMem + memUsage;
            holes = countHoles(mem);
            averageHoles = averageHoles + holes;

            printf("%c loaded, #processes = %d, #holes = %d, %%memusage = %.0f, cumulative %%mem = %.0f\n", 
            p[0].id , loadedProcs, holes, memUsage, cumulativeMem / loads);

            removeProcess(p, p[0], pSize); //remove process from queue
        }
        else {  //*ELSE REMOVE PROCESS FROM MEMORY
            
            Process *temp;
            z = findSwapIndex(memory, memSize, swapId[0]);


            if(z == -1) {
                printf("Something went wrong...\n");
                exit(1);
            }

            for(i = 0; i < MAX_P - 1; i++) {  //SHIFT SWAP PRIORITY
                swapId[i] = swapId[i + 1];
            }

            memory[z].swaps++;
            insertProcess(p, memory[z], pSize);
            pSize++;

            loadedMem = loadedMem - memory[z].size;
            currentMem = currentMem + memory[z].size;

            unloadProcess(memory[z], mem); //unload process from memory array
            removeProcessFromPos(memory, z, memSize); //remove process from memory queue

            memSize--;
            loadedProcs--;
            holes++;
        }
    }

    averageProcs = averageProcs / (double)loads;
    averageHoles = averageHoles / (double)loads;


    printf("Total loads = %d, average #processes = %.2f, average #holes = %.1f, cummulative %%mem = %.0f\n", 
    loads, averageProcs, averageHoles, cumulativeMem / (double)loads );

    printf("\n");
    return 0;
}


//*FUNCTIONS


//*PARSE THE FILE INTO AN ARRAY OF PRORCESSES
void createProcessQueue(FILE *f, Process *p, int *nProc) {

    int ret_val;
    int i = 0;

    while(ret_val != EOF) {

        if(i >= MAX_P)
            break;

        ret_val = fscanf(f, "%c %d\n", &p[i].id, &p[i].size);
        if(p[i].size < 1)
            p[i].size = 1;
        else if(p[i].size > 128)
            p[i].size = 128;
        p[i].swaps = 0;
        p[i].timeInMem = 0;

        i++;
    }

    *nProc = i;
    while(i < MAX_P) {
        p[i].id = '\0';
        p[i].size = 0;
        p[i].swaps = 0;
        p[i].timeInMem = 0;
        i++;
    }
}

//*INSERT PROCESS INTO ARRAY
void insertProcess(Process *in, Process out, int pos) {

    for(int i = pos - 1; i >= pos; i--) {
        in[i] = in[i - 1];
    }

    in[pos - 1] = out;
}

//*REMOVES PROCESS FROM ARRAY
void removeProcess(Process *arr, Process rem, int size) {

    int i = 0;
    int pos = 0;

    for(i = 0; i < size; i++) {
        if(arr->id == rem.id) {
            pos = i;
            break;
        }
    }

    for(i = pos - 1; i < size - 1; i++) {
        arr[i] = arr[i + 1];
    }
}

//*REMOVES PROCESS FROM ARRAY AT POSITION
void removeProcessFromPos(Process *arr, int pos, int size) {

    int i = 0;

    for(i = pos - 1; i < size - 1; i++) {
        arr[i] = arr[i + 1];
    }
}

//*FIND IF PROCESS WILL FIT IN MEMORY (BEST FIT)
int fitInMemory(char *memArr, int bytes, int h) {

    int i = 0, j = 0;
    int holeIndex = -1;
    int space = 0;
    int lastSpace = 0;
    int lastIndex = 0;

    for(i = 0; i < h; i++) {
        lastIndex = holeIndex;
        for(; j < 128; j++) {
            if(memArr[j] == '-') {
                holeIndex = j;
                break;
            }
        }
    
        if(holeIndex != -1) {
            while(memArr[j] == '-' && j < 128) {
                j++;
            }
        }
        else {
            return -1;
        }

        lastSpace = space;
        space = j - holeIndex;

        if(bytes <= space)
            return holeIndex;
        else
            continue;
    }

    return -1; //return -1 if process doesn't fit in memory
}

//*FIND IF PROCESS WILL FIT IN MEMORY (NEXT FIT)
int fitInMemoryNext(char *memArr, int bytes, int h, int last) { 
    
    int i = last, j = 0;
    int holeIndex = -1;
    int space = 0;
    int lastSpace = 0;
    int lastIndex = 0;

    for(; i < h; i++) {
        lastIndex = holeIndex;
        for(; j < 128; j++) {
            if(memArr[j] == '-') {
                holeIndex = j;
                break;
            }
        }
    
        if(holeIndex != -1) {
            while(memArr[j] == '-' && j < 128) {
                j++;
            }
        }
        else {
            return -1;
        }

        lastSpace = space;
        space = j - holeIndex;

        if(bytes <= space)
            return holeIndex;
        else
            continue;
    }

    return -1; //return -1 if process doesn't fit in memory
}

//*FIND IF PROCESS WILL FIT IN MEMORY (BEST FIT)
int fitInMemoryBest(char *memArr, int bytes, int h) { 
    
    int i = 0, j = 0;
    int holeIndex = -1;
    int space = 0;
    int lastSpace = 0;
    int lastIndex = 0;

    for(i = 0; i < h; i++) {
        lastIndex = holeIndex;
        for(; j < 128; j++) {
            if(memArr[j] == '-') {
                holeIndex = j;
                break;
            }
        }
    
        if(holeIndex != -1) {
            while(memArr[j] == '-' && j < 128) {
                j++;
            }
        }
        else {
            return -1;
        }

        lastSpace = space;
        space = j - holeIndex;

        if(lastSpace > 0 && (space - bytes < (lastSpace - bytes)) && bytes <= (space)) {
            space = j - holeIndex;
        }
        else if (lastSpace > 0 && bytes <= lastSpace) {
            holeIndex = lastIndex;
            space = lastSpace;
        }
    }

    if(bytes <= space)
        return holeIndex;

    return -1;
}

//*FIND IF PROCESS WILL FIT IN MEMORY (WORST FIT)
int fitInMemoryWorst(char *memArr, int bytes, int h) { 
    
    int i = 0, j = 0;
    int holeIndex = -1;
    int space = 0;
    int lastSpace = 0;
    int lastIndex = 0;

    for(i = 0; i < h; i++) {
        lastIndex = holeIndex;
        for(; j < 128; j++) {
            if(memArr[j] == '-') {
                holeIndex = j;
                break;
            }
        }
    
        if(holeIndex != -1) {
            while(memArr[j] == '-' && j < 128) {
                j++;
            }
        }
        else {
            return -1;
        }

        lastSpace = space;
        space = j - holeIndex;

        if(lastSpace > 0 && (space - bytes > (lastSpace - bytes)) && bytes <= (space)) {
            space = j - holeIndex;
        }
        else if (lastSpace > 0 && bytes <= lastSpace) {
            holeIndex = lastIndex;
            space = lastSpace;
        }
    }

    if(bytes <= space)
        return holeIndex;

    return -1;  //return -1 if process doesn't fit in memory
}

//*GETS NUMBER OF HOLES IN MEMORY
int countHoles(char *memArr) {

    int numHoles = 0;
    int hole = 0;
    int i = 0;

    while(memArr[i] != '\0' && i < 128) {

        if(memArr[i] == '-' && hole == 0) {
            numHoles++;
            hole = 1;
        }
        else if(memArr[i] != '-' && hole == 1) {
            hole = 0;
        }
        i++;
    }

    return numHoles;
}

//*LOADS PROCESS INTO MEMORY ARRAY
int loadProcess(Process p, char *memArr, int holeI) {

    int i = 0;

    if(holeI != -1) {
        for(i = holeI; i < p.size + holeI; i++) {
            if(i >= 128)
                break;
            memArr[i] = p.id;
        }
    }
    else {
        return -1;
    }

    return 1;
}

//*REMOVES PROCESS FROM MEMORY ARRAY
void unloadProcess(Process p, char *memArr) {
    int i = 0;

    while(memArr[i] != p.id) {
        i++;
    }
    while(memArr[i] == p.id) {
        memArr[i] = '-';
        i++;
    }
}

//*PRINT PROCESS (HELPER FUNCTION)
void printProcess(Process *pArr) {

    for(int i = 0; i < 26; i++) {
        if(pArr[i].id == '\0')
            continue;
    }
}

//*FINDS THE INDEX OF THE PROCESS TO SWAP
int findSwapIndex(Process *m, int size, char swapId) {

    int i = 0;
    int swapIndex = -1;

    for(; i < size; i++)
        if(m[i].id == swapId)
            return i;

    return swapIndex;
}  