/* test 목록
 * 1. pid가 짝수인 프로세스는 [L0 > L2 > L3]이고, 홀수인 프로세스는 [L0 > L1 > L3]인지
 * 2. 100 ticks마다 모두 L0에 올라오는지 (MoQ 포함)
 * 3. MoQ에 넣고 독점 시작하면 MoQ만 실행하는지
 * 4. MoQ empty면 자동으로 독점 푸는지
 * 5. MoQ 비번 틀리면 안되는지
 * 6. yield 잘 되는지
 * 7. 
 */

#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_LOOP 100000
#define NUM_YIELD 20000
#define NUM_SLEEP 1000

#define NUM_THREAD 4
#define MAX_LEVEL 5

int parent;

// fork and sleep 10 ms
int fork_children1()
{
  int i, p;
  for (i = 0; i < NUM_THREAD; i++)
    if ((p = fork()) == 0)  // child
    {
      sleep(10);
      return getpid();
    }
  return parent;
}

// fork in put in MoQ
int fork_children2()
{
  int i, p;
  for (i = 0; i < NUM_THREAD; i++)
  {
    if ((p = fork()) == 0)  // child
    {
      sleep(10);
      return getpid();
    }
  }
  return parent;
}

int max_level;

int fork_children3()
{
  int i, p;
  for (i = 0; i < NUM_THREAD; i++)
  {
    if ((p = fork()) == 0)  // child
    {
      sleep(10);
      max_level = i;
      return getpid();
    }
  }
  return parent;
}

void exit_children()
{
  if (getpid() != parent)
    exit();
  while (wait() != -1);
}

int main(int argc, char *argv[])
{
  exit();
}
