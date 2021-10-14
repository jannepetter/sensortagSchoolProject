#ifndef AANET_H_
#define AANET_H_
#include "buzzer.h"
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

#include <xdc/std.h>                //tulostus ja system_flush, ei muuta käyttöä atm. Ei ehkä tarvita lopullisessa versiossa
#include <xdc/runtime/System.h>     //sama

enum aanet {SILENCE=0,ONEBEEP,TWOBEEPS,MUSIC};        //voit antaa äänimerkin haluamassasi ohjelman kohdassa muuttamalla aanistatea
enum aanet aaniState=SILENCE;

static PIN_Handle hPin;

Void aaniTask(UArg arg0, UArg arg1) {
   uint8_t counter=0;

   while (1) {
      switch (aaniState) {
      case SILENCE:
          break;
      case ONEBEEP:
          if (counter==0){
              buzzerOpen(hPin);
              buzzerSetFrequency(700);
              counter++;
          }else{
              buzzerClose();
              aaniState=SILENCE;
              counter=0;
          }
          break;
      case TWOBEEPS:
          if (counter==0){
              buzzerOpen(hPin);
              buzzerSetFrequency(700);
              counter++;
            }else if (counter==1){
              buzzerClose();
              counter++;
            }else if (counter==2){
                buzzerOpen(hPin);
                buzzerSetFrequency(700);
                counter++;
            }else{
                buzzerClose();
                aaniState=SILENCE;
                counter=0;
            }
          break;
      case MUSIC:
          System_printf("Tähän musiikkia later..\n\r");
          System_flush();
          aaniState=SILENCE;
          break;
      default:
              counter=0;
              aaniState=SILENCE;
          break;
      }
      Task_sleep(500000L / Clock_tickPeriod);
   }
}


#endif /* AANET_H_ */
