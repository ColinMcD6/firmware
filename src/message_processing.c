#include <stdint.h>
#include <stdio.h>
#include "message_processing.h"

uint8_t id_index = 0;

uint8_t value_index = 0;

Message message;

void reset_id() {
    for (int i = 0; i < MAX_ID_LENGTH; i++) {
        message.node_id[i] = '\0';
    }
    id_index = 0;
}

void reset_value() {
    for (int i = 0; i < MAX_VALUE_LENGTH; i++) {
        message.value[i] = '\0';
    }
    value_index = 0;
}

ReadState nextState(ReadState currentState, char read_character) {
    switch (currentState) {
        case WAITING_FOR_MESSAGE:
            if (read_character == '>') return READING_MESSAGE_TYPE;
            else return WAITING_FOR_MESSAGE;
            break;
        case READING_MESSAGE_TYPE:
            if (read_character == 'D') {
//                printf("Read D\r\n");
                message.message_type = DATA;
                return WAITING_FOR_ID;
            }
            else return WAITING_FOR_MESSAGE;
            break;
        case WAITING_FOR_ID:
            if (read_character == ':') {
                reset_id();
                return READING_ID;
            }
            else {
//                printf("When waiting for ID (:), read %c\r\n", read_character);
                return WAITING_FOR_MESSAGE;
            };
            break;
        case READING_ID:
            if (read_character == ':') {
//                printf("Read ID as %s\r\n", message.node_id);
                return WAITING_FOR_VALUE;
            }
            else if (read_character == '$' || read_character == '<' || read_character == 'X' || id_index >= MAX_ID_LENGTH-1) {
                return WAITING_FOR_MESSAGE;
            }
            else {
                message.node_id[id_index] = read_character;
                id_index++;
                return READING_ID;
            }
            break;
        case WAITING_FOR_VALUE:
            if (read_character == '$') {
                reset_value();
                return READING_VALUE;
            }
            else return WAITING_FOR_MESSAGE;
            break;
        case READING_VALUE:
            if (read_character == '$') {
//                printf("Read value as %s\r\n", message.value);
                return WAITING_FOR_MESSAGE_END;
            }
            else if (read_character == ':' || read_character == '<' || read_character == 'X' || id_index >= MAX_VALUE_LENGTH-1) {
                return WAITING_FOR_MESSAGE;
            }
            else {
                message.value[value_index] = read_character;
                value_index++;
                return READING_VALUE;
            }
            break;
        case WAITING_FOR_MESSAGE_END:
            if (read_character == '<' || read_character == 'X') return MESSAGE_READY;
            else return WAITING_FOR_MESSAGE;
            break;
        case MESSAGE_READY:
            return WAITING_FOR_MESSAGE;
            break;
        default:
            return currentState;
    }
}

void get_message(Message * msg) {
    char type = 'N';
    if (message.message_type == DATA) type = 'D';
    printf("Returning message with message type=%c, ID=%s, value=%s\r\n", type, message.node_id, message.value);
    msg->message_type = message.message_type;
    for (int i = 0; i < MAX_ID_LENGTH; i++) {
       msg->node_id[i] = message.node_id[i]; 
    }
    for (int i = 0; i < MAX_VALUE_LENGTH; i++) {
        msg->value[i] = message.value[i];
    }
}

void reset_message() {
    message.message_type = NONE;
    reset_id();
    reset_value();
}