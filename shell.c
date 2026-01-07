<<<<<<< HEAD
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64
#define HISTORY_SIZE 10
#define PROMPT "uinxsh> "


volatile sig_atomic_t background_pids[100];
volatile sig_atomic_t background_count = 0;
char *history[HISTORY_SIZE];
int history_count = 0;
int history_index = 0;


void init_history() {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        history[i] = NULL;
    }
}

void add_to_history(const char *command) {
    if (strlen(command) == 0) return;
    
    if (history_count > 0) {
        int last_index = (history_index - 1 + HISTORY_SIZE) % HISTORY_SIZE;
        if (strcmp(history[last_index], command) == 0) return;
    }
    
    if (history[history_index] != NULL) {
        free(history[history_index]);
    }
    
    history[history_index] = strdup(command);
    history_index = (history_index + 1) % HISTORY_SIZE;
    
    if (history_count < HISTORY_SIZE) {
        history_count++;
    }
}

char *get_last_command() {
    if (history_count == 0) return NULL;
    int last_index = (history_index - 1 + HISTORY_SIZE) % HISTORY_SIZE;
    return history[last_index];
}

void print_history() {
    printf("Command History (last %d):\n", history_count);
    int start = (history_index - history_count + HISTORY_SIZE) % HISTORY_SIZE;
    
    for (int i = 0; i < history_count; i++) {
        int idx = (start + i) % HISTORY_SIZE;
        printf("%3d: %s\n", i + 1, history[idx]);
    }
}

void cleanup_history() {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        if (history[i] != NULL) {
            free(history[i]);
            history[i] = NULL;
        }
    }
    history_count = 0;
    history_index = 0;
}

void handle_cd(char **args) {
    if (args[1] == NULL) {
        char *home = getenv("HOME");
        if (home == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
        if (chdir(home) != 0) {
            perror("cd");
        }
    } else if (strcmp(args[1], "-") == 0) {
        char *oldpwd = getenv("OLDPWD");
        if (oldpwd == NULL) {
            fprintf(stderr, "cd: OLDPWD not set\n");
            return;
        }
        printf("%s\n", oldpwd);
        if (chdir(oldpwd) != 0) {
            perror("cd");
        }
    } else {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            setenv("OLDPWD", cwd, 1);
        }
        
        if (chdir(args[1]) != 0) {
            perror("cd");
        }
    }
}

void handle_pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

void handle_help() {
    
   printf("cd[dir] ,  pwd , exit , help, history , jobs , command & , cmd1 | cmd2 , !!\n");
    
}

void handle_jobs() {
    if (background_count == 0) {
        printf("No background jobs running\n");
        return;
    }
    printf("Background jobs:\n");
    for (int i = 0; i < background_count; i++) {
        printf("[%d] PID %d\n", i + 1, background_pids[i]);
    }
}

void parse_command(char *input, char **args) {
    int i = 0;
    char *token;
    
    for (int j = 0; j < MAX_ARGS; j++) {
        args[j] = NULL;
    }
    
    token = strtok(input, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i] = strdup(token);
        i++;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

void free_args(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
}

void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < background_count; i++) {
            if (background_pids[i] == pid) {
                printf("\n[%d] Done   %d\n", i + 1, pid);
                for (int j = i; j < background_count - 1; j++) {
                    background_pids[j] = background_pids[j + 1];
                }
                background_count--;
                break;
            }
        }
    }
}

void sigint_handler(int sig) {
    printf("\n%s", PROMPT);
    fflush(stdout);
}

int execute_external_command(char **args, int background) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        return -1;
    } else if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "Command not found: %s\n", args[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        if (!background) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            printf("[%d] %d\n", background_count + 1, pid);
            background_pids[background_count++] = pid;
        }
    }
    return 0;
}

int handle_pipe_command(char **args1, char **args2) {
    int pipefd[2];
    pid_t pid1, pid2;
    
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return -1;
    }
    
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork failed");
        return -1;
    }
    
    if (pid1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        if (execvp(args1[0], args1) == -1) {
            fprintf(stderr, "Command not found: %s\n", args1[0]);
            exit(EXIT_FAILURE);
        }
    }
    
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork failed");
        return -1;
    }
    
    if (pid2 == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        
        if (execvp(args2[0], args2) == -1) {
            fprintf(stderr, "Command not found: %s\n", args2[0]);
            exit(EXIT_FAILURE);
        }
    }
    
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    
    return 0;
}

int main() {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARGS];
    char *args2[MAX_ARGS];
    int background = 0;
    int should_run = 1;
    
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    
    init_history();
    
    
    printf("Type 'help' for available commands\n");
    printf("Use 'exit' to quit\n\n");
    
    while (should_run) {
        printf(PROMPT);
        fflush(stdout);
        
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            break;  
        }
        
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) continue;
        
        if (strcmp(input, "!!") == 0) {
            char *last_command = get_last_command();
            if (last_command == NULL) {
                fprintf(stderr, "No commands in history\n");
                continue;
            }
            printf("%s\n", last_command);
            strncpy(input, last_command, MAX_INPUT_SIZE - 1);
            input[MAX_INPUT_SIZE - 1] = '\0';
        }
        
        add_to_history(input);
        
        background = 0;
        if (strlen(input) > 1 && input[strlen(input) - 1] == '&') {
            background = 1;
            input[strlen(input) - 1] = '\0';
            while (strlen(input) > 0 && input[strlen(input) - 1] == ' ') {
                input[strlen(input) - 1] = '\0';
            }
        }
        
        char *pipe_pos = strchr(input, '|');
        if (pipe_pos != NULL) {
            *pipe_pos = '\0';
            char *cmd1 = input;
            char *cmd2 = pipe_pos + 1;
            
            parse_command(cmd1, args);
            parse_command(cmd2, args2);
            
            handle_pipe_command(args, args2);
            free_args(args);
            free_args(args2);
            continue;
        }
        
        parse_command(input, args);
        
        if (args[0] == NULL) {
            continue;
        } else if (strcmp(args[0], "exit") == 0) {
            should_run = 0;
        } else if (strcmp(args[0], "cd") == 0) {
            handle_cd(args);
        } else if (strcmp(args[0], "pwd") == 0) {
            handle_pwd();
        } else if (strcmp(args[0], "help") == 0) {
            handle_help();
        } else if (strcmp(args[0], "history") == 0) {
            print_history();
        } else if (strcmp(args[0], "jobs") == 0) {
            handle_jobs();
        } else {
            execute_external_command(args, background);
        }
        
        free_args(args);
    }
    
    cleanup_history();
    
    return 0;
=======
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64
#define HISTORY_SIZE 10
#define PROMPT "uinxsh> "


volatile sig_atomic_t background_pids[100];
volatile sig_atomic_t background_count = 0;
char *history[HISTORY_SIZE];
int history_count = 0;
int history_index = 0;


void init_history() {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        history[i] = NULL;
    }
}

void add_to_history(const char *command) {
    if (strlen(command) == 0) return;
    
    if (history_count > 0) {
        int last_index = (history_index - 1 + HISTORY_SIZE) % HISTORY_SIZE;
        if (strcmp(history[last_index], command) == 0) return;
    }
    
    if (history[history_index] != NULL) {
        free(history[history_index]);
    }
    
    history[history_index] = strdup(command);
    history_index = (history_index + 1) % HISTORY_SIZE;
    
    if (history_count < HISTORY_SIZE) {
        history_count++;
    }
}

char *get_last_command() {
    if (history_count == 0) return NULL;
    int last_index = (history_index - 1 + HISTORY_SIZE) % HISTORY_SIZE;
    return history[last_index];
}

void print_history() {
    printf("Command History (last %d):\n", history_count);
    int start = (history_index - history_count + HISTORY_SIZE) % HISTORY_SIZE;
    
    for (int i = 0; i < history_count; i++) {
        int idx = (start + i) % HISTORY_SIZE;
        printf("%3d: %s\n", i + 1, history[idx]);
    }
}

void cleanup_history() {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        if (history[i] != NULL) {
            free(history[i]);
            history[i] = NULL;
        }
    }
    history_count = 0;
    history_index = 0;
}

void handle_cd(char **args) {
    if (args[1] == NULL) {
        char *home = getenv("HOME");
        if (home == NULL) {
            fprintf(stderr, "cd: HOME not set\n");
            return;
        }
        if (chdir(home) != 0) {
            perror("cd");
        }
    } else if (strcmp(args[1], "-") == 0) {
        char *oldpwd = getenv("OLDPWD");
        if (oldpwd == NULL) {
            fprintf(stderr, "cd: OLDPWD not set\n");
            return;
        }
        printf("%s\n", oldpwd);
        if (chdir(oldpwd) != 0) {
            perror("cd");
        }
    } else {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            setenv("OLDPWD", cwd, 1);
        }
        
        if (chdir(args[1]) != 0) {
            perror("cd");
        }
    }
}

void handle_pwd() {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

void handle_help() {
    
   printf("cd[dir] ,  pwd , exit , help, history , jobs , command & , cmd1 | cmd2 , !!\n");
    
}

void handle_jobs() {
    if (background_count == 0) {
        printf("No background jobs running\n");
        return;
    }
    printf("Background jobs:\n");
    for (int i = 0; i < background_count; i++) {
        printf("[%d] PID %d\n", i + 1, background_pids[i]);
    }
}

void parse_command(char *input, char **args) {
    int i = 0;
    char *token;
    
    for (int j = 0; j < MAX_ARGS; j++) {
        args[j] = NULL;
    }
    
    token = strtok(input, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1) {
        args[i] = strdup(token);
        i++;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

void free_args(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
}

void sigchld_handler(int sig) {
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < background_count; i++) {
            if (background_pids[i] == pid) {
                printf("\n[%d] Done   %d\n", i + 1, pid);
                for (int j = i; j < background_count - 1; j++) {
                    background_pids[j] = background_pids[j + 1];
                }
                background_count--;
                break;
            }
        }
    }
}

void sigint_handler(int sig) {
    printf("\n%s", PROMPT);
    fflush(stdout);
}

int execute_external_command(char **args, int background) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork failed");
        return -1;
    } else if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        
        if (execvp(args[0], args) == -1) {
            fprintf(stderr, "Command not found: %s\n", args[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        if (!background) {
            int status;
            waitpid(pid, &status, 0);
        } else {
            printf("[%d] %d\n", background_count + 1, pid);
            background_pids[background_count++] = pid;
        }
    }
    return 0;
}

int handle_pipe_command(char **args1, char **args2) {
    int pipefd[2];
    pid_t pid1, pid2;
    
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        return -1;
    }
    
    pid1 = fork();
    if (pid1 < 0) {
        perror("fork failed");
        return -1;
    }
    
    if (pid1 == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        
        if (execvp(args1[0], args1) == -1) {
            fprintf(stderr, "Command not found: %s\n", args1[0]);
            exit(EXIT_FAILURE);
        }
    }
    
    pid2 = fork();
    if (pid2 < 0) {
        perror("fork failed");
        return -1;
    }
    
    if (pid2 == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        
        if (execvp(args2[0], args2) == -1) {
            fprintf(stderr, "Command not found: %s\n", args2[0]);
            exit(EXIT_FAILURE);
        }
    }
    
    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    
    return 0;
}

int main() {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARGS];
    char *args2[MAX_ARGS];
    int background = 0;
    int should_run = 1;
    
    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigint_handler);
    
    init_history();
    
    
    printf("Type 'help' for available commands\n");
    printf("Use 'exit' to quit\n\n");
    
    while (should_run) {
        printf(PROMPT);
        fflush(stdout);
        
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            break;  
        }
        
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) continue;
        
        if (strcmp(input, "!!") == 0) {
            char *last_command = get_last_command();
            if (last_command == NULL) {
                fprintf(stderr, "No commands in history\n");
                continue;
            }
            printf("%s\n", last_command);
            strncpy(input, last_command, MAX_INPUT_SIZE - 1);
            input[MAX_INPUT_SIZE - 1] = '\0';
        }
        
        add_to_history(input);
        
        background = 0;
        if (strlen(input) > 1 && input[strlen(input) - 1] == '&') {
            background = 1;
            input[strlen(input) - 1] = '\0';
            while (strlen(input) > 0 && input[strlen(input) - 1] == ' ') {
                input[strlen(input) - 1] = '\0';
            }
        }
        
        char *pipe_pos = strchr(input, '|');
        if (pipe_pos != NULL) {
            *pipe_pos = '\0';
            char *cmd1 = input;
            char *cmd2 = pipe_pos + 1;
            
            parse_command(cmd1, args);
            parse_command(cmd2, args2);
            
            handle_pipe_command(args, args2);
            free_args(args);
            free_args(args2);
            continue;
        }
        
        parse_command(input, args);
        
        if (args[0] == NULL) {
            continue;
        } else if (strcmp(args[0], "exit") == 0) {
            should_run = 0;
        } else if (strcmp(args[0], "cd") == 0) {
            handle_cd(args);
        } else if (strcmp(args[0], "pwd") == 0) {
            handle_pwd();
        } else if (strcmp(args[0], "help") == 0) {
            handle_help();
        } else if (strcmp(args[0], "history") == 0) {
            print_history();
        } else if (strcmp(args[0], "jobs") == 0) {
            handle_jobs();
        } else {
            execute_external_command(args, background);
        }
        
        free_args(args);
    }
    
    cleanup_history();
    
    return 0;
>>>>>>> cc771fb0b3ae15ec1ce0d2f5c96fdd52b017a4e5
}