// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"
#define NBUCKET 7

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
} bcache[NBUCKET];

void
binit(void)
{
  struct buf *b;

  // Create linked list of buffers
  for(int i=0;i<NBUCKET;++i)
  {
    initlock(&bcache[i].lock, "bcache");
    for(b = bcache[i].buf; b < bcache[i].buf+NBUF; b++)
      initsleeplock(&b->lock, "buffer");
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.

uint hash(uint x)
{
  return x*19260817%NBUCKET;
}

static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  uint id=hash(blockno);

  acquire(&bcache[id].lock);

  // Is the block already cached?
  for(b = bcache[id].buf; b < bcache[id].buf+NBUF; b++){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache[id].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  uint minref=-1;
  struct buf *index=0;
  uint mark=0;
  for(b = bcache[id].buf; b < bcache[id].buf+NBUF; b++)
    if(b->refcnt == 0&&minref>b->time) {
      index=b;
      minref=b->time;
      mark=1;
  }
  if(mark==0)
    panic("bget: no buffers");
  index->dev = dev;
  index->blockno = blockno;
  index->valid = 0;
  index->refcnt = 1;

  release(&bcache[id].lock);
  acquiresleep(&index->lock);
  return index;
}



// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  uint id=hash(b->blockno);


  acquire(&bcache[id].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->time=ticks;
  }
  
  release(&bcache[id].lock);
}

void
bpin(struct buf *b) {
  uint id=hash(b->blockno);

  acquire(&bcache[id].lock);
  b->refcnt++;
  release(&bcache[id].lock);
}

void
bunpin(struct buf *b) {
  uint id=hash(b->blockno);

  acquire(&bcache[id].lock);
  b->refcnt--;
  release(&bcache[id].lock);
}


// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}


// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}