// Includes C Libraries
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Includes Header
#include "input.h"

// Function: fetchInput
bool fetchInput(char **commands, size_t *cmd_size) {
  int c = 0; // Current char int
  int i = 0; // NUmber of loops

  // Loops over stdin and get shell user input starting at char [0]
  while ((c = getchar()) != '\n' && c != EOF) {
    // Checks for buffer overflow and safely resizes it to match buffer size
    if (i >= (*cmd_size) - 1) {
      char *tmp_commands = realloc(*commands, ((*cmd_size) *= 2));

      if (tmp_commands == NULL) {
        perror("\nFailed to allocate commands (input.c) ->");
        return false;
      }

      *commands = tmp_commands;
    }

    (*commands)[i++] = (char)c;
  }

  // Adds null terminator at last block
  (*commands)[i] = '\0';

  return true;
}

// Function: getCommands
bool getCommands(Shell *shell, Command *cmds) {
  // Get the command head (head)
  if (!getCommandHead(shell, cmds)) {
    perror("\nFailed to getCommandHead (input.c) ->");
    free(cmds);
    return false;
  }

  // Get the command body (body)
  if (!getCommandBody(shell, cmds)) {
    perror("\nFailed to getCommandBody (input.c) ->");
    free(cmds);
    return false;
  }

  return true;
}

// Function: getCommandHead
bool getCommandHead(Shell *shell, Command *cmd) {
  int head_length = 64 * sizeof(char);
  cmd->head = malloc(head_length);

  if (cmd->head == NULL) {
    perror("\nFailed to allocate cmd->head (input.c) ->");
    return false;
  }

  // Placeholder if a letter is found
  // to deal with >___command where _ are whitespaces
  bool letterFound = false;

  // Initializes number of loops
  int i = 0;

  // Initilizes current char location
  // On shell->commands shell user input
  int j = 0;

  // Starts loop
  while (true) {
    //  Assigns current character
    char c = shell->commands[j];

    // If end of shell user input data
    if (c == '\0' || c == '\n' || c == ' ' || !c) {
      /// If end is whitespace and letter is not yet found
      if (c == ' ' && !letterFound) {
        j++;
        continue;
      }

      // Else break (end of head)
      break;
    }

    // If number of loops exceeds allocated size
    if (i >= head_length - 1) {
      // Safely reallocates new size by doubling space for new chars
      char *tmp_target = realloc(cmd->head, sizeof(char) * (head_length *= 2));

      if (tmp_target == NULL) {
        perror("\nFailed to re-allocate cmd->head (input.c) ->");
        free(cmd->head);
        return false;
      }

      // Assigns allocated space address to main variable
      cmd->head = tmp_target;
    }

    cmd->head[i++] = c;
    letterFound =
        true; // Assigns true for letter is found, proof after endline checks
    j++;      // Increments for next character
  }

  if (!letterFound) {
    free(cmd->head);
    cmd->head = malloc(sizeof(char));
    cmd->head[0] = '\0';
    cmd->head_length = 0;
    return true;
  }

  // Assign last reserved space for null terminal
  cmd->head[i] = '\0';

  // Assign j as head size
  cmd->head_length = ++j;

  return true;
}

// Function: getCommandBody
bool getCommandBody(Shell *shell, Command *cmd) {
  int curr_c = cmd->head_length;
  char tmp_c = shell->commands[(curr_c > 0) ? curr_c - 1 : 0];

  // Checks for no body
  if (tmp_c == '\n' || tmp_c == '\0' || !tmp_c || cmd->head_length == 0)
    return true;

  int bodySize = 1;
  cmd->body = malloc(sizeof(char *) * bodySize);

  // Assigns number of loops
  int i = 0;

  // Starts loop for arg list
  while (true) {
    // Updates current char for i > 0 for current arg
    tmp_c = shell->commands[curr_c];

    if (tmp_c == ' ') {
      // Skips for next argument
      curr_c++;      
      continue;
    }

    // If end of command, break (no body left)
    if (tmp_c == '\n' || tmp_c == '\0' || !tmp_c)
      break;

    if (i >= bodySize) {
      // Safely resizes allocated arg by +1 (Add new arg to the array)
      char **tmp_body = realloc(cmd->body, (++bodySize) * sizeof(char *));

      if (tmp_body == NULL) {
        perror("\nFailed to re-allocate cmd->body (input.c) ->");

        for (int n = 0; n < (i + 1); n++) {
          free(cmd->body[n]);
        }

        free(cmd->body);

        return false;
      }

      cmd->body = tmp_body;
    }

    int tmp_size = 32 * sizeof(char);
    char *tmp = malloc(tmp_size);

    if (tmp == NULL) {
      perror("\nFailed to allocate tmp (input.c) ->");

      for (int n = 0; n < (i + 1); n++) {
        free(cmd->body[n]);
      }

      free(cmd->body);

      return false;
    }

    // Assigns number of loops for strings
    int j = 0;

    // Starts loop for body list string chars
    while (true) {
      // If number of loops exceeds allocated size
      if (j >= tmp_size - 1) {
        // Doubles allocation size and store safely
        char *tmp_tmp = realloc(tmp, sizeof(char) * (tmp_size *= 2));

        if (tmp_tmp == NULL) {
          perror("\nFailed to re-allocate tmp (input.c) ->");

          for (int n = 0; n < (i + 1); n++) {
            free(cmd->body[n]);
          }

          free(cmd->body);

          return false;
        }

        tmp = tmp_tmp;
      }

      // Update current char with post-increment for next char
      tmp_c = shell->commands[curr_c++];

      // if (tmp_c == '$' && tmp_c == )
      // If char is end break loop for new arg
      if (tmp_c == '\n' || tmp_c == ' ' || tmp_c == '\0' || !tmp_c) {
        // Decrement to avoid missing out the null terminator
        curr_c--;
        break;
      }

      // Else, assign current char to current arg block
      tmp[j++] = tmp_c;
    }

    // Assign last block reserved for null terminator
    tmp[j] = '\0';

    // Assign string to arg number i
    cmd->body[i++] = tmp;
  }

  // Assign body_size
  cmd->body_size = bodySize;
  return true;
}
