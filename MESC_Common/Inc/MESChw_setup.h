/*
 **
 ******************************************************************************
 * @file           : MESChw_setup.c
 * @brief          : Initialisation code for the PCB
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 David Molony.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************

 * MESChw_setup.c
 *
 *  Created on: 25 Jul 2020
 *      Author: David Molony
 */

#include "stm32fxxx_hal.h"
#include "MESCfoc.h"






/////////////END USER DEFINES//////////////////////


//#define HW_SETUP_RSHUNT (1000)
//:
//#define HW_SETUP_IGAIN ((HW_SETUP_RSHUNT*...)/(...))
// _OR

typedef float
    hardware_vars_t;  // Let's have all the hardware and everything in float for
                      // now, until we start running out of clock cycles?

typedef struct {
  hardware_vars_t Imax;    // Max board voltage allowable
  hardware_vars_t Vmax;    // Max board voltage allowable
  hardware_vars_t Vmin;    // Min voltage at which we turn off the PWM to avoid
                           // brownouts, nastiness.
  hardware_vars_t Rshunt;  // Shunt resistance, ohms
  hardware_vars_t RVBT;    // Vbus top divider - Also for switch divider
  hardware_vars_t RVBB;    // Vbus bottom divider - Also for switch divider
  hardware_vars_t
      VBGain;              //=RVBB/(RVBB+RVBT);         //Resistor divider
                           // network gain (fractional)
  hardware_vars_t RIphPU;  // phase current pullup
  hardware_vars_t RIphSR;  // phase current series resistance
  hardware_vars_t OpGain;  // OpAmp gain, if external, or internal PGA
  hardware_vars_t
      Igain;  // e.g. Rshunt*OpGain*RIphPU/(RIphSR+RIphPU);    //network gain
              // network*opamp gain - total gain before the current hits the
              // ADC, might want this inverted to avoid using division?
  uint16_t RawCurrLim;  // Current limit that will trigger a software
                        // generated break from ADC. Actual current equal to
                        // (RawCurrLim-IMid)*3.3/4096/Gain/Rshunt //example
                        // (4096-2048)*3.3/(4096*16*0.001)= 103A
  uint16_t RawVoltLim;  // Voltage limit that will trigger a software
                        //  generated break from ADC. Actual voltage equal to
                        /// RawVoltLim*3.3*Divider/4096            //
                        /// example 2303*3.3/4096*(R1k5+R47k/R1K5)=60V
} hw_setup_s;

extern hw_setup_s g_hw_setup; // TODO PROFILE
// _OR_
// void hw_setup_init( hw_setp_s * hw_setup );

typedef struct {
  hardware_vars_t Rphase;  // float containing phase resistance in mOhms,
                           // populated by MEASURING if not already known;
  hardware_vars_t Lphase;  // float containing phase inductance in uH,
  hardware_vars_t Lqphase;  // range from very very low inductance high kV strong
                           // magnet BLDC motors to low kV weak magnet ones;
  hardware_vars_t Lqd_diff; //Lq-Ld for using MTPA
      uint8_t uncertainty;
  float motor_flux;
  float measure_current;
  float measure_voltage;
} motor_s;

extern motor_s motor; // TODO PROFILE
// _OR_
// void motor_init( struct motor_s *motor);	//Rob created prototype init,
// unused for now

/*
Hardware-specific implementation

The following function prototypes must be defined in the corresponding:
    MESC_Fxxxx/Core/Src/MESChw_setup.c

in addition to pre-processor defines in the corresponding:
    MESC_Fxxxx/Core/Inc/stm32fxxx_hal.h
*/

/*
Hardware identifiers

#define MESC_GPIO_HALL
*/

/*
Function prototypes
*/

void hw_init(MESC_motor_typedef *_motor);  // Fills the parameters of the hardware struct, simplifies
                     // some into useful overall gain values

void setAWDVals();
void getRawADC(MESC_motor_typedef *_motor);
void getRawADCVph(MESC_motor_typedef *_motor);

/*
#define getHallState(...)
OR
int getHallState( void );
*/

void mesc_init_1( MESC_motor_typedef *_motor ); // Perform HW specific initialisation for MESCInit() before delay
void mesc_init_2( MESC_motor_typedef *_motor ); // Perform HW specific initialisation for MESCInit() after delay
void mesc_init_3( MESC_motor_typedef *_motor ); // Perform HW specific initialisation for MESCInit() after hw_init()

/*
Profile defaults

Temperature parameters
#define MESC_PROFILE_TEMP_R_F
#define MESC_PROFILE_TEMP_SCHEMA
#define MESC_PROFILE_TEMP_SH_BETA
#define MESC_PROFILE_TEMP_SH_R
#define MESC_PROFILE_TEMP_SH_R0
*/
