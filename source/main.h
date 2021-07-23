
/* Hasznos link a szogszamitas matematikai levezetesenek megertesehez:
https://www.ti.com/lit/ug/tidua05a/tidua05a.pdf?ts=1611819649961&ref_url=https%253A%252F%252Fwww.google.com%252F

Link az iqmath leirasahoz:
https://www.ti.com/lit/ug/sprugg9/sprugg9.pdf?ts=1624959457335&ref_url=https%253A%252F%252Fwww.google.com%252F  */

//#define NDEBUG

#ifndef NDEBUG
volatile Uint16  ConversionCount;
#endif

#ifndef NDEBUG
volatile Uint16  Voltage1[300];
volatile Uint16  Voltage2[300];
#endif

#ifndef NDEBUG
float tarolo[300];
float tarolo_coarse[300];
float tarolo_QEP[300];
float tarolo_fix[300];
#endif

#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File
#include <math.h>

#define MATH_TYPE  IQ_MATH
#define GLOBAL_Q 19

#include "IQmathLib.h"



typedef struct sincos_param_for_calculation
{
    volatile float heid_fine;
    volatile float heid_coarse;
    volatile float yama_fine;
    volatile float yama_coarse;



}sincos_param_for_calculation_t;

volatile sincos_param_for_calculation_t g_sincos_param_for_calculation_values;

void Init_sincos_param_for_calculation()
{
    g_sincos_param_for_calculation_values.heid_fine = 0.1757812;
    g_sincos_param_for_calculation_values.heid_coarse = 0.043945312;
    g_sincos_param_for_calculation_values.yama_fine = 0.3515625;
    g_sincos_param_for_calculation_values.yama_coarse = 0.08789062;
}

/*Angle calculation */

typedef struct angles
{
    volatile _iq angle_in_fixed_fine;
    volatile _iq angle_in_fixed;
    volatile float angle_coarse; // Coarse Resolution Angle from QEP
    volatile float angle; // Interpolated High-Resolution Angle Calculation from QEP and ADC
    volatile float angle_fine_quadrant;
}angle_t;

/* Global angle struct declare */

volatile angle_t angles;

/* QEP Globals */

volatile int32  g_qepCounter;

/*   ADC Globals       */

volatile Uint16 g_AdcChanel_A;
volatile Uint16 g_AdcChanel_B;
volatile Uint16 g_adc_avg;
volatile int shifted_channel_A;
volatile int shifted_channel_B;

/*ADC average calibration Globals*/

volatile Uint16  g_min_value_actual = 50000;
volatile Uint16  g_min_value_result = 55000;
volatile Uint16  g_max_value_actual = 500;
volatile Uint16  g_max_value_result = 1000;

/* EPWM*/

void epwm_config()
{
    // Assumes ePWM1 clock is already enabled in InitSysCtrl();
    EPwm1Regs.ETSEL.bit.SOCAEN = 1; // Enable SOC on A group
    EPwm1Regs.ETSEL.bit.SOCASEL = 2; // Enable event time-base counter equal to period
    EPwm1Regs.ETPS.bit.SOCAPRD = 1; // Generate pulse on 1st event
    EPwm1Regs.TBPRD = 6000; // Set period for ePWM1 -> 50 kHz Sample
    EPwm1Regs.TBCTL.bit.CTRMODE = 0; // count up and start
}

/*QEP*/

void QepGpioInit(void)
{
   EALLOW;
   /* Pull up*/
    GpioCtrlRegs.GPAPUD.bit.GPIO24 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO25 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO26 = 0;
    /* Clk enable */
    GpioCtrlRegs.GPAQSEL2.bit.GPIO24 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO25 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO26 = 0;
    /*  Function define */
    GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 2;
    GpioCtrlRegs.GPAMUX2.bit.GPIO25 = 2;
    GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 2;
    EDIS;
}

void QepInit(void)
{
    EQep2Regs.QUPRD=150000000;    //Unit timer period, clk -> Sysclock
    EQep2Regs.QDECCTL.bit.QSRC=0;    //quadrature mode
    EQep2Regs.QEPCTL.bit.FREE_SOFT=2; // emulation kikapcsolasa
    EQep2Regs.QEPCTL.bit.PCRM=0; // Reset on COMP R
    EQep2Regs.QEPCTL.bit.UTE=1; // Unit timer enable
    EQep2Regs.QEPCTL.bit.QCLM=1; // Position counter (QPOSLAT), capture timer (QCTMRLAT)  and capture period (QCPRDLAT) values are latched ON TIMEOUT
    EQep2Regs.QPOSMAX=0xffffffff;
    EQep2Regs.QEPCTL.bit.QPEN=1; // eQEP position counter is enabled
    EQep2Regs.QEINT.bit.IEL = 1; // INDEX EVENT INT ENABLE
}

int QepReadDir(void)
{
    return EQep2Regs.QEPSTS.bit.QDF;
}

int QepReadCounter(void)
{
    return EQep2Regs.QPOSCNT;
}

void QEP_reinit_for_next_interrupt()
{
    EQep2Regs.QCLR.bit.IEL = 1; // Index INT FLAG
    EQep2Regs.QCLR.bit.INT = 1; // CLEAR INT FLAG
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP5; // INT A
}

/*ADC*/

void adc_config()
{
    AdcRegs.ADCTRL3.bit.SMODE_SEL = 0x1;
    AdcRegs.ADCMAXCONV.all = 0x0001; // Setup 2 conv's on SEQ1
    AdcRegs.ADCCHSELSEQ1.bit.CONV00 = 0x6; // Setup ADCINA3 as 1st SEQ1 conv.
    AdcRegs.ADCTRL2.bit.EPWM_SOCA_SEQ1 = 1;    // Enable SOCA from ePWM to start SEQ1
    AdcRegs.ADCTRL2.bit.INT_ENA_SEQ1 = 1; // Enable SEQ1 interrupt (every EOS)
}

int AdcReadValue_Channel_A (void)
{
    return (AdcMirror.ADCRESULT0);
}

int AdcReadValue_Channel_B (void)
{
   return (AdcMirror.ADCRESULT1);
}

void adc_reinit_for_next_measurment()
{
    AdcRegs.ADCTRL2.bit.RST_SEQ1 = 1; // Reset SEQ1
    AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1; // Clear INT SEQ1 bit
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

/*SinCos arithmetics*/

void find_adc_MIN_value_and_store_it_in_global_variable()
{
    g_min_value_actual = AdcReadValue_Channel_A();
    if (g_min_value_actual < g_min_value_result)
    {
        g_min_value_result = g_min_value_actual;
    }
}

void find_adc_MAX_value_and_store_it_in_global_variable()
{
    g_max_value_actual = AdcReadValue_Channel_A();
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
    temp_valtozo = temp_valtozo/2;
    return temp_valtozo;
}

void adc_zero_crossing_find()
{
    find_adc_MIN_value_and_store_it_in_global_variable();
    find_adc_MAX_value_and_store_it_in_global_variable();
    g_adc_avg = find_adc_avg();
}

void fine_angle_correction_for_360_degree()
{
    if (shifted_channel_A > 0)
    {
        angles.angle_in_fixed_fine = angles.angle_in_fixed_fine + _IQ(1.5707); //(1.5707 rad = 90 fok)
    }
    else
    {
        angles.angle_in_fixed_fine = angles.angle_in_fixed_fine + _IQ(4.7123); //(4.7123 rad = 270 fok)
    }
}

_iq calculate_atan()
{
    shifted_channel_A = g_AdcChanel_A - g_adc_avg; //kozepertek 0-ba tolasa
    shifted_channel_B = g_AdcChanel_B - g_adc_avg; //kozepertek 0-ba tolasa

    _iq res = _IQdiv((_IQ(shifted_channel_B)),_IQ(shifted_channel_A));
    return _IQatan(res);
}

void QEP_latch_error_fix_due_to_phaseshift()
{
    //FINE IDENTIFICATION
    angles.angle_fine_quadrant = _IQtoF(
            (_IQdiv(angles.angle_in_fixed_fine,_IQ(6.283185307))));
    if (angles.angle_fine_quadrant < 0.25)
    {
        if (g_qepCounter % 4 == 3)
            g_qepCounter++;
    }
    if (angles.angle_fine_quadrant > 0.75)
    {
        if (g_qepCounter % 4 == 0)
            g_qepCounter--;
    }
}

void calculate_interpolated_high_res_angle()
{
    angles.angle_in_fixed = (_IQ((g_qepCounter >> 2)))
            + (_IQdiv((angles.angle_in_fixed_fine), (_IQ(6.2831853)))); // MAGIC NUMBER -> (1/(2*PI)
    angles.angle_in_fixed = _IQrmpy(angles.angle_in_fixed, _IQ(g_sincos_param_for_calculation_values.heid_fine)); // MAGIC NUMBER  -> (6.28318/N) * (180/PI)
    angles.angle = _IQtoF(angles.angle_in_fixed);
}

void calculate_coarse_angle()
{
    angles.angle_coarse = g_qepCounter * g_sincos_param_for_calculation_values.heid_coarse; // MAGIC NUMBER  -> (360/4*N)
}
