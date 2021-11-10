/* C Standard library */
#include <aanet.h>
#include <stdio.h>

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/UART.h>

/* Board Header files */
#include "Board.h"
#include "wireless/comm_lib.h"
#include "sensors/opt3001.h"
#include "sensors/mpu9250.h"
#include "sensors/tmp007.h"
#include "buzzer.h"
#include <driverlib/aon_batmon.h> //patteri

/* Task */
#define STACKSIZE 2048
#define SMALLSTACKSIZE 512              //aanitask kÃ¤yttÃ¤Ã¤ tÃ¤tÃ¤, 2048 nÃ¤ytti niin isolta pelkÃ¤lle Ã¤Ã¤nelle
#define MAXKOKO 15                      //sliding window muuttujille, jos otetaan dataa 5 kertaa 3 sekuntia =15, tÃ¤llÃ¤ voisi luultavasti max 2sekunnin pituisia liikkeitÃ¤ mÃ¤Ã¤ritellÃ¤

Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
Char mpuTaskStack[STACKSIZE];
Char aaniTaskStack[SMALLSTACKSIZE];
Char analyseDataTaskStack[STACKSIZE];

uint8_t uartBuffer[30];
char uartStr[125];
char tulosteluStr[125];
// JTKJ: TehtÃ¤vÃ¤ 3. Tilakoneen esittely
// JTKJ: Exercise 3. Definition of the state machine
enum state { STOP=0,WAITING, DATA_READY,RUOKI,LIIKUNTA,HOIVA,AKTIVOI,LEIKI }; //STOP -> liikkeentunnistus pois pÃ¤Ã¤ltÃ¤
enum state programState = WAITING;

//----------------globaalit muuttujat---------------
double ambientLight = -1000.0;
double temperature =-100.0;
//--sliding window globaalit muuttujat
float accx[MAXKOKO]={0};
float accy[MAXKOKO]={0};
float accz[MAXKOKO]={0};
float gyrox[MAXKOKO]={0};
float gyroy[MAXKOKO]={0};
float gyroz[MAXKOKO]={0};
uint8_t index=0;                        //indeksimuuttuja sliding window

//--------------------------------------------------
#include "liike.h"
#include <apufunktiot.h>

// JTKJ: TehtÃ¤vÃ¤ 1. LisÃ¤Ã¤ painonappien RTOS-muuttujat ja alustus
// JTKJ: Exercise 1. Add pins RTOS-variables and configuration here
static PIN_Handle buttonHandle;             //vasen nappi
static PIN_State buttonState;

static PIN_Handle rightButtonHandle;        //oikea nappi (katsottuna ettÃ¤ johto osoittaa ylÃ¶s ja katsot monitoria)
static PIN_State rightButtonState;

static PIN_Handle ledHandle;
static PIN_State ledState;

static PIN_Handle mpuHandle;    //liikeanturi
static PIN_State mpuState;      //liikeanturi

static PIN_Config mpuConfig[] = {
   Board_MPU_POWER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE
};

PIN_Config buttonConfig[] = {
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE // Asetustaulukko lopetetaan aina tÃ¤llÃ¤ vakiolla
};
PIN_Config buttonWakeConfig[] = {                                           //vasemmalle napille alustettu virtojen katkaisu/pÃ¤Ã¤llelaitto
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PINCC26XX_WAKEUP_NEGEDGE,
   PIN_TERMINATE // Asetustaulukko lopetetaan aina tÃ¤llÃ¤ vakiolla
};

PIN_Config rightButtonConfig[] = {
   Board_BUTTON1  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE // Asetustaulukko lopetetaan aina tÃ¤llÃ¤ vakiolla
};

PIN_Config ledConfig[] = {
   Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE // Asetustaulukko lopetetaan aina tÃ¤llÃ¤ vakiolla
};

// MPU uses its own I2C interface
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};


void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
      //ledi pÃ¤Ã¤lle/pois
    //uint_t pinValue = PIN_getOutputValue( Board_LED0 );
    //pinValue = !pinValue;
    //PIN_setOutputValue( ledHandle, Board_LED0, pinValue );
    /*if(aaniState==SILENCE){
    aaniState=MUSIC;                  //tÃ¤ssÃ¤ voi olla bugeja, jos menee jumiin nii aanet tiedostossa jossakin suljetaan kiinni olevaa buzzeria/aukaistaan aukiolevaa tmv.
    }*/
    //char payload[16] = "ping";
    //Send6LoWPAN(IEEE80154_SERVER_ADDR, payload, strlen(payload));
    // Hox! Radio aina takaisin vastaanottotilaan ao. funktiokutssulla
    // Hox2! Tässä ei enää tarkisteta paluuarvoa.. tarkistus vain alustuksessa.
    //StartReceive6LoWPAN();

      //virtojen katkaisu/pÃ¤Ã¤llelaitto (ei toimi, patterit nÃ¤yttÃ¤is loppuneen -> johtuisko siitÃ¤?)
    Task_sleep(100000 / Clock_tickPeriod);
    PIN_close(buttonHandle);
    PINCC26XX_setWakeup(buttonWakeConfig);
    Power_shutdown(NULL,0);

}
void rightButtonFxn(PIN_Handle handle, PIN_Id pinId) {

//    tulosteleMuuttujia();       //datan kerÃ¤ystÃ¤ liikkeiden analysointiin
//    nollaaMuuttujat();          //testaus vaiheessa muuttujat nollautuu napilla nÃ¤ppÃ¤rÃ¤sti tulostuksen jÃ¤lkeen


    //liiketunnistus pÃ¤Ã¤lle 1 piippaus /pois pÃ¤Ã¤ltÃ¤ 3 piippausta
    if(programState==STOP){
        programState=DATA_READY;
        if(aaniState==SILENCE){
        aaniState=ONEBEEP;
        }
        System_printf("liiketunnistus pÃ¤Ã¤llÃ¤ \n\r");
        System_flush();
    }else{
        programState=STOP;
        if(aaniState==SILENCE){
        aaniState=THREEBEEPS;
        }
        System_printf("liiketunnistus pysÃ¤ytetty \n\r");
        System_flush();
    }

}

//oikean kyljen kautta katolleen
int analyseAktivoi(){
    uint8_t i;
    for (i = 0; i < MAXKOKO; i++) {
        if(aktivoi(i)){
            return 1;
        }
    }
    return 0;
}
//lÃ¤mpimÃ¤ssÃ¤ ja pimeÃ¤ssÃ¤ hoivataan
int analyseHoiva(){
    if(temperature>34 && ambientLight<0.1){
        return 1;
    }
    return 0;
}
int analyseLeiki(){
    uint8_t i;
    uint8_t xylos=0;
    uint8_t y=0;
    uint8_t xalas=0;
    uint32_t time = Clock_getTicks()/10000;
    for (i=0; i<15; i++){
        y+=ymove(i);
        xylos+=xmoveu(i);
        xalas+=xmoved(i);
        if (xylos>0 && xalas>0 && y>1){
            return 1;
        }
    }
    return 0;
}
//2-3 hyppyÃ¤ pÃ¶ydÃ¤ltÃ¤ laite asetettuna nÃ¤yttÃ¶ ylÃ¶spÃ¤in
int analyseLiiku(){
    uint8_t i;
    uint8_t counter=0;

        for (i = 0; i < MAXKOKO; i++) {
          if (jump(i)) {
              counter++;
          }
            if(counter>=3){                 //jos 3 suunnanmuutosta (ylÃ¶s tai alas) globaaleissa muuttujissa (n. 3s sisÃ¤llÃ¤)
                return 1;                   //palauttaa true jonka seurauksena muuttujat nollautuu analyseTaskissa
            }
        }
        return 0;
}

//vasemmalle kyljelle kallistus
int analyseRuoki(){
    uint8_t i;
    for (i = 0; i < MAXKOKO; i++) {
        if(vasemmallaKyljella(i)){
            return 1;
        }
    }
    return 0;
}



void analyseDataFxn(UArg arg0, UArg arg1){

    while (1){
    if (programState == DATA_READY){
        /*ensin tarkastetaan monimutkaisempaa dataa ja muutetaan tilaa jos ehdot tÃ¤yttyy
         *, jos minkÃ¤Ã¤n tilan vaatimukset eivÃ¤t tÃ¤yty odotellaan uutta dataa (WAITING)*/
        if(analyseAktivoi()){
            programState = AKTIVOI;
            if(aaniState==SILENCE){
            aaniState=TWOBEEPS; //liike tunnistettu piippaa 2 kertaa
            }
            nollaaMuuttujat();
        }else if(analyseHoiva()){
            programState = HOIVA;
            temperature=0;
            ambientLight=1;
            if(aaniState==SILENCE){
            aaniState=TWOBEEPS;
            }
        }else if (analyseLiiku()){
            programState = LIIKUNTA;
            if(aaniState==SILENCE){
            aaniState=TWOBEEPS;
            }
            nollaaMuuttujat();
        }else if (analyseLeiki()){
            programState = LEIKI;
            if(aaniState==SILENCE){
            aaniState=TWOBEEPS;
            }
            nollaaMuuttujat();
        }else if(analyseRuoki()){
            programState=RUOKI;
            if(aaniState==SILENCE){
            aaniState=TWOBEEPS;
            }
            nollaaMuuttujat();
        }else{
            programState = WAITING;
        }
    }
    Task_sleep(200000 / Clock_tickPeriod);
    }
}


void uartFxn(UART_Handle handle, void *rxBuf, size_t len){

    //UART_read(handle, rxBuf, 1);
    UART_read(handle, rxBuf, 1);
}

/* Task Functions */
Void uartTaskFxn(UArg arg0, UArg arg1) {
    UART_Handle uart;
    UART_Params uartParams;

    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_TEXT;
    uartParams.readDataMode = UART_DATA_TEXT;
    uartParams.readMode=UART_MODE_CALLBACK;
    uartParams.readCallback = &uartFxn;
    uartParams.baudRate = 9600; // nopeus 9600baud
    uartParams.dataLength = UART_LEN_8; // 8
    uartParams.parityType = UART_PAR_NONE; // n
    uartParams.stopBits = UART_STOP_ONE; // 1

    uart = UART_open(Board_UART, &uartParams);
    if ( uart == NULL){
        System_abort("Error opening the UART\n");
    }

    UART_read(uart, uartBuffer, 1);

       char payload[16]; // viestipuskuri
       uint16_t senderAddr;

       // Radio alustetaan vastaanottotilaan
       int32_t result = StartReceive6LoWPAN();
       if(result != true) {
          System_abort("Wireless receive start failed");
       }
    // JTKJ: TehtÃ¤vÃ¤ 4. LisÃ¤Ã¤ UARTin alustus: 9600,8n1
    // JTKJ: Exercise 4. Setup here UART connection as 9600,8n1
    while (1) {

        if (GetRXFlag()) {

                   // Tyhjennetään puskuri (ettei sinne jäänyt edellisen viestin jämiä)
            memset(payload,0,16);
            // Luetaan viesti puskuriin payload
            Receive6LoWPAN(&senderAddr, payload, 16);
            // Tulostetaan vastaanotettu viesti konsoli-ikkunaan
            System_printf(payload);
            System_flush();
        }
//        UART_write(uart,uartStr, strlen(uartStr));

        // JTKJ: TehtÃ¤vÃ¤ 3. Kun tila on oikea, tulosta sensoridata merkkijonossa debug-ikkunaan
        //       Muista tilamuutos
//        if (programState == DATA_READY){
//            char str[20];
//            sprintf(str, "uartTask: Luminance: %.2f lux\n\r",ambientLight);
//            System_printf(str);
//            System_flush();
//            UART_write(uart, str, strlen(str));
//

//            programState = WAITING;
//        }

        /*viestin vastaanotto taustajÃ¤rjestelmÃ¤stÃ¤ (pitÃ¤isikÃ¶hÃ¤n tÃ¤mÃ¤ hoitaa erillisessÃ¤ priority 1 taskissa?)
         * teejotain()
         * */

        /*viestin lÃ¤hetys taustajÃ¤rjestelmÃ¤Ã¤n*/
        //char str[30];
        //sprintf(str, "koira\n\r");
        //UART_write(uart,str,strlen(str));
        switch (programState) {
           case RUOKI:
               System_printf("Ruokitaan...kommunikoi taustajÃ¤rjestelmÃ¤n kanssa\n\r");
               System_flush();
               programState = WAITING;
               break;
           case LIIKUNTA:
               System_printf("Liikutaan...Kommunikoi taustajÃ¤rjestelmÃ¤n kanssa\n\r");
               System_flush();
               programState = WAITING;
               break;
           case HOIVA:
               System_printf("Hoivataan...Kommunikoi taustajÃ¤rjestelmÃ¤n kanssa\n\r");
               System_flush();
               programState = WAITING;
               break;
           case AKTIVOI:
               System_printf("Aktivoidaan...Kommunikoi taustajÃ¤rjestelmÃ¤n kanssa\n\r");
               System_flush();
               programState = WAITING;
               break;
           case LEIKI:
               System_printf("LeikitÃ¤Ã¤n...Kommunikoi taustajÃ¤rjestelmÃ¤n kanssa\n\r");
               System_flush();
               programState = WAITING;
               break;
           default:
               //programState oli jotain muuta esim. DATA_READY, ei muuteta mitÃ¤Ã¤n
               break;
           }

        Task_sleep(1000000 / Clock_tickPeriod); //1 sekunti
    }
}

Void sensorTaskFxn(UArg arg0, UArg arg1) {

    PIN_setOutputValue( ledHandle, Board_LED0, 1 );

    I2C_Handle      i2c;            //muiden sensorien vÃ¤ylÃ¤
    I2C_Params      i2cParams;      //muiden sensorien vÃ¤ylÃ¤

    I2C_Handle i2cMPU;              //mpu vÃ¤ylÃ¤
    I2C_Params i2cMPUParams;        //mpu vÃ¤ylÃ¤

    I2C_Transaction i2cMessage;

    uint8_t sensorCounter=0;        //ei lueta joka tickillÃ¤ kaikkia sensoreja

    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    //MPU power on (joku kumma bugi ku ei anna laittaa tÃ¤tÃ¤, toimii silti)
//    Pin_setOutputValue(mpuHandle, Board_MPU_POWER, Board_MPU_POWER_ON);

    Task_sleep(100000/Clock_tickPeriod);        //mpu sensor powerup wait
    System_printf("MPU9250: Power ON\n");
    System_flush();

    //MPU open i2c
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL){
        System_abort("Error Initializing I2CMPU\n");
    }

    //MPU setup and calibration
    System_printf("MPU9250: Setup and calibration...\n");
    System_flush();
    mpu9250_setup(&i2cMPU);
    System_printf("MPU9250: Setup and calibration OK\n");
    System_flush();
    I2C_close(i2cMPU);


    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    //lÃ¤mpÃ¶mittari setup ja calibraario
    i2c = I2C_open(Board_I2C_TMP, &i2cParams);
    if (i2c == NULL) {
      System_abort("Error Initializing I2C\n");
    }
    Task_sleep(100000/ Clock_tickPeriod);
    tmp007_setup(&i2c);
    I2C_close(i2c);

    // Avataan yhteys, valoisuusmittari setup ja calibraatio
    i2c = I2C_open(Board_I2C_TMP, &i2cParams);
    if (i2c == NULL) {
       System_abort("Error Initializing I2C\n");
    }
    Task_sleep(100000/ Clock_tickPeriod);
    opt3001_setup(&i2c);
    I2C_close(i2c);




    double light=-1000;                                             //vÃ¤limuuttuja,
    while (1) {
        if(programState == WAITING){
        uint32_t time = Clock_getTicks()/10000;


        //LÃMPÃSENSORI:n luku n. 3 sekunnin vÃ¤lein
        if(sensorCounter%17 == 0){
        i2c = I2C_open(Board_I2C_TMP, &i2cParams);                  //muiden sensorien vÃ¤ylÃ¤ auki
        temperature = tmp007_get_data(&i2c);                        //datan lukemiset globaaliin muuttujaan
//        sprintf(tulosteluStr,"            LÃ¤mpÃ¶tila: %.2f Celsiusta\r", temperature);   //debug tulostelut tarvittaessa
//        System_printf(tulosteluStr);
//        System_flush();
        I2C_close(i2c);                                             //datat luettu niin kiinni



        //VALOSENSORI:n luku n. 2 sekunnin vÃ¤lein
        }else if (sensorCounter%11 == 0){
        i2c = I2C_open(Board_I2C_TMP, &i2cParams);                  //muiden sensorien vÃ¤ylÃ¤ auki
        light = opt3001_get_data(&i2c);                             //datan lukemiset globaaliseen muuttujaan, opt3001 ei meinaa ehtiÃ¤ kaikkea lukea kun haetaan 5 kertaa per sek
            if (light>=0){
                ambientLight=light;                                 //purkkaratkaisu valomittarin hitauteen, saa parantaa jos keksii miten
            }
//        sprintf(tulosteluStr,"%16.2f luxia\r", ambientLight);     //debug tulostelut tarvittaessa
//        System_printf(tulosteluStr);
//        System_flush();
        I2C_close(i2c);                                             //datat luettu niin kiinni



        //LIIKESENSORI:n luku n. 0,2 sekunnin vÃ¤lein
        }else{
        i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);                //mpu (liiketunnistin) vÃ¤ylÃ¤ auki (vain 1kpl vÃ¤yliÃ¤ kerrallaan auki)
        mpu9250_get_data(&i2cMPU, &accx[index], &accy[index], &accz[index], &gyrox[index], &gyroy[index], &gyroz[index]);    //datan lukeminen
//        sprintf(tulosteluStr,"Sensortask: Aika:%d, (kiihtyvyys x:% -.2f, y:% -.2f, z:% -.2f), (gyro x:% -.2f, y:% -.2f, z:% -.2f)\n",
//                time/100000,accx[index], accy[index], accz[index], gyrox[index], gyroy[index], gyroz[index]);
//        System_printf(tulosteluStr);                                         //debug tulostelut tarvittaessa
//        System_flush();
        I2C_close(i2cMPU);                                          //mpu (liiketunnistin) vÃ¤ylÃ¤ kiinni
//        sprintf(uartStr, "%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n\r", time,accx[index], accy[index], accz[index], gyrox[index], gyroy[index], gyroz[index]);
            if (index==MAXKOKO-1){                                  //sliding window indeksin pÃ¤ivitys
                index=0;
            }else{
                index++;
            }
                                                                        //luetaan liikesensori 3 kertaa ennenkuin annetaan lupa analysoida dataa -> DATA_READY
            if(sensorCounter%3==0 && programState!=STOP){               //jos liiketunnistuksen keskeytys (oikea nappi) ei ole laitettu pÃ¤Ã¤lle niin DATA_READY
                programState = DATA_READY;                              //DATA_READY vasta kun tarpeeksi uutta liikedataa kerÃ¤tty
                }
            }
        }


        sensorCounter++;
//        sprintf(tulosteluStr,"%i counter, %f lÃ¤mpÃ¶ %f valo\r", sensorCounter,temperature,ambientLight);
//        System_printf(tulosteluStr);
//        System_flush();

//        uint32_t patteri = HWREG(AON_BATMON_BASE + AON_BATMON_O_BAT); //patterin jÃ¤nnite binaarilukuna (nollaa nÃ¤yttÃ¤Ã¤)
//        sprintf(tulosteluStr,"%f patteri\r", patteri);
//        System_printf(tulosteluStr);
//        System_flush();

        Task_sleep(150000 / Clock_tickPeriod);                      //0,15 s  eli datan kerÃ¤ys 6 kertaa sekunnissa
    }
}


Int main(void) {



    // Task variables
    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;

    Task_Handle uartTaskHandle;
    Task_Params uartTaskParams;

    Task_Handle aaniTaskHandle;
    Task_Params aaniTaskParams;

    Task_Handle analyseDataTaskHandle;
    Task_Params analyseDataTaskParams;

    // Initialize board
    Board_initGeneral();
    Init6LoWPAN();
    Board_initUART();
    Board_initI2C();


    mpuHandle = PIN_open(&mpuState, mpuConfig);
    if (mpuHandle == NULL) {
        System_abort("Pin open failed!");
    }

    buttonHandle = PIN_open(&buttonState, buttonConfig);
    if(!buttonHandle) {
        System_abort("Error initializing button pins\n");
    }
    rightButtonHandle = PIN_open(&rightButtonState, rightButtonConfig);
        if(!rightButtonHandle) {
            System_abort("Error initializing button pins\n");
        }
    ledHandle = PIN_open(&ledState, ledConfig);
    if(!ledHandle) {
        System_abort("Error initializing LED pins\n");
    }
    if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
        System_abort("Error registering left button callback function");
    }
    if (PIN_registerIntCb(rightButtonHandle, &rightButtonFxn) != 0) {
            System_abort("Error registering right button callback function");
    }



    //sensoreitten lukeminen globaaleihin muuttujiin
    Task_Params_init(&sensorTaskParams);
    sensorTaskParams.stackSize = STACKSIZE;
    sensorTaskParams.stack = &sensorTaskStack;
    sensorTaskParams.priority=2;
    sensorTaskHandle = Task_create(sensorTaskFxn, &sensorTaskParams, NULL);
    if (sensorTaskHandle == NULL) {
        System_abort("Task create failed!");
    }


    /*Data analyysi taski, jos liike tunnistetaan muutetaan programState asianmukaiseksi*/
    Task_Params_init(&analyseDataTaskParams);
    analyseDataTaskParams.stackSize = STACKSIZE;
    analyseDataTaskParams.stack = &analyseDataTaskStack;
    analyseDataTaskParams.priority = 2;
    analyseDataTaskHandle = Task_create(analyseDataFxn, &analyseDataTaskParams, NULL);
    if (analyseDataTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    //taustajÃ¤rjestelmÃ¤n kanssa kommunikointi
    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority=1;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    /*Voit kutsua Ã¤Ã¤niÃ¤ eri toiminnallisuuksiin muuttamalla aaniStatea halutuksi esim. aaniState=ONEBEEP;
     *muita Ã¤Ã¤nitiloja SILENCE,ONEBEEP,TWOBEEPS,MUSIC
     *Ã¤Ã¤nitilat palautuvat automaattisesti takaisin SILENCE:een suorituksen jÃ¤lkeen*/
    Task_Params_init(&aaniTaskParams);
    aaniTaskParams.stackSize = SMALLSTACKSIZE;
    aaniTaskParams.stack = &aaniTaskStack;
    aaniTaskParams.priority = 2;
    aaniTaskHandle = Task_create(aaniTask, &aaniTaskParams, NULL);
    if (aaniTaskHandle == NULL) {
       System_abort("Task create failed");
    }


    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
