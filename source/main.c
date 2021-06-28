

/**
 * main.c
 */

//#define NDEBUG
#define Rollback 0




#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File

#include "main.h"

//Arctan fuggvenyhez kell a math.h, valamint PI és Angle a szogszamitashoz
#include <math.h>
#define PI 3.1415926535
double Angle;

double fordulatokszamaproba=0;
double egysegesitett_fordulatokszama=0;
double fordulatszamproba=0;


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
#endif
    }

}

__interrupt void
adc_isr(void)
{


    g_qepCounter = QepReadCounter();

    //Fordulatok szamat szepen szamolja, negativ es pozitiv iranyban is. Problema meg vele,
    //hogy tulcsordul es 0-nal valamiert ket fordulat utan ugrik csak a szamlalo
    //atgondolva helyesen mukodik 0-nal, hisz ott -4096-tol 4096-ig megy a szamlalo es lenyegeben itt azt vizsgaljuk,
    //hogy nulla referencia ponthoz kepest melyik iranyba mennyi kort fordult a tengely

    fordulatokszamaproba=g_qepCounter/4096;


    //Ebben a reszben egyseges reszekre van osztva a tartomany, azaz 0 is egy kort foglal magaba
    if(g_qepCounter>0){
          egysegesitett_fordulatokszama=((g_qepCounter)+2048)/4096;
    }

    if(g_qepCounter<=0){
          egysegesitett_fordulatokszama=((g_qepCounter)-2048)/4096;
    }

    //fordulatszam szamitasa vazlatosan: megmerjuk, hogy egy fordulat mennyi ido alatt megy vegbe sec-ben. Majd egy
    //percet (60 sec) a mert idovel elsoztjuk. Persze lehet orat is vagy amit szeretnenk. Az osztas vegeredmenye lesz
    //az aktualis fordulatszam. Persze ezt csak nagyobb fordulatnal lehet alkalmazni, mert kisebbnel a perióduson belül is
    //mernunk kell




#ifndef NDEBUG

    find_adc_min_value();


    Voltage1[ConversionCount] = readAdcValue_Channel_1(AdcOffset);
    Voltage2[ConversionCount] = readAdcValue_Channel_2(AdcOffset);


    //arctan hasznalatanak modja
    atan2((AdcRegs.ADCRESULT0 >> 4), (AdcRegs.ADCRESULT1 >> 4));

    //szog meghatarozasa
    Angle = (atan2((AdcRegs.ADCRESULT0 >> 4),(AdcRegs.ADCRESULT1 >> 4))*180)/PI;



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
    //egyelore ez a resz nem teljesen vilagos, de hektikusan viselkedik a kod itt. Van amikor tem_szamlalo 1-re valt, de utana
    //ott ragad, maskor nem csinal semmit es vegig 0
    temp_szamlalo++;



    /*
     *
     * IDE JÖHET A KOD
     *
     * */





    EQep2Regs.QCLR.bit.UTO = 1; // CLEAR TIMEOUT FLAG


#if 0
   EQep2Regs.QCLR.bit.IEL = 1;
    //EQep2Regs.QPOSCMP += 200 ;

      // Should be in this order
    //EQep2Regs.QCLR.bit.PCM = 1 ;         // clear PCM
#endif

    EQep2Regs.QCLR.bit.INT = 1; // CLEAR INT FLAG
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP5; // INT A
    return;


}





