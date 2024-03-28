#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

// Simple system call
int
getgpid(void)
{
    struct proc* curr_proc = myproc();
    if (curr_proc == 0 || curr_proc->parent == 0 || curr_proc->parent->parent == 0) return 0;  // null handling
    return curr_proc->parent->parent->pid;
}

// Wrapper function
int
sys_getgpid(void)
{
    return getgpid();
}
