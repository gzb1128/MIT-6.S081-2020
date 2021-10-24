// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

extern uint64 addRefCount(uint64 pa);
extern uint64 delRefCount(uint64 pa);
extern uint64 setRefCount(uint64 pa);

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
uint64 refCount[PHYSTOP/PGSIZE];

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  acquire(&kmem.lock);
  for(int i=0;i<PHYSTOP/PGSIZE;++i)
    refCount[i]=0;
  release(&kmem.lock);
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    memset(p, 1, PGSIZE);
    struct run *r;
    r = (struct run*)p;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  if(delRefCount((uint64)pa)>0)
    return;
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
  {
    memset((char*)r, 5, PGSIZE); // fill with junk
    setRefCount((uint64)r);
  }
  return (void*)r;
}

uint64 addRefCount(uint64 pa)
{
  pa/=PGSIZE;
  acquire(&kmem.lock);
  uint64 t=++refCount[pa];
  release(&kmem.lock);
  if(t>>63) panic("addRef");
  return t;
}
uint64 delRefCount(uint64 pa)
{
  pa/=PGSIZE;
  acquire(&kmem.lock);
  uint64 t=--refCount[pa];
//  printf("%d\n",t);
  release(&kmem.lock);
  return t;
}

uint64 setRefCount(uint64 pa)
{
  pa/=PGSIZE;
  acquire(&kmem.lock);
  refCount[pa]=1;
  release(&kmem.lock);
  return 1;
}