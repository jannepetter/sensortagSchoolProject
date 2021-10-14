/*
 * tmp007.c
 *
 *  Created on: 28.9.2016
 *  Author: Teemu Leppanen / UBIComp / University of Oulu
 *
 *  Datakirja: http://www.ti.com/lit/ds/symlink/tmp007.pdf
 */

#include <xdc/runtime/System.h>
#include <string.h>
#include "Board.h"
#include "tmp007.h"

void tmp007_setup(I2C_Handle *i2c) {

	System_printf("TMP007: Config OK!\n");
    System_flush();
}

/**************** JTKJ: DO NOT MODIFY ANYTHING ABOVE THIS LINE ****************/

double tmp007_get_data(I2C_Handle *i2c) {

	double temperature = 0.0; // return value of the function
    // JTKJ: Find out the correct buffer sizes with this sensor?
//     char txBuffer[1];
//     char rxBuffer[2];
     uint8_t txBuffer[1];
     uint8_t rxBuffer[2];

    // JTKJ: Fill in the i2cMessage data structure with correct values
    //       as shown in the lecture material
    I2C_Transaction i2cMessage;
    i2cMessage.slaveAddress = Board_TMP007_ADDR;
    txBuffer[0] = TMP007_REG_TEMP;  // Rekisterin osoite l‰hetyspuskuriin
    i2cMessage.writeBuf = txBuffer; // L‰hetyspuskurin asetus
    i2cMessage.writeCount = 1;      // L‰hetet‰‰n 1 tavu
    i2cMessage.readBuf = rxBuffer;  // Vastaanottopuskurin asetus
    i2cMessage.readCount = 2;       // Vastaanotetaan 2 tavua

	if (I2C_transfer(*i2c, &i2cMessage)) {
	    uint16_t rekisteri = (rxBuffer[0] <<8 | rxBuffer[1]);
	    uint16_t temp = rekisteri >> 2;
	    temperature=0.03125*temp;
        // JTKJ: Here the conversion from register value to temperature

	} else {

		System_printf("TMP007: Data read failed!\n");
		System_flush();
	}

	return temperature;
}
