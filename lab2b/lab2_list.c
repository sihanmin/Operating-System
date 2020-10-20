//
//  lab2_list.c
//  proj2b
//
//  Created by Mint MSH on 11/7/17.
//  Copyright © 2017 Mint MSH. All rights reserved.
//

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
#include <signal.h>
#include "SortedList.h"

long long counter;
char opt_sync = 'n';
int *spin_lock;
int thr_num = 1;
int it_num = 1;
int list_num = 1;
int opt_yield = 0;
SortedList_t *list;
long * t_time;

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

pthread_mutex_t *mutex;

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

void sig_handler()
{
    fprintf(stderr, "Segmentation fault caught!\n");
    exit(2);
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

int hash(const char *key)
{
    int cur = 0;
    for(int i = 0; key[i] != '\0'; i++)
    {
        cur += key[i] - 'a';
    }
    return cur % list_num;
}

void lock(int index, long *thr_time)
{
    // do nothing if no lock required
    if(opt_sync != 'm' && opt_sync != 's')
        return;
    
    // get start time
    struct timespec start, end;
    int clock_err;
    clock_err = clock_gettime(CLOCK_MONOTONIC, &start);
    if (clock_err)
    {
        int err = errno;
        fprintf(stderr, "Error getting starting time: %d", err);
        exit(1);
    }
    
    // perform lock
    if(opt_sync == 'm')
        Pthread_mutex_lock(&mutex[index]);
    else if(opt_sync == 's')
        while(__sync_lock_test_and_set(&spin_lock[index], 1));
    
    // get end time
    clock_err = clock_gettime(CLOCK_MONOTONIC, &end);
    if (clock_err)
    {
        int err = errno;
        fprintf(stderr, "Error getting ending time: %d", err);
        exit(1);
    }
    
    // add the time waiting for lock to the total thread time
    // no race condition is possible here since only one part of a single thread can call this part
    *thr_time += 1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
}

// Mutex and spin unlock
void unlock(int index)
{
    if(opt_sync == 'm')
        Pthread_mutex_unlock(&mutex[index]);
    else if(opt_sync == 's')
        __sync_lock_release(&spin_lock[index]);
}

struct args
{
    int place;
    SortedList_t *nodes;
};

void *thread_work(void *argument);

int main(int argc, char * argv[])
{
    
    const struct option long_options[] =
    {
        {"threads", required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {"sync", required_argument, NULL, 's'},
        {"yield", required_argument, NULL, 'y'},
        {"lists", required_argument, NULL, 'l'},
        {0, 0, 0, 0}
    };
    
    
    int ret;
    char type = 0;
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
                
            case 'l':
                list_num = atoi(optarg);
                break;
                
            case 's':
                if(strlen(optarg) != 1)
                {
                    fprintf(stderr, "Invalid argument!\nCorrect usage: --threads=#, --iterations=#, --lists=#, --yield={i,d,l,id,il,dl,idl}, --sync={s,m}");
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
                        fprintf(stderr, "Invalid argument!\nCorrect usage: --threads=#, --iterations=#, --lists=#, --yield={i,d,l,id,il,dl,idl}, --sync={s,m}\n");
                    exit(2);
                }
                break;
                
            case 'y':
                if(strlen(optarg) == 0)
                {
                    fprintf(stderr, "Invalid argument!\nCorrect usage: --threads=#, --iterations=#, --lists=#, --yield={i,d,l,id,il,dl,idl}, --sync={s,m}\n");
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
                        fprintf(stderr, "Invalid argument!\nCorrect usage: --threads=#, --iterations=#, --lists=#, --yield={i,d,l,id,il,dl,idl}, --sync={s,m}\n");
                        exit(2);
                    }
                    i++;
                }
                break;
                
            default:
                fprintf(stderr, "Invalid argument!\nCorrect usage: --threads=#, --iterations=#, --lists=#, --yield={i,d,l,id,il,dl,idl}, --sync={s,m}\n");
                exit(2);
                break;
        }
    }
    counter = 0;
    signal(SIGSEGV, sig_handler);
    
    t_time = (long *) malloc(sizeof(long) * thr_num);
    for(int i = 0; i < thr_num; i++)
        t_time[i] = 0;
    
    if (opt_sync == 'm')
    {
        mutex = malloc(sizeof(pthread_mutex_t) * list_num);
        for(int i = 0; i < list_num; i++)
            pthread_mutex_init(&mutex[i], NULL);
    }
    
    if (opt_sync == 's')
    {
        spin_lock = malloc(sizeof(int) * list_num);
        for(int i = 0; i < list_num; i++)
            spin_lock[i] = 0;
    }
    
    // initialize an empty list
    list = (SortedListElement_t *) malloc(sizeof(SortedList_t) * list_num);
    for (int i = 0; i < list_num; i++)
    {
        list[i].key = NULL;
        list[i].next = list[i].prev = &list[i];
    }
    
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
    struct args a[thr_num];
    for (int i = 0; i < thr_num; i++)
    {
        a[i].place = i;
        a[i].nodes = &nodes[i][0];
        Pthread_create(&tid[i], NULL, thread_work, &a[i]);
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
    long long run_time = 1000000000 * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
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
    
    long long lock_time = 0;
    long total_lock = total_opt + list_num * thr_num;
    for (int i = 0; i < thr_num; i++)
    {
        lock_time += t_time[i];
        //printf("%ld\n", t_time[i]);
    }
    lock_time /= total_lock;
    
    
    printf("%s,%d,%d,%d,%ld,%lld,%ld, %lld\n", list_type, thr_num, it_num, list_num, total_opt, run_time, ave_time, lock_time);
    
    // remember to free
    free((void *) list);
    free((void *) mutex);
    free((void *) spin_lock);
    free((void *) t_time);
    
    return 0;
}

void *thread_work(void *argument)
{
    struct args * myargs = (struct args *) argument;
    int place = myargs->place;
    SortedList_t *nodes = myargs->nodes;
    
    // inserts nodes
    int index;
    long thr_time = 0;
    for(int i = 0; i < it_num; i++)
    {
        index = hash(nodes[i].key);
        lock(index, &thr_time);
        SortedList_insert(&list[index], &nodes[i]);
        unlock(index);
    }
    
    // gets the list length
    int total = 0;
    for(int i = 0; i < list_num; i++)
        lock(i, &thr_time);
    for(int i = 0; i < list_num; i++)
    {
        int cur = SortedList_length(&list[i]);
        if(cur == -1)
        {
            fprintf(stderr, "Corrupted list %d spotted at length counting!\n", i);
            exit(2);
        }
        total += cur;
    }
    for(int i = 0; i < list_num; i++)
        unlock(i);
    
    
    // looks up and deletes each of the keys
    SortedList_t *dtemp;
    for(int i = 0; i <it_num; i++)
    {
        index = hash(nodes[i].key);
        lock(index, &thr_time);
        dtemp = SortedList_lookup(&list[index], nodes[i].key);
        if(dtemp == NULL)
        {
            fprintf(stderr, "Corrupted list %d: missing inserted nodes!\n", index);
            exit(2);
        }
        unlock(index);
        lock(index, &thr_time);
        if(SortedList_delete(dtemp) != 0)
        {
            fprintf(stderr, "Corrupted list %d spotted at deletion!\n", index);
            exit(2);
        }
        
        unlock(index);
    }
    
    t_time[place] = thr_time;
    return NULL;
}


