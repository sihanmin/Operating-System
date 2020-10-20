//
//  SortedList.c
//  proj2a
//
//  Created by Mint MSH on 10/30/17.
//  Copyright © 2017 Mint MSH. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <sched.h>
#include <errno.h>
#include "SortedList.h"

/**
 * SortedList_insert ... insert an element into a sorted list
 *
 *	The specified element will be inserted in to
 *	the specified list, which will be kept sorted
 *	in ascending order based on associated keys
 *
 * @param SortedList_t *list ... header for the list
 * @param SortedListElement_t *element ... element to be added to the list
 */
void SortedList_insert(SortedList_t *list, SortedListElement_t *element)
{
    SortedList_t *temp = list;
    while (temp->next->key != NULL && strcmp(element->key, temp->next->key) > 0) // next one is not head and next value is less than element key
    {
        temp = temp->next;
    }
    
    if(opt_yield & INSERT_YIELD)
        sched_yield();
    
    element->next = temp->next;
    element->prev = temp->next->prev;
    temp->next = element;
    element->next->prev = element;
    
}

/**
 * SortedList_delete ... remove an element from a sorted list
 *
 *	The specified element will be removed from whatever
 *	list it is currently in.
 *
 *	Before doing the deletion, we check to make sure that
 *	next->prev and prev->next both point to this node
 *
 * @param SortedListElement_t *element ... element to be removed
 *
 * @return 0: element deleted successfully, 1: corrtuped prev/next pointers
 *
 */
int SortedList_delete( SortedListElement_t *element)
{
    if(element == NULL || element->next == NULL || element->prev == NULL)
        return 1;
    
    if (element->prev->next != element || element->next->prev != element)
        return 1;
    
    if(opt_yield & DELETE_YIELD)
        sched_yield();
    
    element->next->prev = element->prev;
    element->prev->next = element->next;
    free((void *) element->key);
    return 0;
}

/**
 * SortedList_lookup ... search sorted list for a key
 *
 *	The specified list will be searched for an
 *	element with the specified key.
 *
 * @param SortedList_t *list ... header for the list
 * @param const char * key ... the desired key
 *
 * @return pointer to matching element, or NULL if none is found
 */
SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key)
{
    SortedList_t *temp = list;
    while (temp->next->key != NULL && strcmp(temp->next->key,key) != 0)
    {
        if(opt_yield & LOOKUP_YIELD)
            sched_yield();
        
        temp = temp->next;
    }
    
    if(temp->next->key != NULL) // found the key
    {
        return temp->next;
    }
    
    return NULL;
}

/**
 * SortedList_length ... count elements in a sorted list
 *	While enumeratign list, it checks all prev/next pointers
 *
 * @param SortedList_t *list ... header for the list
 *
 * @return int number of elements in list (excluding head)
 *	   -1 if the list is corrupted
 */
int SortedList_length(SortedList_t *list)
{
    int total = 0;
    SortedList_t *temp = list;
    if (temp->next->prev != temp || temp->prev->next != temp)
    {
        return -1;
    }
    
    while (temp->next->key != NULL)
    {
        if (temp->next->prev != temp || temp->prev->next != temp)
        {
            return -1;
        }
        total++;
        
        if(opt_yield & LOOKUP_YIELD)
            sched_yield();
        
        temp = temp->next;
    }
    return total;
}



