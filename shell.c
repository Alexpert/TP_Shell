#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "readcmd.h"

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define READ 0
#define WRITE 1

int
builtIn(char **cmd) {
  if (cmd && !strcmp(cmd[0], "exit")) {
    exit(0);
  }
  if (cmd && !strcmp(cmd[0], "cd")) {
    char *newDir = strcat(cmd[1], "/");
    if (chdir(newDir) == -1) {
      printf("Directory not found\n");
    }
    return 1;
  }
  return 0;
}

void
printPrompt() {
  char *login = getlogin();
  char machine[255];
  char workingDir[255];
  gethostname(machine, 255);
  getcwd(workingDir, 255);
  printf("%s@%s:%s\n--> ", login, machine, workingDir);
}

int
main(int arc, char *argv[]) {

  while(1) {
		struct cmdline *line;

    printPrompt();
		line = readcmd();

    int isBuiltIn = builtIn(line->seq[0]);


    // if (!line->fg)
    //   printf("Background cmd\n");

    //Pipes
    int nbCommands = countCommands(line);
    int pipes[nbCommands - 1][2];

    //FORK
    pid_t pid = 1;
    int cmd_id = 0;

    if (!isBuiltIn) {
      while (pid && cmd_id < nbCommands) {
        //fprintf(stderr, "FORK\n");
        if (cmd_id < nbCommands - 1)
          pipe(pipes[cmd_id]);

        pid = fork();

        if (pid != 0) {
          if (cmd_id > 0) {
            close(pipes[cmd_id - 1][READ]);
            close(pipes[cmd_id - 1][WRITE]);
          }
          cmd_id++;
        }
      }

      //Le Fils gère ses Entrées Sorties
      if (pid == 0) {
        if (cmd_id == 0) {
          // fprintf(stderr, "%d: first command\n", cmd_id);
          if (line->in != NULL) {
            // fprintf(stderr, "%d: Redirect STDIN\n", cmd_id);
            FILE *in = fopen(line->in, "r");
            dup2(fileno(in), STDIN);
            fclose(in);
          }
        } else {
          // fprintf(stderr, "%d: not first command\n", cmd_id);
          close(STDIN);
          dup2(pipes[cmd_id - 1][READ], STDIN);
          close(pipes[cmd_id - 1][READ]);
          close(pipes[cmd_id - 1][WRITE]);
        }

        if (cmd_id == nbCommands - 1) {
          // fprintf(stderr, "%d: last command\n", cmd_id);
          if (line->out != NULL) {
            // fprintf(stderr, "%d: Redirect STDOUT\n", cmd_id);
            FILE *out = fopen(line->out, "a");
            dup2(fileno(out), STDOUT);
            // dup2(fileno(out), STDERR);
            fclose(out);
          }
        } else {
          // fprintf(stderr, "%d: not last command\n", cmd_id);
          close(STDOUT);
          dup2(pipes[cmd_id][WRITE], STDOUT);
          // dup2(pipes[cmd_id][WRITE], STDERR);
          close(pipes[cmd_id][WRITE]);
          close(pipes[cmd_id][READ]);
        }


        // fprintf(stderr, "%d: exec: %s\n", cmd_id, line->seq[cmd_id][0]);
        execvp(line->seq[cmd_id][0], line->seq[cmd_id]);

      //Le père attends la mort des Fils
      } else {
        int status;
        for (int i = 0; line->fg && i < nbCommands; ++i) {
          wait(&status);
          //fprintf(stderr, "Son died: %d / %d\n", i + 1, nbCommands);
        }
      }
    }
  }
}
