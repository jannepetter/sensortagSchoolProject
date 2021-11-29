#ifndef APUFUNKTIOT_H_
#define APUFUNKTIOT_H_

int readMsg();
int checkMsg();
int validMsg(char *msg);
void radioStrs();
/*data-analyysi muuttujat. Voit esim oikealla napilla ker‰t‰ liikeen j‰lkeen dataa t‰ll‰.
 * Kun datat on ker‰tty ja liiketunnistukset toimii nii t‰m‰n funktion vois varmaanki poistaa h‰irittem‰st‰.*/
//void tulosteleMuuttujia(){
//
//    sprintf(tulosteluStr, "acc_x: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \r", accx[0],accx[1],accx[2],accx[3],
//    accx[4],accx[5],accx[6],accx[7],accx[8],accx[9],accx[10],accx[11],accx[12],accx[13],accx[14]);
//    System_printf(tulosteluStr);
//    System_flush();
//    sprintf(tulosteluStr, "acc_y: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \r", accy[0],accy[1],accy[2],accy[3],
//    accy[4],accy[5],accy[6],accy[7],accy[8],accy[9],accy[10],accy[11],accy[12],accy[13],accy[14]);
//    System_printf(tulosteluStr);
//    System_flush();
//    sprintf(tulosteluStr, "acc_z: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \r", accz[0],accz[1],accz[2],accz[3],
//    accz[4],accz[5],accz[6],accz[7],accz[8],accz[9],accz[10],accz[11],accz[12],accz[13],accz[14]);
//    System_printf(tulosteluStr);
//    System_flush();
//    sprintf(tulosteluStr, "gyrox: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \r", gyrox[0],gyrox[1],gyrox[2],gyrox[3],
//    gyrox[4],gyrox[5],gyrox[6],gyrox[7],gyrox[8],gyrox[9],gyrox[10],gyrox[11],gyrox[12],gyrox[13],gyrox[14]);
//    System_printf(tulosteluStr);
//    System_flush();
//    sprintf(tulosteluStr, "gyroy: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \r", gyroy[0],gyroy[1],gyroy[2],gyroy[3],
//    gyroy[4],gyroy[5],gyroy[6],gyroy[7],gyroy[8],gyroy[9],gyroy[10],gyroy[11],gyroy[12],gyroy[13],gyroy[14]);
//    System_printf(tulosteluStr);
//    System_flush();
//    sprintf(tulosteluStr, "gyroz: %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f \n\r", gyroz[0],gyroz[1],gyroz[2],gyroz[3],
//    gyroz[4],gyroz[5],gyroz[6],gyroz[7],gyroz[8],gyroz[9],gyroz[10],gyroz[11],gyroz[12],gyroz[13],gyroz[14]);
//    System_printf(tulosteluStr);
//    System_flush();
//    sprintf(tulosteluStr, "loppuindeksi: %i\n\r",index);
//    System_printf(tulosteluStr);
//    System_flush();
//}

//hyvaksytaan vain viestit jotka alkavat 153
int validMsg(char *msg){
    if(msg[0]=='1'&&msg[1]=='5' && msg[2]=='3'){
        return 1;
    }
    return 0;
}

/*Onnistuneen tunnistuksen j‰lkeen nollaus, jotta laite ei montaa kertaa tunnista samaa liikett‰.
 *Kun ker‰‰t dataa, ei ehk‰ silloin kannata kutsua t‰t‰.*/
void nollaaMuuttujat(){
    uint8_t i;
    for (i=0;i<MAXKOKO;i++){
        accx[i] = 0;
        accy[i] = 0;
        accz[i] = -1;
        gyrox[i] = 0;
        gyroy[i] = 0;
        gyroz[i] = 0;
    }
}
int checkMsg(){
    uint8_t count=0;
    if(validMsg(uartStr)){
        if(strstr(uartStr,"Too late")){
            count+=10;
        }else if((strstr(uartStr,"Severe") || strstr(uartStr,"scratch") || strstr(uartStr,"Running low"))){
            count+=3;
        }else{
            count++;
        }
    }
    if(validMsg(uartStr2)){
        if(strstr(uartStr2,"Too late")){
            count+=10;
        }else if((strstr(uartStr2,"Severe") || strstr(uartStr2,"scratch") || strstr(uartStr2,"Running low"))){
            count+=3;
        }else{
            count++;
        }
    }
//    if(validMsg(radioBuffer)){
//        if(strstr(radioBuffer,"Too late")){
//            count+=10;
//        }else if((strstr(radioBuffer,"Severe") || strstr(radioBuffer,"scratch") || strstr(radioBuffer,"Running low"))){
//            count+=3;
//        }else{
//            count++;
//        }
//    }
    if(aaniState==SILENCE){
        if(count>9){
            aaniState=MUSIC;        //tamagotchi karkasi
        }else if(count>2){          //varoitus etta kohta karkaa
            aaniState=THREEBEEPS;
            if(isHappy){            //tyytyvainen tamagotchi ei heti karkaa
                return 1;
            }
        }else if(count>0){
            aaniState=ONEBEEP;      //ei tarkea viesti
        }
    }
    return 0;
}
void radioStrs(){
    uint8_t i;
    for (i = 0; i < BLENGTH*2; i++) {
        if(i<BLENGTH){
            uartStr[i]=radioBuffer[i];
        }else if (i<BLENGTH*2){
            uartStr2[i-BLENGTH]=radioBuffer[i];
        }
    }
}

int readMsg(){
    uint8_t survive=checkMsg(); //aanet
    //jos haluat nahda viestit -> uncomment
//    if(validMsg(uartStr)){
//    sprintf(tulosteluStr,"%s %i\r\n",uartStr,buffCount);
//    System_printf(tulosteluStr);
//    System_flush();
//    memset(tulosteluStr,0,100);
//    }
//    if(validMsg(uartStr2)){
//    sprintf(tulosteluStr,"%s %i\r\n",uartStr2,buffCount);
//    System_printf(tulosteluStr);
//    System_flush();
//    }
    buffCount=0;
    memset(uartStr,0,BLENGTH);
    memset(uartStr2,0,BLENGTH);
    memset(radioBuffer,0,BLENGTH*2);
    memset(tulosteluStr,0,100);
    if(survive){
        return 1;
    }
    return 0;

}

#endif /* APUFUNKTIOT_H_ */
