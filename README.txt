MINISHELL FINAL VERSION

Compile:
    gcc *.c -Wall -Wextra -Werror -o minishell

Run:
    ./minishell

IMPORTANT:
Run the shell directly in a WSL/Linux terminal.

Ctrl+C at prompt:
    minishell$ ^C
    minishell$

Ctrl+Z at prompt:
    minishell$ ^Z
    minishell$

No extra Enter is required.

Job-control test:
    minishell$ sleep 10

Wait about 2 seconds and press Ctrl+Z.

Expected:
    ^Z
    [1] Stopped    PID sleep 10
    minishell$

Then:
    minishell$ jobs
    [1] Stopped    PID sleep 10

Resume:
    minishell$ bg %1

Check:
    minishell$ jobs
    [1] Running    PID sleep 10

Foreground:
    minishell$ fg %1

Builtins:
    cd mkdir rmdir exit pwd echo jobs fg bg

sleep is intentionally an external command so Ctrl+Z can stop the
child process and save it in the job table.
