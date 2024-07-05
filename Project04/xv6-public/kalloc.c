// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
  int fpcnt;
  int pgrefc[PHYSTOP/PGSIZE];
} kmem;


void
__incr_refc(uint pa)
{
  kmem.pgrefc[pa/PGSIZE]++;
}

void
__decr_refc(uint pa)
{
  kmem.pgrefc[pa/PGSIZE]--;
}

int
__get_refc(uint pa)
{
  int res;
  res = kmem.pgrefc[pa/PGSIZE];
  if (res < 0)
    panic("negative refc");
  return res;
}

void
incr_refc(uint pa)
{
  acquire(&kmem.lock);
  __incr_refc(pa);
  release(&kmem.lock);
}

void
decr_refc(uint pa)
{
  acquire(&kmem.lock);
  __decr_refc(pa);
  release(&kmem.lock);
}

int
get_refc(uint pa)
{
  acquire(&kmem.lock);
  int res = __get_refc(pa);
  release(&kmem.lock);
  return res;
}


// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void
kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  kmem.fpcnt = 0;
  freerange(vstart, vend);
}

void
kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void
freerange(void *vstart, void *vend)
{
  char *p;
  p = (char*)PGROUNDUP((uint)vstart);
  for(; p + PGSIZE <= (char*)vend; p += PGSIZE) {
    kmem.pgrefc[V2P(p)/PGSIZE] = 0;
    kfree(p);
  }
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(char *v)
{
  struct run *r;

  if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  if(kmem.use_lock)
    acquire(&kmem.lock);

  if (__get_refc(V2P(v)))
    __decr_refc(V2P(v));
  if (!__get_refc(V2P(v))) {
    // Fill with junk to catch dangling refs.
    memset(v, 1, PGSIZE);
    r = (struct run*)v;

    r->next = kmem.freelist;
    kmem.freelist = r;
    kmem.fpcnt++;
  }

  if(kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char*
kalloc(void)
{
  struct run *r;

  if(kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;
  if(r) {
    kmem.fpcnt--;
    kmem.pgrefc[V2P((char*)r)/PGSIZE] = 1;
    kmem.freelist = r->next;
  }
  if(kmem.use_lock)
    release(&kmem.lock);
  return (char*)r;
}

// system calls

int
sys_countfp(void)
{
  acquire(&kmem.lock);
  int res = kmem.fpcnt;
  release(&kmem.lock);
  return res;
}

