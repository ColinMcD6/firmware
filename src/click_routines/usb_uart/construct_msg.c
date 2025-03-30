#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "construct_msg.h"

void generate_data_message_rdatab(char *msg, char *uid, int value) {
    int n = snprintf(msg, MSG_LENGTH, ">D:%s:$%d$<", uid, value);
    
    int padding_length = MSG_LENGTH - n;
    
    if (padding_length > 0) {
        for (int i = 0; i < padding_length-1; i++) {
            msg[n+i] = 'X';
        }
        msg[n+padding_length-1] = '\r';
        msg[n+padding_length] = '\0';
    }
}

void generate_sink_message_rdatab(char *msg, char *uid, char *origin_uid, char *value) {
    int n = snprintf(msg, MSG_LENGTH, ">S?%s?:%s:$%s$<", uid, origin_uid, value);
    
    int padding_length = MSG_LENGTH - n;
    
    if (padding_length > 0) {
        for (int i = 0; i < padding_length-1; i++) {
            msg[n+i] = 'X';
        }
        msg[n+padding_length-1] = '\r';
        msg[n+padding_length] = '\0';
    }
}

void generate_cluster_head_message_rdatab(char *msg, char *uid, double random_dbl) {
    int n = snprintf(msg, MSG_LENGTH, ">C:%s:$%f$<", uid, random_dbl);
    
    int padding_length = MSG_LENGTH - n;
    
    if (padding_length > 0) {
        for (int i = 0; i < padding_length-1; i++) {
            msg[n+i] = 'X';
        }
        msg[n+padding_length-1] = '\r';
        msg[n+padding_length] = '\0';
    }
}