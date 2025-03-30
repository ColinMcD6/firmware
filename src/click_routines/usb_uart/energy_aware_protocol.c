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
#include "message_processing.h"
#include "election_utils.h"
#include "state_management.h"

// setup our heartbeat to be 1ms: we overflow at 1ms intervals with a 48MHz clock
// uses the SysTicks unit so that we get reliable debugging (timer stops on breakpoints)
// this is a countdown timer that sends an interrupt when 0
#define MS_TICKS 48000UL

// number of millisecond between LED flashes
#define LED_FLASH_MS    1000UL

// NOTE: this overflows every ~50 days, so I'm not going to care here...
volatile uint32_t msCount = 0;

// For syncing elections
volatile uint32_t election_clock = 0;

// Node Parameters and variables
#define PAIRS 1
#define ENERGY 2000 // total number of messages the node can send before it dies
char node_uid[] = "P2"; //2
uint32_t msg_count = 0;
uint8_t cluster_head = 1;
double random_dbl = 1.0;

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

ReadState read_state = WAITING_FOR_MESSAGE;
PhaseState phase_state = DATA_COLLECTION;

uint8_t msg[MSG_LENGTH+1];
uint8_t read_chars[100];

Message message;

// Fires every 1ms
void SysTick_Handler()
{
  msCount++;
  election_clock++;
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
    
void process_message() {
    char type = 'N';
    if (message.message_type == DATA) type = 'D';
        printf("Received a message: Type=%c, ID=%s, Value=%s\r\n", type, message.node_id, message.value);
        if (cluster_head) {
            generate_sink_message_rdatab((char *)&msg, (char *)&node_uid, (char *) &(message.node_id), (char *) &(message.value));
            print_msg(msg);
            send_message(msg);
        }
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
    
    uint8_t buffer[10];
    
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

    while(1)
    {
        // wait for an interrupt
        __WFI();

        if (!usb_uart_USART_ReadIsBusy()){
            printf("%c", read_chars[0]);
            read_state = next_read_state(read_state, read_chars[0]);
            if (read_state == MESSAGE_READY) {
                printf("Message is ready!\r\n");
                get_message(&message);
                process_message();
                reset_message();
            }
            
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
            if (cluster_head) { //Send data directly to the sink if node is a cluster head
                char value[20];
                snprintf(value, 20, "%ld", msg_count);
                generate_sink_message_rdatab((char *)&msg, (char *)&node_uid, (char *)&node_uid, (char *) value);
                print_msg(msg);
                send_message(msg);
            } else {
                generate_data_message_rdatab((char *)&msg, (char *)&node_uid, (int) msg_count);
                print_msg(msg);
                send_message(msg);
            }
        }
        
        phase_state = next_phase_state(phase_state, election_clock);
        
        if (phase_state == CLUSTER_HEAD_SELECTION) {
            double threshold = get_threshold(0.5);
            random_dbl = get_random_double();
            if (random_dbl < threshold) {
                printf("I should be cluster head!\r\n");
                generate_cluster_head_message_rdatab((char *)&msg, (char *)&node_uid, random_dbl);
                cluster_head = 1;
                print_msg(msg);
                send_message(msg);
            }
            else {
                printf("I should not be cluster head!\r\n");
                cluster_head = 0;
            }
        }
    }
}
