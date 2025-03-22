#ifndef _COMSTRUCT_MSG_H
#define	_COMSTRUCT_MSG_H

#define MSG_LENGTH 80

void generate_data_message_rdatab(char *msg, char *uid, int value);

void generate_sink_message_rdatab(char *msg, char *uid, char *origin_uid, char *value);

#endif // _USB_UART_EXAMPLE_H