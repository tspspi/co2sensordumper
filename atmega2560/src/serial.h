#ifndef __is_included__52cc86fd_dacc_11ec_8370_70f3950389a2
#define __is_included__52cc86fd_dacc_11ec_8370_70f3950389a2 1

#ifndef __cplusplus
    #ifndef true
        #define true 1
        #define false 0
        typedef unsigned char bool;
    #endif
#endif

#ifndef SERIAL_RINGBUFFER_SIZE
    #define SERIAL_RINGBUFFER_SIZE 512
#endif

struct ringBuffer {
    volatile unsigned long int dwHead;
    volatile unsigned long int dwTail;

    volatile unsigned char buffer[SERIAL_RINGBUFFER_SIZE];
};

void serialInit0();
#ifdef SERIAL_UART1_ENABLE
	void serialInit1();
#endif
#ifdef SERIAL_UART2_ENABLE
	void serialInit2();
#endif
#ifdef SERIAL_UART3_ENABLE
	void serialInit3();
#endif

void handleSerial0Messages();
void handleSerial1Messages();
void handleSerial2Messages();
void handleSerial3Messages();

void serialRequestCO2Values(unsigned char bChannel);

void serialReportSensorData(
	uint16_t* lpCO2Values,
	signed long int* lpTempValues,
	unsigned long int dwSensorCount
);

#endif /* __is_included__fd8eb7f9_df0f_11eb_ba7e_b499badf00a1 */
