#ifndef MAIN_H

#define MAIN_H

// Standard input and output functions
#include <stdio.h>

// Memory allocation functions
#include <stdlib.h>

// String handling functions
#include <string.h>

// Unix system calls
#include <unistd.h>

// File control functions
#include <fcntl.h>

// Signal handling functions
#include <signal.h>

// Process related types
#include <sys/types.h>

// Process waiting functions
#include <sys/wait.h>

// File status functions
#include <sys/stat.h>

// Terminal control functions
#include <termios.h>

// Error handling
#include <errno.h>

// System limits
#include <limits.h>

// Maximum input size
#define MAX_INPUT_SIZE 1024

// Maximum number of arguments
#define MAX_ARGS 100

// Maximum number of jobs
#define MAX_JOBS 100

// Command type for no command
#define NO_COMMAND 0

// Command type for built-in command
#define BUILTIN 1

// Command type for external command
#define EXTERNAL 2

// Running job state
#define JOB_RUNNING 1

// Stopped job state
#define JOB_STOPPED 2

// Structure for storing job information
typedef struct

{

    // Job ID
    int id;

    // Process ID
    pid_t pid;

    // Process group ID
    pid_t pgid;

    // Job state
    int state;

    // Job command
    char command[MAX_INPUT_SIZE];

} Job;

// Store external commands
extern char **externalcmds;

// Store job information
extern Job jobs[MAX_JOBS];

// Store number of jobs
extern int job_count;

// Store shell process group ID
extern pid_t shell_pgid;

// Store foreground process group ID
extern pid_t foreground_pgid;

// Store shell terminal
extern int shell_terminal;

// Store last command status
extern int last_status;

// Store shell prompt
extern char prompt[MAX_INPUT_SIZE];

// Read and process user input
void scan_input(char *input);

// Get command from input
char *get_command(char *input);

// Check command type
int check_command_type(char *command);

// Extract external commands
void extract_external_commands(char **commands);

// Execute built-in commands
void execute_internal_commands(char *input);

// Execute external commands
void execute_external_command(char *input);

// Execute pipe commands
void execute_pipe_command(char *input);

// Add a new job
void add_job(pid_t pid, pid_t pgid, int state, const char *command);

// Find job using job ID
int find_job_by_id(int id);

// Remove job using its index
void remove_job_index(int index);

// Display all jobs
void print_jobs(void);

// Bring a job to foreground
void bring_fg(char *arg);

// Continue a job in background
void bring_bg(char *arg);

// Check and remove completed background jobs
void reap_background_processes(void);

#endif