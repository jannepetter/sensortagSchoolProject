#ifndef _LIIKE_H_
#define _LIIKE_H_

//---------------------------------------------------------------------------------
//                                    FUNCTIONS
//---------------------------------------------------------------------------------


#define ZEROTHRESHOLD 0.3               //kynnysarvo 0G:tä nollan tulkitsemiselle kiihtyvyysmittarille. Mitä pienempi sen tarkemmin asennon oltava kohdallaan.
#define SENSITIVEZEROTHRESHOLD 0.4      //herkempi kynnys nollalle -> asento voi olla hieman sallivampi

#define ONEGTHRESHOLD 0.8               //kynnysarvo 1G kiihtyvyydelle
#define SENSITIVEGTHRESHOLD 0.65         //herkempi kynnys esim. yhdistettyyn liikkeeseen -> sallivampi asento
#define XMIN 0.00
#define XMAX 0.04
#define YMAX 0.02
#define ZMIN -1.10

int liftZ(int i);
int liftZ2(int i);
int preserveXY(int i);
int preserveXY2(int i);
int preserveTilt(int i);
int restAccGyro(int i);
int jump(int i);
int sIndx(int i);
int oikeallaKyljella(int i);
int katollaan(int i);
int sIndx(int i);
int aktivoi(int i);
int move(int i);

int move(int i){
    if((accx[i] > XMAX || accx[i] < XMIN) && (accy[i] > YMAX || accy[i] < -YMAX) && 
        (accz[i]>=ZMIN)){
        return 1;
    }
    return 0;
}

/*Sliding index. Voit tutkia dataa aloittaen nolla indeksistä.
 *Jos perus indeksi voi mennä taulun ulkopuolelle niin käytä tätä*/
int sIndx(int i) { return i % MAXKOKO; };

int aktivoi(int i){
    if(oikeallaKyljella(i) && (katollaan(sIndx(i+1)) || katollaan(sIndx(i+2)) || katollaan(sIndx(i+3)))){
      return 1;
    }
    return 0;
}
int oikeallaKyljella(int i) {
  if (accy[i] > SENSITIVEGTHRESHOLD && (accz[i] > -SENSITIVEZEROTHRESHOLD && accz[i] < SENSITIVEZEROTHRESHOLD)) {
    return 1;
  }
  return 0;
}
int katollaan(int i) {
  if (accz[i] > ONEGTHRESHOLD && (accy[i] > -SENSITIVEZEROTHRESHOLD && accy[i] < SENSITIVEZEROTHRESHOLD)) {
    return 1;
  }
  return 0;
}

int vasemmallaKyljella(int i) {
  if ((accz[i] > -ZEROTHRESHOLD && accz[i] < ZEROTHRESHOLD  ) && accy[i] < -ONEGTHRESHOLD) {
    return 1;
  };
  return 0;
};

int jump(int i){

//    if (liftZ(i) && preserveXY(i) && preserveTilt(i) &&
//            restAccGyro(sIndx(i+3))){
//
//        return 1;
//    }
    //vähän muokkasin ja herkistelin tätä hyppyfunktiota
    if (liftZ2(i) && preserveXY2(i)){
           return 1;
       }

    return 0;
}
//kiihtyvyys poikkeaa -1G:stä (maan vetovoima) mutta ei kuitenkaan ole positiivinen (esim. katollaan)
int liftZ2(int i){
    if (accz[i] > -ONEGTHRESHOLD && accz[i] < 0 || accz[i] < -1.2){return 1;}
    return 0;
}
// x ja y kiihtyvyys nollan tuntumassa
int preserveXY2(int i){
    if ((accx[i] > -ZEROTHRESHOLD && accx[i] < ZEROTHRESHOLD) && (accy[i] > -ZEROTHRESHOLD && accy[i] < ZEROTHRESHOLD)){
        return 1;
    }
    return 0;
}

//int liftZ(int i){
//    if ((accz[i] > -0.8 && accz[i] < 0) || (accz[i] < -1.2 && accz[i] > -1.8)){return 1;}
//    return 0;
//}
//int preserveXY(int i){
//    float uthX = 0.6; // threshold for movement in x and y direction
//    float lthX = -0.6;
//    float thY = 0.2;
//    if ((accx[i] > lthX && accx[i] < uthX) && (accy[i] > -thY && accy[i] < thY)){
//        return 1;
//    }
//    return 0;
//}
//int preserveTilt(int i){
//    float gxTh = 50.0;
//    float gyTh = 150.0;
//    float gzTh = 50.0;
//
//    if((gyrox[i] < gxTh && gyrox[i] > -gxTh) && (gyroy[i] < gyTh && gyroy[i] > -gyTh) &&
//            (gyrox[i] < gzTh && gyrox[i] > -gzTh)){
//        return 1;
//    }
//    return 0;
//}
//int restAccGyro(int i){
//    float accRest = 0.1;
//    float gyroRest = 20;
//
//    if ((accx[i] < accRest && accx[i] > -accRest) && (accy[i] < accRest && accy[i] > -accRest)
//            && (accz[i] < -1+accRest && accz[i] > -1-accRest) && (gyrox[i] < gyroRest && gyrox[i] > -gyroRest)
//            && (gyroy[i] < gyroRest && gyroy[i] > -gyroRest) && (gyroy[i] < gyroRest && gyroy[i] > -gyroRest)){
//        return 1;
//    }
//    return 0;
//}

#endif
