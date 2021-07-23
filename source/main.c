

/**
 * main.c
 */




#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File
#include "main.h"



// Function Prototypes

//
__interrupt void adc_isr(void);
__interrupt void Qep_timeout_isr(void);



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

    EDIS;

/* ADC freki beaalitas */

    EALLOW;
    SysCtrlRegs.HISPCP.all = ADC_MODCLK;
    EDIS;




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
    /* FLASHBOL FUTASHOZ */
    MemCopy(&RamfuncsLoadStart, &RamfuncsLoadEnd, &RamfuncsRunStart);
    InitFlash();
#endif


    EALLOW; // This is needed to write to EALLOW protected register
    PieVectTable.ADCINT = &adc_isr;
    PieVectTable.EQEP2_INT = &Qep_timeout_isr;

    EDIS;   // This is needed to disable write to EALLOW protected registers


    InitAdc();

    PieCtrlRegs.PIEIER1.bit.INTx6 = 1;  // Enable ADCINT in PIE
    PieCtrlRegs.PIEIER5.bit.INTx2 = 1;  // Enable QEP int
    IER |= M_INT1 | M_INT5;      // Enable CPU Interrupt 1 and CPU Interrupt 5 (QEP)
    EINT; // Enable Global interrupt INTMk
    ERTM;  // Enable Global realtime interrupt DBGM

#ifndef NDEBUG
    ConversionCount = 0;
#endif

    Init_sincos_param_for_calculation();
    adc_config();
    epwm_config();
    QepInit();
    QepGpioInit();

    for (;;)
    {

    }

}



__interrupt void
adc_isr(void)
{

    g_AdcChanel_B = AdcReadValue_Channel_B();
    g_AdcChanel_A = AdcReadValue_Channel_A();

    g_qepCounter = QepReadCounter();

#ifndef NDEBUG
    tarolo_QEP[ConversionCount] = g_qepCounter;
#endif


    adc_zero_crossing_find();

    angles.angle_in_fixed_fine = calculate_atan();

    fine_angle_correction_for_360_degree();

#ifndef NDEBUG
    tarolo_fix[ConversionCount] = _IQtoF(angles.angle_in_fixed_fine);
#endif
    QEP_latch_error_fix_due_to_phaseshift();

    calculate_interpolated_high_res_angle();

    calculate_coarse_angle();

#ifndef NDEBUG
    tarolo[ConversionCount] = angles.angle;
    tarolo_coarse[ConversionCount] = angles.angle_coarse;
#endif

#ifndef NDEBUG

    Voltage1[ConversionCount] = AdcReadValue_Channel_A();
    Voltage2[ConversionCount] = AdcReadValue_Channel_B();

#endif

#ifndef NDEBUG
    if(ConversionCount == 300)
    {
        ConversionCount = 0;
    }
    else
    {
        ConversionCount++;
    }
#endif

    adc_reinit_for_next_measurment();
    return;
}


__interrupt void
Qep_timeout_isr(void)
{
    QEP_reinit_for_next_interrupt();
    return;
}





