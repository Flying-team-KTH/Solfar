#ifndef STM32L4XX_H
#include "stm32l4xx_hal.h"
#endif


#include "stdint.h"

//Tell files about the global arrays with offset and gain coefficients
extern signed char offset_corr[7];
extern signed char gain_corr[7];
extern unsigned int vref_corr;         // Corrected VREF x 1000

//Give access to peripheral handels in function file
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

//Defenitions

// =============================================================================
// Register addresses
// =============================================================================
#define I2C_GROUP_ADDR  0x4
#define STATUS          (I2C_GROUP_ADDR << 3) + 0x00    // Read & Write
#define CELL_CTL        (I2C_GROUP_ADDR << 3) + 0x01    // Read & Write
#define BAL_CTL         (I2C_GROUP_ADDR << 3) + 0x02    // Read & Write
#define CONFIG_1        (I2C_GROUP_ADDR << 3) + 0x03    // Read & Write
#define CONFIG_2        (I2C_GROUP_ADDR << 3) + 0x04    // Read & Write
#define POWER_CTL       (I2C_GROUP_ADDR << 3) + 0x05    // Read & Write
#define CHIP_ID         (I2C_GROUP_ADDR << 3) + 0x07    // Read Only
#define VREF_CAL        (I2C_GROUP_ADDR << 3) + 0x10    // Read Only
#define VC1_CAL         (I2C_GROUP_ADDR << 3) + 0x11    // Read Only
#define VC2_CAL         (I2C_GROUP_ADDR << 3) + 0x12    // Read Only
#define VC3_CAL         (I2C_GROUP_ADDR << 3) + 0x13    // Read Only
#define VC4_CAL         (I2C_GROUP_ADDR << 3) + 0x14    // Read Only
#define VC5_CAL         (I2C_GROUP_ADDR << 3) + 0x15    // Read Only
#define VC6_CAL         (I2C_GROUP_ADDR << 3) + 0x16    // Read Only
#define VC_CAL_EXT_1    (I2C_GROUP_ADDR << 3) + 0x17    // Read Only
#define VC_CAL_EXT_2    (I2C_GROUP_ADDR << 3) + 0x18    // Read Only
#define VREF_CAL_EXT    (I2C_GROUP_ADDR << 3) + 0x1B    // Read Only

// =============================================================================
// Define references for measurement calculations
//
// VREF_NOM : Nominal reference voltage. Set by writing REF_SEL in the CONFIG_2
//            register (REF_SEL = 0, 1 => VREF_NOM = 1.5, 3.0).
// GVCOUT   : Nominal cell voltage gain. Also set by writing REF_SEL in the
//            CONFIG_2 register (REF_SEL = 0, 1 => GVCOUT = 0.3, 0.6)  .
// GVSENSE  : Current amplifier gain. Set by writing the I_GAIN bit in the
//            CONFIG_1 register (I_GAIN = 0, 1 => GVSENSE = 4, 8).
// FS_CNT   : ADC full-scale count. For an 'n' bit ADC, FS_CNT = pow(2,n) - 1.
// =============================================================================
#define VREF_NOM        3300 	//Voltage reference for the ADC typically 3.3 for STM32
#define GVCOUT          0.6
#define FS_CNT          4095L   // 12-bit ADC


#define OVER_VOLT       5000   // Over voltage threshold in mV

#define BALANCE_DELAY	10000  //How many ms the cell balance transistor will be open


// =============================================================================
// Function prototypes
// =============================================================================


uint16_t Cell_Read (uint8_t channel);

void Start_Balance_Cell (uint8_t channel);

void write_UART (unsigned int print_number, uint8_t channel);

void BMS_Init(void);

void Master_Balance(void);

void Offset_n_Gain (void);

void update_VREF (void);

