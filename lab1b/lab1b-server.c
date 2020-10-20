//
//  lab1b.c
//  proj1b
//
//  Created by Mint MSH on 10/15/17.
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
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <mcrypt.h>

struct termios saved_attributes;
MCRYPT en_td;
MCRYPT de_td;

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

void closeEncryption()
{
    mcrypt_generic_deinit(en_td);
    mcrypt_module_close(en_td);
    mcrypt_generic_deinit(de_td);
    mcrypt_module_close(de_td);
}

void sig_handler(int signum)
{
    if (signum == SIGPIPE)
    {
        fprintf(stderr, "SIGPIPE received!\r\n");
        exit(0);
    }
}

int main(int argc, char * argv[]) {
    int p_flag = 0;
    int e_flag = 0;
    int port = 0;
    int sock_fd;
    int key_fd;
    int keysize = 0;
    char key[20] = "\0";
    char* EIV = NULL;
    char* DIV = NULL;
    
    int new_sock_fd, clilen;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    
    struct option long_options[] = {
        {"port", required_argument, NULL, 'p'},
        {"encrypt", required_argument, NULL, 'e'},
        {0, 0, 0, 0}
    };
    
    int ret;
    while(1){
        ret = getopt_long(argc, argv, "", long_options, NULL);
        if (ret == -1) // parsed all option
            break;
        
        switch (ret) {
            case 'p':
                p_flag = 1;
                port = atoi(optarg);
                break;
            case 'e':
                e_flag = 1;
                key_fd = open(optarg, O_RDONLY);
                if (key_fd < 0)
                {
                    fprintf(stderr, "Error opening keyfile: %s", optarg);
                    exit(1);
                }
                keysize = read(key_fd, key, 16);
                if (keysize < 0)
                {
                    fprintf(stderr, "Error reading keyfile: %s", optarg);
                    exit(1);
                }
                break;
            default:
                fprintf(stderr, "Invalid argument!\nCorrect usage: --port=portnumber, --encrypt=keyfile\n");
                exit(1);
                break;
        }
    }
    if(!p_flag) {
        fprintf(stderr, "--port=portnumber option required!\n");
        exit(1);
    }
    
    if (e_flag) {
        // encryption
        en_td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
        if (en_td == MCRYPT_FAILED)
        {
            fprintf(stderr, "Error initializing encryption key\n");
            exit(1);
        }
        EIV = malloc(mcrypt_enc_get_iv_size(en_td));
        int i;
        for (i = 0; i< mcrypt_enc_get_iv_size(en_td); i++)
        {
            //EIV[i]=rand();
            EIV[i] = i;
        }
        i = mcrypt_generic_init(en_td, key, keysize, EIV);
        if (i<0)
        {
            mcrypt_perror(i);
            fprintf(stderr, "Error calling mcrypt_generic_init\n");
            exit(1);
        }
        
        
        // decreption
        de_td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
        if (de_td == MCRYPT_FAILED)
        {
            fprintf(stderr, "Error initializing encryption key\n");
            exit(1);
        }
        DIV = malloc(mcrypt_enc_get_iv_size(de_td));
        for (i = 0; i< mcrypt_enc_get_iv_size(de_td); i++)
        {
            //DIV[i]=rand();
            DIV[i] = i;
        }
        i = mcrypt_generic_init(de_td, key, keysize, DIV);
        if (i<0)
        {
            mcrypt_perror(i);
            fprintf(stderr, "Error calling mcrypt_generic_init\n");
            exit(1);
        }
        
        atexit(closeEncryption);
    }
    
    
    
    int buf_to_sh[2];
    int sh_to_buf[2];
    pid_t child_pid = -1;
    
    
    set_input_mode ();
    
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
        // close unused pipes first
        close(buf_to_sh[0]);
        close(sh_to_buf[1]);
        
        // first call to socket() function
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) {
            fprintf(stderr, "Error opening socket!\r\n");
            kill(child_pid, SIGINT);
            exit(1);
        }
        
        // initialize socket structure
        memset((char *) &serv_addr, '\0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_addr.sin_port = htons(port);
        
        // bind the host address using bind() call
        if (bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            fprintf(stderr, "Error binding the host address!\r\n");
            kill(child_pid, SIGINT);
            exit(1);
        }
        
        // Now start listening for the clients, here process will
        // go in sleep mode and will wait for the incoming connection
        listen(sock_fd, 1);
        
        // accept actual connection from the client
        clilen = sizeof(cli_addr);
        new_sock_fd = accept(sock_fd, (struct sockaddr *) &cli_addr, &clilen);
        if (new_sock_fd < 0)
        {
            fprintf(stderr, "Error accepting port: %d\r\n", port);
            kill(child_pid, SIGINT);
            exit(1);
        }
        else
        {
            close(sock_fd);
            //write(STDERR_FILENO, "One client accepted\r\n", 21);
        }

        
        
        struct pollfd pfds[2];
        pfds[0].fd = new_sock_fd;  // from keyboard
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
            
            // socket input to shell
            if (pfds[0].revents & POLLIN)
            {
                char buf[1024] = "\0";
                ssize_t count = read(new_sock_fd, buf, 1024);
                int i;
                if (e_flag)
                    mdecrypt_generic(de_td, buf, count);
                //write(STDERR_FILENO, buf, count);
                for (i = 0; i < count; i++)
                {
                    if (buf[i] == '\r' || buf[i] == '\n')
                    {
                        write(buf_to_sh[1], "\n", 1);
                    }
                    else if (buf[i] == 4)
                    {
                        close(buf_to_sh[1]);
                        close(new_sock_fd);
                        //exit(0);
                    }
                    else if (buf[i] == 3)
                    {
                        kill(child_pid, SIGINT);
                        close(new_sock_fd);
                    }
                    else
                    {
                        write(buf_to_sh[1], &buf[i], 1);
                    }
                }
            }
            
            // shell input to socket
            if (pfds[1].revents & POLLIN)
            {
                char buf[1024] = "\0";
                ssize_t count = read(sh_to_buf[0], buf, 1024);
                if(e_flag){
                    mcrypt_generic(en_td, buf, count);
                }
                write(new_sock_fd, buf, count);
            }
            
            if (pfds[0].revents & (POLLHUP + POLLERR))
            {
                int err = errno;
                fprintf(stderr, "Error polling from client!\r\n");
                printf("Error type: %s\r\n",strerror(err));
                close(new_sock_fd);
                kill(child_pid, SIGINT);
                exit(1);
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
