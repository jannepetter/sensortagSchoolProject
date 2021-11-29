#ifndef _LIIKE_H_
#define _LIIKE_H_

//---------------------------------------------------------------------------------
//                                   LIIKE FUNCTIONS
//---------------------------------------------------------------------------------

#define ZEROTHRESHOLD 0.3               //kynnysarvo 0G:tä nollan tulkitsemiselle kiihtyvyysmittarille. Mitä pienempi sen tarkemmin asennon oltava kohdallaan.
#define SENSITIVEZEROTHRESHOLD 0.4      //herkempi kynnys nollalle -> asento voi olla hieman sallivampi

#define ONEGTHRESHOLD 0.8               //kynnysarvo 1G kiihtyvyydelle
#define SENSITIVEGTHRESHOLD 0.6         //herkempi kynnys esim. yhdistettyyn liikkeeseen -> sallivampi asento


//int preserveXY(int i);
//int liftZ(int i);
//int preserveTilt(int i);
//int restAccGyro(int i);
int liftZ2(int i);
int preserveXY2(int i);
int jump(int i);
int sIndx(int i);
int oikeallaKyljella(int i);
int vasemmallaKyljella(int i);
int katollaan(int i);
int sIndx(int i);
int aktivoi(int i);
int xmoveu(int i);
int xmoved(int i);
int ymove(int i);

int xmoveu(int i){
    if(accx[i] > SENSITIVEZEROTHRESHOLD ){
        return 1;
    }
    return 0;
}

int ymove(int i){
    if((accy[i] > SENSITIVEZEROTHRESHOLD || accy[i] < -SENSITIVEZEROTHRESHOLD)){
        return 1;
    }
    return 0;
}

int xmoved(int i){
    if(accx[i] < -SENSITIVEZEROTHRESHOLD){
        return 1;
    }
    return 0;
}

/*Sliding index. Voit tutkia dataa aloittaen nolla indeksista.
 *Jos perus indeksi voi menna taulun ulkopuolelle niin kayta tata*/
int sIndx(int i) { return i % MAXKOKO; };

int aktivoi(int i){
    if(katollaan(i) && (oikeallaKyljella(sIndx(i+1))||oikeallaKyljella(sIndx(i+2))|| oikeallaKyljella(sIndx(i+3)))){
      return 1;
    }
    return 0;
}
int ruoki(int i){
    if(katollaan(i) && (vasemmallaKyljella(sIndx(i+1))|| vasemmallaKyljella(sIndx(i+2))|| vasemmallaKyljella(sIndx(i+3)))){
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
  if ((accz[i] > -SENSITIVEZEROTHRESHOLD && accz[i] < SENSITIVEZEROTHRESHOLD  ) && accy[i] < -SENSITIVEGTHRESHOLD) {
    return 1;
  };
  return 0;
};

int jump(int i){
    if (liftZ2(i) && preserveXY2(i)){
        return 1;
    }
    return 0;
}
//kiihtyvyys poikkeaa -1G:stä (maan vetovoima) mutta ei kuitenkaan ole positiivinen (esim. katollaan)
int liftZ2(int i){
    if (accz[i] > -ONEGTHRESHOLD && accz[i] < 0 || accz[i] < -1.4){return 1;}
    return 0;
}
// x ja y kiihtyvyys nollan tuntumassa
int preserveXY2(int i){
    if ((accx[i] > -ZEROTHRESHOLD && accx[i] < ZEROTHRESHOLD) && (accy[i] > -ZEROTHRESHOLD && accy[i] < ZEROTHRESHOLD)){
        return 1;
    }
    return 0;
}

#endif
