#ifndef AANET_H_
#define AANET_H_
#include "buzzer.h"
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

#include <xdc/std.h>                //tulostus ja system_flush, ei muuta käyttöä atm. Ei ehkä tarvita lopullisessa versiossa
#include <xdc/runtime/System.h>     //sama

enum aanet {SILENCE=0,ONEBEEP,TWOBEEPS,THREEBEEPS,MUSIC};        //voit antaa äänimerkin haluamassasi ohjelman kohdassa muuttamalla aanistatea
enum aanet aaniState=SILENCE;
enum music {ALKU=0,KESKI,LOPPU};
enum music musicState=ALKU;

static PIN_Handle hPin;
uint8_t buzzOpen=0;
uint8_t counter=0;
uint8_t round=0;

void soita(int hz){
    if(buzzOpen==0){
    buzzerOpen(hPin);
    buzzOpen=1;
    buzzerSetFrequency(hz);
    }else{
        aaniState=SILENCE;
        musicState=ALKU;
        buzzerClose();
        counter=0;
        round=0;
        buzzOpen=0;
    }
}
void savel(int hz){
    if(buzzOpen){
        buzzerSetFrequency(hz);
    }else{
        aaniState=SILENCE;
        musicState=ALKU;
        buzzerClose();
        counter=0;
        round=0;
    }
}
void tauko(){
    if(buzzOpen){
    buzzerClose();
    buzzOpen=0;
    }else{
        aaniState=SILENCE;
        musicState=ALKU;
        buzzerClose();
        counter=0;
        round=0;
    }
}

Void aaniTask(UArg arg0, UArg arg1) {

   while (1) {
      switch (aaniState) {
      case SILENCE:
          break;
      case ONEBEEP:
          if (counter==0){
              soita(700);
              counter++;
          }else{
              tauko();
              aaniState=SILENCE;
              counter=0;
              musicState=ALKU;
              round=0;
          }
          break;
      case TWOBEEPS:
          if (counter==0){
              soita(700);
              counter++;
            }else if (counter==1){
              tauko();
              counter++;
            }else if (counter==2){
                soita(700);
                counter++;
            }else{
                tauko();
                aaniState=SILENCE;
                counter=0;
                musicState=ALKU;
                round=0;
            }
          break;
      case THREEBEEPS:
                if (counter%2==0){
                    soita(700);
                    counter++;
                  }else if (counter%2==1){
                    tauko();
                    counter++;
                  }
                if (counter>=6){
                    aaniState=SILENCE;
                    counter=0;
                    musicState=ALKU;
                    round=0;
                }
                break;
      case MUSIC:
              switch (musicState){
              case ALKU:
                  if(counter==1){
                      soita(700);
                  }else if(counter==2){
                      tauko();
                  }else if(counter==3){
                      soita(700);
                  }else if(counter==4){
                      tauko();
                  }else if(buzzOpen==0 && counter==5){
                      soita(700);
                  }else if(counter==7){
                      savel(800);
                  }
                  if(counter>=8){
                      round++;
                      counter=0;
                      tauko();
                  }
                  if(round==2 || round>=7){
                    musicState=KESKI;
                  }
                  counter++;
              break;
              case KESKI:
                  if(counter==1){
                      soita(700);
                    }else if(counter==2){
                        tauko();
                    }else if(counter==3){
                        soita(700);
                    }else if(counter==4){
                        tauko();
                    }else if(counter==5){
                        soita(600);
                    }else if(counter==6){
                        tauko();
                    }else if(counter==7){
                        soita(600);
                    }else if(counter==8){
                        tauko();
                    }else if(counter==9){
                        soita(500);
                    }
                  if(counter>=12){
                       round++;
                       counter=0;
                       tauko();
                   }
                  if(round==3){
                     musicState=LOPPU;
                   }else if(round>=8){
                     aaniState=SILENCE;
                     musicState=ALKU;
                     counter=0;
                     round=0;
                     break;
                   }
                  counter++;
                  break;
              case LOPPU:
                  if(counter==1){
                      soita(700);
                  }else if(counter==2){
                      tauko();
                  }else if(counter==3){
                      soita(700);
                  }else if(counter==4){
                      tauko();
                  }else if(counter==5){
                      soita(700);
                  }else if(counter==6){
                      tauko();
                  }else if(counter==7){
                      soita(700);
                  }else if(counter==8){
                      tauko();
                  }else if(counter==9){
                      soita(900);
                  }else if(counter==11){
                      tauko();
                  }else if(counter==12){
                      soita(800);
                  }
                if(counter>=15){
                     round++;
                     counter=0;
                     tauko();
                 }
                if(round>4){
                  musicState=ALKU;
                  counter=0;
                  break;
                 }
                counter++;
                  break;
              default:
                  tauko();
                  break;
              }

          break;
      default:
//              counter=0;
//              aaniState=SILENCE;
              tauko();
          break;
      }
      Task_sleep(200000L / Clock_tickPeriod);
   }
}


#endif /* AANET_H_ */
