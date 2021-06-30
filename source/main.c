

/**
 * main.c
 */

//#define NDEBUG
#define Rollback 0

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

#ifndef NDEBUG




/*Ideiglenes szamlalo QEP teszteleshez  */

int temp_szamlalo = 0;

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

    /*Adc konfiguracio*/
    adc_config();
    /*EPWM konfiguracio*/
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
    #if 0
    //iq maximalis arany tesztfuggveny
    iq_max_arany_teszt();
    #endif

    //Line count
    g_qepCounter = QepReadCounter();

    //SIN és COS beolvasasa
    g_AdcChanel_A = AdcReadValue_Channel_A();
    g_AdcChanel_B = AdcReadValue_Channel_B();

    //Kozepertek megkeresese annak erdekeben, hogy az atan fuggveny helyesen mukodjon
    adc_zero_crossing_find();

    //Fine Resolution Angle Calculation
    angles.angle_in_fixed_fine = calculate_atan();

    //Amennyiben SIN nagyobb, mint 0, akkor 90 fokot (radianban), ha kisebb akkor
    //270 fokot (radianban) adunk az arctan eredmenyehez, hogy a megfelelo felsikon helyezkedjunk el
    if(shifted_channel_A > 0)
    {
        angles.angle_in_fixed_fine = angles.angle_in_fixed_fine + _IQ(1.5707); //(1.5707 rad = 90 fok)
    }
    else
    {
        angles.angle_in_fixed_fine = angles.angle_in_fixed_fine + _IQ(4.7123); //(4.7123 rad = 270 fok)
    }

    //Interpolated High-Resolution Angle Calculation (360 fok radianban)

    angles.angle_in_fixed = (_IQ((g_qepCounter >> 2))) + (_IQrmpy((angles.angle_in_fixed_fine),(_IQ(0.1591549)))); // MAGIC NUMBER -> (1/2*PI)
    angles.angle_in_fixed = _IQrmpy(angles.angle_in_fixed , _IQ(0.3515625));  // MAGIC NUMBER  -> (6.28318/N) * (180/PI)
    angles.angle = _IQtoF(angles.angle_in_fixed);
    angles.angle_coarse = g_qepCounter * 0.08789062; // MAGIC NUMBER  -> (360/4*N)

#ifndef NDEBUG
    tarolo[ConversionCount] = angles.angle;
    tarolo_coarse[ConversionCount] = angles.angle_coarse;
#endif

#if 0



    Voltage1[ConversionCount] = AdcReadValue_Channel_1();
    Voltage2[ConversionCount] = AdcReadValue_Channel_2();

#endif

#ifndef NDEBUG
    if(ConversionCount == 1500)
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
#ifndef NDEBUG
    temp_szamlalo++;
#endif

#if 0
    EQep2Regs.QCLR.bit.UTO = 1; // CLEAR TIMEOUT FLAG
    EQep2Regs.QPOSCMP += 200 ;
    EQep2Regs.QCLR.bit.PCM = 1;         // clear PCM
#endif

    QEP_reinit_for_next_interrupt();
    return;
}





