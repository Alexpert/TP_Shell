#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "readcmd.h"

#define STDIN 0
#define STDOUT 1
#define STDERR 2

int
main(int arc, char *argv[]) {
  char *login = "placeholder";
  char *machine = "placeholder";

  int saved_stdin = dup(STDIN);
  int saved_stdout = dup(STDOUT);

  while(1) { //A changer quand je saurai utiliser la commande exit
		struct cmdline *line;

    printf("%s@%s>", login, machine);
		line = readcmd();

    //Gestion des redirections STDIN STDOUT STDERR
    if (line->in != NULL) {
      FILE *in = fopen(line->in, "r");
      dup2(fileno(in), STDIN);
      fclose(in);
    }

    if (line->out != NULL) {
      FILE *out = fopen(line->out, "a");
      dup2(fileno(out), STDOUT);
      dup2(fileno(out), STDERR);
      fclose(out);
    }

    pid_t pid = fork();
    if (pid < 0) { //PID < 0: Erreur lors de fork
      exit(1);
    } else if (pid > 0) { //PID > 0: Père
      int spid;

      dup2(saved_stdin, STDIN);
      dup2(saved_stdout, STDOUT);
      dup2(saved_stdout, STDERR);

      wait(&spid);
    } else { //PID = 0: Fils
      execvp(line->seq[0][0], line->seq[0]);
    }

  }
}
