#include "bms.h"
#include "stdint.h"
#include <stdio.h>
#include <string.h>

//extern I2C_HandleTypeDef hi2c1;?

//Declare arrys.
signed char offset_corr[7];
signed char gain_corr[7];
//Declare Reference correction
unsigned int vref_corr;         // Corrected VREF x 1000 Unused

float  gain_adc;	//Unused
float  offset_adc;	//Unused


HAL_StatusTypeDef ret;

void BMS_Init (void) {
    uint8_t data;
    // Write initialization code to chip
    //Set Gain of Amp to 0.6 and VREF = 3V
    data = 0b0000001;
    HAL_Delay(500); // Give chip time to properly boot
	ret = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(CONFIG_2 << 1 | 0x00), &data, 1, HAL_MAX_DELAY);
	//Enable amplifier
	data = 0b0000101;
	ret = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(POWER_CTL << 1 | 0x00), &data, 1, HAL_MAX_DELAY);
    
    Offset_n_Gain(); //Fetch constants in registers
    HAL_Delay(100);

}

void Master_Balance (void) {
    // Master Function to keep all cells within over voltage limit

	// For loop goes through all 5 cells
    for ( int i = 0; i < 5; i++) {
        uint16_t cell_volt = Cell_Read ((uint8_t) i ); // Read cell voltage
        write_UART (cell_volt, (uint8_t)i);	// Write voltage to computer voltage
        //While Loop is the balancing function
        //Limited to 42mA discharge current
		while (cell_volt > OVER_VOLT) {
			Start_Balance_Cell ((uint8_t) i ); //Open and close the transistor
			cell_volt = Cell_Read ((uint8_t) i ) + 50; // Read new analog
			//cell_volt = cell_volt + 0.3; // Add some padding so we don't leave cell at threshold
		}
    }
}

void Start_Balance_Cell (uint8_t channel) {
    //Transform to one hot
    channel = 1 << channel;
    uint8_t zero = 0x00; //Zero variable
    //Open transistor to balance selected cell
    HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(BAL_CTL << 1 | 0x00), &channel, 1, HAL_MAX_DELAY);
    HAL_Delay(BALANCE_DELAY); // Open for BALANCE_DELAY ms
    //Close Transistor
    ret = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(BAL_CTL << 1 | 0x00), &zero, 1, HAL_MAX_DELAY);
    HAL_Delay(50);//Give some time to close

}


uint16_t Cell_Read (uint8_t channel) {
    //set up register value to route cell_n to adc
    uint8_t to_Reg = (0x01 << 4) | channel;
	//Write to reg to let mux route cell voltage
    ret = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(CELL_CTL << 1 | 0x00), &to_Reg, 1, HAL_MAX_DELAY);
    HAL_Delay(500); //Wait to let voltage stabilize on ADC Input

    HAL_ADC_Start(&hadc1); //Start the ADC for reading cell voltages
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	uint16_t val = HAL_ADC_GetValue(&hadc1); //Read cell voltage through ADC

    // Corrected VCOUT voltage = 
    //
    //       ADC Count              offset_corr          gain_corr
    //  ( ---------------- x VREF + ----------- ) x (1 + ---------)
    //    Full-scale count             1000                1000
    //
	//Long calculation to convert to corrected cell voltage
	//Could be improved to not used floats for better performance
	long adc_data_l = ((long) val * VREF_NOM/FS_CNT+offset_corr[channel+1]) * (1+gain_corr[channel+1]/1000L);
    uint16_t adc_data = (float)adc_data_l/(GVCOUT); // Calculate actual cell voltage

    return adc_data;
}


void Offset_n_Gain (void) {
	//Function fetches data stored in the registers to get compensation factors
    uint8_t reg_data;
    for (int index = 0; index < 7; index++) {
        HAL_I2C_Master_Receive(&hi2c1, ((VREF_CAL | index) << 1 | 0x01), &reg_data, 1, HAL_MAX_DELAY);

        offset_corr[index] = reg_data >> 4;
        gain_corr[index]   = reg_data & 0x0F;
    }

    HAL_I2C_Master_Receive(&hi2c1, (VREF_CAL_EXT << 1 | 0x01), &reg_data, 1, HAL_MAX_DELAY);
    // Shift in msb and sign extend
    offset_corr[0] |= (((reg_data & 0x06) << 3) ^ 0x20) - 0x20;
    gain_corr[0]   |= (((reg_data & 0x01) << 4) ^ 0x10) - 0x10;
        
    // Get the msb for offset and gain corrections for VC1 and VC2
    HAL_I2C_Master_Receive(&hi2c1, (VC_CAL_EXT_1 << 1 | 0x01), &reg_data, 1, HAL_MAX_DELAY);

    // Shift in msb and sign extend
    offset_corr[1] |= (((reg_data & 0x80) >> 3) ^ 0x10) - 0x10;
    gain_corr[1]   |= (((reg_data & 0x40) >> 2) ^ 0x10) - 0x10;
        
    offset_corr[2] |= (((reg_data & 0x20) >> 1) ^ 0x10) - 0x10;
    gain_corr[2]   |= (((reg_data & 0x10)) ^ 0x10) - 0x10;
            
    // Get the msb for offset and gain corrections for VC3, VC4, VC5, VC6
    HAL_I2C_Master_Receive(&hi2c1, (VC_CAL_EXT_2 << 1 | 0x01), &reg_data, 1, HAL_MAX_DELAY);

    // Shift in msb and sign extend
    offset_corr[3] |= (((reg_data & 0x80) >> 3) ^ 0x10) - 0x10;
    gain_corr[3]   |= (((reg_data & 0x40) >> 2) ^ 0x10) - 0x10;
    
    offset_corr[4] |= (((reg_data & 0x20) >> 1) ^ 0x10) - 0x10;
    gain_corr[4]   |= (((reg_data & 0x10)) ^ 0x10) - 0x10;
    
    offset_corr[5] |= (((reg_data & 0x08) << 1) ^ 0x10) - 0x10;
    gain_corr[5]   |= (((reg_data & 0x04) << 2) ^ 0x10) - 0x10;
    
    offset_corr[6] |= (((reg_data & 0x02) << 3) ^ 0x10) - 0x10;
    gain_corr[6]   |= (((reg_data & 0x01) << 4) ^ 0x10) - 0x10;

    //vref_corr = VREF_NOM * (1000L + gain_corr[0]) + offset_corr[0]; //Not used in this version of code
}

void write_UART (unsigned int print_number, uint8_t channel) {
	//Function writes Cell number and and voltage in mV over UART to be read in Terminal

	char buf[25];
	channel++;
	sprintf(buf, "Cell %u: %u \r\n", (unsigned int)channel, (unsigned int)print_number);


	HAL_UART_Transmit(&huart2, buf, strlen((char*)buf), HAL_MAX_DELAY);
	HAL_Delay(500);

}

void update_VREF (void ) {
	//Unused function as better results was achieved by simply saying the ADC reference is 3.3V
	//Attempt to do 2-point ADC offset and gain correction by using BQ76925 3V reference
    uint8_t data;
    uint16_t ADC_1;
    uint16_t ADC_2;

    //Route 0.5Vref to ADC
    data = 0x01 << 5;
	ret = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(CELL_CTL << 1 | 0x00), &data, 1, HAL_MAX_DELAY);
	HAL_Delay(30);
	HAL_ADC_Start(&hadc1); //Start the ADC for reading cell voltages
	HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	ADC_1 = HAL_ADC_GetValue(&hadc1);

	//vref_adc = (vref_corr) * FS_CNT / (ADC_1 << 1);

	//Route 0.85Vref to ADC
	data = 0x03 << 4;
	ret = HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)(CELL_CTL << 1 | 0x00), &data, 1, HAL_MAX_DELAY);
	HAL_Delay(30);

	HAL_ADC_Start(&hadc1); //Start the ADC for reading cell voltages
	HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	ADC_2 = HAL_ADC_GetValue(&hadc1);

	//ADC_1 = (vref_corr) * FS_CNT * 0.85 / (ADC_2); //Get reading from 0.85 Vref.
	//vref_adc = (vref_adc+ADC_1) >> 1; //Get mean value from both readings and set it as vref for ADC
	//vref_adc = 3300;

	//Calculate Gain and Offset for adc
	gain_adc = ((0.85-0.5)*vref_corr)/(ADC_2-ADC_1);
	offset_adc = (vref_corr >> 1) - (gain_adc*(float)ADC_1);




}



