#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{

   if (argc < 3) {
      printf(1, "Usage: spawn ticks tickets1 [tickets2]...\n\n");
      printf(1, "Spawns subprocesses, each of which will run for \n"
            "approximately the given number of ticks. For each ticket\n"
            "ammount given, a usage process will spawn and request that\n"
            "number of tickets.\n");
      exit();
   }

   /* arguments for spawned processes */
   char *args[4];
   args[0] = "usage";
   args[1] = argv[1];
   args[2] = "(tickets)";
   args[3] = NULL;

  int i;
  for (i = 2; i < argc; i++) {
     int pid = fork();
     if (pid == 0) {
        args[2] = argv[i];
        exec(args[0], args);
     }
  }

  for (i = 2; i < argc; i++) {
     wait();
  }

  exit();
}
