#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <util/twi.h>
#include <stdint.h>
#include <limits.h>

#include "./co2dumper.h"
#include "./serial.h"
#include "./sysclock.h"


/*
    Ringbuffer utilis
*/
static inline void ringBuffer_Init(volatile struct ringBuffer* lpBuf) {
    lpBuf->dwHead = 0;
    lpBuf->dwTail = 0;
}
static inline bool ringBuffer_Available(volatile struct ringBuffer* lpBuf) {
    return (lpBuf->dwHead != lpBuf->dwTail) ? true : false;
}
static inline bool ringBuffer_Writable(volatile struct ringBuffer* lpBuf) {
    return (((lpBuf->dwHead + 1) % SERIAL_RINGBUFFER_SIZE) != lpBuf->dwTail) ? true : false;
}
static inline unsigned long int ringBuffer_AvailableN(volatile struct ringBuffer* lpBuf) {
    if(lpBuf->dwHead >= lpBuf->dwTail) {
        return lpBuf->dwHead - lpBuf->dwTail;
    } else {
        return (SERIAL_RINGBUFFER_SIZE - lpBuf->dwTail) + lpBuf->dwHead;
    }
}
static inline unsigned long int ringBuffer_WriteableN(volatile struct ringBuffer* lpBuf) {
    return SERIAL_RINGBUFFER_SIZE - ringBuffer_AvailableN(lpBuf);
}

static unsigned char ringBuffer_ReadChar(volatile struct ringBuffer* lpBuf) {
    char t;

    if(lpBuf->dwHead == lpBuf->dwTail) {
        return 0x00;
    }

    t = lpBuf->buffer[lpBuf->dwTail];
    lpBuf->dwTail = (lpBuf->dwTail + 1) % SERIAL_RINGBUFFER_SIZE;

    return t;
}
#if 0
	static unsigned char ringBuffer_PeekChar(volatile struct ringBuffer* lpBuf) {
	    if(lpBuf->dwHead == lpBuf->dwTail) {
	        return 0x00;
	    }

	    return lpBuf->buffer[lpBuf->dwTail];
	}
#endif
static unsigned char ringBuffer_PeekCharN(
    volatile struct ringBuffer* lpBuf,
    unsigned long int dwDistance
) {
    if(lpBuf->dwHead == lpBuf->dwTail) {
        return 0x00;
    }
    if(ringBuffer_AvailableN(lpBuf) <= dwDistance) {
        return 0x00;
    }

    return lpBuf->buffer[(lpBuf->dwTail + dwDistance) % SERIAL_RINGBUFFER_SIZE];
}
static inline void ringBuffer_discardN(
    volatile struct ringBuffer* lpBuf,
    unsigned long int dwCount
) {
    lpBuf->dwTail = (lpBuf->dwTail + dwCount) % SERIAL_RINGBUFFER_SIZE;
    return;
}
#if 0
	static unsigned long int ringBuffer_ReadChars(
	    volatile struct ringBuffer* lpBuf,
	    unsigned char* lpOut,
	    unsigned long int dwLen
	) {
	    char t;
	    unsigned long int i;

	    if(dwLen > ringBuffer_AvailableN(lpBuf)) {
	        return 0;
	    }

	    for(i = 0; i < dwLen; i=i+1) {
	        t = lpBuf->buffer[lpBuf->dwTail];
	        lpBuf->dwTail = (lpBuf->dwTail + 1) % SERIAL_RINGBUFFER_SIZE;
	        lpOut[i] = t;
	    }

	    return i;
	}
#endif

static void ringBuffer_WriteChar(
    volatile struct ringBuffer* lpBuf,
    unsigned char bData
) {
    if(((lpBuf->dwHead + 1) % SERIAL_RINGBUFFER_SIZE) == lpBuf->dwTail) {
        return; /* Simply discard data */
    }

    lpBuf->buffer[lpBuf->dwHead] = bData;
    lpBuf->dwHead = (lpBuf->dwHead + 1) % SERIAL_RINGBUFFER_SIZE;
}
static void ringBuffer_WriteChars(
    volatile struct ringBuffer* lpBuf,
    unsigned char* bData,
    unsigned long int dwLen
) {
    unsigned long int i;

    for(i = 0; i < dwLen; i=i+1) {
        ringBuffer_WriteChar(lpBuf, bData[i]);
    }
}

static void ringBuffer_WriteASCIIUnsignedInt(
    volatile struct ringBuffer* lpBuf,
    uint32_t ui
) {
    uint8_t i;
    char bTemp[10];
    uint8_t len;
    uint32_t current;

    /*
        We perform a simple conversion of the unsigned int
        and push all numbers into the ringbuffer
    */
    if(ui == 0) {
        ringBuffer_WriteChar(lpBuf, 0x30);
    } else {
        current = ui;
        len = 0;
        while(current != 0) {
            bTemp[len] = ((uint8_t)(current % 10)) + 0x30;
            len = len + 1;
            current = current / 10;
        }

        for(i = 0; i < len; i=i+1) {
            ringBuffer_WriteChar(lpBuf, bTemp[len - 1 - i]);
        }
    }
}



/*
	Initialization for the four ports ...
*/
volatile struct ringBuffer serialRB0_TX;
volatile struct ringBuffer serialRB0_RX;

void serialInit0() {
    uint8_t sregOld = SREG;
    #ifndef FRAMAC_SKIP
        cli();
    #endif

    ringBuffer_Init(&serialRB0_TX);
    ringBuffer_Init(&serialRB0_RX);

    UBRR0   = 103; // 16 : 115200, 103: 19200, 212: 9600
    UCSR0A  = 0x02;
    UCSR0B  = 0x10 | 0x80; /* Enable receiver and RX interrupt */
    UCSR0C  = 0x06;

    SREG = sregOld;

    return;
}

#ifdef SERIAL_UART1_ENABLE
	volatile struct ringBuffer serialRB1_TX;
	volatile struct ringBuffer serialRB1_RX;

	void serialInit1() {
	    uint8_t sregOld = SREG;
	    #ifndef FRAMAC_SKIP
	        cli();
	    #endif

	    ringBuffer_Init(&serialRB1_TX);
	    ringBuffer_Init(&serialRB1_RX);

	    UBRR1   = 212; // 16 : 115200, 103: 19200, 212: 9600
	    UCSR1A  = 0x02;
	    UCSR1B  = 0x10 | 0x80; /* Enable receiver and RX interrupt */
	    UCSR1C  = 0x06;

	    SREG = sregOld;

	    return;
	}
#endif

#ifdef SERIAL_UART2_ENABLE
	volatile struct ringBuffer serialRB2_TX;
	volatile struct ringBuffer serialRB2_RX;

	void serialInit2() {
	    uint8_t sregOld = SREG;
	    #ifndef FRAMAC_SKIP
	        cli();
	    #endif

	    ringBuffer_Init(&serialRB2_TX);
	    ringBuffer_Init(&serialRB2_RX);

	    UBRR2   = 212; // 16 : 115200, 103: 19200, 212: 9600
	    UCSR2A  = 0x02;
	    UCSR2B  = 0x10 | 0x80; /* Enable receiver and RX interrupt */
	    UCSR2C  = 0x06;

	    SREG = sregOld;

	    return;
	}
#endif

#ifdef SERIAL_UART3_ENABLE
	volatile struct ringBuffer serialRB3_TX;
	volatile struct ringBuffer serialRB3_RX;

	void serialInit3() {
	    uint8_t sregOld = SREG;
	    #ifndef FRAMAC_SKIP
	        cli();
	    #endif

	    ringBuffer_Init(&serialRB3_TX);
	    ringBuffer_Init(&serialRB3_RX);

	    UBRR3   = 212; // 16 : 115200, 103: 19200, 212: 9600
	    UCSR3A  = 0x02;
	    UCSR3B  = 0x10 | 0x80; /* Enable receiver and RX interrupt */
	    UCSR3C  = 0x06;

	    SREG = sregOld;

	    return;
	}
#endif

void serialTX(unsigned char bPort) {
    /*
        This starts the transmitter ... other than for the RS485 half duplex
        lines it does _not_ interfer with the receiver part and would not
        interfer with an already running transmitter.
    */
    uint8_t sregOld = SREG;
    #ifndef FRAMAC_SKIP
        cli();
    #endif

	switch(bPort) {
		case 0:
		    UCSR0A = UCSR0A | 0x40; /* Reset TXCn bit */
		    UCSR0B = UCSR0B | 0x08 | 0x20;
			break;
		#ifdef SERIAL_UART1_ENABLE
			case 1:
				UCSR1A = UCSR1A | 0x40; /* Reset TXCn bit */
				UCSR1B = UCSR1B | 0x08 | 0x20;
				break;
		#endif
		#ifdef SERIAL_UART2_ENABLE
			case 2:
				UCSR2A = UCSR2A | 0x40; /* Reset TXCn bit */
				UCSR2B = UCSR2B | 0x08 | 0x20;
				break;
		#endif
		#ifdef SERIAL_UART3_ENABLE
			case 3:
				UCSR3A = UCSR3A | 0x40; /* Reset TXCn bit */
				UCSR3B = UCSR3B | 0x08 | 0x20;
				break;
		#endif
		default:
			break;
	}
    #ifndef FRAMAC_SKIP
        SREG = sregOld;
    #endif
}

/*
	Interrupt handler

	These just shovel data into the ringbuffers while receiving and shovel
	data out of the transmit ringbuffer when in transmit mode
*/
ISR(USART0_RX_vect) {
    ringBuffer_WriteChar(&serialRB0_RX, UDR0);
}
ISR(USART0_UDRE_vect) {
    if(ringBuffer_Available(&serialRB0_TX) == true) {
        /* Shift next byte to the outside world ... */
        UDR0 = ringBuffer_ReadChar(&serialRB0_TX);
    } else {
        /*
            Since no more data is available for shifting simply stop
            the transmitter and associated interrupts
        */
        UCSR0B = UCSR0B & (~(0x08 | 0x20));
    }
}

#ifdef SERIAL_UART1_ENABLE
	ISR(USART1_RX_vect) {
	    ringBuffer_WriteChar(&serialRB1_RX, UDR1);
	}
	ISR(USART1_UDRE_vect) {
	    if(ringBuffer_Available(&serialRB1_TX) == true) {
	        /* Shift next byte to the outside world ... */
	        UDR1 = ringBuffer_ReadChar(&serialRB1_TX);
	    } else {
	        /*
	            Since no more data is available for shifting simply stop
	            the transmitter and associated interrupts
	        */
	        UCSR1B = UCSR1B & (~(0x08 | 0x20));
	    }
	}
#endif
#ifdef SERIAL_UART2_ENABLE
	ISR(USART2_RX_vect) {
	    ringBuffer_WriteChar(&serialRB2_RX, UDR1);
	}
	ISR(USART2_UDRE_vect) {
	    if(ringBuffer_Available(&serialRB2_TX) == true) {
	        /* Shift next byte to the outside world ... */
	        UDR2 = ringBuffer_ReadChar(&serialRB2_TX);
	    } else {
	        /*
	            Since no more data is available for shifting simply stop
	            the transmitter and associated interrupts
	        */
	        UCSR2B = UCSR2B & (~(0x08 | 0x20));
	    }
	}
#endif
#ifdef SERIAL_UART3_ENABLE
	ISR(USART3_RX_vect) {
	    ringBuffer_WriteChar(&serialRB3_RX, UDR3);
	}
	ISR(USART3_UDRE_vect) {
	    if(ringBuffer_Available(&serialRB3_TX) == true) {
	        /* Shift next byte to the outside world ... */
	        UDR3 = ringBuffer_ReadChar(&serialRB3_TX);
	    } else {
	        /*
	            Since no more data is available for shifting simply stop
	            the transmitter and associated interrupts
	        */
	        UCSR3B = UCSR3B & (~(0x08 | 0x20));
	    }
	}
#endif

/*
	Synchronous message handler
*/

void handleSerial0Messages() {
	/*
		Currently discard everything ...
	*/
	for(;;) {
		dwAvailableLength = ringBuffer_AvailableN(&serialRB0_RX);
		if(dwAvailableLength == 0) {
			break;
		}
		ringBuffer_discardN(&serialRB1_RX, dwAvailableLength);
	}
}

static bool handleSerialMessage_DecodePotentialMessage(
	unsigned char bChannel,
	unsigned char* lpBuffer
) {
	/* Verify checksum */
	unsigned char bChkSum = 0x00;
	unsigned long int i;

	for(i = 1; i < 8; i=i+1) {
		bChkSum = bChkSum + lpBuffer[i];
	}
	bChkSum = (0xFF - bChkSum) + 0x01;

	if(bChkSum != lpBuffer[8]) {
		/*
			Checksum error ...
			ToDo: Later on do statistics / call callback / etc. ....
		*/
		return false;
	}

	/* Decode value */
	uint16_t valueCO2 = ((((uint16_t)(lpBuffer[2])) << 8) & 0xFF00) | (((uint16_t)(lpBuffer[3])) & 0x00FF);
	signed long int valueTemp = ((signed long int )(lpBuffer[4])) - 40; /* Undocumented feature, usually this is only used for internal temperature compensation and totally unreliable */

	co2ConcentrationReceived(bChannel, valueCO2, valueTemp);

	return true;
}

#ifdef SERIAL_UART1_ENABLE
	void handleSerial1Messages() {
		unsigned char bBuffer[9];
		unsigned long int i;
		unsigned long int dwAvailableLength;

		for(;;) {
			dwAvailableLength = ringBuffer_AvailableN(&serialRB1_RX);
			if(dwAvailableLength < 9) {
				/* We have not received a complete message for sure ... */
				return;
			}

			/* Check if there was a sync pattern ... */
			if(ringBuffer_PeekCharN(&serialRB1_RX, 0) != 0xFF) {
				/* Not the sync char. Discard and retry ... */
				ringBuffer_discardN(&serialRB1_RX, 1);
				continue;
			}

			/*
				We use peek. If we fail to decode the message we just drop
				the first byte and re-synchronize
			*/
			for(i = 0; i < 9; i=i+1) {
				bBuffer[i] = ringBuffer_PeekCharN(&serialRB1_RX, i);
			}
			if(handleSerialMessage_DecodePotentialMessage(1, bBuffer) != true) {
				ringBuffer_discardN(&serialRB1_RX, 1);
				continue;
			}
			/*
				We succeeded decoding so we discard 9 bytes and continue handling
				potentially cached data until we're emtpy
			*/
			ringBuffer_discardN(&serialRB1_RX, 9);
		}
	}
#endif

#ifdef SERIAL_UART2_ENABLE
	void handleSerial2Messages() {
		unsigned char bBuffer[9];
		unsigned long int i;
		unsigned long int dwAvailableLength;

		for(;;) {
			dwAvailableLength = ringBuffer_AvailableN(&serialRB2_RX);
			if(dwAvailableLength < 9) {
				/* We have not received a complete message for sure ... */
				return;
			}

			/* Check if there was a sync pattern ... */
			if(ringBuffer_PeekCharN(&serialRB2_RX, 0) != 0xFF) {
				/* Not the sync char. Discard and retry ... */
				ringBuffer_discardN(&serialRB2_RX, 1);
				continue;
			}

			/*
				We use peek. If we fail to decode the message we just drop
				the first byte and re-synchronize
			*/
			for(i = 0; i < 9; i=i+1) {
				bBuffer[i] = ringBuffer_PeekCharN(&serialRB2_RX, i);
			}
			if(handleSerialMessage_DecodePotentialMessage(1, bBuffer) != true) {
				ringBuffer_discardN(&serialRB2_RX, 1);
				continue;
			}
			/*
				We succeeded decoding so we discard 9 bytes and continue handling
				potentially cached data until we're emtpy
			*/
			ringBuffer_discardN(&serialRB2_RX, 9);
		}
	}
#endif

#ifdef SERIAL_UART3_ENABLE
	void handleSerial3Messages() {
		unsigned char bBuffer[9];
		unsigned long int i;
		unsigned long int dwAvailableLength;

		for(;;) {
			dwAvailableLength = ringBuffer_AvailableN(&serialRB3_RX);
			if(dwAvailableLength < 9) {
				/* We have not received a complete message for sure ... */
				return;
			}

			/* Check if there was a sync pattern ... */
			if(ringBuffer_PeekCharN(&serialRB3_RX, 0) != 0xFF) {
				/* Not the sync char. Discard and retry ... */
				ringBuffer_discardN(&serialRB3_RX, 1);
				continue;
			}

			/*
				We use peek. If we fail to decode the message we just drop
				the first byte and re-synchronize
			*/
			for(i = 0; i < 9; i=i+1) {
				bBuffer[i] = ringBuffer_PeekCharN(&serialRB3_RX, i);
			}
			if(handleSerialMessage_DecodePotentialMessage(1, bBuffer) != true) {
				ringBuffer_discardN(&serialRB3_RX, 1);
				continue;
			}
			/*
				We succeeded decoding so we discard 9 bytes and continue handling
				potentially cached data until we're emtpy
			*/
			ringBuffer_discardN(&serialRB3_RX, 9);
		}
	}
#endif

/*
	Requesting function
*/
static unsigned char serialRequestCO2Values__RequestCommand[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 };

void serialRequestCO2Values(unsigned char bChannel) {
	switch(bChannel) {
		case 0:
			#ifdef SERIAL_UART1_ENABLE
				ringBuffer_WriteChars(&serialRB1_TX, serialRequestCO2Values__RequestCommand, 9);
				serialTX(1);
			#endif
			#ifdef SERIAL_UART2_ENABLE
				ringBuffer_WriteChars(&serialRB2_TX, serialRequestCO2Values__RequestCommand, 9);
				serialTX(2);
			#endif
			#ifdef SERIAL_UART3_ENABLE
				ringBuffer_WriteChars(&serialRB3_TX, serialRequestCO2Values__RequestCommand, 9);
				serialTX(3);
			#endif
			break;
		#ifdef SERIAL_UART1_ENABLE
			case 1:
				ringBuffer_WriteChars(&serialRB1_TX, serialRequestCO2Values__RequestCommand, 9);
				serialTX(1);
				break;
		#endif
		#ifdef SERIAL_UART2_ENABLE
			case 2:
				ringBuffer_WriteChars(&serialRB2_TX, serialRequestCO2Values__RequestCommand, 9);
				serialTX(2);
				break;
		#endif
		#ifdef SERIAL_UART3_ENABLE
			case 3:
				ringBuffer_WriteChars(&serialRB3_TX, serialRequestCO2Values__RequestCommand, 9);
				serialTX(3);
				break;
		#endif
	}
}

void serialReportSensorData(
	uint16_t* lpCO2Values,
	signed long int* lpTempValues,
	unsigned long int dwSensorCount
) {
	unsigned long int i;

	for(i = 0; i < dwSensorCount; i=i+1) {
		ringBuffer_WriteASCIIUnsignedInt(&serialRB0_TX, (uint32_t)lpCO2Values[i]);
		ringBuffer_WriteChar(&serialRB0_TX, ' ');
		if(lpTempValues[i] >= 0) {
			ringBuffer_WriteASCIIUnsignedInt(&serialRB0_TX, (uint32_t)lpTempValues[i]);
		} else {
			ringBuffer_WriteChar(&serialRB0_TX, '-');
			ringBuffer_WriteASCIIUnsignedInt(&serialRB0_TX, (uint32_t)(0 - lpTempValues[i]));
		}

		if(i != (dwSensorCount - 1)) {
			ringBuffer_WriteChar(&serialRB0_TX, ' ');
		} else {
			ringBuffer_WriteChar(&serialRB0_TX, '\n');
		}
	}
	serialTX(0);
}
