#ifndef _LIIKE_H_
#define _LIIKE_H_

//---------------------------------------------------------------------------------
//                                    FUNCTIONS
//---------------------------------------------------------------------------------


#define ZEROTHRESHOLD 0.3               //kynnysarvo 0G:t� nollan tulkitsemiselle kiihtyvyysmittarille. Mit� pienempi sen tarkemmin asennon oltava kohdallaan.
#define SENSITIVEZEROTHRESHOLD 0.4      //herkempi kynnys nollalle -> asento voi olla hieman sallivampi

#define ONEGTHRESHOLD 0.8               //kynnysarvo 1G kiihtyvyydelle
#define SENSITIVEGTHRESHOLD 0.65         //herkempi kynnys esim. yhdistettyyn liikkeeseen -> sallivampi asento

int liftZ2(int i);
//int preserveXY(int i);
int preserveXY2(int i);
//int preserveTilt(int i);
//int restAccGyro(int i);
int jump(int i);
int oikeallaKyljella(int i);
int vasemmallaKyljella(int i);
int katollaan(int i);
int sIndx(int i);
int aktivoi(int i);
int move(int i);

int move(int i){
    double xmin = -0.00;
    double xmax = 0.04;
    double ymin = -0.02;
    double ymax = 0.02;
    double zmin = -1.10;
    if((accx[i] > xmax || accx[i] < xmin) && (accy[i] > ymax || accy[i] < ymin) && 
        (accz[i]>=zmin)){
        return 1;
    }
    return 0;
}

/*Sliding index. Voit tutkia dataa aloittaen nolla indeksist�.
 *Jos perus indeksi voi menn� taulun ulkopuolelle niin k�yt� t�t�*/
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
    //v�h�n muokkasin ja herkistelin t�t� hyppyfunktiota
    if (liftZ2(i) && preserveXY2(i)){
           return 1;
       }

    return 0;
}
//kiihtyvyys poikkeaa -1G:st� (maan vetovoima) mutta ei kuitenkaan ole positiivinen (esim. katollaan)
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