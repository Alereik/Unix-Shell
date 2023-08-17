#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

char error_message[30] = "An error has occurred\n";
const int MAX = 20;

/**
 * Takes a line and splits it into args similar to how argc
 * and argv work in main.
 *
 * @parameter: line - The line being split up. Mangled in function execution.
 * @parameter: args - Pointer to array of strings, filled with args from line.
 * @parameter: num_args - Poiunter to integer number of arguments parsed.
 * @return: 0 on success, -1 on failure.
 */
int lexer(char *line, char ***args, int *num_args) {
  *num_args = 0;
  // count number of args
  char *l = strdup(line);
  if (l == NULL) {
    return -1;
  }
  char *token = strtok(l, " \t\n");
  while (token != NULL) {
    (*num_args)++;
    token = strtok(NULL, " \t\n");
  }
  free(l);
  // split line into args
  *args = malloc(sizeof(char **) * *num_args);
  *num_args = 0;
  token = strtok(line, " \t\n");
  while (token != NULL) {
    char *token_copy = strdup(token);
    if (token_copy == NULL) {
      return -1;
    }
    (*args)[(*num_args)++] = token_copy;
    token = strtok(NULL, " \t\n");
  }
  return 0;
}

/**
 * Finds and returns the number of arguments in a command.
 * 
 * @parameter: args - The array of arguments.
 * @return: The number of arguments in the array.
 */
int getnumargs(char** args) {
  int num_args = 0;
  while (args[num_args] != NULL) {
    ++num_args;
  }
  return num_args;
}

/**
 * Forks a child process and executes the command, piping or redirecting the 
 * output to the next process or specified file if required.
 * 
 * @parameter: cmds - The array of commands to be executed.
 * @parameter: num_cmds - The number of commands in the array.
 */
int pipeline(char*** cmds, size_t num_cmds) {
  int pipe_fds[num_cmds - 1][2];
  pid_t pids[num_cmds];
  int output_fd = -1;

  for (int i = 0; i < num_cmds - 1; ++i) {
    if (pipe(pipe_fds[i]) == -1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      _Exit(0);
    }
  }

  for (int i = 0; i < num_cmds; ++i) {
    int num_args = getnumargs(cmds[i]);
    pids[i] = fork();
    if (pids[i] < 0) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      _Exit(0);
    }
    else if (pids[i] == 0) { // child processes
      if (i > 0) { // all but first set up input
        close(pipe_fds[i - 1][1]);
        dup2(pipe_fds[i - 1][0], STDIN_FILENO);
        close(pipe_fds[i - 1][0]);
      }
      if (i < num_cmds - 1) { // all but last set up output
        close(pipe_fds[i][0]);
        dup2(pipe_fds[i][1], STDOUT_FILENO);
        close(pipe_fds[i][1]);
      }
      else if (num_args > 2 && cmds[i][num_args - 2] != NULL
               && strcmp(cmds[i][num_args - 2], ">") == 0) {
        output_fd = open(cmds[i][num_args - 1],
                         O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_fd == -1) {
          write(STDERR_FILENO, error_message, strlen(error_message));
          _Exit(0);
        }
        dup2(output_fd, STDOUT_FILENO); // Redirect stdout to the file
        close(output_fd);
        cmds[i][num_args - 2] = NULL; // only output data prior to ">"
      }
      execv(cmds[i][0], cmds[i]); // execute process
      write(STDERR_FILENO, error_message, strlen(error_message));
      _Exit(0);
    }
  }

  for (int i = 0; i < num_cmds - 1; ++i) { // close parent pipes
    close(pipe_fds[i][0]);
    close(pipe_fds[i][1]);
  }
  while (wait(NULL) > 0); // wait for child procs to finish
  if (output_fd != -1) {
    close(output_fd);
  }
  return 0;
}

/**
 * Checks a token to verify whether or not it is comprised of number
 * characters.
 *
 * @parameter: token - The string token to be checked.
 * @return: 1 if number, 0 otherwise.
 */
int isnumber(char* token) {
  for (int i = 0; token[i] != '\0'; ++i) {
    if (token[i] < '0' || token[i] > '9') {
      return 0;
    }
  }
  return 1;
}

/**
 * Trims the leqading whitespace from the string of commands.
 *
 * @parameter: line - The string of commands.
 */
void trimleading(char* line) {
  int i = 0, j;
  while (line[i] == ' ' || line[i] == '\t' || line[i] == '\n') {
    i++;
  }
  if (i != 0) {
    j = 0;
    while (line[j + i] != '\0') {
      line[j] = line[j + i];
      j++;
    }
    line[j] = '\0';
  }
}

/**
 * Drops the first token of the string of commands.
 *
 * @parameter: line - The string of commands.
 */
void droptoken(char* line) {
  char* linecpy;
  char* token;
  if ((linecpy = strdup(line)) == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    _Exit(0);
  }
  token = strtok(linecpy, " \t\n");
  if (token != NULL) {
    memmove(line, line + strlen(token), strlen(line) - strlen(token));
    trimleading(line);
    line[strlen(line) - strlen(token)] = '\0';
  }
  else {
    *line = '\0';
  }
  free(linecpy);
}

/**
 * Splits the string of commands based on a specified delimiter. Returns
 * the pointer to the resulting 2D array of strings, and updates the
 * value pointed to by num_pcs.
 *
 * @parameter: str_arr - Pointer to the resulting split line of commands.
 * @parameter: input_ln - The line of commands to be split.
 * @parameter: delim - The specified delimiter to split input_ln on.
 * @parameter: num_pcs - Pointer to the number of pieces str_arr contains.
 * @return: str_arr - Pointer to the resulting split line of commands
 */
char** splitline(char** str_arr, char* input_ln, char* delim, int* num_pcs) {
  int i = 0;
  while ((str_arr[i] = strsep(&input_ln, delim)) != NULL) {
    if (strcmp(str_arr[i], "\0") != 0 && strcmp(str_arr[i], "\n") != 0
        && strcmp(str_arr[i], "\t") != 0  && strcmp(str_arr[i], "") != 0) {
      i++;
    }
  }
  *num_pcs = i;
  return str_arr;
}

/**
 * Checks the string of commands 'line' to see if it must be looped.
 * Calls trimleading() to first remove leading whitespace.
 *
 * @parameter: line - The string of commands.
 * @return: has_loop - 1 if loop, 0 if no loop.
 */
int loopcheck(char* line) {
  int has_loop = 0;
  char* linecpy;
  char* token;
  if ((linecpy = strdup(line)) == NULL) {
    return -1;
  }
  trimleading(linecpy);
  if ((token = strtok(linecpy, " \t\n")) != NULL 
      && strcmp(token, "loop") == 0) {
    has_loop = 1;
  }
  free(linecpy);
  return has_loop;
}

/**
 * Retrieves the number after the loop command and returns it as an integer.
 * When called, the token containing the word 'loop' has already been dropped
 * and the number to be retrieved should now be the first token in the line.
 * Calls trimleading() to first remove leading whitespace, and isnumber() to
 * verify that a number follows the loop command.
 *
 * @parameter: line - The string containing the loop command.
 * @return: num_loops - The number of times to run the looped command. If
 *                      no valid number given, returns -1.
 */
int getnumloops(char* line) {
  int num_loops = -1;
  char* linecpy;
  char* token;
  if ((linecpy = strdup(line)) == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    _Exit(0);
  }
  trimleading(linecpy);
  if ((token = strtok(linecpy, " \t\n")) != NULL && isnumber(token) == 1
      && atoi(token) > 0) {
    num_loops = atoi(token);
  }
  free(linecpy);
  return num_loops;
}

/**
 * Uses the chdir() system call with the argument supplied by the user
 * in order to change the current working directory.
 *
 * @parameter: args - The arguments supplied by the user.
 */
void changedir(char** args) {
  if ((strcmp(args[1], "~") == 0 && chdir(getenv("HOME")) == -1)
      || chdir(args[1]) == -1) {
    write(STDERR_FILENO, error_message, strlen(error_message));
  }
}

/**
 * Uses the getcwd() system call to retrieve and print out the path
 * of the current working directory to the user.
 */
void printworkingdir(void) {
  int path_max = 256;
  char path[path_max];
  if (getcwd(path, path_max) == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    _Exit(0);
  }
  printf("%s\n", path);
}

/**
 * Checks for the 'exit', 'pwd', and 'cd' built-in commands and then
 * executes them. The lexer() function is first called in order to
 * parse the current line of commands.
 *
 * @parameter: line - The line of commands to be parsed and checked.
 * @return: retval - 1 if built-in command was executed, 0 otherwise.
 */
int builtinhandler(char* line) {
  int retval = 0;
  int num_args;
  char* linecpy = strdup(line);
  char** args = (char**) malloc(sizeof(char *) * MAX);
  if (linecpy == NULL || args == NULL
      || lexer(linecpy, &args, &num_args) == -1) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    _Exit(0);
  }

  if (strcmp(args[0], "exit") == 0) {
    exit(0);
  }
  else if (strcmp(args[0], "pwd") == 0) {
    printworkingdir();
    retval = 1;
  }
  else if ((num_args == 2) && (strcmp(args[0], "cd") == 0)) {
    changedir(args);
    retval = 1;
  }

  for (int i = 0; i < num_args; ++i) {
    free(args[i]);
  }
  free(linecpy);
  free(args);
  return retval;
}

/**
 * Allocates memory for the array of commands to be executed.
 * 
 * @parameter: cmd_arr - The array of commands to be executed.
 */
void cmdarrayalloc(char *** cmd_arr) {
  for (int i = 0; i < MAX; ++i) {
    cmd_arr[i] = (char **) malloc(sizeof(char *) * MAX);
    if (cmd_arr[i] == NULL) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      _Exit(0);
    }
  }
}

/**
 * Splits the string of commands into an array of strings by the '|' delimiter,
 * then calls lexer() to parse each string into an array of commands. The array
 * of commands is then executed by the pipeline() function.
 * 
 * @parameter: line - The string of commands to be executed.
 */
void splitpiped(char* line) {
  char* linecpy = strdup(line);
  char** str_arr = (char **) malloc(sizeof(char *) * MAX);
  char*** cmd_arr = (char ***) malloc(sizeof(char **) * MAX);
  if (linecpy == NULL || cmd_arr == NULL ||str_arr == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    _Exit(0);
  }
  int num_pcs;
  str_arr = splitline(str_arr, linecpy, "|", &num_pcs);
  cmdarrayalloc(cmd_arr);
  int num_args[num_pcs];
  for (int i = 0; i < num_pcs; ++i) {
    lexer(str_arr[i], &cmd_arr[i], &num_args[i]);
    cmd_arr[i][num_args[i]] = NULL;
  }

  pipeline(cmd_arr, num_pcs);

  for (int i = 0; i < num_pcs; ++i) {
    free(cmd_arr[i]);
  }
  free(cmd_arr);
}

/**
 * Splits the string of commands into an array of strings by the ';' delimiter,
 * then checks each piece for the loop command by calling loopcheck(). If there
 * is a loop, the loop command is dropped by calling droptoken() and the number
 * of loops is retrieved by calling the getnumloops() function. After the
 * commands are checked for loops, builtinhandler() is called to check for and
 * execute built-in commands such as 'pwd', 'cd', and 'exit'. Afterwards,
 * splitpiped() is called to further process the command lines for pipes,
 * redirects, and eventual execution.
 *
 * @parameter: line - The string to be split.
 */
void splitchained(char* line) {
  char** str_arr = (char **) malloc(sizeof(char *) * MAX);
  if (str_arr == NULL) {
    write(STDERR_FILENO, error_message, strlen(error_message));
    _Exit(0);
  }
  int num_pcs;
  str_arr = splitline(str_arr, line, ";", &num_pcs);

  for (int i = 0; i < num_pcs; ++i) {
    int num_runs = 1;
    if (loopcheck(str_arr[i]) == 1) {
      int num_loops;
      droptoken(str_arr[i]);
      if ((num_loops = getnumloops(str_arr[i])) >= 1) {
        num_runs = num_loops;
        droptoken(str_arr[i]);
      }
      else {
        write(STDERR_FILENO, error_message, strlen(error_message));
        return;
      }
    }
    for (int j = 0; j < num_runs; ++j) {
      if (builtinhandler(str_arr[i]) == 0) { // execute built-in cmds, if any
        splitpiped(str_arr[i]);
      }
    }
  }
  free(str_arr);
}

/**
 * Main function, contains the shell prompt. Calls splitchained() to handle
 * command lines input by the user.
 *
 * @parameter: argc - Number of arguments, should not be > 1.
 * @parameter: argv - The argument as a string.
 */
int main(int argc, char* argv[]) {
  if (argc > 1) {
    write(STDERR_FILENO, error_message, strlen(error_message));
  }
  char* line = NULL;
  size_t len = 0;
  ssize_t nread;

  while (1) {
    printf("smash> ");
    fflush(NULL);
    if ((nread = getline(&line, &len, stdin)) == -1) {
      write(STDERR_FILENO, error_message, strlen(error_message));
      _Exit(0);
    }
    if (strcmp(line, "\n") == 0) {
      continue;
    }
    splitchained(line);
  }
}
