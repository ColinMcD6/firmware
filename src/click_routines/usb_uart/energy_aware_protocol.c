/*******************************************************************************
  USB UART click routine example source file

  Company
    Microchip Technology Inc.

  File Name
    usb_uart_example.c

  Summary
    USB UART click routine example implementation file.

  Description
    This file defines the usage of the USB UART click routine APIs.

  Remarks:
    None.
 
 *******************************************************************************/

// DOM-IGNORE-BEGIN
/*
    (c) 2021 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/
// DOM-IGNORE-END

/**
  Section: Included Files
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "definitions.h"                // SYS function prototypes
#include "usb_uart.h"
#include "construct_msg.h"
#include "energy_aware_protocol.h"
#include <xc.h>

// setup our heartbeat to be 1ms: we overflow at 1ms intervals with a 48MHz clock
// uses the SysTicks unit so that we get reliable debugging (timer stops on breakpoints)
// this is a countdown timer that sends an interrupt when 0
#define MS_TICKS 48000UL

// number of millisecond between LED flashes
#define LED_FLASH_MS    1000UL

// NOTE: this overflows every ~50 days, so I'm not going to care here...
volatile uint32_t msCount = 0;

#define MAX_ID_LENGTH 3
#define MAX_VALUE_LENGTH 10

// Node Parameters and variables
#define PAIRS 1
#define ENERGY 2000 // total number of messages the node can send before it dies
char node_uid[] = "P2"; //2
uint32_t msg_count = 0;
uint8_t cluster_head = 1;

#define MESSAGE_RATE_MS 10000UL

/**
  Section: Example Code
 */
// AT+SENDMCAST:01,FFFF,01,01,C091,0002,Test
// AT+RDATAB:XX then data of size XX

// Commands    
uint8_t restart[] = "ATZ\r";
uint8_t get_info[] = "ATI\r";

uint8_t join_own_network[] = "AT+EN\r";
uint8_t change_channel[] = "AT+CCHANGE:0D\r"; // that's 10 hex! channels go from 11-26, aka 0B to 1A
uint8_t leave_network[] = "AT+DASSL\r";
uint8_t join_network[] = "AT+JN\r";
uint8_t rdatab[] = "AT+RDATAB:50\r";

// Fires every 1ms
void SysTick_Handler()
{
  msCount++;
}

static void configure_node() {
    uint8_t read_chars[100];
    
    usb_uart_USART_Write(leave_network,10);
    while(usb_uart_USART_WriteIsBusy());
    // need to wait for an "OK" to come through
    usb_uart_USART_Read((uint8_t *)&read_chars,2);
    while(usb_uart_USART_ReadIsBusy()); // do nothing
    
    //Enable for RDATAB
    usb_uart_USART_Write(join_own_network,6);
    while(usb_uart_USART_WriteIsBusy());
    // need to wait for an "OK" to come through
    usb_uart_USART_Read((uint8_t *)&read_chars,2);
    while(usb_uart_USART_ReadIsBusy()); // do nothing
    usb_uart_USART_Write(change_channel,14);
    while(usb_uart_USART_WriteIsBusy());
    // need to wait for an "OK" to come through
    usb_uart_USART_Read((uint8_t *)&read_chars,10);
    while(usb_uart_USART_ReadIsBusy()); // do nothing
}

void energy_aware_protocol(void) {   
    
    // LED output
    PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA14;
    PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;
    
    // sleep when idling
    PM_REGS->PM_SLEEPCFG = PM_SLEEPCFG_SLEEPMODE_IDLE;
    
    SysTick_Config(MS_TICKS);

    // sleep for a bit to let the zigbee initialize
    for (int i=0; i<100000; i++);
    printf("booted\r\n");
    
    uint8_t read_chars[100];
    uint8_t buffer[10];
    uint8_t msg[MSG_LENGTH+1];
    
    typedef enum {
        NONE,
        DATA,
        SINK,
        CLUSTER_HEAD,
        PAIR,
    } MSGType;
    
    MSGType msg_type = NONE;
    
    usb_uart_USART_Write(restart,4);
    while(usb_uart_USART_WriteIsBusy());
    // need to wait for an "OK" to come through
    usb_uart_USART_Read((uint8_t *)&read_chars,2);
    while(usb_uart_USART_ReadIsBusy()); // do nothing
    
    configure_node();
    

    // preload loop
    // start reading
    usb_uart_USART_Read((uint8_t *)&read_chars,1);
    printf("Let's go\r\n");
    // Define states
    typedef enum {
        WAITING_FOR_MESSAGE,
        READING_MESSAGE_TYPE,
        WAITING_FOR_ID,
        READING_ID,
        WAITING_FOR_VALUE,
        READING_VALUE,
        WAITING_FOR_MESSAGE_END,
        PROCESS_MESSAGE,
    } ReadState;
    
    ReadState state = WAITING_FOR_MESSAGE;
    
    void print_msg(uint8_t *msg) {
        size_t i = 0;
        while (i < MSG_LENGTH && msg[i] != '\0') {
            printf("%c", msg[i]);
            i++;
        }
        printf("\r\n");
    }
    
    void send_message(uint8_t *msg) {
        usb_uart_USART_Write(rdatab,13);
        while(usb_uart_USART_WriteIsBusy());
        // need to wait for an "OK" to come through
        usb_uart_USART_Read((uint8_t *)&read_chars,2);
        while(usb_uart_USART_ReadIsBusy()); // do nothing
        usb_uart_USART_Write(msg, MSG_LENGTH);
        while(usb_uart_USART_WriteIsBusy());
        msg_count++;
    }
    
    uint8_t id_index = 0;
    char id[MAX_ID_LENGTH];
    
    void reset_id() {
        for (int i = 0; i < MAX_ID_LENGTH; i++) {
            id[i] = '\0';
        }
        id_index = 0;
    }
    
    uint8_t value_index = 0;
    char value[MAX_VALUE_LENGTH];
    
    void reset_value() {
        for (int i = 0; i < MAX_VALUE_LENGTH; i++) {
            value[i] = '\0';
        }
        value_index = 0;
    }
    
    void process_message() {
        char type = 'N';
        if (msg_type == DATA) type = 'D';
        printf("Received a message: Type=%c, ID=%s, Value=%s\r\n", type, id, value);
        if (cluster_head) {
            generate_sink_message_rdatab((char *)&msg, (char *)&node_uid, (char *)&id, (char *) value);
            print_msg(msg);
            send_message(msg);
        }
    }
    
    ReadState nextState(ReadState currentState, char read_character) {
        switch (currentState) {
            case WAITING_FOR_MESSAGE:
                if (read_character == '>') return READING_MESSAGE_TYPE;
                else return WAITING_FOR_MESSAGE;
                break;
            case READING_MESSAGE_TYPE:
                if (read_character == 'D') {
                    msg_type = DATA;
                    return WAITING_FOR_ID;
                }
                else return WAITING_FOR_MESSAGE;
                break;
            case WAITING_FOR_ID:
                if (read_character == ':') {
                    reset_id();
                    return READING_ID;
                }
                else return WAITING_FOR_MESSAGE;
                break;
            case READING_ID:
                if (read_character == ':') {
                    return WAITING_FOR_VALUE;
                }
                else if (read_character == '$' || read_character == '<' || read_character == 'X' || id_index >= MAX_ID_LENGTH-1) {
                    return WAITING_FOR_MESSAGE;
                }
                else {
                    id[id_index] = read_character;
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
                    return WAITING_FOR_MESSAGE_END;
                }
                else if (read_character == ':' || read_character == '<' || read_character == 'X' || id_index >= MAX_VALUE_LENGTH-1) {
                    return WAITING_FOR_MESSAGE;
                }
                else {
                    value[value_index] = read_character;
                    value_index++;
                    return READING_VALUE;
                }
                break;
            case WAITING_FOR_MESSAGE_END:
                if (read_character == '<' || read_character == 'X') return PROCESS_MESSAGE;
                else return WAITING_FOR_MESSAGE;
                break;
            case PROCESS_MESSAGE:
                process_message();
                return WAITING_FOR_MESSAGE;
                break;
            default:
                return currentState;
        }
    }
    
    while(1)
    {
        // wait for an interrupt
        __WFI();

        if (!usb_uart_USART_ReadIsBusy()){
            state = nextState(state, read_chars[0]);
            
            // read the next one
            usb_uart_USART_Read((uint8_t *)&read_chars,1);
        }
        SERCOM5_USART_Read(&buffer, 10, true);
        if(buffer[0] != '\0'){
            //printf((char*)&buffer); // comment this out if your serial program does a local echo.
            // send the string to the zigbee one char at a time
            for(int i = 0; i < 10 && buffer[i] != '\0'; i++){
                usb_uart_USART_Write((uint8_t*)&buffer[i],1);
            }
        }
        
        // handle blinking the light
        if ((msCount % LED_FLASH_MS) == 0)
        {
          PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14; // literally "toggle"
        }
        
        //Enable for RDATAB
        if ((msCount % MESSAGE_RATE_MS) == 0){
            generate_data_message_rdatab((char *)&msg, (char *)&node_uid, (int) msg_count);
            print_msg(msg);
            send_message(msg);
        }
    }
}
