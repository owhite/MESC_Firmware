/*
 * MP2_V0_1.h
 *
 *  Created on: Dec 16, 2022
 *      Author: HPEnvy
 */

#ifndef INC_MP2_V0_1_H_
#define INC_MP2_V0_1_H_
//Pick a motor for default
#define QS165
#define PWM_FREQUENCY 20000
#define CUSTOM_DEADTIME 800 //ns, MAX 1500ns! implementation in MESCInit().

#define SHUNT_POLARITY -1.0f

#define ABS_MAX_PHASE_CURRENT 100.0f
#define ABS_MAX_BUS_VOLTAGE 80.0f
#define ABS_MIN_BUS_VOLTAGE 38.0f
#define R_SHUNT 0.00033f
#define OPGAIN 10.5f

#define R_VBUS_BOTTOM 3300.0f //Phase and Vbus voltage sensors
#define R_VBUS_TOP 100000.0f

#define MAX_ID_REQUEST 2.0f
#define MAX_IQ_REQUEST 20.0f

#define SEVEN_SECTOR		//Normal SVPWM implemented as midpoint clamp. If not defined, you will get 5 sector, bottom clamp
#define DEADTIME_COMP		//This injects extra PWM duty onto the timer which effectively removes the dead time.
#define DEADTIME_COMP_V 10
//#define MAX_MODULATION 1.05f //Use this with 5 sector modulation if you want extra speed
//Inputs
#define GET_THROTTLE_INPUT _motor->Raw.ADC_in_ext1 = hadc1.Instance->JDR4;  // Throttle for MP2 with F405 pill

//#define USE_FIELD_WEAKENING
#define USE_FIELD_WEAKENINGV2

//#define USE_LR_OBSERVER

/////////////////////Related to ANGLE ESTIMATION////////////////////////////////////////
#define INTERPOLATE_V7_ANGLE
#define DEFAULT_SENSOR_MODE MOTOR_SENSOR_MODE_SENSORLESS
//#define DEFAULT_SENSOR_MODE MOTOR_SENSOR_MODE_HALL
//#define DEFAULT_SENSOR_MODE MOTOR_SENSOR_MODE_OPENLOOP
//#define DEFAULT_SENSOR_MODE MOTOR_SENSOR_MODE_ENCODER
//#define DEFAULT_SENSOR_MODE MOTOR_SENSOR_MODE_HFI

#define USE_HFI
#define HFI_VOLTAGE 4.0f
#define HFI_TEST_CURRENT 0.0f
#define HFI_THRESHOLD 0.0f
#define HFI45
#define DEFAULT_HFI_TYPE HFI_TYPE_NONE
//#define DEFAULT_HFI_TYPE HFI_TYPE_45
//#define DEFAULT_HFI_TYPE HFI_TYPE_D
//#define DEFAULT_HFI_TYPE HFI_TYPE_SPECIAL

#define USE_HALL_START
#define HALL_VOLTAGE_THRESHOLD 1.5f

//#define USE_ENCODER //Only supports TLE5012B in SSC mode using onewire SPI on SPI3 F405...
#define POLE_PAIRS 7
#define ENCODER_E_OFFSET 25000
#define POLE_ANGLE (65536/POLE_PAIRS)

//#define USE_SALIENT_OBSERVER //If not defined, it assumes that Ld and Lq are equal, which is fine usually.

#define FASTLED GPIOC
#define FASTLEDIO GPIO_PIN_12
#define FASTLEDIONO 12
#define SLOWLED GPIOC
#define SLOWLEDIO GPIO_PIN_9
#define SLOWLEDIONO 9


#define LOGGING

#endif /* INC_MP2_V0_1_H_ */
