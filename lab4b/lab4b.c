//
//  lab4b.c
//  proj4b
//
//  Created by Mint MSH on 11/13/17.
//  Copyright © 2017 Mint MSH. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <mraa.h>
#include <mraa/gpio.h>
#include <poll.h>
#include <time.h>
#include <math.h>

int tempType = 0; // 0 - Fahrenheit(default) 1 - Celsius
int period = 1;
char scale = 'F';
int f_stop = 0;
int f_log = 0;
mraa_aio_context tempo;
mraa_gpio_context button;
struct timespec start, stop;
int time_count = 0;
int start_count = 0;


// Initialize AIO/GPIO
void io_init()
{
    tempo = mraa_aio_init(1);
    button = mraa_gpio_init(60);
    mraa_gpio_dir(button, MRAA_GPIO_IN);  // not 0
}

void exit_handle(int num)
{
    mraa_aio_close(tempo);
    mraa_gpio_close(button);
    exit(num);
}

void shut_down()
{
    int hour, min, sec;
    time_t t = time(NULL);
    struct tm tt = *localtime(&t);
    hour = tt.tm_hour;
    min = tt.tm_min;
    sec = tt.tm_sec;
    dprintf(STDOUT_FILENO, "%02d:%02d:%02d SHUTDOWN\n", hour, min, sec);
    
    exit_handle(0);
}

void button_check()
{
    int ret;
    ret = mraa_gpio_read(button);
    if (ret < 0)
    {
        fprintf(stderr, "Error reading button!\n");
        exit_handle(1);
    }
    
    if (ret == 1)
        shut_down();
}

void record_tempo()
{
    clock_gettime(CLOCK_MONOTONIC, &stop);
    long runtime = (stop.tv_sec - start.tv_sec) + time_count;
    
    if ((start_count || runtime > 0) && (runtime % period) == 0)
    {
        const int B = 4275;
        const int R0 = 100000;
        int a = mraa_aio_read(tempo);
        double R = 1023.0/a-1.0;
        R = R0*R;
        // Celsius
        double tempo = 1.0/(log(R/R0)/B+1/298.15)-273.15;
        
        int hour, min, sec;
        time_t t = time(NULL);
        struct tm tt = *localtime(&t);
        hour = tt.tm_hour;
        min = tt.tm_min;
        sec = tt.tm_sec;
        if (scale == 'F')
            tempo = tempo * 1.8 + 32;
        dprintf(STDOUT_FILENO, "%02d:%02d:%02d %04.1f\n", hour, min, sec, tempo);
        
        //start counting again
        clock_gettime(CLOCK_MONOTONIC, &start);
        time_count = 0;
        start_count = 0;
    }
    return;
}

int main(int argc, char * argv[])
{
    
    const struct option long_options[] =
    {
        {"period",required_argument,0,'p'},
        {"scale",required_argument,0,'s'},
        {"log",required_argument,0,'l',},
        {0, 0, 0, 0}
    };
    
    // parse all arguments
    int ret = 0;
    int ofd = -1;
    while(1)
    {
        ret = getopt_long(argc, argv, "", long_options, NULL);
        if (ret == -1) // parsed all option
            break;
        
        switch (ret) {
            case 'p':
                period = atoi(optarg);
                if(period <= 0)
                {
                    fprintf(stderr, "Period paramenter should be more than zero!\n");
                    exit(1);
                }
                //p = 1;
                break;
                
            case 's':
                scale = *optarg;
                if(scale != 'C' && scale != 'F')
                {
                    fprintf(stderr, "Invalid argument!\nCorrect usage: --period=#, --scale={C,F}, --log=filename\n");
                    exit(1);
                }
                //s = 1;
                break;
                
            case 'l':
                ofd = open(optarg, O_RDWR | O_CREAT | O_TRUNC, 0666);
                f_log = 1;
                if (ofd >= 0)
                {
                    close(STDOUT_FILENO);
                    dup(ofd);
                    close(ofd);
                }
                else
                {
                    int err = errno;
                    fprintf(stderr, "Error opening output file %s\n", optarg);
                    printf("Error type: %s\n",strerror(err));
                    exit(1);
                }
                break;
                
            default:
                fprintf(stderr, "Invalid argument!\nCorrect usage: --period=#, --scale={C,F}, --log=filename\n");
                exit(1);
                break;
        }
    }
    
    io_init();
    
    struct pollfd pfd[1];
    pfd[0].fd = STDIN_FILENO;
    pfd[0].events = POLLIN;
    char input_buf[1024];
    char output_buf[1024];
    
    while (1)
    {
        ret = poll(pfd, 1, 0);
        if (ret < 0)    // error occured
        {
            int err = errno;
            fprintf(stderr, "Error using poll()!\r\n");
            printf("Error type: %s\r\n",strerror(err));
            exit_handle(1);
        }
        
        button_check();
        if(!f_stop)
        {
            record_tempo();
        }
        
        if (pfd[0].revents & POLLIN)
        {
            memset((char *)&input_buf, '\0', 1024);
            memset((char *)&output_buf, '\0', 1024);
            int len = read(pfd[0].fd, input_buf, 1024);
            if (len == 0)
            { // EOF
                shut_down();
            }
            
            int j = 0;
            int i = 0;
            for (; i < len; i++)
            {
                if (input_buf[i] == '\n')
                {
                    if (strncmp(input_buf, "OFF", 3) == 0)
                    {
                        if (f_log)
                        {
                            dprintf(STDOUT_FILENO, "%s\n", output_buf);
                        }
                        shut_down();
                    }
                    
                    else if (strncmp(input_buf, "STOP",4) == 0)
                    {
                        if(f_log)
                        {
                            dprintf(STDOUT_FILENO, "%s\n", output_buf);
                        }
                        f_stop = 1;
                        clock_gettime(CLOCK_MONOTONIC, &stop);
                        time_count = stop.tv_sec - start.tv_sec; // 为什么
                    }
                    
                    else if (strncmp(input_buf, "START",5) == 0)
                    {
                        if (f_log)
                        {
                            dprintf(STDOUT_FILENO, "%s\n", output_buf);
                        }
                        f_stop = 0;
                        clock_gettime(CLOCK_MONOTONIC, &start);
                    }
                    
                    else if (strncmp(input_buf, "SCALE=F", 7) == 0)
                    {
                        if (f_log)
                        {
                            dprintf(STDOUT_FILENO, "%s\n", output_buf);
                        }
                        scale = 'F';
                    }
                    
                    else if (strncmp(input_buf, "SCALE=C", 7) == 0)
                    {
                        if (f_log)
                        {
                            dprintf(STDOUT_FILENO, "%s\n", output_buf);
                        }
                        scale = 'C';
                    }
                    
                    else if (strncmp(input_buf, "PERIOD=", 7) == 0)
                    {
                        period = atoi(input_buf + 7);
                        if (period <= 0){
                            fprintf(stderr, "Period paramenter should be more than zero!\n");
                            exit_handle(1);
                        }
                        if (f_log)
                        {
                            dprintf(STDOUT_FILENO, "%s\n", output_buf);
                        }
                    }
                    
                    else
                    {
                        if (f_log)
                        {
                            dprintf(STDOUT_FILENO, "%s\n", output_buf);
                        }
                    }
                    
                    memset((char *)&output_buf, '\0', 1024);
                    j = 0;
                }
                else
                    output_buf[j++] = input_buf[i];
            }
            
            
        }
        
        if (pfd[0].revents & POLLERR)
        {
            int err = errno;
            fprintf(stderr, "Error polling from keyboard!\r\n");
            printf("Error type: %s\r\n",strerror(err));
            exit_handle(1);
        }
        
        if(pfd[0].revents & POLLHUP)
        {
            break;
        }
    }
    
    
    exit_handle(0);
    
    return 0;
}
