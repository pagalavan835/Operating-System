#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

struct CircularQueue {
    int in;
    int buffer[10];
};

int main() {
    int out = 0, num, si;
    int size = 10;
    int i;

    // Use IPC_PRIVATE instead of a hardcoded key to avoid collisions
    si = shmget(IPC_PRIVATE, sizeof(struct CircularQueue), IPC_CREAT | 0600);
    if (si == -1) {
        perror("Shmget");
        exit(1);
    }
    
    struct CircularQueue *queue = (struct CircularQueue *)shmat(si, NULL, 0);
    if (queue == (void *)-1) { // Fixed cast to void* for cleaner C standard compliance
        perror("shmat");
        // Clean up the segment if attachment fails
        shmctl(si, IPC_RMID, NULL);
        exit(1);
    }

    queue->in = 0;

    for (i = 0; i < size; i++) {
        printf("Enter the data: ");
        
        if (scanf("%d", &num) != 1) {
            printf("Invalid input! Please enter a valid number.\n");
            while (getchar() != '\n'); 
            i--; 
            continue;
        }
        
        queue->buffer[queue->in] = num;
        queue->in = (queue->in + 1) % size;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        shmdt(queue);
        shmctl(si, IPC_RMID, NULL);
        exit(1);
    } 
    else if (pid > 0) {
        // Parent: Wait for the child to finish reading FIRST
        wait(NULL); 
        // Now it is safe to detach and remove memory
        shmdt(queue); 
        shmctl(si, IPC_RMID, NULL); 
    } 
    else {
        // Child: Reads data safely because parent is waiting
        printf("\nData from Shared Memory:\n");
        for (i = 0; i < size; i++) {
            num = queue->buffer[out];
            printf("Data [%d]: %d\n", out + 1, num);
            out = (out + 1) % size;
        }
        shmdt(queue); 
        exit(0); // Explicitly exit child to prevent it from running parent cleanup
    }
    return 0;
}

