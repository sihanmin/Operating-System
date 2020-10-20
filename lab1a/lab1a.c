//
//  lab1a.c
//  proj1
//
//  Created by Mint MSH on 10/7/17.
//  Copyright © 2017 Mint MSH. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/termios.h>
#include <termios.h>
#include <errno.h>
#include <poll.h>
#include <sys/poll.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

// Use this variable to remember original terminal attributes
struct termios saved_attributes;

void reset_input_mode (void)
{
    tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
}

void set_input_mode (void)
{
    // Make sure stdin is a terminal
    if (!isatty (STDIN_FILENO))
    {
        fprintf (stderr, "Not a terminal.\r\n");
        exit(1);
    }
    
    // Save the terminal attributes to restore them later
    tcgetattr (STDIN_FILENO, &saved_attributes);
    atexit (reset_input_mode);
    
    // Set the required terminal modes
    struct termios tattr = saved_attributes;
    tattr.c_iflag = ISTRIP;	/* only lower 7 bits	*/
    tattr.c_oflag = 0;		/* no processing	*/
    tattr.c_lflag = 0;		/* no processing	*/
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr (STDIN_FILENO, TCSANOW, &tattr);
}

void sig_handler(int signum)
{
    if (signum == SIGPIPE)
    {
        fprintf(stderr, "SIGPIPE received!\r\n");
        exit(0);
    }
}


int main (int argc, char * argv[])
{
    // need a signal handler
    
    int buf_to_sh[2];
    int sh_to_buf[2];
    pid_t child_pid = -1;
    
    int shell = 0;
    struct option long_options[]=
    {
        {"shell", no_argument, &shell, 1},
        {0, 0, 0, 0}
    };
    int opt = 0;
    
    // process flag
    while (1)
    {
        opt = getopt_long(argc, argv, "s", long_options, NULL);
        if (opt == -1) // no more flag
            break;
        if (opt == '?')
        {
            fprintf(stderr, "Invalid flag!\nCorrect flag usage: --shell\n");
            exit(1);
            break;
        }
    }

    // set noncanonical mode
    set_input_mode ();
    
    
    if (shell)
    {
        // construct two-way-pipe
        if(pipe(buf_to_sh) == -1)
        {
            int err = errno;
            fprintf(stderr, "Error creating pipe!\r\n");
            printf("Error type: %s\r\n",strerror(err));
            exit(1);
        }
        if(pipe(sh_to_buf) == -1)
        {
            int err = errno;
            fprintf(stderr, "Error creating pipe!\r\n");
            printf("Error type: %s\r\n",strerror(err));
            exit(1);
        }
        
        signal(SIGPIPE,sig_handler);
        
        child_pid = fork();
        
        if (child_pid < 0)
        {
            int err = errno;
            fprintf(stderr, "Error using fork()!\r\n");
            printf("Error type: %s\r\n",strerror(err));
            exit(1);
        }
        
        // parent process
        if (child_pid > 0)
        {
            //close unused pipes first
            close(buf_to_sh[0]);
            close(sh_to_buf[1]);
            
            struct pollfd pfds[2];
            pfds[0].fd = STDIN_FILENO;  // from keyboard
            pfds[0].events = POLLIN;
            pfds[1].fd = sh_to_buf[0];    // from shell
            pfds[1].events = POLLIN;
            
            int ret;
            while (1)
            {
                ret = poll(pfds, 2, 0);
                if (ret < 0)    // error occured
                {
                    int err = errno;
                    fprintf(stderr, "Error using poll()!\r\n");
                    printf("Error type: %s\r\n",strerror(err));
                    exit(1);
                }
                
                // keyboard input to screen and shell
                if (pfds[0].revents & POLLIN)
                {
                    char buf[1024];
                    ssize_t count = read(STDIN_FILENO, buf, 1024);
                    int i;
                    for (i = 0; i < count; i++)
                    {
                        if (buf[i] == '\r' || buf[i] == '\n')
                        {
                            write(STDOUT_FILENO, "\r\n", 2);
                            write(buf_to_sh[1], "\n", 1);
                        }
                        else if (buf[i] == 4)
                        {
                            write(STDOUT_FILENO, "^D\r\n", 4);
                            close(buf_to_sh[1]);
                        }
                        else if (buf[i] == 3)
                        {
                            write(STDOUT_FILENO, "^C\r\n", 4);
                            kill(child_pid, SIGINT);
                        }
                        else
                        {
                            write(STDOUT_FILENO, &buf[i], 1);
                            write(buf_to_sh[1], &buf[i], 1);
                        }
                    }
                }
                
                // shell input to screen
                if (pfds[1].revents & POLLIN)
                {
                    char buf[1024];
                    ssize_t count = read(sh_to_buf[0], buf, 1024);
                    int i;
                    
                    for (i = 0; i < count; i++)
                    {
                        if (buf[i] == '\n')
                            write(STDOUT_FILENO, "\r\n", 2);
                        else
                            write(STDOUT_FILENO, &buf[i], 1);
                    }
                }
                
                if (pfds[0].revents & (POLLHUP + POLLERR))
                {
                    int err = errno;
                    fprintf(stderr, "Error polling from keyboard!\r\n");
                    printf("Error type: %s\r\n",strerror(err));
                    break;
                }
                
                if (pfds[1].revents & (POLLHUP + POLLERR))
                {
                    close(buf_to_sh[1]);
                    close(sh_to_buf[0]);
                    
                    int wstatus;
                    pid_t close_pid = waitpid(child_pid, &wstatus, WNOHANG);
                    if (close_pid == -1)
                    {
                        int err = errno;
                        fprintf(stderr, "Error reaping child!\r\n");
                        printf("Error type: %s\r\n",strerror(err));
                        exit(1);
                    }
                    else
                    {
                        fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\r\n",WTERMSIG(wstatus), WEXITSTATUS(wstatus));
                        exit(0);
                    }
                }
                
                
            }
            
            
        }
        
        // child process
        else if (child_pid == 0)
        {
            // close unused pipes first
            close(buf_to_sh[1]);
            close(sh_to_buf[0]);
            
            // input/output redirection
            dup2(buf_to_sh[0],STDIN_FILENO);
            close(buf_to_sh[0]);
            dup2(sh_to_buf[1],STDOUT_FILENO);
            dup2(sh_to_buf[1],STDERR_FILENO);
            close(sh_to_buf[1]);
            
            // open bash
            if(execl("/bin/bash", "/bin/bash", NULL) == -1)
            {
                int err = errno;
                fprintf(stderr, "Error using execl()!\r\n");
                printf("Error type: %s\r\n",strerror(err));
                exit(1);
            }
        }
        
    }
    
    else
    {
        char buf[1024];
        ssize_t count = 0;
        while(1)
        {
            count = read(STDIN_FILENO, buf, 1024);
            if (count < 0)
            {
                int err = errno;
                fprintf(stderr, "Error reading from keyboard!\r\n");
                printf("Error type: %s\r\n",strerror(err));
                exit(1);
            }
            
            int i;
            for (i = 0; i < count; i++)
            {
                if (buf[i] == '\r' || buf[i] == '\n') {
                    write(STDOUT_FILENO, "\r\n", 2);
                }
                else if (buf[i] == 4) {
                    write(STDOUT_FILENO, "^D\r\n", 4);
                    exit(0);
                }
                else
                {
                    write(STDOUT_FILENO, &buf[i], 1);
                }
            }
            // check read and write return -1
        }
    }
    
    exit(0);
    return EXIT_SUCCESS;
}
