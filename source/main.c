

/**
 * main.c
 */

#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File


// Function Prototypes
//
__interrupt void adc_isr(void);

//
// Globals
//
Uint16 LoopCount;
Uint16 ConversionCount;
Uint16 Voltage1[1500];
Uint16 Voltage2[1500];


int16 min_value_actual = 2000;
int16 min_value_last = 2000;
int16 min_value_result;


int main(void)
{
    InitSysCtrl();



}
