//
//  lab2_add.c
//  proj2a
//
//  Created by Mint MSH on 10/29/17.
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

long long counter;

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



int opt_yield;
void add(long long *pointer, long long value)
{
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield();
    *pointer = sum;
}

void add_m(long long *pointer, long long value)
{
    Pthread_mutex_lock(&mutex);     // lock
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield();
    *pointer = sum;
    Pthread_mutex_unlock(&mutex);   // unlock
}

int spin_lock = 0;
void add_s(long long *pointer, long long value)
{
    while(__sync_lock_test_and_set(&spin_lock, 1)); // lock
    
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield();
    *pointer = sum;
    
    __sync_lock_release(&spin_lock);    // unlock
}

void add_c(long long *pointer, long long value)
{
    long long sum, old;
    do
    {
        old = *pointer;
        sum = old + value;
        if (opt_yield)
            sched_yield();
    }while (__sync_val_compare_and_swap(pointer, old, sum) != old);

}

// main adding function
void (*func_to_call)(long long*, long long) = add;

void* add_func(void* argment)
{
    int *it_num = (int *)argment;
    for (int i = 0; i < *it_num; i++)
    {
        func_to_call(&counter, 1);
    }
    
    for (int i = 0; i < *it_num; i++)
    {
        func_to_call(&counter, -1);
    }
    
    return NULL;
}

int main(int argc, char * argv[])
{
    
    const struct option long_options[] =
    {
        {"threads", required_argument, NULL, 't'},
        {"iterations", required_argument, NULL, 'i'},
        {"sync", required_argument, NULL, 's'},
        {"yield", no_argument, NULL, 'y'},
        {0, 0, 0, 0}
    };
    
    
    int ret;
    int thr_num = 1;
    int it_num = 1;
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
                    fprintf(stderr, "Invalid argument!\nCorrect usage: ----threads=#, --iterations=#, --yield --sync={s,m,c}\n");
                    exit(2);
                }
                type = optarg[0];
                switch(type)
                {
                    case 'm':
                        func_to_call = add_m;
                        break;
                    case 's':
                        func_to_call = add_s;
                        break;
                    case 'c':
                        func_to_call = add_c;
                        break;
                    default:
                        fprintf(stderr, "Invalid argument!\nCorrect usage: ----threads=#, --iterations=#, --yield --sync={s,m,c}\n");
                        exit(2);
                }
                break;
                
            case 'y':
                opt_yield = 1;
                break;
                
            default:
                fprintf(stderr, "Invalid argument!\nCorrect usage: ----threads=#, --iterations=#, --yield --sync={s,m,c}\n");
                exit(2);
                break;
        }
    }
    counter = 0;

    
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
    for (int i = 0; i < thr_num; i++)
    {
        Pthread_create(&tid[i], NULL, add_func, &it_num);
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
    long total_opt = thr_num * it_num * 2;
    long ave_time = run_time / total_opt;
    
    char add_type[20] = "add";
    if (opt_yield == 1)
        strcat(add_type,"-yield");
    
    if (func_to_call == add_m)
        strcat(add_type,"-m");
    else if (func_to_call == add_s)
        strcat(add_type,"-s");
    else if (func_to_call == add_c)
        strcat(add_type,"-c");
    else
        strcat(add_type,"-none");
    
    printf("%s,%d,%d,%ld,%ld,%ld,%lld\n", add_type, thr_num, it_num, total_opt, run_time, ave_time, counter);
    
    return 0;
}
