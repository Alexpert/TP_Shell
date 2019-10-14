#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "readcmd.h"

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define READ 0
#define WRITE 1


int
main(int arc, char *argv[]) {
  char *login = getlogin();
  char machine[255];
  gethostname(machine, 255);

  while(1) { //A changer quand je saurai utiliser la commande exit
		struct cmdline *line;

    printf("%s@%s>", login, machine);
		line = readcmd();

    //Pipes
    int nbCommands = countCommands(line);
    int pipes[nbCommands - 1][2];

    for (int i = 0; i < nbCommands; ++i)
      pipe(pipes[i]);

    //FORK
    pid_t pid = 1;
    int cmd_id = 0;

    while (pid && cmd_id < nbCommands) {
      fprintf(stderr, "FORK\n");
      pid = fork();
      if (pid != 0) {
        cmd_id++;
      }
    }

    //Le Fils gère ses Entrées Sorties
    if (pid == 0) {
      if (cmd_id == 0) {
        fprintf(stderr, "%d: first command\n", cmd_id);
        if (line->in != NULL) {
          fprintf(stderr, "%d: Redirect STDIN\n", cmd_id);
          FILE *in = fopen(line->in, "r");
          dup2(fileno(in), STDIN);
          fclose(in);
        }
      } else {
        fprintf(stderr, "%d: not first command\n", cmd_id);
        dup2(pipes[cmd_id - 1][READ], STDIN);
        close(pipes[cmd_id - 1][READ]);
      }

      if (cmd_id == nbCommands - 1) {
        fprintf(stderr, "%d: last command\n", cmd_id);
        if (line->out != NULL) {
          fprintf(stderr, "%d: Redirect STDOUT\n", cmd_id);
          FILE *out = fopen(line->out, "a");
          dup2(fileno(out), STDOUT);
          // dup2(fileno(out), STDERR);
          fclose(out);
        }
      } else {
        fprintf(stderr, "%d: not last command\n", cmd_id);
        dup2(pipes[cmd_id][WRITE], STDOUT);
        // dup2(pipes[cmd_id][WRITE], STDERR);
        close(pipes[cmd_id][WRITE]);
      }


      fprintf(stderr, "%d: exec: %s\n", cmd_id, line->seq[cmd_id][0]);
      execvp(line->seq[cmd_id][0], line->seq[cmd_id]);

    //Le père attends la mort des Fils
    } else {
      int status;
      for (int i = 0; i < nbCommands; ++i) {
        wait(&status);
        fprintf(stderr, "Son died: %d / %d\n", i + 1, nbCommands);
      }
    }
  }
}
