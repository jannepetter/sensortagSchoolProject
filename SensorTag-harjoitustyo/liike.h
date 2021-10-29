#ifndef _LIIKE_H_
#define _LIIKE_H_

//---------------------------------------------------------------------------------
//                                    FUNCTIONS
//---------------------------------------------------------------------------------




int liftZ(int i);
int preserveXY(int i);
int preserveTilt(int i);
int restAccGyro(int i);
int jump(int i);

int jump(int i){

    if (liftZ(i) && preserveXY(i) && preserveTilt(i) &&
            restAccGyro(sIndx(i+3))){

        return 1;
    }

    return 0;
}

int liftZ(int i){
    if ((accz[i] > -0.8 && accz[i] < 0) || (accz[i] < -1.2 && accz[i] > -1.8)){return 1;}
    return 0;
}
int preserveXY(int i){
    double uthX = 0.6; // threshold for movement in x and y direction
    double lthX = -0.6;
    double thY = 0.2;
    if ((accx[i] > lthX && accx[i] < uthX) && (accy[i] > -thY && accy[i] < thY)){
        return 1;
    }
    return 0;
}
int preserveTilt(int i){
    double gxTh = 50.0;
    double gyTh = 150.0;
    double gzTh = 50.0;

    if((gyrox[i] < gxTh && gyrox[i] > -gxTh) && (gyroy[i] < gyTh && gyroy[i] > -gyTh) &&
            (gyrox[i] < gzTh && gyrox[i] > -gzTh)){
        return 1;
    }
    return 0;
}
int restAccGyro(int i){
    double accRest = 0.1;
    double gyroRest = 20;

    if ((accx[i] < accRest && accx[i] > -accRest) && (accy[i] < accRest && accy[i] > -accRest)
            && (accz[i] < -1+accRest && accz[i] > -1-accRest) && (gyrox[i] < gyroRest && gyrox[i] > -gyroRest)
            && (gyroy[i] < gyroRest && gyroy[i] > -gyroRest) && (gyroy[i] < gyroRest && gyroy[i] > -gyroRest)){
        return 1;
    }
    return 0;
}

#endif
