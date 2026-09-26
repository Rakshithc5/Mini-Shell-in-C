#include "main.h"

// Remove spaces and tabs from the beginning and end
static void trim(char *str)
{
    char *start = str;
    size_t length;

    // Skip spaces and tabs at the beginning
    while (*start == ' ' || *start == '\t')
        start++;

    // Move the remaining string to the beginning
    if (start != str)
        memmove(str, start, strlen(start) + 1);

    length = strlen(str);

    // Remove spaces, tabs and newline from the end
    while (length > 0 &&
           (str[length - 1] == ' ' ||
            str[length - 1] == '\t' ||
            str[length - 1] == '\n'))
    {
        str[length - 1] = '\0';
        length--;
    }
}

// Get the first command from the input
char *get_command(char *input)
{
    static char command[128];
    int i = 0;

    // Copy characters until a separator is found
    while (input[i] &&
           input[i] != ' ' &&
           input[i] != '\t' &&
           input[i] != '|' &&
           input[i] != '<' &&
           input[i] != '>')
    {
        // Prevent command buffer overflow
        if (i >= (int)sizeof(command) - 1)
            break;

        command[i] = input[i];
        i++;
    }

    command[i] = '\0';

    return command;
}

// Expand environment variables in an argument
static int expand_arg(const char *input, char *output, size_t size)
{
    size_t used = 0;
    size_t i = 0;

    // Process the input character by character
    while (input[i])
    {
        // Copy normal characters
        if (input[i] != '$')
        {
            if (used + 1 >= size)
                return -1;

            output[used++] = input[i++];
            continue;
        }

        i++;

        // Expand the last command status
        if (input[i] == '?')
        {
            char value[32];

            snprintf(value, sizeof(value), "%d", last_status);

            if (used + strlen(value) >= size)
                return -1;

            strcpy(output + used, value);
            used += strlen(value);
            i++;
        }
        // Expand the current process ID
        else if (input[i] == '$')
        {
            char value[32];

            snprintf(value, sizeof(value), "%d", (int)getpid());

            if (used + strlen(value) >= size)
                return -1;

            strcpy(output + used, value);
            used += strlen(value);
            i++;
        }
        // Expand an environment variable
        else
        {
            char name[128];
            int n = 0;
            const char *value;

            // Read the environment variable name
            while (input[i] &&
                   (input[i] == '_' ||
                    (input[i] >= 'a' && input[i] <= 'z') ||
                    (input[i] >= 'A' && input[i] <= 'Z') ||
                    (input[i] >= '0' && input[i] <= '9')))
            {
                if (n < (int)sizeof(name) - 1)
                    name[n++] = input[i];

                i++;
            }

            // Keep the dollar sign if no variable name is found
            if (n == 0)
            {
                if (used + 1 >= size)
                    return -1;

                output[used++] = '$';
                continue;
            }

            name[n] = '\0';

            // Get the value of the environment variable
            value = getenv(name);

            if (value == NULL)
                value = "";

            if (used + strlen(value) >= size)
                return -1;

            strcpy(output + used, value);
            used += strlen(value);
        }
    }

    output[used] = '\0';

    return 0;
}

// Convert the input string into an argument list
static int make_argv(char *str, char **argv)
{
    int count = 0;
    char *token;

    // Get the first argument
    token = strtok(str, " \t");

    // Get all remaining arguments
    while (token != NULL && count < MAX_ARGS - 1)
    {
        argv[count++] = token;
        token = strtok(NULL, " \t");
    }

    argv[count] = NULL;

    return count;
}

// Expand variables in all arguments
static void expand_argv(char **argv, int argc)
{
    char value[MAX_INPUT_SIZE];

    // Expand each argument
    for (int i = 0; i < argc; i++)
    {
        if (expand_arg(argv[i], value, sizeof(value)) == 0)
            strcpy(argv[i], value);
    }
}

// Execute built-in commands
static void run_builtin(char **argv, int argc)
{
    // Exit the shell
    if (strcmp(argv[0], "exit") == 0)
        exit(last_status);

    // Execute pwd command
    if (strcmp(argv[0], "pwd") == 0)
    {
        char cwd[PATH_MAX];

        // Get and print the current directory
        if (getcwd(cwd, sizeof(cwd)) != NULL)
        {
            printf("%s\n", cwd);
            last_status = 0;
        }
        else
        {
            perror("pwd");
            last_status = 1;
        }

        return;
    }

    // Execute cd command
    if (strcmp(argv[0], "cd") == 0)
    {
        char old[PATH_MAX];
        char cwd[PATH_MAX];
        char *path;

        // Save the current directory
        getcwd(old, sizeof(old));

        // Use the given path or HOME
        path = argc > 1 ? argv[1] : getenv("HOME");

        if (path == NULL)
            path = "/";

        // Handle cd -
        if (strcmp(path, "-") == 0)
        {
            path = getenv("OLDPWD");

            if (path == NULL)
            {
                printf("cd: OLDPWD not set\n");
                last_status = 1;
                return;
            }
        }

        // Change to the requested directory
        if (chdir(path) == 0)
        {
            setenv("OLDPWD", old, 1);

            if (getcwd(cwd, sizeof(cwd)) != NULL)
                setenv("PWD", cwd, 1);

            last_status = 0;
        }
        else
        {
            perror("cd");
            last_status = 1;
        }

        return;
    }

    // Execute mkdir and rmdir commands
    if (strcmp(argv[0], "mkdir") == 0 ||
        strcmp(argv[0], "rmdir") == 0)
    {
        // Check for missing operand
        if (argc < 2)
        {
            printf("%s: missing operand\n", argv[0]);
            last_status = 1;
            return;
        }

        last_status = 0;

        // Process each directory argument
        for (int i = 1; i < argc; i++)
        {
            int result;

            // Create or remove the directory
            if (strcmp(argv[0], "mkdir") == 0)
                result = mkdir(argv[i], 0777);
            else
                result = rmdir(argv[i]);

            // Handle command error
            if (result == -1)
            {
                perror(argv[i]);
                last_status = 1;
            }
        }

        return;
    }

    // Execute echo command
    if (strcmp(argv[0], "echo") == 0)
    {
        // Print all arguments
        for (int i = 1; i < argc; i++)
        {
            if (i > 1)
                putchar(' ');

            printf("%s", argv[i]);
        }

        putchar('\n');
        last_status = 0;

        return;
    }

    // Display background jobs
    if (strcmp(argv[0], "jobs") == 0)
    {
        print_jobs();
        last_status = 0;
        return;
    }

    // Continue a job in the background
    if (strcmp(argv[0], "bg") == 0)
    {
        bring_bg(argc > 1 ? argv[1] : NULL);
        return;
    }

    // Bring a job to the foreground
    if (strcmp(argv[0], "fg") == 0)
    {
        bring_fg(argc > 1 ? argv[1] : NULL);
        return;
    }
}

// Execute an internal command
void execute_internal_commands(char *input)
{
    char copy[MAX_INPUT_SIZE];
    char *argv[MAX_ARGS];
    int argc;

    // Make a copy of the input
    strcpy(copy, input);
    trim(copy);

    // Convert input into arguments
    argc = make_argv(copy, argv);

    // Return if there is no command
    if (argc == 0)
        return;

    // Expand environment variables
    expand_argv(argv, argc);

    // Run the built-in command
    run_builtin(argv, argc);
}

// Execute an external command in a child process
static void exec_child(char *input)
{
    char copy[MAX_INPUT_SIZE];
    char *argv[MAX_ARGS];
    int argc;

    // Make a copy of the input
    strcpy(copy, input);
    trim(copy);

    // Convert input into arguments
    argc = make_argv(copy, argv);

    // Exit if there is no command
    if (argc == 0)
        exit(0);

    // Expand environment variables
    expand_argv(argv, argc);

    // Execute the external command
    execvp(argv[0], argv);

    // Print error if command execution fails
    perror(argv[0]);
    exit(127);
}
void execute_external_command(char *input)
{
    char copy[MAX_INPUT_SIZE];
    size_t length;
    int background = 0;
    pid_t pid;
    int status;

    // Make a copy of the input
    strcpy(copy, input);
    trim(copy);

    length = strlen(copy);

    // Check if the command should run in background
    if (length > 0 && copy[length - 1] == '&')
    {
        background = 1;
        copy[length - 1] = '\0';
        trim(copy);
    }

    // Create a child process
    pid = fork();

    // Check if fork failed
    if (pid == -1)
    {
        perror("fork");
        last_status = 1;
        return;
    }

    // Code executed by the child process
    if (pid == 0)
    {
        // Create a new process group
        setpgid(0, 0);

        // Restore default signal handling
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        // Execute the external command
        exec_child(copy);
    }

    // Set the process group for the child
    setpgid(pid, pid);

    // Handle background command
    if (background)
    {
        add_job(pid, pid, JOB_RUNNING, copy);

        printf("[%d] %d\n",
               jobs[job_count - 1].id,
               pid);

        last_status = 0;
        return;
    }

    // Set the foreground process group
    foreground_pgid = pid;

    // Give terminal control to the child
    if (tcsetpgrp(shell_terminal, pid) == -1)
    {
        perror("tcsetpgrp");
        kill(pid, SIGTERM);
        waitpid(pid, NULL, 0);
        foreground_pgid = -1;
        last_status = 1;
        return;
    }

    // Wait for the foreground process
    while (1)
    {
        pid_t result = waitpid(pid, &status, WUNTRACED);

        if (result == pid)
        {
            // Check if the process was stopped
            if (WIFSTOPPED(status))
            {
                add_job(pid, pid, JOB_STOPPED, copy);

                printf("\n[%d] Stopped    %d %s\n",
                       jobs[job_count - 1].id,
                       pid,
                       copy);

                last_status = 128 + WSTOPSIG(status);
            }
            // Get the exit status
            else if (WIFEXITED(status))
            {
                last_status = WEXITSTATUS(status);
            }
            // Get the signal termination status
            else if (WIFSIGNALED(status))
            {
                last_status = 128 + WTERMSIG(status);
            }

            break;
        }

        // Continue waiting if interrupted by a signal
        if (result == -1 && errno == EINTR)
            continue;

        // Handle waitpid error
        if (result == -1)
        {
            perror("waitpid");
            last_status = 1;
            break;
        }
    }

    // Clear foreground process group
    foreground_pgid = -1;

    // Give terminal control back to the shell
    if (tcsetpgrp(shell_terminal, shell_pgid) == -1)
        perror("tcsetpgrp");
}

void execute_pipe_command(char *input)
{
    char copy[MAX_INPUT_SIZE];
    char *commands[MAX_ARGS];
    pid_t pids[MAX_ARGS];
    int pipes[MAX_ARGS][2];
    int count = 0;
    char *part;

    // Make a copy of the input
    strcpy(copy, input);

    // Get the first command separated by pipe
    part = strtok(copy, "|");

    // Split the input into separate commands
    while (part != NULL && count < MAX_ARGS)
    {
        trim(part);

        // Check for an empty command
        if (*part == '\0')
        {
            printf("minishell: invalid pipe\n");
            return;
        }

        commands[count++] = part;
        part = strtok(NULL, "|");
    }

    // Return if there is no pipe
    if (count < 2)
        return;

    // Create pipes between commands
    for (int i = 0; i < count - 1; i++)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("pipe");
            return;
        }
    }

    pid_t pgid = -1;

    // Create processes for each command
    for (int i = 0; i < count; i++)
    {
        pids[i] = fork();

        // Check if fork failed
        if (pids[i] == -1)
        {
            perror("fork");
            continue;
        }

        // Code executed by child process
        if (pids[i] == 0)
        {
            // Create or join the process group
            if (i == 0)
                setpgid(0, 0);
            else
                setpgid(0, pgid);

            // Connect input from previous pipe
            if (i > 0)
                dup2(pipes[i - 1][0], STDIN_FILENO);

            // Connect output to next pipe
            if (i < count - 1)
                dup2(pipes[i][1], STDOUT_FILENO);

            // Close all pipe file descriptors
            for (int j = 0; j < count - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Restore default signal handling
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);

            // Execute the command
            exec_child(commands[i]);
        }

        // Set process group for the first command
        if (i == 0)
        {
            pgid = pids[i];
            setpgid(pids[i], pgid);
        }
        // Add remaining commands to the same group
        else
        {
            setpgid(pids[i], pgid);
        }
    }

    // Close pipe file descriptors in the parent
    for (int i = 0; i < count - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Set pipeline as foreground process group
    foreground_pgid = pgid;
    tcsetpgrp(shell_terminal, pgid);

    // Wait for all pipeline processes
    for (int i = 0; i < count; i++)
    {
        int status;

        while (1)
        {
            pid_t result = waitpid(pids[i], &status, WUNTRACED);

            if (result == pids[i])
            {
                // Check if the process was stopped
                if (WIFSTOPPED(status))
                {
                    add_job(pgid, pgid, JOB_STOPPED, input);

                    printf("\n[%d] Stopped    %d %s\n",
                           jobs[job_count - 1].id,
                           pgid,
                           input);

                    last_status = 128 + WSTOPSIG(status);
                }
                // Get the exit status
                else if (WIFEXITED(status))
                {
                    last_status = WEXITSTATUS(status);
                }
                // Get the signal termination status
                else if (WIFSIGNALED(status))
                {
                    last_status = 128 + WTERMSIG(status);
                }

                break;
            }

            // Continue waiting if interrupted
            if (result == -1 && errno == EINTR)
                continue;

            break;
        }
    }

    // Clear foreground process group
    foreground_pgid = -1;

    // Give terminal control back to the shell
    tcsetpgrp(shell_terminal, shell_pgid);
}

static int is_ps1_command(char *input)
{
    // Check if the input starts with PS1=
    if (strncmp(input, "PS1=", 4) != 0)
        return 0;

    // PS1 command should not contain spaces or tabs
    if (strchr(input, ' ') != NULL ||
        strchr(input, '\t') != NULL)
        return 0;

    return 1;
}

void scan_input(char *input)
{
    // Keep reading commands until the shell exits
    while (1)
    {
        // Remove completed background processes
        reap_background_processes();

        // Display the shell prompt
        printf("%s", prompt);
        fflush(stdout);

        errno = 0;

        // Read input from the user
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL)
        {
            // Handle interrupted input
            if (errno == EINTR)
            {
                clearerr(stdin);
                continue;
            }

            putchar('\n');
            break;
        }

        // Remove the newline character
        input[strcspn(input, "\n")] = '\0';
        trim(input);

        // Ignore empty input
        if (*input == '\0')
            continue;

        // Handle PS1 command
        if (is_ps1_command(input))
        {
            strncpy(prompt, input + 4, sizeof(prompt) - 1);
            prompt[sizeof(prompt) - 1] = '\0';
            last_status = 0;
            continue;
        }

        // Execute pipeline commands
        if (strchr(input, '|') != NULL)
        {
            execute_pipe_command(input);
            continue;
        }

        char copy[MAX_INPUT_SIZE];
        strcpy(copy, input);

        // Get the command name
        char *command = get_command(copy);

        // Check the command type
        int type = check_command_type(command);

        // Execute built-in or external command
        if (type == BUILTIN)
            execute_internal_commands(input);
        else
            execute_external_command(input);
    }
}