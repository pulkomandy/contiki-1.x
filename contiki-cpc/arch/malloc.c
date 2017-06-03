#include <sdcc-lib.h>
#include <stdlib.h>

typedef struct _MEMHEADER MEMHEADER;

struct _MEMHEADER
{
  MEMHEADER *  next;
  MEMHEADER *  prev;
  unsigned int       len;
  unsigned char      mem;
};

#define HEADER_SIZE (sizeof(MEMHEADER)-sizeof(char))

/* These veriables are defined through the crt0 functions. */
/* Base of this variable is the first byte of the heap. */
//extern MEMHEADER _sdcc_heap_start;
/* Address of this variable is the last byte of the heap. */
//extern char _sdcc_heap_end;

extern char progend;

unsigned int _heapmaxavail;

static MEMHEADER *firstheader;
/* setup two headers. One at start of free ram, second at end of free ram.
 *
 * Free ram starts after the _BSS segment.
 * Free RAM ends before the memory used by expansion ROMs. For now we only
 * consider ROM 7 (AMSDOS), which must be initialized before calling
 * _sdcc_heap_init - probably in crt0.s...
 */

void
_sdcc_heap_init(void)
{
  MEMHEADER *lastheader;
  int ramend;

  /* this is our first mem header - right after the end of the code area */
  firstheader = (MEMHEADER *)(&progend);
  
  /* this is the size of ram available - read from firmware */
  ramend = *(int*)(0xb8e8);

  /* calc address of last header */
  lastheader = (MEMHEADER *)(ramend - HEADER_SIZE); 
  
  /* setup last header */
  lastheader->next = NULL;
  lastheader->prev = firstheader;
  lastheader->len = 0;

  /* setup first header */
  firstheader->next = lastheader;
  firstheader->prev       = NULL; //and mark first as first
  firstheader->len        = 0;    //Empty and ready.

  _heapmaxavail = ramend - &progend;
}

void *
malloc (unsigned int size)
{
  MEMHEADER * current_header;
  MEMHEADER * new_header;

  if (size>(0xFFFF-HEADER_SIZE))
    {
      return NULL; //To prevent overflow in next line
    }

  size += HEADER_SIZE; //We need a memory for header too
  current_header = firstheader;

  while (1)
    {
      //    current
      //    |   len       next
      //    v   v         v
      //....*****.........******....
      //         ^^^^^^^^^
      //           spare

      if ((((unsigned int)current_header->next) -
           ((unsigned int)current_header) -
           current_header->len) >= size) 
        {
          break; //if spare is more than need
        }
      current_header = current_header->next;    //else try next             
      if (!current_header->next)  
        {
          return NULL;  //if end_of_list reached    
        }
    }

  if (!current_header->len)
    { //This code works only for first_header in the list and only
      current_header->len = size; //for first allocation
      return &current_header->mem;
    } 
  else
    {
      //else create new header at the begin of spare
      new_header = (MEMHEADER * )((char *)current_header + current_header->len);
      new_header->next = current_header->next; //and plug it into the chain
      new_header->prev = current_header;
      current_header->next  = new_header;
      if (new_header->next)
        {
          new_header->next->prev = new_header;
        }
      new_header->len  = size; //mark as used
      return &new_header->mem;
    }
}

void
free (void *p)
{
  MEMHEADER *prev_header, *pthis;

  if ( p ) //For allocated pointers only!
    {
      pthis = (MEMHEADER * )((char *)  p - HEADER_SIZE); //to start of header
      if ( pthis->prev ) // For the regular header
        {
          prev_header = pthis->prev;
          prev_header->next = pthis->next;
          if (pthis->next)
            {
              pthis->next->prev = prev_header;
            }
        }
      else
        {
          pthis->len = 0; //For the first header
        }
    }
}


/* Compute free memory */
unsigned int _heapmemavail()
{
	unsigned int avail = _heapmaxavail;
  	MEMHEADER* header; 
		
	for (header = firstheader; header; header = header->next)
		avail -= header->len + sizeof(MEMHEADER);

	return avail;
}
