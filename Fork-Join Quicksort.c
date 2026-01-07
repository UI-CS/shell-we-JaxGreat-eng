#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <time.h>
#include <string.h>

#define THRESHOLD 10000  // Switch to insertion sort for small arrays

void insertion_sort(int *arr, int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        int key = arr[i];
        int j = i - 1;
        
        while (j >= left && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

int partition(int *arr, int left, int right) {
    int mid = left + (right - left) / 2;
    
    if (arr[left] > arr[mid]) {
        int temp = arr[left];
        arr[left] = arr[mid];
        arr[mid] = temp;
    }
    if (arr[left] > arr[right]) {
        int temp = arr[left];
        arr[left] = arr[right];
        arr[right] = temp;
    }
    if (arr[mid] > arr[right]) {
        int temp = arr[mid];
        arr[mid] = arr[right];
        arr[right] = temp;
    }
    
    int pivot = arr[mid];
    arr[mid] = arr[right - 1];
    arr[right - 1] = pivot;
    
    int i = left;
    int j = right - 1;
    
    while (1) {
        while (arr[++i] < pivot);
        while (arr[--j] > pivot);
        
        if (i >= j) break;
        
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
    
    arr[right - 1] = arr[i];
    arr[i] = pivot;
    
    return i;
}

void parallel_quicksort(int *arr, int left, int right, int depth) {
    if (right - left < THRESHOLD) {
        insertion_sort(arr, left, right);
        return;
    }
    
    if (left < right) {
        int pivot_index = partition(arr, left, right);
        
        if (depth > 0) {
            pid_t pid = fork();
            
            if (pid < 0) {                // Fork failed, do sequential
                parallel_quicksort(arr, left, pivot_index - 1, 0);
                parallel_quicksort(arr, pivot_index + 1, right, 0);
            } else if (pid == 0) {
                parallel_quicksort(arr, left, pivot_index - 1, depth - 1);
                exit(0);
            } else {
                parallel_quicksort(arr, pivot_index + 1, right, depth - 1);
                waitpid(pid, NULL, 0); // Wait for child
            }
        } else {
            parallel_quicksort(arr, left, pivot_index - 1, 0);
            parallel_quicksort(arr, pivot_index + 1, right, 0);
        }
    }
}

void sequential_quicksort(int *arr, int left, int right) {
    if (right - left < THRESHOLD) {
        insertion_sort(arr, left, right);
        return;
    }
    
    if (left < right) {
        int pivot_index = partition(arr, left, right);
        sequential_quicksort(arr, left, pivot_index - 1);
        sequential_quicksort(arr, pivot_index + 1, right);
    }
}

void generate_random_array(int *arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % 1000000;
    }
}

int is_sorted(int *arr, int size) {
    for (int i = 1; i < size; i++) {
        if (arr[i] < arr[i - 1]) {
            return 0;
        }
    }
    return 1;
}

void print_array_sample(int *arr, int size, char *label) {
    printf("%s: [", label);
    for (int i = 0; i < 5 && i < size; i++) {
        printf("%d", arr[i]);
        if (i < 4 && i < size - 1) printf(", ");
    }
    printf(", ..., ");
    for (int i = size - 5; i < size && i >= 0; i++) {
        printf("%d", arr[i]);
        if (i < size - 1) printf(", ");
    }
    printf("]\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <array_size> <parallel_depth>\n", argv[0]);
        printf("Example: %s 1000000 3\n", argv[0]);
        printf("Note: depth 0 = sequential, depth > 0 = parallel with 2^depth processes\n");
        return 1;
    }
    
    int size = atoi(argv[1]);
    int depth = atoi(argv[2]);
    
    if (size <= 0) {
        printf("Array size must be positive\n");
        return 1;
    }
    
    if (depth < 0) {
        printf("Depth must be non-negative\n");
        return 1;
    }
    
    printf("Array size: %d\n", size);
    printf("Parallel depth: %d\n", depth);
    printf("Max processes: %d\n", (1 << depth));
    printf("Threshold for insertion sort: %d\n", THRESHOLD);
    int *array = mmap(NULL, size * sizeof(int),
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS,
                     -1, 0);
    
    if (array == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    
    srand(time(NULL));
    generate_random_array(array, size);
    
    int *array_copy = malloc(size * sizeof(int));
    if (!array_copy) {
        perror("malloc failed");
        return 1;
    }
    memcpy(array_copy, array, size * sizeof(int));
    
    print_array_sample(array, size, "Before sort");
    
    printf("\nStarting PARALLEL quicksort...\n");
    clock_t parallel_start = clock();
    
    parallel_quicksort(array, 0, size - 1, depth);
    
    clock_t parallel_end = clock();
    double parallel_time = (double)(parallel_end - parallel_start) / CLOCKS_PER_SEC;
    
    int parallel_sorted = is_sorted(array, size);
    
    printf("Starting SEQUENTIAL quicksort...\n");
    clock_t seq_start = clock();
    
    sequential_quicksort(array_copy, 0, size - 1);
    
    clock_t seq_end = clock();
    double seq_time = (double)(seq_end - seq_start) / CLOCKS_PER_SEC;
    
    int seq_sorted = is_sorted(array_copy, size);
    
    print_array_sample(array, size, "After parallel sort");
    
    printf("Parallel sort:\n");
    printf("  Time: %.3f seconds\n", parallel_time);
    printf("  Sorted: %s\n", parallel_sorted ? "YES" : "NO");
    printf("\n");
    printf("Sequential sort:\n");
    printf("  Time: %.3f seconds\n", seq_time);
    printf("  Sorted: %s\n", seq_sorted ? "YES" : "NO");
    printf("\n");
    printf("Speedup: %.2fx faster\n", seq_time / parallel_time);
    printf("Efficiency: %.1f%%\n", 
           (seq_time / parallel_time) / (1 << depth) * 100);
    
    int identical = 1;
    for (int i = 0; i < size; i++) {
        if (array[i] != array_copy[i]) {
            identical = 0;
            break;
        }
    }
    printf("Arrays identical: %s\n", identical ? "YES" : "NO");
    
    munmap(array, size * sizeof(int));
    free(array_copy);
    
    return 0;
}