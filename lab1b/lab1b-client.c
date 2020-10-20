//
//  lab1b-client.c
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

int main(int argc, char * argv[]) {

    int p_flag = 0;
    int l_flag = 0;
    int e_flag = 0;
    int port = 0;
    int key_fd, sock_fd;
    int keysize = 0;
    
    char *log_file = NULL;
    char key[20] = "\0";
    char* EIV = NULL;
    char* DIV = NULL;
    
    FILE* log_fd;
    
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    struct option long_options[] =
    {
        {"port", required_argument, NULL, 'p'},
        {"log", required_argument, NULL, 'l'},
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
            case 'l':
                l_flag = 1;
                log_file = (char *)optarg;
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
                close(key_fd);
                break;
            default:
                fprintf(stderr, "Invalid argument!\nCorrect usage: --port=portnumber, --log=filename, --encrypt=keyfile\n");
                exit(1);
                break;
        }
    }
    
    if(!p_flag) {
        fprintf(stderr, "--port=portnumber option required!\n");
        exit(1);
    }
    
    if (l_flag) {
        log_fd = fopen(log_file, "a");
        if (log_fd == NULL) {
            fprintf(stderr, "Error opening log file: %s\n", log_file);
            exit(1);
        }
    }
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        fprintf(stderr, "Error opening socket\n");
        exit(1);
    }
    
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"Error finding the host: localhost\n");
        exit(1);
    }
    
    memset((char *) &serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(port);
    
    if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "Error connecting to port: %d\n", port);
        exit(1);
    }
    //else
        //write(STDOUT_FILENO, "Success in connecting\n", 22);
    
    
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
    
    // set noncanonical mode
    set_input_mode ();
    
    struct pollfd pfds[2];
    pfds[0].fd = STDIN_FILENO;  // from keyboard
    pfds[0].events = POLLIN;
    pfds[1].fd = sock_fd;    // from socket
    pfds[1].events = POLLIN;
    
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
        
        if (pfds[0].revents & POLLIN)
        {
            char buf[1024] = "\0";
            ssize_t count = read(STDIN_FILENO, buf, 1024);
            int i;
            for (i = 0; i < count; i++)
            {
                if (buf[i] == '\r' || buf[i] == '\n')
                {
                    write(STDOUT_FILENO, "\r\n", 2);
                    //write(sock_fd, &buf[i], 1);
                }
                else
                {
                    write(STDOUT_FILENO, &buf[i], 1);
                    //write(sock_fd, &buf[i], 1);
                }
            }
            
            if (e_flag)
            {
                mcrypt_generic(en_td, buf, count);
            }

            write(sock_fd, buf, count);
            
            if (l_flag)
            {
                char log_message[1100];
                sprintf(log_message, "SENT %zd bytes: %s\n", count, buf);
                write(fileno(log_fd), log_message, strlen(log_message));
            }
        }
        
        if (pfds[1].revents & POLLIN)
        {
            char buf[1024] = "\0";
            ssize_t count = read(sock_fd, buf, 1024);
            int i;
            if (count < 1)
            {
                exit(0);
            }
            if (l_flag)
            {
                char log_message[1100];
                sprintf(log_message, "RECEIVED %zd bytes: %s\n", count, buf);
                write(fileno(log_fd), log_message, strlen(log_message));
            }
            if (e_flag)
                mdecrypt_generic(de_td, buf, count);
            
            for (i = 0; i < count; i++ )
            {
                if (buf[i] == '\n' || buf[i] == '\r')
                    write(STDOUT_FILENO, "\r\n", 2);
                else
                    write(STDOUT_FILENO, &buf[i], 1);
            }
        }
        
        if (pfds[0].revents & (POLLHUP + POLLERR)) {
            int err = errno;
            fprintf(stderr, "Error polling from keyboard!\r\n");
            printf("Error type: %s\r\n",strerror(err));
            write(sock_fd, "\x04", 1);
            exit(1); // or break?
        }
        
        if (pfds[1].revents & (POLLHUP + POLLERR)) {
            exit(0);
        }
    }
    exit(0);
    return EXIT_SUCCESS;
}
