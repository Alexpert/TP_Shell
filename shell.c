#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "readcmd.h"

int
main(int arc, char *argv[]) {
  char *login = "placeholder";
  char *machine = "placeholder";

  while(1) { //A changer quand je saurai utiliser la commande exit
		struct cmdline *line;

    printf("%s@%s>", login, machine);
		line = readcmd();

    pid_t pid = fork();
    if (pid < 0) { //PID < 0: Erreur lors de fork
      exit(1);
    } else if (pid > 0) { //PID > 0: Père
      int spid;
      wait(&spid);
    } else { //PID = 0: Fils
      execvp(line->seq[0][0], line->seq[0]);
    }

  }
}
