#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   
#include <linux/module.h>   
#include <linux/fs.h>       
#include <linux/uaccess.h> 
#include <linux/string.h> 
#include <linux/slab.h> 
#include "message_slot.h"

MODULE_LICENSE("GPL");

// Function headers

static int device_open(struct inode* inode, struct file* file);
static ssize_t device_read(struct file* file, char __user* buffer, size_t length, loff_t* offset);
static ssize_t device_write(struct file* file, const char __user* buffer, size_t length, loff_t* offset);
static long device_ioctl(struct file* file, unsigned int ioctl_command_id, unsigned long ioctl_param);
void insert_slot(SLOT* new_slot);
void insert_channel(SLOT* slot, CHANNEL* channel);
SLOT* search_minor(int minor);
CHANNEL* set_channel(SLOT* slot, unsigned long channel_id);
void free_channels_list(CHANNEL* head);
void free_slots_list(SLOT* slot);


static SLOT* slot_head;


static int device_open(struct inode* inode, struct file*  file){
    SLOT* new_slot;
    int minor; 
    minor = iminor(inode);
    if (search_minor(minor) == NULL){
        new_slot = (SLOT*)kmalloc(sizeof(SLOT), GFP_KERNEL);
        if (new_slot == NULL) return -ENOMEM;
        new_slot -> head = NULL;
        new_slot -> minor = minor;
        insert_slot(new_slot);
    }
    file -> private_data = NULL;
    return 0;
}


static ssize_t device_read( struct file* file, char __user* buffer, size_t length, loff_t* offset){
    CHANNEL* ch; 
    int i;
    char* txt;
    ch = (CHANNEL*) file->private_data;
    if (ch == NULL) return -EINVAL;
    if (ch -> msg_len > length || ch -> msg_len == 0){
        if (ch -> msg_len > length){
            return -ENOSPC;
        }
        else{
            return -EWOULDBLOCK;
        }
    }

    txt = (char*)kmalloc(sizeof(char)*(BUFFER_LENGTH), GFP_KERNEL);
    if (txt == NULL) return -ENOMEM; 
    for (i = 0; i < BUFFER_LENGTH; i++){
        if (get_user(txt[i], &buffer[i]) < 0) return -EFAULT;
    }

    for (i = 0; i < ch -> msg_len; i++){
        if(put_user((ch -> buffer)[i], &buffer[i]) < 0) return -EFAULT;
    }  

    kfree(txt);
    return i;
}

static ssize_t device_write( struct file* file, const char __user* buffer, size_t length,loff_t* offset) {
    int i;
    CHANNEL* ch;
    char* temp;
    char* txt;
    ch = (CHANNEL*)file -> private_data;
    if (ch == NULL) return -EINVAL;
    if ((length > BUFFER_LENGTH ) || (length <= 0)) return -EMSGSIZE;

    txt = (char*)kmalloc(length*sizeof(char), GFP_KERNEL);
    if (txt == NULL) return -ENOMEM;
    
    temp = ch -> buffer;
    for (i = 0; i < length; i++){
        if (get_user(txt[i], &buffer[i])<0) return -EFAULT;     
    }

    ch -> msg_len = length;
    ch -> buffer = txt;

    kfree(temp);
    return length; 
}

static long device_ioctl( struct   file* file, unsigned int   ioctl_command_id, unsigned long  ioctl_param ){
    CHANNEL* curr_ch;
    SLOT* slot;

    if ((ioctl_param == 0) ||  (ioctl_command_id != MSG_SLOT_CHANNEL)) return -EINVAL;
    
    slot = search_minor(iminor(file -> f_inode));
    if (slot == NULL) return -EFAULT;

    curr_ch = set_channel(slot, ioctl_param);
    if (curr_ch == NULL) return -ENOMEM;

    file -> private_data = curr_ch;
    return 0;
}


struct file_operations Fops = {
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
};

// Assisting functions 

// function to insret list
void insert_slot(SLOT* new_slot){
    if (slot_head == NULL){
        slot_head = new_slot;
    }
    else{
        slot_head->prev = new_slot;
        new_slot->next = slot_head;
        slot_head = new_slot;
    }
}

// function to insert channel
void insert_channel(SLOT* slot, CHANNEL* channel){
    if (slot->head != NULL){
        (slot->head)->prev = channel;
        channel->next = slot->head;
    }
    slot->head = channel;
}

// search current minor in the slots list
SLOT* search_minor(int minor){
    SLOT* curr_slot;
    curr_slot = slot_head;
    while (curr_slot != NULL){
        if (curr_slot -> minor == minor){
            return curr_slot;
        }
        curr_slot = curr_slot -> next;
    }
    return NULL;
}


// set correct channel by id 
CHANNEL* set_channel(SLOT* slot, unsigned long channel_id){
    CHANNEL* curr_ch;
    CHANNEL* new_ch;
    curr_ch = slot->head;
    while(curr_ch != NULL){
        if (curr_ch -> id == channel_id){
            return curr_ch;
        }
        curr_ch = curr_ch -> next;
    }
    new_ch = (CHANNEL*) kmalloc(sizeof(CHANNEL),GFP_KERNEL);
    if (new_ch == NULL){
        return NULL;
    }
    new_ch -> msg_len = 0;    
    new_ch -> id = channel_id;
    insert_channel(slot, new_ch);
    return new_ch;
}

// simple free list function
void free_channels_list(CHANNEL* head){
    if (head != NULL){
        free_channels_list(head->next);
        kfree(head -> buffer);
        kfree(head);
    }  
}

// simple free list function
void free_slots_list(SLOT* slot){
    if (slot != NULL){
        free_slots_list(slot -> next);
        free_channels_list(slot -> head);
        kfree(slot);
    }
}


static int __init simple_init(void){
    int rc;
    rc = register_chrdev(MAJOR_NUM, DEVICE_NAME, &Fops );
    if( rc < 0 ) {
        printk(KERN_ALERT "%s failed to register :  %d\n", DEVICE_NAME, MAJOR_NUM );
        return rc;
    }
    slot_head = NULL;
    return 0;
}

static void __exit simple_cleanup(void)
{
    free_slots_list(slot_head);
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

