[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/zKQr05vG)
# Unix Shell Project Documentation

## Project Overview

This project implements a custom Unix shell interface in C. The shell provides command execution, process management, inter-process communication, and parallel computing capabilities.

### Project Goals

1. Create a functional Unix shell that accepts and executes user commands
2. Implement built-in shell commands and features
3. Support parallel command execution
4. Provide command history functionality
5. Implement pipe-based inter-process communication
6. Develop parallel computing applications (Sudoku validator, fork-join sorting algorithms, Monte Carlo simulation)

### Project Requirements

**Mandatory Component**:

- Unix Shell Implementation (60 points)

**Optional Components** (Choose 2):

- Parallel Sudoku Validator (20 points)
- Fork-Join Merge Sort (20 points)
- Fork-Join Quicksort (20 points)
- Monte Carlo Pi Estimation (20 points)
- Additional projects may be added (20 points each)

**Total Project Score**: 100 points (60 from shell + 40 from two additional projects)

## Resources

**Shell Implementation Tutorials**:

- **[Writing a Unix Shell - Part I](https://igupta.in/blog/writing-a-unix-shell-part-1/)** - Comprehensive three-part tutorial covering `fork()`, `exec()`, process management, and shell implementation from scratch

  - Part I: Understanding `fork()` and process creation
  - Part II: Command parsing and execution
  - Part III: Advanced features and I/O redirection

- **[Build Your Own Shell](https://github.com/tokenrove/build-your-own-shell)** - GitHub repository with step-by-step instructions and code examples for building a Unix shell

- **[CodeCrafters Shell Course](https://app.codecrafters.io/courses/shell/introduction)** - Interactive course with hands-on exercises for building a shell from scratch

**Operating Systems Concepts**:

- **[Operating System Concepts (Dinosaur Book)](https://www.os-book.com/)** by Silberschatz, Galvin, and Gagne

  - Chapter 3: Processes (fork, exec, wait)
  - Chapter 4: Threads & Concurrency
  - Project descriptions and implementations

- **[UW-Madison CS537 Operating Systems](https://pages.cs.wisc.edu/~shivaram/cs537-sp20/)**
  - [Project 2a: Unix Shell](https://pages.cs.wisc.edu/~shivaram/cs537-sp20/p2a.html)
  - Lecture notes and project specifications

## Table of Contents

1. [Grading and Scoring](#grading-and-scoring)
2. [System Architecture](#system-architecture)
3. [Core Components](#core-components)
4. [Features](#features)
5. [Implementation Details](#implementation-details)
6. [Additional Projects](#additional-projects)
7. [Build and Usage](#build-and-usage)
8. [System Calls Reference](#system-calls-reference)

## Grading and Scoring

### Score Distribution

This project is worth **100 points** total, with emphasis placed on the shell implementation as the core component.

#### Unix Shell Implementation (Mandatory) - 60 Points

The shell component is required for all students and accounts for the majority of the project grade:

| Component             | Points | Description                                                                                                                                                                                 |
| --------------------- | ------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **The Basics**        | 25     | Core shell functionality: main loop, command parsing, process creation (`fork()`), command execution (`execvp()`), process waiting (`wait()`), proper memory management, and error handling |
| **Built-in Commands** | 10     | Implementation of shell built-ins: `exit`, `cd`, `pwd`, and at least 2 additional built-in commands                                                                                         |
| **Parallel Commands** | 8      | Background execution support using `&`, proper handling of concurrent processes, and zombie process prevention                                                                              |
| **Pipe Support**      | 12     | Inter-process communication using pipes (`\|`), proper use of `pipe()` and `dup2()`, support for single pipe between two commands                                                           |
| **History Feature**   | 5      | Command history with `!!` support, proper error handling for empty history                                                                                                                  |
| **Total Shell**       | **60** |                                                                                                                                                                                             |

#### Additional Projects (Choose Any 2) - 40 Points

Students must complete **2 out of N available projects** (20 points each):

| Project                       | Points | Description                                                                                                                 |
| ----------------------------- | ------ | --------------------------------------------------------------------------------------------------------------------------- |
| **Parallel Sudoku Validator** | 20     | Multithreaded validation of Sudoku puzzles using pthreads, proper thread synchronization, and result aggregation            |
| **Fork-Join Merge Sort**      | 20     | Parallel merge sort implementation using fork-join pattern, shared memory management, and performance optimization          |
| **Fork-Join Quicksort**       | 20     | Parallel quicksort implementation using fork-join pattern, pivot selection, partitioning, and parallel recursive sorting    |
| **Monte Carlo Pi Estimation** | 20     | Parallel Monte Carlo simulation to estimate π using random sampling, process-based parallelism, and statistical aggregation |

## System Architecture

### Overview

The shell operates using a parent-child process model:

- **Parent Process**: Reads user input, parses commands, and manages child processes
- **Child Processes**: Execute individual commands using the `exec()` family of system calls

```
┌─────────────────────────────────────────┐
│          Shell Parent Process           │
│  - Read user input (readline)           │
│  - Parse commands                       │
│  - Fork child processes                 │
│  - Wait for completion (unless &)       │
└─────────────────────────────────────────┘
                 │
                 ├─► fork()
                 │
        ┌────────▼────────┐
        │  Child Process  │
        │  - execvp()     │
        │  - Execute cmd  │
        └─────────────────┘
```

### Key System Calls Used

- `fork()` - Create new processes
- `execvp()` - Execute commands
- `wait()` - Wait for child process completion
- `pipe()` - Create inter-process communication channels
- `dup2()` - Duplicate file descriptors for I/O redirection
- `readline()` - Read user input with line editing support

## Core Components

### 1. Basic Shell Interface

**File**: `shell/main.c`

The main shell loop continuously:

1. Displays prompt: `uinxsh>`
2. Reads user input using `readline()`
3. Parses input into command and arguments
4. Forks a child process
5. Executes the command in the child process
6. Waits for completion (unless background execution requested)

**Key Functions**:

```c
char **parse_input(char *input)
```

- Tokenizes user input into command and arguments
- Uses space and newline as delimiters
- Returns NULL-terminated array of strings suitable for `execvp()`

### 2. Built-in Commands

Built-in commands are executed directly by the shell without forking:

#### Essential Built-ins

- **`exit`**: Terminate the shell

  - Sets `should_run` flag to 0
  - Performs cleanup before exiting

- **`cd <directory>`**: Change working directory

  - Uses `chdir()` system call
  - Handles error cases (invalid path, permission denied)
  - Special cases: `cd ~` (home directory), `cd ..` (parent directory)

- **`pwd`**: Print working directory

  - Uses `getcwd()` system call

- **`help`**: Display available commands and usage information

**Implementation Pattern**:

```c
if (strcmp(command[0], "exit") == 0) {
    should_run = 0;
    continue;
}
if (strcmp(command[0], "cd") == 0) {
    if (chdir(command[1]) != 0) {
        perror("cd failed");
    }
    continue;
}
// ... other built-ins
```

### 3. Command Parsing

The parser handles:

- **Simple commands**: `ls -la`
- **Background execution**: `sleep 10 &`
- **Pipes**: `ls -l | grep txt`

**Token Separation**:

- Whitespace delimiters
- Special character detection (`|`, `&`)
- Argument array construction

## Features

### 1. Parallel Command Execution

**Concept**: Allow commands to run in the background while the shell continues accepting new commands.

**Syntax**: Append `&` to the end of a command

```bash
uinxsh> long_running_process &
uinxsh> # Shell immediately returns to prompt
```

### 2. History Feature

**Concept**: Allow users to re-execute the most recent command using `!!`.

**Functionality**:

- Store the last executed command in a buffer
- When user enters `!!`, retrieve and execute the previous command
- Echo the command being re-executed
- Handle error case: "No commands in history"

**Enhanced History** (Optional Extension):

- Store multiple commands (e.g., last 10)
- Implement `!n` to execute nth command
- Implement `history` built-in to list recent commands

### 3. Pipe Support

**Concept**: Connect the output of one command to the input of another.

**Syntax**: `command1 | command2`

```bash
uinxsh> ls -l | grep txt
uinxsh> cat file.txt | sort | uniq
```

**Implementation Strategy**:

1. **Parse pipe character**: Detect `|` and split commands
2. **Create pipe**: Use `pipe()` system call
3. **Fork twice**: Create two child processes
4. **Redirect I/O**: Use `dup2()` to connect stdout of first command to stdin of second

**Key Points**:

- Both parent and children must close unused pipe ends
- Parent waits for both children to complete

## Implementation Details

### Memory Management

**Critical Considerations**:

1. **Free allocated memory**: `readline()` returns dynamically allocated strings
2. **Free parsed commands**: The `parse_input()` function allocates memory
3. **Avoid memory leaks**: Free resources in both parent and child processes

### Error Handling

**Command Execution Errors**:

```c
if (execvp(command[0], command) == -1) {
    printf("Command not found: %s\n", command[0]);
    exit(1);
}
```

**Fork Errors**:

```c
pid_t pid = fork();
if (pid < 0) {
    perror("Fork failed");
    continue;
}
```

**File Operation Errors**:

```c
int fd = open(filename, flags, mode);
if (fd < 0) {
    perror("File open error");
    exit(1);
}
```

### Process Synchronization

**Wait Strategies**:

- **Foreground**: `wait(NULL)` - Block until child completes
- **Background**: Continue immediately without waiting
- **Zombie Prevention**: Use `waitpid()` with `WNOHANG` to reap completed background processes

```c
// Reap zombies in main loop
while (waitpid(-1, NULL, WNOHANG) > 0);
```

## Additional Projects

### 1. Parallel Sudoku Validator

**Objective**: Validate a 9×9 Sudoku solution using multiple threads.

**Problem Description**:
A valid Sudoku puzzle requires:

- Each row contains digits 1-9 (no duplicates)
- Each column contains digits 1-9 (no duplicates)
- Each 3×3 subgrid contains digits 1-9 (no duplicates)

**Multithreading Strategy**:

Total: **11 threads**

- 1 thread to validate all 9 rows (or 9 threads, one per row)
- 1 thread to validate all 9 columns (or 9 threads, one per column)
- 9 threads to validate each 3×3 subgrid

**Data Structure**:

```c
typedef struct {
    int row;
    int column;
} parameters;

// Global result array
int valid[11]; // One entry per validation thread
```

**Thread Function**:

```c
void *validate_row(void *param) {
    parameters *data = (parameters *) param;
    // Validate row logic
    // Set valid[thread_id] = 1 if valid, 0 otherwise
    pthread_exit(NULL);
}
```

### 2. Fork-Join Merge Sort

**Objective**: Implement parallel merge sort using fork-join parallelism.

**Algorithm**:

1. **Divide**: Split array into two halves
2. **Conquer**: Recursively sort each half (in parallel)
3. **Combine**: Merge sorted halves

**Key Considerations**:

- **Threshold**: Switch to simple sort for small subarrays (e.g., size ≤ 20)
- **Shared Memory**: Use shared memory segment for array access across processes
- **Process Overhead**: Limit recursion depth to avoid excessive process creation

**Shared Memory Setup**:

```c
#include <sys/mman.h>

// Create shared memory for array
int *shared_array = mmap(NULL, size * sizeof(int),
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_ANONYMOUS, -1, 0);
```

**Alternative: Using Threads**:

```c
pthread_t thread;
pthread_create(&thread, NULL, sort_thread, &left_params);

// Sort right half in current thread
sort_right_half();

pthread_join(thread, NULL);
merge();
```

### 3. Fork-Join Quicksort

**Objective**: Implement parallel quicksort using fork-join parallelism.

**Algorithm**:

1. **Choose Pivot**: Select a pivot element (median-of-three recommended)
2. **Partition**: Rearrange array so elements < pivot are on left, elements > pivot are on right
3. **Conquer**: Recursively sort both partitions (in parallel)

### 4. Monte Carlo Pi Estimation

**Objective**: Estimate the value of π using the Monte Carlo method with parallel computation.

**Problem Description**:

The Monte Carlo method for estimating π is based on a simple geometric principle:

- Consider a unit circle (radius = 1) inscribed in a square (side length = 2)
- Area of circle = π × r² = π × 1² = π
- Area of square = 2 × 2 = 4
- Ratio of areas = π/4

By randomly generating points within the square and counting how many fall inside the circle, we can estimate π:

**π ≈ 4 × (points inside circle) / (total points)**

**Algorithm**:

1. **Generate Random Points**: Create random (x, y) coordinates in the range [-1, 1]
2. **Test if Inside Circle**: Check if x² + y² ≤ 1
3. **Count and Calculate**: Ratio of inside points to total points approximates π/4
4. **Parallelize**: Divide work among multiple processes/threads

**Key Considerations**:

1. **Random Number Generation**:

   - Use `rand_r()` for thread-safe random numbers
   - Different seeds for each process/thread to ensure independence
   - Seed format: `time(NULL) ^ (process_id << 16)`

2. **Accuracy vs. Performance**:

   - More points → better accuracy (error ∝ 1/√N)
   - Diminishing returns: 4x points for 2x accuracy
   - Typical: 10M-100M points for reasonable accuracy

3. **Parallel Efficiency**:

   - Embarrassingly parallel problem (no dependencies)
   - Near-linear speedup with more processes/threads
   - Limited only by number of CPU cores

4. **Shared Memory**:

   - Use `mmap()` with `MAP_SHARED | MAP_ANONYMOUS` for process communication
   - Alternative: pipes or message queues (slower)
   - Threads naturally share memory

5. **Statistical Properties**:
   - Error decreases as 1/√N (Law of Large Numbers)
   - Confidence intervals can be calculated
   - Multiple runs yield distribution around π

**Mathematical Background**:

The method works because:

- Uniform random points in square: probability of landing in any region ∝ area
- P(point in circle) = (area of circle) / (area of square) = π/4
- As N → ∞, observed frequency → theoretical probability
- Therefore: (points in circle) / (total points) → π/4

**Error Analysis**:

- Standard error: σ ≈ √(π(4-π)/N) ≈ 1.64/√N
- For 100M points: error ≈ 0.0001 (±0.01%)
- 95% confidence interval: π̂ ± 2σ

**Usage**:

```bash
# Run with 4 processes, 100M points
unixsh> mont_carlo 4 100000000
```

### Running the Shell

```bash
./shell
````

**Example Session**:

```
uinxsh> ls -la
total 48
drwxr-xr-x  8 user  staff   256 Nov 21 10:30 .
drwxr-xr-x  5 user  staff   160 Nov 21 09:15 ..
-rw-r--r--  1 user  staff  1024 Nov 21 10:30 file.txt

uinxsh> echo "Hello, World!"
Hello, World!

uinxsh> pwd
/Users/mohammad/OS/shell

uinxsh> cd ..

uinxsh> pwd
/Users/mohammad/OS

uinxsh> ls | grep txt
file.txt
readme.txt

uinxsh> sleep 5 &
[Running in background]

uinxsh> !!
sleep 5 &

uinxsh> exit
```

## System Calls Reference

### Process Management

**`fork()`**

- Creates a new process (child)
- Returns 0 in child, child PID in parent, -1 on error
- Child inherits parent's memory, file descriptors

**`execvp(const char *file, char *const argv[])`**

- Replaces current process image with new program
- Searches PATH for executable
- Does not return on success

**`wait(int *status)`**

- Blocks until a child process terminates
- Returns PID of terminated child
- `waitpid()` variant allows non-blocking wait

### Inter-Process Communication

**`pipe(int pipefd[2])`**

- Creates unidirectional data channel
- `pipefd[0]` - read end
- `pipefd[1]` - write end
- Returns 0 on success, -1 on error

**`dup2(int oldfd, int newfd)`**

- Duplicates file descriptor
- Makes `newfd` a copy of `oldfd`
- Used for I/O redirection