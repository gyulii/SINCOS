
//
// VÁLTOZÓ DEFINIÁLÁS

volatile Uint16  LoopCount;
volatile Uint16  ConversionCount;
volatile Uint16  Voltage1[1500];
volatile Uint16  Voltage2[1500];





/*    //Arctan fuggvenyhez kell a math.h, valamint PI és Angle a szogszamitashoz    */



#include <math.h>
#define PI 3.1415926535


double fordulatokszamaproba=0;
double egysegesitett_fordulatokszama=0;
double fordulatszamproba=0;



/*ADC offset konstans -> also ertek legyen 0, tapasztalat alapján valsztottam  */

#define AdcOffset -1535

/* QEP Globals */

volatile int32  g_qepCounter;

/*   ADC Globals       */

volatile Uint16 g_AdcChanel_A;
volatile Uint16 g_AdcChanel_B;


volatile Uint16  g_min_value_actual = 50000;
volatile Uint16  g_min_value_result = 55000;

volatile Uint16  g_max_value_actual = 20000;
volatile Uint16  g_max_value_result = 25000;

volatile Uint16 g_adc_avg;



/*Angle calculation */


volatile float angle;



void adc_reinit_for_next_measurment()
{
    //
    // Reinitialize for next ADC sequence
    //
    AdcRegs.ADCTRL2.bit.RST_SEQ1 = 1; // Reset SEQ1
    AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1; // Clear INT SEQ1 bit
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

int AdcReadValue_Channel_1 (void)
{
    return (AdcRegs.ADCRESULT0);
}

int AdcReadValue_Channel_2 (void)
{
   return (AdcRegs.ADCRESULT1);
}

void adc_config()
{
    //
    // Configure ADC
    //
    AdcRegs.ADCTRL3.bit.SMODE_SEL = 0x1;
    AdcRegs.ADCMAXCONV.all = 0x0001; // Setup 2 conv's on SEQ1
    AdcRegs.ADCCHSELSEQ1.bit.CONV00 = 0x6; // Setup ADCINA3 as 1st SEQ1 conv.
    //
    // Enable SOCA from ePWM to start SEQ1
    //
    //
    AdcRegs.ADCTRL2.bit.EPWM_SOCA_SEQ1 = 1;
    AdcRegs.ADCTRL2.bit.INT_ENA_SEQ1 = 1; // Enable SEQ1 interrupt (every EOS)
}


void epwm_config()
{
    /******************/
    // Assumes ePWM1 clock is already enabled in InitSysCtrl();
    //
    EPwm1Regs.ETSEL.bit.SOCAEN = 1; // Enable SOC on A group
    EPwm1Regs.ETSEL.bit.SOCASEL = 2; // Enable event time-base counter equal to period
    EPwm1Regs.ETPS.bit.SOCAPRD = 1; // Generate pulse on 1st event

    EPwm1Regs.TBPRD = 3000; // Set period for ePWM1 -> 100kHz Sample
    EPwm1Regs.TBCTL.bit.CTRMODE = 0; // count up and start
}


void QepGpioInit(void)
{

   EALLOW;

   /* Pull up*/
    GpioCtrlRegs.GPAPUD.bit.GPIO24 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO25 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO26 = 0;
    /* Clk */
    GpioCtrlRegs.GPAQSEL2.bit.GPIO24 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO25 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO26 = 0;
    /*  Funkcio */
    GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 2;
    GpioCtrlRegs.GPAMUX2.bit.GPIO25 = 2;
    GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 2;

    EDIS;
}

void find_adc_min_value()
{
    /*       MIN megtalalas        */
    g_min_value_actual = AdcReadValue_Channel_1();
    if (g_min_value_actual < g_min_value_result)
    {
        g_min_value_result = g_min_value_actual;
    }
}

void find_adc_max_value()
{
    /*       Max megtalalas        */
    g_max_value_actual = AdcReadValue_Channel_1();
    if (g_max_value_actual > g_max_value_result)
    {
        g_max_value_result = g_max_value_actual;
    }
}

Uint16 find_adc_avg()
{
    int32 tmp_max = g_max_value_result;
    int32 tmp_min = g_min_value_result;
    int32 temp_valtozo = tmp_max + tmp_min;
    temp_valtozo = temp_valtozo / 2;
    return temp_valtozo;
}


void QepInit(void)
{

# if 1
    EQep2Regs.QPOSCNT = 78; // testest init
    EQep2Regs.QPOSCMP = 500;
#endif


    EQep2Regs.QUPRD=150000000;    //Unit timer period, clk -> Sysclock


    EQep2Regs.QDECCTL.bit.QSRC=0;    //quadrature mode

    EQep2Regs.QEPCTL.bit.FREE_SOFT=2; // emulation kikapcs

    EQep2Regs.QEPCTL.bit.PCRM=0; // Reset on COMP R

    EQep2Regs.QEPCTL.bit.UTE=1; // Unit timer enable

    EQep2Regs.QEPCTL.bit.QCLM=1; // Position counter (QPOSLAT), capture timer (QCTMRLAT)  and capture period (QCPRDLAT) values are latched ON TIMEOUT
    /* TO DO PERIOD */
# if 0
    EQep2Regs.QEPCTL.bit.IEL = 3;
#endif
    EQep2Regs.QPOSMAX=0xffffffff;


    EQep2Regs.QEPCTL.bit.QPEN=1; // eQEP position counter is enabled


    EQep2Regs.QEINT.bit.IEL = 1; // INDEX EVENT INT ENABLE

#if 0
    EQep2Regs.QEINT.bit.UTO = 1; // TIMEOUT TIMER INT ENABLE

#endif

    /*TO DO meg tesztelni kell a megfelelo ertekeket*/
    EQep2Regs.QCAPCTL.bit.UPPS=5;       // 1/32 alacsony sebeseghez jo lehet
    EQep2Regs.QCAPCTL.bit.CCPS=7;       // SYS/ 2exp7

    EQep2Regs.QCAPCTL.bit.CEN=1;

# if 0
    EQep2Regs.QPOSCTL.bit.PCE = 1 ; // enable position compare unit
    EQep2Regs.QEINT.bit.PCM = 1 ;   // enable pos compare interrupt
#endif

}

int QepReadDir(void)
{
    return EQep2Regs.QEPSTS.bit.QDF;
}

int QepReadCounter(void)
{
    return EQep2Regs.QPOSCNT;
}





