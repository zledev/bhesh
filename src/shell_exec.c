#define _XOPEN_SOURCE 700

// C Libraries
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Included Headers
#include "input.h"
#include "shell_exec.h"
#include "utilities.h"

// Function: manageCommands
bool manageCommands(Command *cmds, Shell *shell)
{
  // HANDLE PIPES NOW

  bool err_status = true;
  bool isExternal = false;

  exec_internal_commands(cmds, &err_status, shell, &isExternal);
  if (isExternal)
    exec_external_commands(cmds, &err_status);

  // createExecList(cmds, shell);

  return err_status;
}

// Function: exec_external_commands
void exec_external_commands(Command *cmd, bool *err_status)
{
  pid_t pid = fork();

  if (pid < 0)
  {
    perror("\nFork failed: (shell_exec.h) ->");
    *err_status = false;
    return;
  }

  if (pid == 0)
  {
    // Size of the command (+2 for head and NULL)
    int size = cmd->body_size + 2;
    char **args = malloc(size * sizeof(char *));

    if (args == NULL)
    {
      perror("\nFailed to allocate args (shell_exec.c) ->");
      *err_status = false;
      return;
    }

    int n = 1;

    // Attach head at the beginning
    args[0] = malloc((strlen(cmd->head) * sizeof(char)) + 1);

    if (args[0] == NULL)
    {
      perror("\nFailed to allocate args[0] (shell_exec.c) ->");
      *err_status = false;
      free(args);
      return;
    }

    strcpy(args[0], cmd->head);

    if (cmd->body_size > 0)
    {
      // Attach command arguments
      while (true)
      {
        // Checks if command body is already attached
        if ((n - 1) == cmd->body_size)
          break;

        // n - 1 for starting at index 0
        args[n] = malloc((strlen(cmd->body[n - 1]) * sizeof(char)) + 1);

        if (args == NULL)
        {
          perror("\nFailed to allocate args (shell_exec.c) ->");
          *err_status = false;
          break;
        }

        // n - 1 for starting at index 0
        strcpy(args[n], cmd->body[n - 1]);
        n++;
      }

      if (!err_status)
      {
        for (int r = 0; args[r] != NULL; r++)
          free(args[r]);

        free(args);
        return;
      }
    }

    // Attaches NULL at the end
    args[n] = NULL;

    // Execute the external command and check for errors
    if (execvp(args[0], args) == -1)
    {
      fprintf(stderr, "'%s' -> No such command exists! :(\n", args[0]);
      *err_status = false;

      for (int r = 0; args[r] != NULL; r++)
        free(args[r]);

      free(args);
      exit(1);
    }
  }
  else
    wait(NULL);
}

// Function: exec_internal_commands
bool exec_internal_commands(Command *cmd, bool *err_status, Shell *shell, bool *isExternal)
{
  // Checks if shell user is trying to exit
  if ((strcmp(cmd->head, "exit") == 0) && (cmd->body_size == 0))
  {
    *err_status = false;
    return true;
  }

  // Initializes built-in shell commands
  char *shellCmdTargets[] = {
      "bhesh",
      "cd",
      "pwd",
      "export",
  };

  size_t command_size = sizeof(shellCmdTargets) / sizeof(shellCmdTargets[0]);

  int command = 0;
  bool external = true;

  // Check if command exists
  for (int i = 0; i < command_size; i++)
  {
    // If it matches command, return true thus executing condition
    if (strcmp(shellCmdTargets[i], cmd->head) == 0)
    {
      external = false;
      command = i + 1;
      break;
    }
  }

  if (external)
  {
    *isExternal = external;
    *err_status = true;
    return true;
  }

  switch (command)
  {
  // bhesh
  case 1:
    char *bhesh = "Bhesh v0.5.0\n"
                  "A random shell that nobody cares about.\n"
                  "MIT License © 2025 zleDev\n";

    printf("%s", bhesh);
    break;
  // cd
  case 2:
    if (cmd->body_size > 0)
    {
      if (cmd->body_size > 1)
      {
        printf("'%s' -> Too many arguments! >:(\n", cmd->head);
        break;
      }

      if (cmd->body[0] != NULL)
      {
        if (strcmp(cmd->body[0], "~") == 0)
        {
          chdir(shell->home_dir);
          setenv("PWD", shell->home_dir, 1);
          break;
        }

        int ch = chdir(cmd->body[0]);

        if (ch != 0)
        {
          switch (errno)
          {
          case EACCES:
            printf("'%s' -> Permission denied! >:(\n", cmd->body[0]);
            break;
          default:
            printf("'%s' -> No such file directory! >:(\n", cmd->body[0]);
            break;
          }

          break;
        }

        char *cwd = get_curr_dir();

        if (cwd == NULL)
        {
          perror("\nFailed to get cwd (shell_exec.c) ->");
          *err_status = false;
          break;
        }

        setenv("PWD", cwd, 1);
        free(cwd);
      }
    }
    break;
  // pwd
  case 3:
    if (cmd->body_size > 0)
    {
      if (strcmp(cmd->body[0], "-P") == 0)
      {
        if (cmd->body_size > 1)
        {
          printf("'%s' -> Too many arguments! >:(\n", cmd->body[0]);
          break;
        }

        char *link_path = get_curr_dir();

        if (link_path == NULL)
        {
          perror("\nPWD Physical Failed (shell_exec.c) ->");
          *err_status = false;
          break;
        }

        printf("Physical Directory: %s\n", link_path);
        free(link_path);
        break;
      }

      if (strcmp(cmd->body[0], "-L") == 0)
      {
        char *cwd = getenv("PWD");

        if (cwd != NULL && access(cwd, F_OK) == 0)
        {
          printf("Working Directory: %s\n", cwd);
          break;
        }

        printf("Working Directory: Unknown");
        break;
      }
    }

    char *cwd = getenv("PWD");

    if (cwd != NULL && access(cwd, F_OK) == 0)
    {
      printf("Working Directory: %s\n", cwd);
      break;
    }

    cwd = get_curr_dir();
    printf("Physical Directory: %s\n", cwd);
    free(cwd);
    break;
  // export
  case 4:
    /*
    if (cmd->body_size > 0)
    {
        if (cmd->body_size > 1)
        {
            printf("'%s' -> Too many arguments! >:(\n", cmd->head);
            break;
        }

        int var_name_length = 2 * sizeof(char);
        char *var_name = malloc(var_name_length);

        if (var_name == NULL)
        {
            perror("\nFailed to allocate var_name (shell_exec.c) ->");
            *err_status = false;
            break;
        }

        int path_length = 2 * sizeof(char);
        char *path = malloc(path_length);

        if (path == NULL)
        {
            perror("\nFailed to allocate path (shell_exec.c) ->");
            *err_status = false;
            break;
        }

        int j = 0;
        int i = 0;
        bool isPath = false;

        while (true)
        {
            if (cmd->body[0][i] == '\0')
                break;

            if (cmd->body[0][i] == '=')
            {
                isPath = true;
                i++;
                continue;
            }

            if (!isPath)
            {
                if ((var_name_length - 1) <= i)
                {
                    char *tmp = realloc(var_name, var_name_length *= 2);

                    if (tmp == NULL)
                    {
                        perror("\nFailed to re-allocate var_name (shell_exec.c)
    ->"); *err_status = false; break;
                    }

                    var_name = tmp;
                    continue;
                }

                var_name[i] = cmd->body[0][i];
                i++;
                continue;
            }

            if ((path_length - 1) <= j)
            {
                char *tmp = realloc(path, path_length *= 2);

                if (tmp == NULL)
                {
                    perror("\nFailed to re-allocate path (shell_exec.c) ->");
                    *err_status = false;
                    break;
                }

                path = tmp;
                continue;
            }

            path[j] = cmd->body[0][j + i];
            j++;
        }

        var_name[++i] = '\0';
        path[++j] = '\0';

        if (!errno)
        {
            free(var_name);
            free(path);
            break;
        }

        if (access(path, F_OK) != 0)
        {
            printf("'%s' -> No such file directory! >:(\n", path);
            free(path);
            free(var_name);
            break;
        }

        setenv(var_name, path, 1);
        break;
    }

    for (int n = 0; n < shell->exports_size; n++)
    {
        printf("    %s\n", shell->exports[n]);
    }
    */
    break;
  // unset
  case 5:
    break;
  default:
    *err_status = true;
    return false;
    break;
  } // More commands will be added later on

  return true;
}

CMD_Node *createExecList(Command *cmds, Shell *shell) {}
