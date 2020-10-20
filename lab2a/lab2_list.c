//
//  lab2_list.c
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

long long counter;
char opt_sync = 'n';
int spin_lock = 0;
int thr_num = 1;
int it_num = 1;
int opt_yield = 0;
SortedList_t *list;

// wrapper functions
void Pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine)(void*),void *arg)
{
    int ret = pthread_create(thread, attr, start_routine, arg);
    if (ret)
    {
        fprintf(stderr, "Error creating threads\n");
        exit(1);
    }
}

void Pthread_join(pthread_t thread, void **value_ptr)
{
    int ret = pthread_join(thread, value_ptr);
    if (ret)
    {
        fprintf(stderr, "Error joining threads\n");
        exit(1);
    }
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void Pthread_mutex_lock(pthread_mutex_t *mutex)
{
    int ret = pthread_mutex_lock(mutex);
    if (ret)
    {
        fprintf(stderr, "Error locking critical section\n");
        exit(1);
    }
}

void Pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    int ret = pthread_mutex_unlock(mutex);
    if (ret)
    {
        fprintf(stderr, "Error unlocking critical section\n");
        exit(1);
    }
}

char* randKey(int size)
{
    char *key = malloc(sizeof(char) * size + 1);
    for (int i = 0; i < size; i++)
    {
        key[i] = (char) ((rand() % 26) + 'a');
    }
    key[size] = '\0';
    return key;
}

void lock()
{
    if(opt_sync == 'm')
        Pthread_mutex_lock(&mutex);
    else if(opt_sync == 's')
        while(__sync_lock_test_and_set(&spin_lock, 1));
}

// Mutex and spin unlock
void unlock()
{
    if(opt_sync == 'm')
        Pthread_mutex_unlock(&mutex);
    else if(opt_sync == 's')
        __sync_lock_release(&spin_lock);
}

void *thread_work(void *argument)
{
    SortedList_t *nodes = (SortedList_t *) argument;
    //lock();
    // inserts nodes
    for(int i = 0; i <it_num; i++)
    {
        lock();
        SortedList_insert(list, &nodes[i]);
        unlock();
    }
    
    // gets the list length
    lock();
    int total = SortedList_length(list);
    unlock();
    
    if(total == -1)
    {
        fprintf(stderr, "Corrupted list spotted at length counting!\n");
        exit(2);
    }
    
    // looks up and deletes each of the keys
    SortedList_t *dtemp;
    for(int i = 0; i <it_num; i++)
    {
        lock();
        dtemp = SortedList_lookup(list, nodes[i].key);
        if(dtemp == NULL)
        {
            fprintf(stderr, "Corrupted: missing inserted nodes!\n");
            exit(2);
        }
        unlock();
        lock();
        if(SortedList_delete(dtemp) != 0)
        {
            fprintf(stderr, "Corrupted list spotted at deletion\n");
            exit(2);
        }
            
        unlock();
    }
    //unlock();
    return NULL;
}

int main(int argc, char * argv[])
{
    
    const struct option long_options[] =
    {
        {"threads", required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {"sync", required_argument, NULL, 's'},
        {"yield", required_argument, NULL, 'y'},
        {0, 0, 0, 0}
    };
    
    
    int ret;
    char type;
    while(1)
    {
        ret = getopt_long(argc, argv, "", long_options, NULL);
        if (ret == -1) // parsed all option
            break;
        
        switch (ret) {
            case 't':
                thr_num = atoi(optarg);
                break;
                
            case 'i':
                it_num = atoi(optarg);
                break;
                
            case 's':
                if(strlen(optarg) != 1)
                {
                    fprintf(stderr, "Invalid argument!\nCorrect usage: ----threads=#, --iterations=#, --yield={i,d,l,id,il,dl,idl}, --sync={s,m}");
                    exit(2);
                }
                type = optarg[0];
                switch(type)
                {
                    case 'm':
                        opt_sync = 'm';
                        break;
                    case 's':
                        opt_sync = 's';
                        break;
                    default:
                        fprintf(stderr, "Invalid argument!\nCorrect usage: ----threads=#, --iterations=#, --yield={i,d,l,id,il,dl,idl}, --sync={s,m}\n");
                        exit(2);
                }
                break;
                
            case 'y':
                if(strlen(optarg) == 0)
                {
                    fprintf(stderr, "Invalid argument!\nCorrect usage: ----threads=#, --iterations=#, --yield={i,d,l,id,il,dl,idl}, --sync={s,m}\n");
                    exit(2);
                }
                int i = 0;
                while (optarg[i] != '\0')
                {
                    if (optarg[i] == 'i')
                    {
                        opt_yield |= INSERT_YIELD;
                    }
                    else if (optarg[i] == 'd')
                    {
                        opt_yield |= DELETE_YIELD;
                    }
                    else if (optarg[i] == 'l')
                    {
                        opt_yield |= LOOKUP_YIELD;
                    }
                    else
                    {
                        fprintf(stderr, "Invalid argument!\nCorrect usage: ----threads=#, --iterations=#, --yield={i,d,l,id,il,dl,idl}, --sync={s,m}\n");
                        exit(2);
                    }
                    i++;
                }
                break;
                
            default:
                fprintf(stderr, "Invalid argument!\nCorrect usage: ----threads=#, --iterations=#, --yield={i,d,l,id,il,dl,idl}, --sync={s,m}\n");
                exit(2);
                break;
        }
    }
    counter = 0;
    
    // initialize an empty list
    list = (SortedListElement_t *) malloc(sizeof(SortedList_t));
    list->key = NULL;
    list->next = list->prev = list;
    
    int thr = thr_num;
    int it = it_num;
    // create all the nodes in a 2D array nodes[thr_num][it_num]
    SortedListElement_t nodes[thr][it];

    for (int i = 0; i < thr_num; i++)
    {
        for (int j = 0; j < it_num; j++)
        {
            nodes[i][j].prev = NULL;
            nodes[i][j].next = NULL;
            nodes[i][j].key = randKey(15);
        }
    }
    
    
    // notes the (high resolution) starting time for the run
    struct timespec start, end;
    int clock_err;
    clock_err = clock_gettime(CLOCK_MONOTONIC, &start);
    if (clock_err)
    {
        int err = errno;
        fprintf(stderr, "Error getting starting time: %d", err);
        exit(1);
    }
    
    // starts the specified number of threads, each of which will use add function
    pthread_t tid[thr_num];
    //struct thr_arg temp = {list, &nodes[0][0]};
    for (int i = 0; i < thr_num; i++)
    {
        Pthread_create(&tid[i], NULL, thread_work, &nodes[i][0]);
    }
    
    // waits for all threads to complete
    for (int i = 0; i < thr_num; i++)
    {
        Pthread_join(tid[i], NULL);
    }
    
    // notes the (high resolution) ending time for the run
    clock_err = clock_gettime(CLOCK_MONOTONIC, &end);
    if (clock_err)
    {
        int err = errno;
        fprintf(stderr, "Error getting ending time: %d", err);
        exit(1);
    }
    
    // prints to stdout a comma-separated-value (CSV) record
    long run_time = 1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
    long total_opt = thr_num * it_num * 3;
    long ave_time = run_time / total_opt;
    
    char list_type[20] = "list-";
    char yield_type[10] = "";
    if (opt_yield & INSERT_YIELD)
        strcat(yield_type, "i");
    if (opt_yield & DELETE_YIELD)
        strcat(yield_type, "d");
    if (opt_yield & LOOKUP_YIELD)
        strcat(yield_type, "l");
    if (opt_yield == 0)
        strcat(yield_type, "none");
    
    strcat(yield_type, "-");
    
    strcat(list_type, yield_type);
    
    if (opt_sync == 'm')
        strcat(list_type,"m");
    else if (opt_sync == 's')
        strcat(list_type,"s");
    else
        strcat(list_type,"none");
    
    printf("%s,%d,%d,1,%ld,%ld,%ld\n", list_type, thr_num, it_num, total_opt, run_time, ave_time);
    
    // remember to free
    free((void *) list);
    
    return 0;
}

