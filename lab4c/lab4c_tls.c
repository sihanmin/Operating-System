//
//  lab4c_tls.c
//  proj4c
//
//  Created by Mint MSH on 12/5/17.
//  Copyright © 2017 Mint MSH. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <time.h>
#include "mraa.h"
#include "mraa/aio.h"
#include <math.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/err.h>

char* id = "504807176";
char* host = "lever.cs.ucla.edu";
int port = 0;
int period = 1;
int sock_fd;
char scale = 'F';
int f_stop = 0;
int f_log = 0;
mraa_aio_context tempo;
mraa_gpio_context button;
struct timespec start, stop;
int time_count = 0;
int start_count = 0;
char buffer[1024];
int log_fd = -1;
SSL_CTX *ctx = NULL;
SSL *ssl_client;

void ssl_init()
{
    SSL_library_init();
    
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    ctx = SSL_CTX_new(SSLv23_client_method());
    if (ctx == NULL)
    {
        fprintf(stderr, "Error initializing SSL client\n");
        exit(2);
    }
    
    ssl_client = SSL_new(ctx);
    SSL_set_fd(ssl_client, sock_fd);
    if (SSL_connect(ssl_client) < 0)
    {
        fprintf(stderr, "Error connecting to TLS server\n");
        exit(2);
    }
    
}

void print_func()
{
    if (f_log)
    {
        dprintf(log_fd, "%s", buffer);
    }
    SSL_write(ssl_client, buffer, strlen(buffer));
}

void socket_init()
{
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        fprintf(stderr, "Error opening socket\n");
        exit(2);
    }
    
    struct hostent *server = gethostbyname(host);
    if (server == NULL){
        fprintf(stderr,"Error finding the host: %s\n", host);
        exit(1);
    }
    struct sockaddr_in serv_addr;
    //memset((char *) &serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    //memcpy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);
    
    if (connect(sock_fd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0 )
    {
        fprintf(stderr, "Error connecting to port: %d\n", port);
        exit(2);
    }
    
    ssl_init();
    
    memset((char *)buffer, '\0', 1024);
    sprintf(buffer, "ID=%s\n", id);
    print_func();
    
}

void io_init()
{
    tempo = mraa_aio_init(1);
    if (tempo == NULL)
    {
        fprintf(stderr, "Error initializing temperature sensor!\n");
        fflush(stderr);
        mraa_aio_close(tempo);
        exit(2);
    }
    button = mraa_gpio_init(60);
    if (button == NULL)
    {
        fprintf(stderr, "Error initializing button!\n");
        fflush(stderr);
        mraa_gpio_close(button);
        exit(2);
    }
    mraa_gpio_dir(button, MRAA_GPIO_IN);  // not 0
}

void exit_handle(int num)
{
    mraa_aio_close(tempo);
    mraa_gpio_close(button);
    exit(num);
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
        
        memset((char *)buffer, '\0', 1024);
        sprintf(buffer, "%02d:%02d:%02d %04.1f\n", hour, min, sec, tempo);
        print_func();
        
        
        //start counting again
        clock_gettime(CLOCK_MONOTONIC, &start);
        time_count = 0;
        start_count = 0;
    }
    return;
}

void shut_down()
{
    int hour, min, sec;
    time_t t = time(NULL);
    struct tm tt = *localtime(&t);
    hour = tt.tm_hour;
    min = tt.tm_min;
    sec = tt.tm_sec;
    memset((char *)buffer, '\0', 1024);
    sprintf(buffer, "%02d:%02d:%02d SHUTDOWN\n", hour, min, sec);
    print_func();
    exit_handle(0);
}

void button_check()
{
    int ret;
    ret = mraa_gpio_read(button);
    if (ret < 0)
    {
        fprintf(stderr, "Error reading button!\n");
        fflush(stderr);
        exit_handle(2);
    }
    
    if (ret == 1)
        shut_down();
}

int main(int argc, char * argv[]) {
    int ret = 0;
    int optindex = 0;
    int args_num = 1;
    
    const struct option long_options[] =
    {
        {"period",required_argument,0,'p'},
        {"scale",required_argument,0,'s'},
        {"id", required_argument, NULL, 'i'},
        {"host", required_argument, NULL, 'h'},
        {"log", required_argument, NULL, 'l'},
        {0, 0, 0, 0}
    };
    
    while(1)
    {
        ret = getopt_long(argc, argv, "p:s:i:h:l:", long_options, &optindex);
        if (ret == -1) // parsed all option
            break;
        
        switch (ret) {
            case 'p':
                period = atoi(optarg);
                if(period <= 0)
                {
                    fprintf(stderr, "Period paramenter should be more than zero!\n");
                    fprintf(stderr, "Invalid argument!\nCorrect usage: --period=#, --scale={C,F}, --id=9-digit-number, --host=name_or_address, --log=filename, port_number\n");
                    fflush(stderr);
                    exit(1);
                }
                break;
                
            case 's':
                scale = *optarg;
                if(scale != 'C' && scale != 'F')
                {
                    fprintf(stderr, "Invalid argument!\nCorrect usage: --period=#, --scale={C,F}, --id=9-digit-number, --host=name_or_address, --log=filename, port_number\n");
                    fflush(stderr);
                    exit(1);
                }
                break;
                
            case 'i':
                id = optarg;
                if (strlen(id) != 9){
                    fprintf(stderr, "Invalid argument!\nCorrect usage: --period=#, --scale={C,F}, --id=9-digit-number, --host=name_or_address, --log=filename, port_number\n");
                    fflush(stderr);
                    exit(1);
                }
                break;
                
            case 'h':
                host = optarg;
                break;
                
            case 'l':
                log_fd = open(optarg, O_RDWR | O_CREAT | O_TRUNC, 0666);
                f_log = 1;
                //                if (ofd >= 0)
                //                {
                //                    close(STDOUT_FILENO);
                //                    dup(ofd);
                //                    close(ofd);
                //                }
                if (log_fd < 0)
                {
                    int err = errno;
                    fprintf(stderr, "Error opening output file %s\n", optarg);
                    fflush(stderr);
                    printf("Error type: %s\n",strerror(err));
                    exit(1);
                }
                break;
                
            default:
                fprintf(stderr, "Invalid argument!\nCorrect usage: --period=#, --scale={C,F}, --id=9-digit-number, --host=name_or_address, --log=filename, port_number\n");
                fflush(stderr);
                exit(1);
                break;
        }
        args_num++;
    }
    while(args_num != argc){
        port = atoi(argv[args_num]);
        args_num++;
        if (args_num < argc)
        {
            fprintf(stderr, "Invalid argument!\nCorrect usage: --id=9-digit-number, --host=name_or_address, --log=filename, port_number\n");
            fflush(stderr);
            exit(1);
        }
    }
    
    socket_init();
    
    io_init();
    
    struct pollfd pfd[1];
    pfd[0].fd = sock_fd;
    pfd[0].events = POLLIN;
    char input_buf[1024];
    char output_buf[1024];
    
    while (1)
    {
        ret = poll(pfd, 1, 0);
        if (ret < 0)    // error occured
        {
            int err = errno;
            fprintf(stderr, "Error using poll()!\n");
            fflush(stderr);
            printf("Error type: %s\n",strerror(err));
            exit_handle(2);
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
            int len = SSL_read(ssl_client, input_buf, 1024);
//            if (len == 0)
//            { // EOF
//                shut_down();
//            }
            
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
                            dprintf(log_fd, "%s\n", output_buf);
                        }
                        shut_down();
                    }
                    
                    else if (strncmp(input_buf, "STOP",4) == 0)
                    {
                        if(f_log)
                        {
                            dprintf(log_fd, "%s\n", output_buf);
                        }
                        f_stop = 1;
                        clock_gettime(CLOCK_MONOTONIC, &stop);
                        time_count = stop.tv_sec - start.tv_sec;
                    }
                    
                    else if (strncmp(input_buf, "START",5) == 0)
                    {
                        if (f_log)
                        {
                            dprintf(log_fd, "%s\n", output_buf);
                        }
                        f_stop = 0;
                        clock_gettime(CLOCK_MONOTONIC, &start);
                    }
                    
                    else if (strncmp(input_buf, "SCALE=F", 7) == 0)
                    {
                        if (f_log)
                        {
                            dprintf(log_fd, "%s\n", output_buf);
                        }
                        scale = 'F';
                    }
                    
                    else if (strncmp(input_buf, "SCALE=C", 7) == 0)
                    {
                        if (f_log)
                        {
                            dprintf(log_fd, "%s\n", output_buf);
                        }
                        scale = 'C';
                    }
                    
                    else if (strncmp(input_buf, "PERIOD=", 7) == 0)
                    {
                        period = atoi(input_buf + 7);
                        if (period <= 0){
                            fprintf(stderr, "Period paramenter should be more than zero!\n");
                            exit_handle(2);
                        }
                        if (f_log)
                        {
                            dprintf(log_fd, "%s\n", output_buf);
                        }
                    }
                    
                    else
                    {
                        if (f_log)
                        {
                            dprintf(log_fd, "%s\n", output_buf);
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
            fprintf(stderr, "Error polling from keyboard!\n");
            fflush(stderr);
            printf("Error type: %s\n",strerror(err));
            exit_handle(2);
        }
        
        if(pfd[0].revents & POLLHUP)
        {
            break;
        }
    }
    
    exit_handle(0);
    return 0;
}
