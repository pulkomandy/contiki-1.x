/**
 * \defgroup list Linked list library
 * @{
 *
 * The linked list library provides a set of functions for
 * manipulating linked lists.
 *
 * A linked list is made up of elements where the first element \b
 * must be a pointer. This pointer is used by the linked list library
 * to form lists of the elements.
 *
 * Lists are declared with the LIST() macro. The declaration specifies
 * the name of the list that later is used with all list functions.
 *
 * Example:
 \code

struct packet {
   struct packet *next;
   char data[1500];
   int len;
};

LIST(packets);

static void
init_function(void) {
   list_init(packets);
}

static void
another_function(struct packet *p) {
   list_add(packets, p);

   p = list_head(packets);

   p = list_tail(packets);
}
 \endcode 
 *
 * Lists can be manipulated by inserting or removing elements from
 * either sides of the list (list_push(), list_add(), list_pop(),
 * list_chop()). A specified element can also be removed from inside a
 * list with list_remove(). The head and tail of a list can be
 * extracted using list_head() and list_tail(), respecitively.
 */

/**
 * \file
 * Linked list library implementation.
 *
 * \author Adam Dunkels <adam@sics.se>
 *
 */

/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 * This file is part of the Contiki operating system.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: list.c,v 1.4 2005/02/07 07:52:31 adamdunkels Exp $
 */
#include "list.h"

#define NULL 0

struct list {
  struct list *next;
};

/*---------------------------------------------------------------------------*/
/**
 * Initialize a list.
 *
 * This function initalizes a list. The list will be empty after this
 * function has been called.
 *
 * \param list The list to be initialized.
 */
void
list_init(void** list)
{
  *list = NULL;
}
/*---------------------------------------------------------------------------*/
/**
 * Get a pointer to the first element of a list.
 *
 * This function returns a pointer to the first element of the
 * list. The element will \b not be removed from the list.
 *
 * \param list The list.
 * \return A pointer to the first element on the list.
 *
 * \sa list_tail()
 */
void *
list_head(void** list)
{
  return *list;
}
/*---------------------------------------------------------------------------*/
/**
 * Duplicate a list.
 *
 * This function duplicates a list by copying the list reference, but
 * not the elements.
 *
 * \note This function does \b not copy the elements of the list, but
 * merely duplicates the pointer to the first element of the list.
 *
 * \param dest The destination list.
 * \param src The source list.
 */
void
list_copy(void** dest, void** src)
{
  *dest = *src;
}
/*---------------------------------------------------------------------------*/
/**
 * Get the tail of a list.
 *
 * This function returns a pointer to the elements following the first
 * element of a list. No elements are removed by this function.
 *
 * \param list The list
 * \return A pointer to the element after the first element on the list.
 *
 * \sa list_head()
 */
void *
list_tail(void** list)
{
  struct list *l;
  
  if(*list == NULL) {
    return NULL;
  }
  
  for(l = *list; l->next != NULL; l = l->next);
  
  return l;
}
/*---------------------------------------------------------------------------*/
/**
 * Add an item at the end of a list.
 *
 * This function adds an item to the end of the list.
 *
 * \param list The list.
 * \param item A pointer to the item to be added.
 *
 * \sa list_push()
 *
 */
void
list_add(void** list, void *item)
{
  struct list *l;

  ((struct list *)item)->next = NULL;
  
  l = list_tail(list);

  if(l == NULL) {
    *list = item;
  } else {
    l->next = item;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * Add an item to the start of the list.
 */
void
list_push(void** list, void *item)
{
  /*  struct list *l;*/

  ((struct list *)item)->next = *list;
  *list = item;
}
/*---------------------------------------------------------------------------*/
/**
 * Remove the last object on the list.
 *
 * This function removes the last object on the list and returns it.
 * 
 * \param list The list
 * \return The removed object
 *
 */
void *
list_chop(void** list)
{
  struct list *l, *r;
  
  if(*list == NULL) {
    return NULL;
  }
  if(((struct list *)*list)->next == NULL) {
    l = *list;
    *list = NULL;
    return l;
  }
  
  for(l = *list; l->next->next != NULL; l = l->next);

  r = l->next;
  l->next = NULL;
  
  return r;
}
/*---------------------------------------------------------------------------*/
/**
 * Remove the first object on a list.
 *
 * This function removes the first object on the list and returns it.
 *
 * \param list The list.
 * \return The new head of the list.
 */
/*---------------------------------------------------------------------------*/
void *
list_pop(void** list)
{
  struct list *l;
  
  if(*list != NULL) {   
    *list = ((struct list *)*list)->next;
  }

  return *list;
}
/*---------------------------------------------------------------------------*/
/**
 * Remove a specific element from a list.
 *
 * This function removes a specified element from the list.
 *
 * \param list The list.
 * \param item The item that is to be removed from the list.
 *
 */
/*---------------------------------------------------------------------------*/
void
list_remove(void** list, void *item)
{
  struct list *l, *r;
  
  if(*list == NULL) {
    return;
  }
  
  r = NULL;
  for(l = *list; l != NULL; l = l->next) {
    if(l == item) {
      if(r == NULL) {
	/* First on list */
	*list = l->next;
      } else {
	/* Not first on list */
	r->next = l->next;
      }
      l->next = NULL;
      return;
    }
    r = l;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * Get the length of a list.
 *
 * This function counts the number of elements on a specified list.
 *
 * \param list The list.
 * \return The length of the list.
 */
/*---------------------------------------------------------------------------*/
int
list_length(void** list)
{
  struct list *l; 
  int n = 0;

  for(l = *list; l != NULL; l = l->next) {
    ++n;
  }

  return n;
}
/*---------------------------------------------------------------------------*/

/** @} */
