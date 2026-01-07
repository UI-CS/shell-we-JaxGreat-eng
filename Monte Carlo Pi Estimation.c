#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>
#include <math.h>

typedef struct {
    long points_in_circle;
    long total_points;
} SharedData;

double random_double() {
    return rand() / (double)RAND_MAX;
}

void monte_carlo_worker(long points_per_process, long *local_in_circle) {
    *local_in_circle = 0;
    
    unsigned int seed = time(NULL) ^ getpid();
    
    for (long i = 0; i < points_per_process; i++) {
        double x = random_double();  // 0 to 1
        double y = random_double();  // 0 to 1
        
        if (x * x + y * y <= 1.0) {
            (*local_in_circle)++;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <num_processes> <total_points>\n", argv[0]);
        printf("Example: %s 4 1000000\n", argv[0]);
        return 1;
    }
    
    int num_processes = atoi(argv[1]);
    long total_points = atol(argv[2]);
    
    if (num_processes <= 0 || total_points <= 0) {
        printf("Both arguments must be positive integers\n");
        return 1;
    }
    
    long points_per_process = total_points / num_processes;
    total_points = points_per_process * num_processes; // Adjust total
    
    SharedData *shared = mmap(NULL, sizeof(SharedData),
                             PROT_READ | PROT_WRITE,
                             MAP_SHARED | MAP_ANONYMOUS,
                             -1, 0);
    
    if (shared == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    
    shared->points_in_circle = 0;
    shared->total_points = total_points;
    

    printf("Processes: %d\n", num_processes);
    printf("Points per process: %ld\n", points_per_process);
    printf("Total points: %ld\n", shared->total_points);
    
    
    clock_t start = clock();
    
    for (int i = 0; i < num_processes; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork failed");
            return 1;
        }
        
        if (pid == 0) {
            long local_in_circle = 0;
            monte_carlo_worker(points_per_process, &local_in_circle);
            

            __sync_fetch_and_add(&shared->points_in_circle, local_in_circle);
            
            exit(0);
        }
    }
    

    for (int i = 0; i < num_processes; i++) {
        wait(NULL);
    }
    

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    

    double pi_estimate = 4.0 * shared->points_in_circle / shared->total_points;
    

    double error = fabs(pi_estimate - M_PI);
    double error_percent = (error / M_PI) * 100.0;
    
    
    printf("Points in circle: %ld / %ld\n", 
           shared->points_in_circle, shared->total_points);
    printf("Ratio: %.10f\n", 
           (double)shared->points_in_circle / shared->total_points);
    printf("\n");
    printf("Estimated π: %.10f\n", pi_estimate);
    printf("Actual π:    %.10f\n", M_PI);
    printf("\n");
    printf("Absolute error:   %.10f\n", error);
    printf("Percent error:    %.6f%%\n", error_percent);
    printf("\n");
    printf("Performance:\n");
    printf("Time elapsed:     %.3f seconds\n", elapsed);
    printf("Points/second:    %.0f\n", total_points / elapsed);
    
    

    munmap(shared, sizeof(SharedData));
    
    return 0;
}