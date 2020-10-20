//
//  lab0.c
//  proj0
//
//  Created by Mint MSH on 10/1/17.
//  Copyright © 2017 Mint MSH. All rights reserved.
//

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

void signal_handler(){
    fprintf(stderr, "Segmentation fault detected.\n");
    exit(4);
}

void seg_fault_trigger(){
    char *invalid_ptr = NULL;
    *invalid_ptr = 's';
}

int main(int argc, char * argv[]) {
    // long options
    struct option long_options[] =
    {
        {"input", required_argument, NULL, 'i'},
        {"output", required_argument, NULL, 'o'},
        {"segfault", no_argument, NULL, 's'},
        {"catch", no_argument, NULL, 'c'},
        {0, 0, 0, 0}
    };
    
    int ret = 0;
    int ifd = -1;
    int ofd = -1;
    int seg_fault = 0;
    
    
    while(1)
    {
        ret = getopt_long(argc, argv, "i::o::sc", long_options, NULL);
        if (ret == -1) // parsed all option
            break;
        
        switch (ret) {
            case 'i':
                ifd = open(optarg, O_RDONLY);
                if (ifd >= 0) {
                    close(0);
                    dup(ifd);
                    close(ifd);
                }
                else
                {
                    int err = errno;
                    fprintf(stderr, "Error opening input file %s\n", optarg);
                    printf("Error type: %s\n",strerror(err));
                    exit(2);
                }
                break;
            
            case 'o':
                ofd = creat(optarg, 0666);
                if (ofd >= 0) {
                    close(1);
                    dup(ofd);
                    close(ofd);
                }
                else
                {
                    int err = errno;
                    fprintf(stderr, "Error creating output file %s\n", optarg);
                    printf("Error type: %s\n",strerror(err));
                    exit(3);
                }
                break;
            
            case 's':
                seg_fault = 1;
                break;
            
            case 'c':
                signal(SIGSEGV, signal_handler);
                break;
            
            default:
                fprintf(stderr, "Invalid argument!\nCorrect usage: --input=filename, --output=filename, --segfault, --catch\n");
                exit(1);
                break;
        }
    }
    
    if (seg_fault)
        seg_fault_trigger();
    
    char* buf[10000];
    long count = 0;
    long w = 0;
    while (1)
    {
        count = read(0, &buf, 10000);
        if (count < 0) {
            fprintf(stderr, "Error reading file\n");
            exit(2);
        }
        else if (count == 0)
            break;
        
        w = write(1, &buf, count);
        if (w < 0) {
            fprintf(stderr, "Error writing file\n");
            exit(3);
        }
    }
    exit(0);
}
