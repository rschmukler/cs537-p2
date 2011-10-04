#include "types.h"
#include "stat.h"
#include "user.h"

/* Spin for a while */
int
spin() {
   int i, j;
   for (i = 0; i < 10000000; i++) {
      j = i % 11;
   }
   return j;
}

int
main(int argc, char *argv[])
{
  if (argc != 3) {
     printf(1, "Usage: usage ticks tickets\n\n");
     printf(1, "Requests the given number of tickets, then prints out usage\n"
           "info until approximately the given number of ticks have passed.\n");
     exit();
  }
  int pid = getpid();
  int end = uptime() + atoi(argv[1]);
  int tickets = atoi(argv[2]);

  int ret = settickets(tickets);
  if (ret < 0) {
     printf(1, "settickets failed\n");
     exit();
  }

  while (uptime() < end) {
    spin();
    int usage = getusage();
    printf(1, "pid: %d  tickets: %d  time: %d  usage: %d\n", pid, tickets,
          uptime(), usage);
  }
  exit();
}
