//������� ������� ���� ������ � 3.5 ������� (8 + 1 + 1 ���)
//� STM32 ��� ����� ���� ������� RTOR. � Atmega ������ ���, ������� ������� ����� ��������
//��� ��������� ����� 19200 ����������� ������������ ����-��� � 1.75 ��

//#include <stdint.h>
//#define F_CPU  14745600
#define F_CPU  16000000
#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h> 

#define BAUDRATE 9600
#define UBRRVAL ((F_CPU/(BAUDRATE*16UL))-1)
unsigned int ubrr = UBRRVAL;

volatile uint8_t *buffer;
volatile uint8_t timerOverflow, buffer_index;

volatile unsigned char data1[] = "Packet len 12"; //len 13
volatile unsigned char data2[] = "2nd_06"; //len 6
volatile unsigned char data3[] = "Third string 16"; //len 15

int buffer_MAX = 0;

int i, k, temp = 0;
char c;

enum statemachine {
	idle = 0,
	packetSending = 1,
	packetReadyToSend = 2,
	packetReceiving = 3,
	packetReceived = 4,
};

volatile uint8_t isReceiving; //1 - ������� ������ ���� � �� �� �����, ����� �� ���
volatile uint8_t isSending;
volatile uint8_t isPacketEnds; //0 - ����� ��� �����������, 1 - ����� ������, ����-��� ��������� ������ ��������

//volatile enum statemachine usartState;


//ISR ��� �������� �����
ISR (USART_UDRE_vect) {
	//if (isReceiving == 1) return; //���� USART ������� ������� ���� ������, �� �� ��� ����� �� ��������

	isReceiving = 0;

	buffer_index++;			// ����������� ������

	if (buffer_index == buffer_MAX) { // ������ ���� ������? 
	UCSR0B &=~ (1<<UDRIE0); // ��������� ���������� �� ����������� - �������� ���������
	}
	else {
	UDR0 = buffer[buffer_index];	// ����� ������ �� �������. 
}
	
	}

//ISR ��� ��������� �����
ISR (USART_RX_vect) {
	char ReceivedByte;
	ReceivedByte = UDR0;
	UDR0 = ReceivedByte;
	isReceiving = 1;
	isPacketEnds = 0;
}


//ISR �������-0, 8 bit
ISR (TIMER0_OVF_vect) {
	PORTB = PORTB ^ (1<<PB5); // toggle LED
	//if (TCNT0 > 2) {
	//TCNT0 = 0;
	//if (isReceiving == 0) isPacketEnds = 1;}
}


//ISR �������-1, 16 bit
ISR (TIMER1_OVF_vect) {
	PORTB = PORTB ^ (1<<PB5); // toggle LED
	TCNT1 = 0x0ff0; //������������� ��������� �������� ��������
	timerOverflow++;
}

//������������� ������� 1, 16 ���
void timer1Init(void) {
	//TCCR1B = (1<<CS12) | (0<<CS11) | (0<<CS10); // Timer clock = I/O clock / 256
	TCCR1B = (0 << CS12) | (1 << CS11) | (1 << CS10); // Timer clock = I/O clock / 64
	TIFR1 = 1 << TOV1; // Clear overflow flag
	TIMSK1 = 1 << TOIE1; //Enable Overflow Interrupt
}

//������������� ������� 0, 8 ���. ������ ��������� �������� - ���� ��� �� 64 �����
void timer0Init(void) {
	TCCR0B = (0 << CS02) | (1 << CS01) | (1 << CS00); // Timer clock = I/O clock / 64
	TIFR0 = 1 << TOV0; // Clear overflow flag
	TIMSK0 = 1 << TOIE0; //Enable Overflow Interrupt
}


void usartInit (void) {
	UBRR0H = (ubrr>>8);
	UBRR0L = (ubrr);
	UCSR0C = 0x06; // Set frame format: 8data, 1stop bit
	UCSR0B = ((1<<TXEN0) | (1<<RXEN0) | (1<<RXCIE0) | (0<<TXCIE0) | (0<<UDRIE0));
}

/* ��������� ���������� - �� ������, �� ����������� ������ � �� ��������� ��������.
�� ���� ��������� ������ ��� � �� ������ � �� ��������, ����� ��� ������ �� ����� ��
���� �� ���������� UDRIE � ����� �� ���� */

/*
When interrupt-driven data reception is used, the receive complete routine must read the
received data from UDR in order to clear the RXC Flag, otherwise a new interrupt will occur
once the interrupt routine terminates.
That's important to remember - if you are using the RXC interrupt, you must read a byte from 
the UDR register to clear the interrupt flag. We do that in our above code, but keep it in
mind for your future projects!
*/


int main() {
	DDRB  |= (1<<PB5);	//���������� PD4 ��� �����
	PORTB = 255;
		         
	cli();
	timerOverflow = 0;
	usartInit();
	//timer0Init();
	timer1Init();
    //set_sleep_mode(SLEEP_MODE_IDLE);
	sei();

	//while (1) {}


	while(1) {

	if (timerOverflow == 10) { //��������� �������� ����� 10-� ������������ �������
	timerOverflow = 11;
	buffer_MAX = 6;
	buffer_index = 0;		// ���������� ������
	buffer = data2;
	UDR0 = buffer[0];		// ���������� ������ ����
	UCSR0B |= (1<<UDRIE0);	// ��������� ���������� UDRE
	}

	if (timerOverflow == 35) { //��������� �������� ����� 10-� ������������ �������
	//usartState = packetSending;
	timerOverflow = 36;
	buffer_MAX = 13;
	buffer_index = 0;		// ���������� ������
	buffer = data1;
	UDR0 = buffer[0];		// ���������� ������ ����
	UCSR0B |= (1<<UDRIE0);	// ��������� ���������� UDRE
	}

	if (timerOverflow == 60) { //��������� �������� ����� 10-� ������������ �������
	//usartState = packetSending;
	timerOverflow = 0;
	buffer_MAX = 15;
	buffer_index = 0;		// ���������� ������
	buffer = data3;
	UDR0 = buffer[0];		// ���������� ������ ����
	UCSR0B |= (1<<UDRIE0);	// ��������� ���������� UDRE
	}


};


}	
