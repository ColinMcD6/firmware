#ifndef _MESSAGE_PROCESSING_H
#define	_MESSAGE_PROCESSING_H

#define MAX_ID_LENGTH 3
#define MAX_VALUE_LENGTH 10
// Define states
typedef enum {
    WAITING_FOR_MESSAGE,
    READING_MESSAGE_TYPE,
    WAITING_FOR_ID,
    READING_ID,
    WAITING_FOR_VALUE,
    READING_VALUE,
    WAITING_FOR_MESSAGE_END,
    MESSAGE_READY,
} ReadState;

typedef enum {
    NONE,
    DATA,
    SINK,
    CLUSTER_HEAD,
    PAIR,
} MSGType;

typedef struct {
    MSGType message_type;
    char node_id[MAX_ID_LENGTH];
    char value[MAX_VALUE_LENGTH];
} Message;

ReadState next_read_state(ReadState currentState, char read_character);

void get_message(Message *message);

void reset_message();

#endif // _USB_UART_EXAMPLE_H