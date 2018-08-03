/*
 * uart_tools.h
 *
 * Created: 11/5/2015 11:08:35 PM
 *  Author: dcorey
 */ 

#ifndef UART_TOOLS_H_INCLUDED
#define UART_TOOLS_H_INCLUDED

void UART_computer_init(USART_t* comms_usart, PORT_t* comms_port, ioport_pin_t tx_pin, ioport_pin_t rx_pin);

#endif