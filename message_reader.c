#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include "message_slot.h"

void print_and_exit(){
    fprintf(stderr,"%s\n", strerror(errno));
    exit(1);
}

int main (int argc, char **argv){
    long ch_id;    
    int fd;
    char buffer[BUFFER_LENGTH];
    size_t msg_len;
    
    if (argc != 3){
        fprintf(stderr,"%s\n", strerror(EINVAL));
        return 1;
    }

    fd = open(argv[1],O_RDWR);
    if (fd < 0) print_and_exit();

    ch_id = strtol(argv[2],NULL,10);
    if (ch_id <= 0){
        fprintf(stderr,"%s\n", strerror(EINVAL));
        return 1;
    }
    if (ioctl(fd, MSG_SLOT_CHANNEL, ch_id) < 0) print_and_exit();
    
    msg_len = read(fd, buffer, BUFFER_LENGTH);
    if ((msg_len < 0) || (close(fd) < 0) || (write(STDOUT_FILENO, buffer, msg_len) != msg_len)) print_and_exit();
    return 0;
}



