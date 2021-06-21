

/**
 * main.c
 */

//#define NDEBUG

#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File

#include "main.h"

//Arctan fuggvenyhez kell
#include <math.h>

// Function Prototypes
//
__interrupt void adc_isr(void);


#if 0
/* Flashbol futashoz */
extern Uint16 RamfuncsLoadStart;
extern Uint16 RamfuncsLoadEnd;
extern Uint16 RamfuncsRunStart;

#endif



int main(void)
{
    InitSysCtrl();


    /*CPU freki beallitas*/

    EALLOW;
#if (CPU_FRQ_150MHZ)     // Default - 150 MHz SYSCLKOUT
    //
    // HSPCLK = SYSCLKOUT/2*ADC_MODCLK2 = 150/(2*3)   = 25.0 MHz
    //
#define ADC_MODCLK 0x3
#endif
#if (CPU_FRQ_100MHZ)
    //
    // HSPCLK = SYSCLKOUT/2*ADC_MODCLK2 = 100/(2*2)   = 25.0 MHz
    //
#define ADC_MODCLK 0x2
#endif
    EDIS;

/* ADC freki beaalitas */

    EALLOW;
    SysCtrlRegs.HISPCP.all = ADC_MODCLK;
    EDIS;



#if 0
     InitGpio();
#endif


    DINT;     // Disable CPU interrupts

    /*
     Initialize the PIE control registers to their default state.
     The default state is all PIE interrupts disabled and flags
     are cleared.
    */

    InitPieCtrl();

    // Disable CPU interrupts and clear all CPU interrupt flags:

    IER = 0x0000;
    IFR = 0x0000;

    /*
     Initialize the PIE vector table with pointers to the shell Interrupt
     Service Routines (ISR).
     This will populate the entire table, even if the interrupt
     is not used in this example.  This is useful for debug purposes.
     The shell ISR routines are found in DSP2833x_DefaultIsr.c.
     This function is found in DSP2833x_PieVect.c.
    */

    InitPieVectTable();

#if 0
    /* FLASHBOL FUTÁSHOZ */
    MemCopy(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);
    InitFlash();
#endif


    EALLOW; // This is needed to write to EALLOW protected register
    PieVectTable.ADCINT = &adc_isr;
    EDIS;   // This is needed to disable write to EALLOW protected registers


    InitAdc();

    PieCtrlRegs.PIEIER1.bit.INTx6 = 1;  // Enable ADCINT in PIE
    IER |= M_INT1;      // Enable CPU Interrupt 1
    EINT; // Enable Global interrupt INTMk
    ERTM;  // Enable Global realtime interrupt DBGM


    LoopCount = 0;
    ConversionCount = 0;

    /*Adc konfiguracio*/

    adc_config();


    /*EPWM konfiguracio*/

    epwm_config();

    /* TO DO -> SAMPLING freki kalibralasa*/

    QepInit();
    QepGpioInit();




    for (;;)
    {
#ifndef NDEBUG
        LoopCount++;
    }
#endif
}

__interrupt void
adc_isr(void)
{


    g_qepCounter = QepReadCounter();

#ifndef NDEBUG

    find_adc_min_value();


    Voltage1[ConversionCount] = readAdcValue_Channel_1(-1535);
    Voltage2[ConversionCount] = readAdcValue_Channel_2(-1535);

    atan2((AdcRegs.ADCRESULT0 >> 4), (AdcRegs.ADCRESULT1 >> 4));

    if(ConversionCount == 1500)
    {
        ConversionCount = 0;
    }
    else
    {
        ConversionCount++;
    }


#endif
//Test
//Gyula szerint

    adc_reinit_for_next_measurment();
    
    return;
}







