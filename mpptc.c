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

#define PANEL_ENABLE__GPIO 60 // digital output -- P9-12
#define BUCK_PIN__PWM  "P8_13.16" // pwm output -- P8-13
#define BOOST_PIN__PWM "P9_14.17" // pwm output -- P9-14
#define PANEL_VOLTAGE__ADC 0 // ADC Input       -- P9-39
#define BATT_VOLTAGE__ADC 1  // ADC Input       -- P9-40
#define CRNT_SENSE_IN__ADC 2 // ADC Input       -- P9-37
#define CRNT_SENSE_OUT__ADC 3 // ADC Input      -- P9-38

#define PWM_FREQ 10000 // in Hz
#define BATT_VOLTAGE_100_PERCENT 12.65     // in volts
#define VREF 13.5       // in volts

// Constants
#define TRUE 1
#define FALSE 0
#define ON 1
#define OFF 0

// function prototypes
void panelMode(void);
void buckMode(double chargerVoltage);
void boostMode(double chargerVoltage);
void idleMode(void);
void setDutyCyclePercentForOutput(int percent, int fd);
void setOutputForDigitalPin(int aState, int fd);
double getVoltageforInput(int aPin);
int configurePinAsInput(int aPin);
int configurePinAsPWM(const char *aPin, int aFreq);
int configurePinAsOutput(int aPin);

int daemonize(void);
void sigINT_handler(int signum);
void sigTERM_handler(int signum);
void closeConnections(void);

// Global Variables
int buckDuty, boostDuty, panelModeState;
unsigned long period_ns;
int buckPin, boostPin, panelEnablePin, panelVoltagePin, battVoltagePin;

    /* 
    Notes:
      Vref=13.5V in charge mode; Vb100=12.65V 100% charged, Vb75=12.45V 75% charged, Vb50=12.24V 50% charge
      d_buck_start and d_boost_start are duty cycle for starte up
    */

int main(void)
{

    double panelVoltage = -1, vBattPanelOff = -1, vBattPanelOn = -1;

    buckDuty = 0;
    boostDuty = 0;
    period_ns = 0;

    /* Register Signal Handlers*/
    signal(SIGTERM, sigTERM_handler);
    signal(SIGINT, sigINT_handler);
    //daemonize();

    buckPin = configurePinAsPWM(BUCK_PIN__PWM, PWM_FREQ);
    boostPin = configurePinAsPWM(BOOST_PIN__PWM, PWM_FREQ);
    panelEnablePin = configurePinAsOutput(PANEL_ENABLE__GPIO);
    panelVoltagePin = configurePinAsInput(PANEL_VOLTAGE__ADC);
    battVoltagePin = configurePinAsInput(BATT_VOLTAGE__ADC);


    while(TRUE) 
    {
        // turn off all charging
        setDutyCyclePercentForOutput(OFF, buckPin);
        setDutyCyclePercentForOutput(OFF, boostPin);
        setOutputForDigitalPin(OFF, panelEnablePin);

        //read in V_IN_SENSE
        panelVoltage = getVoltageforInput(panelVoltagePin); 
        //read in V_BATT_SENSE
        vBattPanelOff = getVoltageforInput(battVoltagePin);
        printf("Panel Voltage: %lf\n", panelVoltage);
        printf("vBattPanelOff: %lf\n", vBattPanelOff);

        //if V_BATT_SENSE is equal or greater to Vb100 stop charging go into idle mode
        if (vBattPanelOff >= BATT_VOLTAGE_100_PERCENT)
        {
            // fully charged, so shutdown all charging
            idleMode();
        }
        else
        {
            // restore previous duty cycle because it was correct
            setDutyCyclePercentForOutput(buckDuty, buckPin);
            setDutyCyclePercentForOutput(boostDuty, boostPin);

            vBattPanelOn = getVoltageforInput(battVoltagePin);

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
                printf("ERROR: Something went verrrry wrong. No valid charge case");
            }
        }
        // sleep before repeating
        sleep(1);
    }
}

void panelMode(void)
{
    buckDuty = OFF;
    boostDuty = OFF;
    panelModeState = ON;

    setDutyCyclePercentForOutput(buckDuty, buckPin);
    setDutyCyclePercentForOutput(boostDuty, boostPin);
    setOutputForDigitalPin(panelModeState, panelEnablePin);
}

void buckMode(double chargerVoltage)
{
    panelModeState = OFF;
    setOutputForDigitalPin(panelModeState, panelEnablePin);

    // turn boost off
    boostDuty = OFF;
    setDutyCyclePercentForOutput(boostDuty, boostPin);

   //read in V_BATT_SENSE
   //if V_BATT_SENSE greater than Vref
    if (chargerVoltage > VREF)
    {
    	if (buckDuty < 100)
    	{
    		buckDuty++;
    	}
        // increase buck dutycycle by 1%
        setDutyCyclePercentForOutput(buckDuty, buckPin);
    }
}

void boostMode(double chargerVoltage)
{

    panelModeState = OFF;
    setOutputForDigitalPin(panelModeState, panelEnablePin);

     // turn boost off
    buckDuty = OFF;
    setDutyCyclePercentForOutput(buckDuty, buckPin);

    if (chargerVoltage < VREF)
    {
        // increase buck dutycycle  by 1%
        if (boostDuty < 100)
        {
        	boostDuty++;
        }
        setDutyCyclePercentForOutput(boostDuty, boostPin);
    }
}

void idleMode(void) 
{
    buckDuty = OFF;
    boostDuty = OFF;
    panelModeState = OFF;

    setDutyCyclePercentForOutput(buckDuty, buckPin);
    setDutyCyclePercentForOutput(boostDuty, boostPin);
    setOutputForDigitalPin(panelModeState, panelEnablePin);
}

void setDutyCyclePercentForOutput(int percent, int fd)
{
   char command[5];

    int newDuty = (1.0e9/PWM_FREQ)-((percent / 100.0) * (1.0e9/PWM_FREQ));
    if (newDuty > (1.0e9/PWM_FREQ) || newDuty < 0)
    {
        printf("ERROR: Invalid duty cycle: %d", newDuty);
        return;
    }
    sprintf(command, "%d", newDuty);
    printf("Set duty: %s\n", command);
    write(fd, command, strlen(command));
}

void setOutputForDigitalPin(int aState, int fd)
{
    if (aState == 1)
    {
        write(fd, "1", 1);
    } else
    {
        write(fd, "0", 1);
    }
}

double getVoltageforInput(int aPin)
{
    int value;

    read(aPin, &value, 1);
    lseek(aPin, 0, SEEK_SET);

    return value*8.7891e-4;
}

int configurePinAsInput(int aPin)
{
    int fd;
    char buf[50];

    sprintf(buf, "/sys/bus/iio/devices/iio:device0/in_voltage%d_raw", aPin);

    fd = open(buf, O_RDONLY);
    if (fd < 1)
    {
        printf("ERROR: Unable to enable ADC pin: %d", aPin);
        closeConnections();
        exit(1);

    }
    return fd;
}

int configurePinAsPWM(const char *aPin, int aFreq)
{
    int fd;
    char buf[100];

    // enable PWM pin
    fd = open("/sys/devices/bone_capemgr.9/slots", O_WRONLY);
    if(fd < 1) {
        printf("Unable to enable PWM Pin: %s", aPin);
        closeConnections();
        exit(1);
    }
    sprintf(buf, "bone_pwm_%s", aPin); 
    write(fd, buf, strlen(buf));
    close(fd);

    // configure Period
    sprintf(buf, "/sys/devices/ocp.3/pwm_test_%s/period", aPin);
    fd = open(buf, O_WRONLY);
    if (fd < 1)
    {
        printf("Unable to initialize PWM period: %s", aPin);
        closeConnections();
        exit(1);
    }
    period_ns = (unsigned long)(1e9 / aFreq);
    sprintf(buf, "%lu", period_ns);
    write(fd, buf, strlen(buf));
    close(fd);

    // configure duty cycle
    sprintf(buf, "/sys/devices/ocp.3/pwm_test_%s/duty", aPin);
    fd = open(buf, O_WRONLY);
    if (fd < 1)
    {
        printf("Unable to initialize PWM duty: %s", aPin);
        closeConnections();
        exit(1);
    }
    
    setDutyCyclePercentForOutput(0, fd);
    return fd;
}

int configurePinAsOutput(int aPin)
{
    int fd;
    char buf[100];

    // turn pin into IO
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if(fd < 1) {
        printf("Unable to export IO Pin: %d", aPin);
        closeConnections();
        exit(1);
    }
    sprintf(buf, "%d", aPin); 
    write(fd, buf, strlen(buf));
    close(fd);


    // turn pin into Output
    sprintf(buf, "/sys/class/gpio/gpio%d/direction", aPin);
    fd = open(buf, O_WRONLY);
    if(fd < 1) {
        printf("Unable to initialize IO Pin direction: %d", aPin);
        closeConnections();
        exit(1);
    }
    write(fd, "out", 3); 
    close(fd);


    // set output low
    sprintf(buf, "/sys/class/gpio/gpio%d/value", aPin);
    fd = open(buf, O_WRONLY);
    if(fd < 1) {
        printf("Unable to Initialize IO Pin Output to LOW: %d", aPin);
        closeConnections();
        exit(1);
    }

    write(fd, "0", 1); 
    return fd;
}

int daemonize(void) 
{
    /* Our process ID and Session ID */
    pid_t pid, sid;
    
    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);
            
    /* Open any logs here */ 
    openlog("MPPTC", LOG_PID|LOG_CONS, LOG_USER);
    printf("Daemon Started.\n");
            
    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* Log the failure */
        printf("ERROR :: Unable to create new SID. Exiting.");
        exit(EXIT_FAILURE);
    }
    printf("finished forking");
    
    /* Change the current working directory */
    if ((chdir("/")) < 0) {
        /* Log the failure */
        printf("ERROR :: Unable to change working directory. Exiting");
        exit(EXIT_FAILURE);
    }
    
    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    /* Daemon-specific initialization goes here */
    

    return 0;
 }
 
void sigINT_handler(int signum)
{
    syslog (LOG_INFO, "Caught SIGINT. Exiting...");
    closeConnections();
    exit(0);
}

void sigTERM_handler(int signum)
{
    printf("Caught SIGTERM. Exiting...");
    closeConnections();
    exit(0);
}

void closeConnections(void)
{
    close(buckPin);
    close(boostPin);
    close(panelEnablePin);
    close(panelVoltagePin);
    close(battVoltagePin);   
}