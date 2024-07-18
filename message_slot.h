#ifndef MESSAGE_SLOT_H
#define MESSAGE_SLOT_H

#include <linux/ioctl.h>

#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)
#define DEVICE_NAME "message_slot"
#define BUFFER_LENGTH 128
#define MAJOR_NUM 235

typedef struct channel{
    struct channel *next;
    struct channel *prev;
    unsigned long id;
    int msg_len;
    char* buffer;
}CHANNEL;

typedef struct slot{
    struct slot *next;
    struct slot *prev;
    CHANNEL *head;
    int minor;
}SLOT;

#endif
