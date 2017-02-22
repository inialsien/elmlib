#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#if 0
struct can_frame {
	canid_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
	__u8    can_dlc; /* frame payload length in byte (0 .. CAN_MAX_DLEN) */
	__u8    data[CAN_MAX_DLEN] __attribute__((aligned(8)));
};
#endif

#define init_count 8
char * init_msg[init_count] = {
    "AT Z\r\n",     // reset device
    "AT @1\r\n",    // print device identifier
    //"AT E1\r\n",  // echo enable
    "AT L0\r\n",    // line-feeds off
    "AT S1\r\n",    // printing spaces off
    "AT H1\r\n",    // printing spaces off
    "AT D1\r\n",    // printing spaces off
    "AT V1\r\n",    // printing spaces off
    "AT MA\r\n",    // monitor all 
};   

char * test_buffer = "330 4 00 00 00 10\r150 8 40 00 0F FF 3F FF F0 00 <DATA ERROR\r151 8 00 FF 09 BE 00 00 00 00\r151 8 00 FF 0A 02 00 00 00 00\r380 8 12 1A 00 00 80 00 FC 0E\r388 6 01 01\r110 8 00 0B FA 00 00 29 00 00\r120 8 03 3A 03 A6\r124 8 20 03 3A 03 3A 00 00 00\r150 8 00 00 0F FF 00 00 F0 01\r151 8 00 FF 0A 02 00 00 00 00\r110 8 00 0B EE 00 00 2A 00 00\r120 8 03 3A 03 A6\r124 8 20 03 3A 03 3A 00 00 00\r410 8 00 00 00 28\r";

#define INPUT_BUF_SIZE 1024
typedef struct device_handle{
    struct tframe * head;
    char * ascii_buffer;
    size_t buffer_size;
    int output_socket;
    int input_fd;
    char buf[INPUT_BUF_SIZE];
    char storedsz;
}device_handle_t;

int elm_get_frame(device_handle_t * dev, struct can_frame *frame){
    char * index;
    int retval = 0; 
    int i;
    int nbytes = elm327_read(dev->input_fd, dev->buf + dev->storedsz, INPUT_BUF_SIZE - dev->storedsz - 1 );
    //printf("%d = elm327_read(%d, dev->buf +%d, %d);\n",nbytes,  dev->input_fd,  dev->storedsz, INPUT_BUF_SIZE - dev->storedsz - 1 );
    
    if(nbytes < 0){
        printf("Read %d bytes from %d\n",nbytes, dev->input_fd);
        return 0;
    }  
    dev->storedsz += nbytes;
    dev->buf[dev->storedsz] = '\0';
    //printf("Read %d bytes from %d: \n%s\n",nbytes, dev->input_fd, dev->buf);
 
    if(dev->storedsz > 0){
        int substring_sz;

        index = memchr(dev->buf, '\r', dev->storedsz);
        if(index == NULL)
            return 0;

        //printf("Got index %p +%d\t\tss=%d\n", index, index - dev->buf, dev->storedsz);

        substring_sz = index - dev->buf + 1;
        if(index != dev-> buf){

            //printf("[pre] sub_sz= %d ss=%d\n", substring_sz, dev->storedsz);
            *index = '\0';
            printf("rx ( % 2d ): %s\n", strlen(dev->buf), dev->buf);

            // process here 

            // printf("[post] ss=%d\n", dev->storedsz);
            retval = -1;
        }

        memmove(dev->buf, index + 1, dev->storedsz - substring_sz);
        dev->storedsz -= substring_sz;
        
    }
    return retval;
}

int bridge(device_handle_t * dev, char * usb_device, char * can_device){
    int nbytes;
    int i;
    struct sockaddr_can addr;
    struct ifreq ifr;
    struct can_frame frame = {.can_id = 0x123, .can_dlc = 8, .data={0x01,0x23,0x45,0x67,0x89,0x11,0x22,0x33}};
    //dev->storedsz = 0;
    dev->storedsz = strlen(test_buffer);
    memcpy(dev->buf, test_buffer, dev->storedsz);

    if(!dev)
        return -1;

    if((dev->output_socket = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        perror("Error while opening socket");
        return -1;
    }

    strcpy(ifr.ifr_name, can_device);
    ioctl( dev->output_socket, SIOCGIFINDEX, &ifr);
    
    addr.can_family  = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    //printf("%s at index %d\n", can_device, ifr.ifr_ifindex);

    if(bind(dev->output_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Error in socket bind");
        return -2;
    }

    dev->input_fd = elm327_connect(usb_device);
    if(dev->input_fd <= 0){
        fprintf(stderr, "Error connecting to %s\n",usb_device);
        return 1;
    }
    else
        printf("USB socket: %d\n", dev->input_fd);

    for(i=0;i<init_count;++i){
        printf("tx ( % 2d ): %s", strlen(init_msg[i]),  init_msg[i]);
        elm327_write(dev->input_fd, init_msg[i], strlen(init_msg[i]));
        while(elm_get_frame(dev, &frame) != -1);
        
        sleep(1);
    }
        
    while(1){
        if(elm_get_frame(dev, &frame) > 0){
            nbytes = write(dev->output_socket, &frame, sizeof(struct can_frame));
            //printf("Wrote %d bytes\n", nbytes);
        }
        else{
            elm327_write(dev->input_fd, "\r", 2);
        }
    }

    close(dev->output_socket);
    close(dev->input_fd);

    return 0;
}

int main(int argc, char * argv[]){
    device_handle_t can_bridge;
    return bridge(&can_bridge, "/dev/ttyUSB0", "vcan0");
}
