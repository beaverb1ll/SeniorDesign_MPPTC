#include <stdio.h>

#define PM_PIN__DO 9 // digital output
#define BUCK_PIN__PWM  "P8_13.16"  // pwm
#define BOOST_PIN__PWM "P9_14.18" // pwm
#define V_IN_SENSE__ADC 12 // ADC Input
#define V_BATT_SENSE__ADC 13  // ADC Input
#define CRNT_SENSE_IN__ADC 14 // ADC Input
#define CRNT_SENSE_OUT__ADC 15 // ADC Input

#define PWM_PERIOD 9000 // in ??
#define VB100 12.65     // in volts
#define VREF 13.5       // in volts

#define TRUE 1
#define FALSE 0

#define ON 1
#define OFF 0


void setDutyCyclePercentForOutput(int percent, const char *pin);
void setOutputForDigitalPin(int aState, int pin);

int buckPWM, bootPWM, panelModeState;

int main(void)
{
    /* 
    Notes:
      Vref=13.5V in charge mode; Vb100=12.65V 100% charged, Vb75=12.45V 75% charged, Vb50=12.24V 50% charge
      d_buck_start and d_boost_start are duty cycle for starte up
    */

    double panelVoltage = -1, vBattPanelOff = -1, vBattPanelOn = -1;

    /*start up mode*/
    //set BUCK_PIN and BOOST_PIN to LOW
    //read in V_IN_SENSE
    //read in V_BATT_SENSE
    //if V_BATT_SENSE is equal or greater to Vb100 stop chargeing go into idle mode
    //if V_IN_SENSE is equal to Vref, go into panel mode
    //if panel voltage is higher than Vref, go into buck mode, send d_buck_start = Vref/V_IN_SENSE
    //if panel voltage is lower than Vref, go into boost mode, send d_boost_start = V_IN_SENSE/Vref

    while(TRUE) 
    {
        /*charge mode*/
        ////sense panel voltage

        //set BUCK_PIN and BOOST_PIN to LOW
        setDutyCyclePercentForOutput(0, BUCK_PIN__PWM);
        setDutyCyclePercentForOutput(0, BOOST_PIN__PWM);

        //read in V_IN_SENSE
        panelVoltage = getVoltageforInput(V_IN_SENSE__ADC); 
        //read in V_BATT_SENSE
        vBattPanelOff = getVoltageforInput(V_BATT_SENSE__ADC);

        //if V_BATT_SENSE is equal or greater to Vb100 stop charging go into idle mode
        if (vBattPanelOff >= VB100)
        {
            // fully charged, so shutdown all charging
            idleMode();
        }
        else
        {
            // restore previous duty cycle because it was correct
            setDutyCyclePercentForOutput(buckPWM, BUCK_PIN__PWM);
            setDutyCyclePercentForOutput(boostPWM, BOOST_PIN__PWM);

            vBattPanelOn = getVoltageforInput(V_BATT_SENSE__ADC);

            //if V_IN_SENSE is equal to Vref, go into panel mode
            if (panelVoltage == vREF)
            {
                panelMode();

            } else if(panelVoltage > VREF) 
            {
                buckMode(vBattPanelOn);

            } else if(panelVoltage < VREF) 
            {
                boostMode(vBattPanelOn);

            } else 
            {
                syslog(LOG_INFO, "ERROR: Something went verrrry wrong. No valid charge case");
            }
        }
        // sleep before repeating
        sleep(1);
    }
}
   ////panel mode
void panelMode()
{
    buckPWM = 0;
    boostPWM = 0;
    panelModeState = ON;

    setDutyCyclePercentForOutput(buckPWM, BUCK_PIN__PWM);
    setDutyCyclePercentForOutput(boostPWM, BOOST_PIN__PWM);
    setOutputForDigitalPin(panelModeState, PM_PIN__DO);

}

   ////buck mode
void buckMode(double chargerVoltage)
 {

    panelModeState = OFF;
    setOutputForDigitalPin(panelModeState, PM_PIN__DO);

    // turn boost off
    boostPWM = 100;
    setDutyCyclePercentForOutput(boostPWM, BOOST_PIN__PWM);

   //read in V_BATT_SENSE
   //if V_BATT_SENSE greater than Vref
    if (chargerVoltage > VREF)
    {
        // increase buck dutycycle  by 1%
        buckPWM++;
        setDutyCyclePercentForOutput(buckPWM, BUCK_PIN__PWM);
    }
}

   ////boost mode
void boostMode(double chargerVoltage)
{

    panelModeState = OFF;
    setOutputForDigitalPin(panelModeState, PM_PIN__DO);

     // turn boost off
    buckPWM = 100;
    setDutyCyclePercentForOutput(buckPWM, BUCK_PIN__PWM);

    if (chargerVoltage < VREF)
    {
        // increase buck dutycycle  by 1%
        boostPWM++;
        setDutyCyclePercentForOutput(boostPWM, BOOST_PIN__PWM);
    }
}

void idleMode() 
{
    buckPWM = 0;
    boostPWM = 0;
    panelModeState = OFF;

    setDutyCyclePercentForOutput(buckPWM, BUCK_PIN__PWM);
    setDutyCyclePercentForOutput(boostPWM, BOOST_PIN__PWM);
    setOutputForDigitalPin(panelModeState, PM_PIN__DO);
}

void setDutyCyclePercentForOutput(int percent, const char *pin)
{    
    char command[200];

    int dutyC = (percent / 100.0) * PERIOD;
    sprintf(command, "echo %d >  /sys/devices/ocp.3/pwm_test_%s/duty", dutyC, pin);
    printf("DEBUG :: %s\n", command);
    system(command);
}

void setOutputForDigitalPin(int aState, int pin)
{
    char command[200];

    sprintf(command, "echo %d > /sys/class/gpio/gpio%d/value", aState, pin);
    printf("DEBUG :: %s\n", command);
    system(command);
}

double getVoltageforInput(int aPin) 
{
    cat /sys/bus/iio/devices/iio:device0/in_voltage0_raw
}


