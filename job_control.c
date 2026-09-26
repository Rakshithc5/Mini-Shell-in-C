#include "main.h"

void add_job(pid_t pid, pid_t pgid, int state, const char *command)
{
    // Check if the job list is full
    if (job_count >= MAX_JOBS)
    {
        printf("minishell: too many jobs\n");
        return;
    }

    jobs[job_count].id = job_count + 1;
    jobs[job_count].pid = pid;
    jobs[job_count].pgid = pgid;
    jobs[job_count].state = state;

    // Copy the command into the job
    strncpy(jobs[job_count].command, command, MAX_INPUT_SIZE - 1);
    jobs[job_count].command[MAX_INPUT_SIZE - 1] = '\0';

    job_count++;
}

int find_job_by_id(int id)
{
    // Search for the job with the given ID
    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].id == id)
            return i;
    }

    return -1;
}

void remove_job_index(int index)
{
    // Check if the job index is valid
    if (index < 0 || index >= job_count)
        return;

    // Shift remaining jobs to fill the removed job
    for (int i = index; i < job_count - 1; i++)
    {
        jobs[i] = jobs[i + 1];
        jobs[i].id = i + 1;
    }

    job_count--;
}

void reap_background_processes(void)
{
    int status;

    // Check all background jobs
    for (int i = 0; i < job_count; i++)
    {
        // Do not reap stopped jobs
        if (jobs[i].state == JOB_STOPPED)
            continue;

        pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);

        // Remove completed background jobs
        if (result == jobs[i].pid &&
            (WIFEXITED(status) || WIFSIGNALED(status)))
        {
            remove_job_index(i);
            i--;
        }
    }
}

void print_jobs(void)
{
    int status;

    // Display all current jobs
    for (int i = 0; i < job_count; i++)
    {
        // Display stopped jobs
        if (jobs[i].state == JOB_STOPPED)
        {
            printf("[%d] Stopped    %d %s\n",
                   jobs[i].id,
                   jobs[i].pid,
                   jobs[i].command);
            continue;
        }

        pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);

        // Display running jobs
        if (result == 0)
        {
            printf("[%d] Running    %d %s\n",
                   jobs[i].id,
                   jobs[i].pid,
                   jobs[i].command);
        }
        // Display completed jobs
        else if (result == jobs[i].pid)
        {
            if (WIFEXITED(status))
            {
                printf("[%d] Done       %d %s\n",
                       jobs[i].id,
                       jobs[i].pid,
                       jobs[i].command);
            }
            else if (WIFSIGNALED(status))
            {
                printf("[%d] Terminated %d %s\n",
                       jobs[i].id,
                       jobs[i].pid,
                       jobs[i].command);
            }

            remove_job_index(i);
            i--;
        }
    }
}

static int get_job_id(char *arg)
{
    // Use the most recent job if no ID is given
    if (arg == NULL)
    {
        if (job_count == 0)
            return -1;

        return jobs[job_count - 1].id;
    }

    // Handle job ID with %
    if (arg[0] == '%')
        return atoi(arg + 1);

    return atoi(arg);
}

void bring_bg(char *arg)
{
    int id = get_job_id(arg);
    int index = find_job_by_id(id);

    // Check if the job exists
    if (index == -1)
    {
        printf("bg: no such job\n");
        last_status = 1;
        return;
    }

    // Continue the stopped job
    if (kill(-jobs[index].pgid, SIGCONT) == -1)
    {
        perror("bg");
        last_status = 1;
        return;
    }

    jobs[index].state = JOB_RUNNING;

    printf("[%d] %d %s &\n",
           jobs[index].id,
           jobs[index].pid,
           jobs[index].command);

    last_status = 0;
}

void bring_fg(char *arg)
{
    int id = get_job_id(arg);
    int index = find_job_by_id(id);
    int status;

    // Check if the job exists
    if (index == -1)
    {
        printf("fg: no such job\n");
        last_status = 1;
        return;
    }

    pid_t pid = jobs[index].pid;
    pid_t pgid = jobs[index].pgid;

    // Give terminal control to the foreground job
    if (tcsetpgrp(shell_terminal, pgid) == -1)
    {
        perror("tcsetpgrp");
        last_status = 1;
        return;
    }

    foreground_pgid = pgid;

    // Continue the job if it was stopped
    if (jobs[index].state == JOB_STOPPED)
    {
        if (kill(-pgid, SIGCONT) == -1)
        {
            perror("fg");
            tcsetpgrp(shell_terminal, shell_pgid);
            foreground_pgid = -1;
            last_status = 1;
            return;
        }

        jobs[index].state = JOB_RUNNING;
    }

    // Wait for the foreground job
    while (1)
    {
        pid_t result = waitpid(pid, &status, WUNTRACED);

        if (result == pid)
        {
            // Check if the job was stopped
            if (WIFSTOPPED(status))
            {
                jobs[index].state = JOB_STOPPED;

                printf("\n[%d] Stopped    %d %s\n",
                       jobs[index].id,
                       jobs[index].pid,
                       jobs[index].command);

                last_status = 128 + WSTOPSIG(status);
                break;
            }

            // Get the exit status of the job
            if (WIFEXITED(status))
                last_status = WEXITSTATUS(status);
            else if (WIFSIGNALED(status))
                last_status = 128 + WTERMSIG(status);

            remove_job_index(index);
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

    // Give terminal control back to the shell
    foreground_pgid = -1;
    tcsetpgrp(shell_terminal, shell_pgid);
}