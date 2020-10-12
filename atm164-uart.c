//Таймаут посылки надо делать в 3.5 символа (8 + 1 + 1 бит)
//У STM32 для этого есть регистр RTOR. У Atmega такого нет, поэтому считать будет таймером
//Для скоростей более 19200 допускается использовать тайм-аут в 1.75 мс

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

volatile uint8_t isReceiving; //1 - недавно пришёл байт и мы не знаем, будут ли ещё
volatile uint8_t isSending;
volatile uint8_t isPacketEnds; //0 - пакет ещё принимается, 1 - пакет принят, тайм-аут окончания пакета сработал

//volatile enum statemachine usartState;


//ISR при отправке байта
ISR (USART_UDRE_vect) {
	//if (isReceiving == 1) return; //если USART недавно получил байт данных, то он его сразу же отправил

	isReceiving = 0;

	buffer_index++;			// Увеличиваем индекс

	if (buffer_index == buffer_MAX) { // Вывели весь буффер? 
	UCSR0B &=~ (1<<UDRIE0); // Запрещаем прерывание по опустошению - передача закончена
	}
	else {
	UDR0 = buffer[buffer_index];	// Берем данные из буффера. 
}
	
	}

//ISR при получении байта
ISR (USART_RX_vect) {
	char ReceivedByte;
	ReceivedByte = UDR0;
	UDR0 = ReceivedByte;
	isReceiving = 1;
	isPacketEnds = 0;
}


//ISR таймера-0, 8 bit
ISR (TIMER0_OVF_vect) {
	PORTB = PORTB ^ (1<<PB5); // toggle LED
	//if (TCNT0 > 2) {
	//TCNT0 = 0;
	//if (isReceiving == 0) isPacketEnds = 1;}
}


//ISR таймера-1, 16 bit
ISR (TIMER1_OVF_vect) {
	PORTB = PORTB ^ (1<<PB5); // toggle LED
	TCNT1 = 0x0ff0; //устанавливаем начальное значение счётчика
	timerOverflow++;
}

//инициализация таймера 1, 16 бит
void timer1Init(void) {
	//TCCR1B = (1<<CS12) | (0<<CS11) | (0<<CS10); // Timer clock = I/O clock / 256
	TCCR1B = (0 << CS12) | (1 << CS11) | (1 << CS10); // Timer clock = I/O clock / 64
	TIFR1 = 1 << TOV1; // Clear overflow flag
	TIMSK1 = 1 << TOIE1; //Enable Overflow Interrupt
}

//инициализация таймера 0, 8 бит. Таймер окончания передачи - один раз за 64 такта
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

/* Разрешаем прерывания - по приему, по опустошению буфера и по окончанию передачи.
Но пока разрешаем только два — по приему и по передаче, иначе при старте мы сразу же
уйдём на обработчик UDRIE — буфер то пуст */

/*
When interrupt-driven data reception is used, the receive complete routine must read the
received data from UDR in order to clear the RXC Flag, otherwise a new interrupt will occur
once the interrupt routine terminates.
That's important to remember - if you are using the RXC interrupt, you must read a byte from 
the UDR register to clear the interrupt flag. We do that in our above code, but keep it in
mind for your future projects!
*/


int main() {
	DDRB  |= (1<<PB5);	//конфигурим PD4 как выход
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

	if (timerOverflow == 10) { //запускаем передачу после 10-и переполнений таймера
	timerOverflow = 11;
	buffer_MAX = 6;
	buffer_index = 0;		// Сбрасываем индекс
	buffer = data2;
	UDR0 = buffer[0];		// Отправляем первый байт
	UCSR0B |= (1<<UDRIE0);	// Разрешаем прерывание UDRE
	}

	if (timerOverflow == 35) { //запускаем передачу после 10-и переполнений таймера
	//usartState = packetSending;
	timerOverflow = 36;
	buffer_MAX = 13;
	buffer_index = 0;		// Сбрасываем индекс
	buffer = data1;
	UDR0 = buffer[0];		// Отправляем первый байт
	UCSR0B |= (1<<UDRIE0);	// Разрешаем прерывание UDRE
	}

	if (timerOverflow == 60) { //запускаем передачу после 10-и переполнений таймера
	//usartState = packetSending;
	timerOverflow = 0;
	buffer_MAX = 15;
	buffer_index = 0;		// Сбрасываем индекс
	buffer = data3;
	UDR0 = buffer[0];		// Отправляем первый байт
	UCSR0B |= (1<<UDRIE0);	// Разрешаем прерывание UDRE
	}


};


}	
