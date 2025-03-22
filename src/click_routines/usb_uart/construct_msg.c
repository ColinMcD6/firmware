#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "construct_msg.h"

void generate_data_message_rdatab(char *msg, char *uid, int msg_number) {
    int n = snprintf(msg, MSG_LENGTH, ">D:%s:$%d$<", uid, msg_number);
    
    int padding_length = MSG_LENGTH - n;
    
    if (padding_length > 0) {
        for (int i = 0; i < padding_length-1; i++) {
            msg[n+i] = 'X';
        }
        msg[n+padding_length-1] = '\r';
        msg[n+padding_length] = '\0';
    }
}