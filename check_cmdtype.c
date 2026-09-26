#include "main.h" 
 
int check_command_type(char *command) 
{ 
    // List of built-in commands
    const char *builtins[] = { 
        "cd", "mkdir", "rmdir", "exit", "pwd", 
        "echo", "jobs", "fg", "bg", NULL 
    }; 
 
    // Check for empty command
    if (command == NULL || *command == '\0') 
        return NO_COMMAND; 
 
    // Check if command is a built-in command
    for (int i = 0; builtins[i] != NULL; i++) 
    { 
        // Compare the command with each built-in
        if (strcmp(command, builtins[i]) == 0) 
            return BUILTIN; 
    } 
 
    // Command is not built-in
    return EXTERNAL; 
} 
 
void extract_external_commands(char **commands) 
{ 
    FILE *fp; 
    char buffer[256]; 
    int count = 0; 
 
    // Open the external commands file
    fp = fopen("externalcmds.txt", "r"); 
 
    // Check if the file opened successfully
    if (fp == NULL) 
    { 
        commands[0] = NULL; 
        return; 
    } 
 
    // Read external commands from the file
    while (fgets(buffer, sizeof(buffer), fp) != NULL && count < 499) 
    { 
        // Remove newline character from the command
        buffer[strcspn(buffer, "\r\n")] = '\0'; 
 
        // Skip empty lines
        if (buffer[0] == '\0') 
            continue; 
 
        // Allocate memory for the command
        commands[count] = malloc(strlen(buffer) + 1); 
 
        // Stop if memory allocation fails
        if (commands[count] == NULL) 
            break; 
 
        // Copy the command into the allocated memory
        strcpy(commands[count], buffer); 
        count++; 
    } 
 
    // Mark the end of the command list
    commands[count] = NULL; 
    fclose(fp); 
} 