#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
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
    char *newDir;
    if (cmd[1])
      newDir = strcmp(cmd[1], "~") ? cmd[1] : getpwuid(getuid())->pw_dir;
    else
      newDir = getpwuid(getuid())->pw_dir;

    if (chdir(newDir) == -1) {
      printf("Directory not found: %s\n", newDir);
    }
    return 1;
  }
  return 0;
}

char *
simplWorkingDir(char *workingDir) {
  char *homeDir = getpwuid(getuid())->pw_dir;
  char *simplWD = malloc(sizeof(char) * strlen(workingDir));

  int i = 0;
  while (homeDir[i] && homeDir[i] == workingDir[i])
    ++i;

  if (homeDir[i]) {
    strcpy(simplWD, workingDir);
  } else {
    simplWD[0] = '~';
    strcpy(&simplWD[1], &workingDir[i]);
  }

  return simplWD;
}

void
printPrompt() {
  char *login = getlogin();
  char machine[255];
  char workingDir[255];
  gethostname(machine, 255);
  getcwd(workingDir, 255);
  printf("%s@%s:%s\n--> ", login, machine, simplWorkingDir(workingDir));
}

int
main(int arc, char *argv[]) {

  while(1) {
		struct cmdline *line;

    printPrompt();
		line = readcmd();

    int nbCommands = countCommands(line);

    int isBuiltIn = builtIn(line->seq[0]);

    //Pipes
    int pipes[nbCommands - 1][2];

    //FORK
    pid_t pid = 1;
    int cmd_id = 0;

    if (!isBuiltIn) {
      while (pid && cmd_id < nbCommands) {
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
        if (cmd_id == 0) { //Si c'est la première commande
          if (line->in != NULL) {
            FILE *in = fopen(line->in, "r");
            dup2(fileno(in), STDIN);
            fclose(in);
          }
        } else { //Si ce n'est pas la première commande
          close(STDIN);
          dup2(pipes[cmd_id - 1][READ], STDIN);
          close(pipes[cmd_id - 1][READ]);
          close(pipes[cmd_id - 1][WRITE]);
        }

        if (cmd_id == nbCommands - 1) { //Si c'est la dernière commande
          if (line->out != NULL) {
            FILE *out = fopen(line->out, "a");
            dup2(fileno(out), STDOUT);
            // dup2(fileno(out), STDERR);
            fclose(out);
          }
        } else { //Si ce n'est pasla dernière commande
          close(STDOUT);
          dup2(pipes[cmd_id][WRITE], STDOUT);
          // dup2(pipes[cmd_id][WRITE], STDERR);
          close(pipes[cmd_id][WRITE]);
          close(pipes[cmd_id][READ]);
        }


        execvp(line->seq[cmd_id][0], line->seq[cmd_id]);

      //Le père attends la mort des Fils
      } else {
        int status;
        for (int i = 0; line->fg && i < nbCommands; ++i) {
          wait(&status);
        }
      }
    }
  }
}
