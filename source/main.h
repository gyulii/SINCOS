
//
// VÁLTOZÓ DEFINIÁLÁS

Uint16 LoopCount;
Uint16 ConversionCount;
Uint16 Voltage1[1500];
Uint16 Voltage2[1500];

int16 min_value_actual = 2000;
int16 min_value_last = 2000;
int16 min_value_result;



void find_adc_min_value()
{
    /*       MIN megtalalas        */
    min_value_actual = (AdcRegs.ADCRESULT0 >> 4) - 1535;
    if (min_value_actual < min_value_result)
        min_value_result = min_value_actual;

    min_value_last = min_value_actual;
}

void adc_reinit_for_next_measurment()
{
    //
    // Reinitialize for next ADC sequence
    //
    AdcRegs.ADCTRL2.bit.RST_SEQ1 = 1; // Reset SEQ1
    AdcRegs.ADCST.bit.INT_SEQ1_CLR = 1; // Clear INT SEQ1 bit
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
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
    EPwm1Regs.ETSEL.bit.SOCASEL = 4; // Select SOC from from CPMA on upcount
    EPwm1Regs.ETPS.bit.SOCAPRD = 1; // Generate pulse on 1st event
    EPwm1Regs.CMPA.half.CMPA = 0x0080; // Set compare A value
    EPwm1Regs.TBPRD = 0x0FFF; // Set period for ePWM1
    EPwm1Regs.TBCTL.bit.CTRMODE = 0; // count up and start
}


void QepGpioInit(void)
{
   EALLOW;


    GpioCtrlRegs.GPAPUD.bit.GPIO24 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO25 = 0;
    GpioCtrlRegs.GPAPUD.bit.GPIO26 = 0;

    GpioCtrlRegs.GPAQSEL2.bit.GPIO24 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO25 = 0;
    GpioCtrlRegs.GPAQSEL2.bit.GPIO26 = 0;

    GpioCtrlRegs.GPAMUX2.bit.GPIO24 = 1;
    GpioCtrlRegs.GPAMUX2.bit.GPIO25 = 1;
    GpioCtrlRegs.GPAMUX2.bit.GPIO26 = 1;

    EDIS;
}

