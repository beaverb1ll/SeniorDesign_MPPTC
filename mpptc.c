#include <time.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>

#define PM_PIN__DO 60 // digital output
#define BUCK_PIN__PWM  "P8_13.16"  // pwm
#define BOOST_PIN__PWM "P9_14.17" // pwm
#define V_IN_SENSE__ADC 0 // ADC Input
#define V_BATT_SENSE__ADC 1  // ADC Input
#define CRNT_SENSE_IN__ADC 2 // ADC Input
#define CRNT_SENSE_OUT__ADC 3 // ADC Input

#define PWM_PERIOD 10000 // in ??
#define VB100 12.65     // in volts
#define VREF 13.5       // in volts

#define TRUE 1
#define FALSE 0

#define ON 1
#define OFF 0

void panelMode(void);
void buckMode(double chargerVoltage);
void boostMode(double chargerVoltage);
void idleMode(void);
void setDutyCyclePercentForOutput(int percent, const char *pin);
void setOutputForDigitalPin(int aState, int pin);
double getVoltageforInput(int aPin);

int buckPWM, boostPWM, panelModeState;

int main(void)
{
    /* 
    Notes:
      Vref=13.5V in charge mode; Vb100=12.65V 100% charged, Vb75=12.45V 75% charged, Vb50=12.24V 50% charge
      d_buck_start and d_boost_start are duty cycle for starte up
    */

    double panelVoltage = -1, vBattPanelOff = -1, vBattPanelOn = -1;

    buckPWM = 0;
    buckPWM = 0;
    panelModeState = 0;
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
        printf("Started while loop\n");

        //set BUCK_PIN and BOOST_PIN to LOW
        setDutyCyclePercentForOutput(0, BUCK_PIN__PWM);
        setDutyCyclePercentForOutput(0, BOOST_PIN__PWM);
        setOutputForDigitalPin(OFF, PM_PIN__DO);

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
            if (panelVoltage == VREF)
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
void panelMode(void)
{
    printf("Entered panelMode\n");
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

    printf("Entered buckMode\n");
    panelModeState = OFF;
    setOutputForDigitalPin(panelModeState, PM_PIN__DO);

    // turn boost off
    boostPWM = 100;
    setDutyCyclePercentForOutput(boostPWM, BOOST_PIN__PWM);

   //read in V_BATT_SENSE
   //if V_BATT_SENSE greater than Vref
    if (chargerVoltage > VREF)
    {
        if (buckPWM < 100)
        {
            // increase buck dutycycle  by 1%
           buckPWM++;
        }
        
        setDutyCyclePercentForOutput(buckPWM, BUCK_PIN__PWM);
    }
}

   ////boost mode
void boostMode(double chargerVoltage)
{

    printf("Entered boostMode\n");
    panelModeState = OFF;
    setOutputForDigitalPin(panelModeState, PM_PIN__DO);

     // turn boost off
    buckPWM = 100;
    setDutyCyclePercentForOutput(buckPWM, BUCK_PIN__PWM);

    if (chargerVoltage < VREF)
    {
        if (boostPWM < 100)
        {
            // increase buck dutycycle  by 1%
            boostPWM++;
        }
        setDutyCyclePercentForOutput(boostPWM, BOOST_PIN__PWM);
    }
}

void idleMode(void)
{
    buckPWM = 0;
    boostPWM = 0;
    panelModeState = OFF;

    printf("Entered idleMode\n");
    setDutyCyclePercentForOutput(buckPWM, BUCK_PIN__PWM);
    setDutyCyclePercentForOutput(boostPWM, BOOST_PIN__PWM);
    setOutputForDigitalPin(panelModeState, PM_PIN__DO);
}

void setDutyCyclePercentForOutput(int percent, const char *pin)
{    
    char command[200];

    printf("Entered setDutyCyclePercentForOutput\n");
    int dutyC = PWM_PERIOD - ((percent / 100.0) * PWM_PERIOD);
    sprintf(command, "echo %d >  /sys/devices/ocp.3/pwm_test_%s/duty", dutyC, pin);
    printf("DEBUG :: %s\n", command);
    system(command);
}

void setOutputForDigitalPin(int aState, int pin)
{
    char command[200];

    printf("Entered setOutputForDigitalPin\n");
    sprintf(command, "echo %d > /sys/class/gpio/gpio%d/value", aState, pin);
    printf("DEBUG :: %s\n", command);
    system(command);
}

double getVoltageforInput(int aPin)
{
    //cat /sys/bus/iio/devices/iio:device0/in_voltage0_raw
    char buf[15];
    int fd, value;

    printf("Entered getVoltageforInput\n");
    sprintf(buf, "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw", aPin);
    fd = open(buf, O_RDONLY);
    read(fd, &value, 1);
    close(fd);

    return value*8.7891e-4;
}


