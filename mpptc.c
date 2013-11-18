#define PM_PIN 9
#define BUCK_PIN 10
#define BOOST_PIN 11
#define V_IN_SENSE 12
#define V_BATT_SENSE 13
#define CRNT_SENSE_IN 14
#define CRNT_SENSE_OUT 15


int main(void)
{
    // Vref=13.5V in charge mode; Vb100=12.65V 100% charged, Vb75=12.45V 75% charged, Vb50=12.24V 50% charge
    //d_buck_start and d_boost_start are duty cycle for starte up

    /*start up mode
    //set BUCK_PIN and BOOST_PIN to LOW
   //read in V_IN_SENSE
   //read in V_BATT_SENSE
   //if V_BATT_SENSE is equal or greater to Vb100 stop chargeing go into idle mode
   //if V_IN_SENSE is equal to Vref, go into panel mode
   //if panel voltage is higher than Vref, go into buck mode, send d_buck_start = Vref/V_IN_SENSE
   //if panel voltage is lower than Vref, go into boost mode, send d_boost_start = V_IN_SENSE/Vref

    /*charge mode
   ////sense panel voltage
   //set BUCK_PIN and BOOST_PIN to LOW
   //read in V_IN_SENSE
   //read in V_BATT_SENSE
   //if V_BATT_SENSE is equal or greater to Vb100 stop chargeing go into idle mode
   //continue previous charge mode duty cycle
   //if V_IN_SENSE is equal to Vref, go into panel mode
   //if panel voltage is higher than Vref, go into buck mode, //send Vref
   //if panel voltage is lower than Vref, go into boost mode, //send Vref

   ////panel mode
   //set PM_PIN HIGH for 100ms
   //return to sense panel voltage

   ////buck mode
   //read in V_BATT_SENSE
   //if V_BATT_SENSE greater than Vref
   {
   //increase duty cycle by 1%
   // new_dutycycle = previous_dutycycle + 1%
   //return to sense panel voltage
   }
   //else keep previous_dutycycle
   //return to sense panel voltage

   ////boost mode
   //if V_BATT_SENSE is less than Vref
   {
   //increase/decrease duty cycle by 1% //need to test circuit for increase or decrease in duty cycle
   // new_dutycycle = previous_dutycycle +/- 1%
   //return to sense panel voltage
   }
   //else keep previous_dutycycle
   //return to sense panel voltage
   */


   /*idle mode
   //read V_BATT_SENSE every 1 second
   //if V_BATT_SENSE is less than 12.55 volts return to charge mode
   */


}
