#ifndef APUFUNKTIOT_H_
#define APUFUNKTIOT_H_

int readMsg();
int checkMsg();
int validMsg(char *msg);
void radioStrs();

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
