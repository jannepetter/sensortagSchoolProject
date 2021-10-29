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


/* Task */
#define STACKSIZE 2048
#define SMALLSTACKSIZE 256              //aanitask k‰ytt‰‰ t‰t‰, 2048 n‰ytti niin isolta pelk‰lle ‰‰nelle
#define MAXKOKO 15                       //sliding window muuttujille, jos otetaan dataa 5 kertaa 3 sekuntia =15, t‰ll‰ voisi luultavasti max 2sekunnin pituisia liikkeit‰ m‰‰ritell‰
Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
Char mpuTaskStack[STACKSIZE];
Char aaniTaskStack[SMALLSTACKSIZE];
Char analyseDataTaskStack[STACKSIZE];

uint8_t uartBuffer[30];
char uartStr[125];
uint8_t jumpCounter = 0;
// JTKJ: Teht‰v‰ 3. Tilakoneen esittely
// JTKJ: Exercise 3. Definition of the state machine
enum state { WAITING=1, DATA_READY,RUOKI,LIIKUNTA,HOIVA,AKTIVOI };
enum state programState = WAITING;

//----------------globaalit muuttujat---------------
double ambientLight = -1000.0;
double temperature =-100.0;
//--sliding window globaalit muuttujat
uint32_t aika[MAXKOKO]={0};             //timestamp millisekunteina laitteen k‰ynnistyksest‰
float accx[MAXKOKO]={0};
float accy[MAXKOKO]={0};
float accz[MAXKOKO]={0};
float gyrox[MAXKOKO]={0};
float gyroy[MAXKOKO]={0};
float gyroz[MAXKOKO]={0};
uint8_t index=0;                        //indeksimuuttuja sliding window

//--------------------------------------------------
#include "liike.h"

// JTKJ: Teht‰v‰ 1. Lis‰‰ painonappien RTOS-muuttujat ja alustus
// JTKJ: Exercise 1. Add pins RTOS-variables and configuration here
static PIN_Handle buttonHandle;             //vasen nappi
static PIN_State buttonState;

static PIN_Handle rightButtonHandle;        //oikea nappi (katsottuna ett‰ johto osoittaa ylˆs ja katsot monitoria)
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
   PIN_TERMINATE // Asetustaulukko lopetetaan aina t‰ll‰ vakiolla
};
PIN_Config rightButtonConfig[] = {
   Board_BUTTON1  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE // Asetustaulukko lopetetaan aina t‰ll‰ vakiolla
};

PIN_Config ledConfig[] = {
   Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE // Asetustaulukko lopetetaan aina t‰ll‰ vakiolla
};

// MPU uses its own I2C interface
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};


void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
    // JTKJ: Teht‰v‰ 1. Vilkuta jompaa kumpaa ledi‰
    // JTKJ: Exercise 1. Blink either led of the device
    uint_t pinValue = PIN_getOutputValue( Board_LED0 );
    pinValue = !pinValue;
    PIN_setOutputValue( ledHandle, Board_LED0, pinValue );
    aaniState=ONEBEEP;
}
void rightButtonFxn(PIN_Handle handle, PIN_Id pinId) {
//    ei tee viel‰ mit‰‰n j‰rkev‰‰
    tulosteleMuuttujia();
    aaniState=ONEBEEP;
    nollaaMuuttujat();          //testaus vaiheessa muuttujat nollautuu napilla n‰pp‰r‰sti tulostuksen j‰lkeen
}
/*data-analyysi muuttujat*/
void tulosteleMuuttujia(){
    char str[60];
    sprintf(str, "aika: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d \r", aika[0], aika[1],aika[2],aika[3],aika[4],aika[5],aika[6],aika[7],aika[8],aika[9],aika[10],aika[11],aika[12],aika[13],aika[14]);
    System_printf(str);
    System_flush();
    sprintf(str, "acc_x: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \r", accx[0],accx[1],accx[2],accx[3],
    accx[4],accx[5],accx[6],accx[7],accx[8],accx[9],accx[10],accx[11],accx[12],accx[13],accx[14]);
    System_printf(str);
    System_flush();
    sprintf(str, "acc_y: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \r", accy[0],accy[1],accy[2],accy[3],
    accy[4],accy[5],accy[6],accy[7],accy[8],accy[9],accy[10],accy[11],accy[12],accy[13],accy[14]);
    System_printf(str);
    System_flush();
    sprintf(str, "acc_z: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \r", accz[0],accz[1],accz[2],accz[3],
    accz[4],accz[5],accz[6],accz[7],accz[8],accz[9],accz[10],accz[11],accz[12],accz[13],accz[14]);
    System_printf(str);
    System_flush();
    sprintf(str, "gyrox: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \r", gyrox[0],gyrox[1],gyrox[2],gyrox[3],
    gyrox[4],gyrox[5],gyrox[6],gyrox[7],gyrox[8],gyrox[9],gyrox[10],gyrox[11],gyrox[12],gyrox[13],gyrox[14]);
    System_printf(str);
    System_flush();
    sprintf(str, "gyroy: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \r", gyroy[0],gyroy[1],gyroy[2],gyroy[3],
    gyroy[4],gyroy[5],gyroy[6],gyroy[7],gyroy[8],gyroy[9],gyroy[10],gyroy[11],gyroy[12],gyroy[13],gyroy[14]);
    System_printf(str);
    System_flush();
    sprintf(str, "gyroz: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \n\r", gyroz[0],gyroz[1],gyroz[2],gyroz[3],
    gyroz[4],gyroz[5],gyroz[6],gyroz[7],gyroz[8],gyroz[9],gyroz[10],gyroz[11],gyroz[12],gyroz[13],gyroz[14]);
    System_printf(str);
    System_flush();
    sprintf(str, "loppuindeksi: %i\n\r",index);
    System_printf(str);
    System_flush();
}

/*Onnistuneen tunnistuksen j‰lkeen nollaus, jotta laite ei montaa kertaa tunnista samaa liikett‰.
 *Kun ker‰‰t dataa, ei ehk‰ silloin kannata kutsua t‰t‰.*/
Void nollaaMuuttujat(){
    uint8_t i;
    for (i=0;i<15;i++){
        accx[i] = 0;
        accy[i] = 0;
        accz[i] = 0;
        gyrox[i] = 0;
        gyroy[i] = 0;
        gyroz[i] = 0;
    }
}
/*Sliding index. Voit tutkia dataa aloittaen nolla indeksist‰.
 *Jos perus indeksi voi menn‰ taulun ulkopuolelle niin k‰yt‰ t‰t‰*/
int sIndx(int i) { return i % MAXKOKO; };

/*apufunktio ruoki analyysiin. En ehtiny tehd‰ monik‰yttˆisemp‰‰ ja siirt‰‰ toiseen fileen n‰it‰*/
int normalTiltzy(int i) {
  if (accz[i] < -0.8 && accy[i] > -0.1) {
    return 1;
  };
  return 0;
};
/*apufunktio ruoki analyysiin. En ehtiny tehd‰ monik‰yttˆisemp‰‰ ja siirt‰‰ toiseen fileen n‰it‰*/
int reversedTiltzy(int i) {
  if (accz[i] > -0.1 && accy[i] < -0.74) {
    return 1;
  };
  return 0;
};

int analyseAktivoi(){
    /*tarkasta t‰yttyv‰tkˆ aktivoinnin edellytykset*/
    return 0;
}
int analyseHoiva(){
    return 0;
}
int analyseLiiku(){
    char str[30];
    uint8_t i;
    uint32_t time = Clock_getTicks()/10000;
    //sprintf(str,"%d\n",jumpCounter);
    //System_printf(str);
    //System_flush();
        if (time % 20 == 0){
            jumpCounter = 0;
        }
        for (i = 0; i < 15; i++) {

          if (jump(i)) {
              jumpCounter++;
              //sprintf(str,"%d\n",jumpCounter);
              //System_printf(str);
              //System_flush();
              nollaaMuuttujat();
          }
        }
        if (jumpCounter == 2){

            //sprintf(str,"%d\n",jumpCounter);
            //System_printf(str);
            //System_flush();
            jumpCounter = 0;
            return 1;
        }
        return 0;
}
/*Tunnistaa vasemmalle kallistuksen, melkeinp‰ suunnitellusti..*/
int analyseRuoki(){
    uint8_t i;
    for (i = 0; i < 15; i++) {

      if (normalTiltzy(i) &&
          (reversedTiltzy(sIndx(i + 1)) || reversedTiltzy(sIndx(i + 2))) &&
          (normalTiltzy(sIndx(i + 3) || normalTiltzy(sIndx(i + 4))))) {
        return 1;
      }
    }
    return 0;
}


void analyseDataFxn(UArg arg0, UArg arg1){

    while (1){

    if (programState == DATA_READY){


        /*ensin tarkastetaan monimutkaisempaa dataa ja muutetaan tilaa jos ehdot t‰yttyy
         *, jos mink‰‰n tilan vaatimukset eiv‰t t‰yty odotellaan uutta dataa (WAITING)*/
        if(analyseAktivoi()){
            programState = AKTIVOI;
            aaniState=TWOBEEPS; //liike tunnistettu piippaa 2 kertaa
        }else if(analyseHoiva()){
            programState = HOIVA;
            aaniState=TWOBEEPS;
        }else if (analyseLiiku()){
            programState = LIIKUNTA;
            aaniState=ONEBEEP;
            nollaaMuuttujat();
        }else if(analyseRuoki()){
            programState=RUOKI;
            aaniState=TWOBEEPS;
//            nollaaMuuttujat();        //sitte liven‰ hyv‰ ehk‰ nollata, niin ei montaa kertaa tuu sama liike. Testivaiheessa k‰tev‰mpi olla pois p‰‰lt‰
        }else{
            programState = WAITING;
        }
    }
    Task_sleep(100000 / Clock_tickPeriod);
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
    // JTKJ: Teht‰v‰ 4. Lis‰‰ UARTin alustus: 9600,8n1
    // JTKJ: Exercise 4. Setup here UART connection as 9600,8n1
    while (1) {

        UART_write(uart,uartStr, strlen(uartStr));

        // JTKJ: Teht‰v‰ 3. Kun tila on oikea, tulosta sensoridata merkkijonossa debug-ikkunaan
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

        /*viestin vastaanotto taustaj‰rjestelm‰st‰ (pit‰isikˆh‰n t‰m‰ hoitaa erillisess‰ priority 1 taskissa?)
         * teejotain()
         * */

        /*viestin l‰hetys taustaj‰rjestelm‰‰n*/
        //char str[30];
        //sprintf(str, "koira\n\r");
        //UART_write(uart,str,strlen(str));
        switch (programState) {
           case RUOKI:
               System_printf("Ruokitaan...kommunikoi taustaj‰rjestelm‰n kanssa\n\r");
               System_flush();
               programState = WAITING;
               break;
           case LIIKUNTA:
               System_printf("Liikutaan...Kommunikoi taustaj‰rjestelm‰n kanssa\n\r");
               System_flush();
               programState = WAITING;
               break;
           case HOIVA:
               System_printf("Hoivataan...Kommunikoi taustaj‰rjestelm‰n kanssa\n\r");
               System_flush();
               programState = WAITING;
               break;
           case AKTIVOI:
               System_printf("Aktivoidaan...Kommunikoi taustaj‰rjestelm‰n kanssa\n\r");
               System_flush();
               programState = WAITING;
               break;
           default:
               //programState oli jotain muuta esim. DATA_READY, ei muuteta mit‰‰n
               break;
           }

        Task_sleep(100000 / Clock_tickPeriod); //0.1 sekuntia
    }
}

Void sensorTaskFxn(UArg arg0, UArg arg1) {

    I2C_Handle      i2c;            //muiden sensorien v‰yl‰
    I2C_Params      i2cParams;      //muiden sensorien v‰yl‰

    I2C_Handle i2cMPU;              //mpu v‰yl‰
    I2C_Params i2cMPUParams;        //mpu v‰yl‰

    I2C_Transaction i2cMessage;

    float ax, ay, az, gx, gy, gz;

    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    //MPU power on (joku kumma bugi ku ei anna laittaa t‰t‰, toimii silti)
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

    //l‰mpˆmittari setup ja calibraario
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



    char str[20];
    double light=-1000;                                             //v‰limuuttuja,
    while (1) {
        if(programState == WAITING){
        uint32_t time = Clock_getTicks();
        i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);                //mpu (liiketunnistin) v‰yl‰ auki (vain 1kpl v‰yli‰ kerrallaan auki)
        mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);    //datan lukeminen
        sprintf(str,"Sensortask: Aika:%d, (kiihtyvyys x:% -.2f, y:% -.2f, z:% -.2f), (gyro x:% -.2f, y:% -.2f, z:% -.2f)\n", time/100000,ax,ay,az,gx,gy,(gz));


        //System_printf(str);                                         //debug tulostelut tarvittaessa
        System_flush();
        I2C_close(i2cMPU);                                          //mpu (liiketunnistin) v‰yl‰ kiinni
        sprintf(uartStr, "%d,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n\r", time,ax,ay,az,gx,gy,gz);
        accx[index] = ax;                                           //talletetaan data sliding window globaaleihin muuttujiin
        accy[index] = ay;                                           //myˆhemp‰‰ analyysi‰ varten
        accz[index] = az;
        gyrox[index] = gx;
        gyroy[index] = gy;
        gyroz[index] = gz;
        aika[index] = Clock_getTicks()/100;                         //millisekunti stampit
        if (index==MAXKOKO-1){                                      //sliding window indeksin p‰ivitys
            index=0;
        }else{
            index++;
        }

        //l‰mpˆluku
        i2c = I2C_open(Board_I2C_TMP, &i2cParams);                  //muiden sensorien v‰yl‰ auki
        temperature = tmp007_get_data(&i2c);                        //datan lukemiset globaaliin muuttujaan
        sprintf(str,"            L‰mpˆtila: %.2f Celsiusta\r", temperature);   //debug tulostelut tarvittaessa
        //System_printf(str);
        //System_flush();

        //valoluku
        light = opt3001_get_data(&i2c);                             //datan lukemiset globaaliseen muuttujaan, opt3001 ei meinaa ehti‰ kaikkea lukea kun haetaan 5 kertaa per sek
        if (light>=0){
            ambientLight=light;                                     //purkkaratkaisu valomittarin hitauteen, saa parantaa jos keksii miten
        }
        sprintf(str,"%16.2f luxia\r", ambientLight);                //debug tulostelut tarvittaessa
        //System_printf(str);
        //System_flush();
        I2C_close(i2c);                                             //datat luettu niin kiinni


        programState = DATA_READY;

        }

        Task_sleep(200000 / Clock_tickPeriod);                      //0,2 s  eli datan ker‰ys 5 kertaa sekunnissa
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
        System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(rightButtonHandle, &rightButtonFxn) != 0) {
            System_abort("Error registering button callback function");
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

    //taustaj‰rjestelm‰n kanssa kommunikointi
    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority=2;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    /*Voit kutsua ‰‰ni‰ eri toiminnallisuuksiin muuttamalla aaniStatea halutuksi esim. aaniState=ONEBEEP;
     *muita ‰‰nitiloja SILENCE,ONEBEEP,TWOBEEPS,MUSIC
     *‰‰nitilat palautuvat automaattisesti takaisin SILENCE:een suorituksen j‰lkeen*/
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
