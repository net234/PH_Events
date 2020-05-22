/*************************************************
 *************************************************
    Sketch PH_Events  librairie pour les Events
    Pierre HENRY  V1.3.1 15/03/2020

    Gestion d'evenement en boucle GetEvent HandleEvent

     V1.0 P.HENRY  24/02/2020
    From scratch with arduino.cc totorial :)
    bug possible :
    si GetEvent n,est pas appelé au moins 1 fois durant 1 seconde on pert 1 seconde
    Ajout pulse LED de vie
    V1.1 P.HENRY 29/02/2020
    passage du timestamp en read_only
    les events 100Hz et 10Hz sont non cumulatif & non prioritaire
    l'event 1HZ deviens cumulatif et prioritaire
    V1.2 P.Henry 09/03/2020
  Version sans TimerOne
  Integration de freeram
  Integration de _stringComplete
     V1.2.1 P.Henry 10/03/2020
  Ajout de pushEvent (un seul) todo: mettre plusieurs pushevent avec un delay
    V1.2.2 P.Henry 15/03/2020
  Mise en public de timestamp
   time stamp (long) est destiné maintenir l'heure sur 24 Heures (0.. 86400L 0x15180)
   ajout du ev24H pour gerer les jours

 *************************************************/



//#include <avr/sleep.h>
//#include <avr/time.h>

#include "PH_Events.h"
//const byte Led = LED_BUILTIN;  // the pin with a LED
EventTrack MyEvent;   // le gestionaire d'event local a ce stretch


//#include <LiquidCrystal_PCF8574.h>


//#define LCD_I2C_ADDR  (0x4E/2)    // I2C Adresse in I2C notation
//LiquidCrystal_PCF8574 lcd(LCD_I2C_ADDR); // set the LCD address to 0x27 for a 16 chars and 2 line display



void setup() {
  // Initialisation Hard des IO

  Serial.begin(9600);
  Serial.println("\r\n\nSketch PH_Events V1.3.1");


  // Activation du timer
  MyEvent.begin();

  Serial.println("Bonjour ....");
  // delay(1000);

}
//long nillEventCompteur = 0;
//int ev100HzMissed = 0;
//int ev10HzMissed = 0;




// L'evenement en cours


void loop() {
  // test
  MyEvent.GetEvent(true);
  MyEvent.HandleEvent();
  switch (MyEvent.currentEvent.codeEvent)
  {

    case ev24H:
      Serial.print("---- 24H ---");
      break;
    case evUser:
      Serial.print("Pushed Event");
      break;



#ifdef   USE_SERIALEVENT
    case evInChar:
      switch (toupper(MyEvent.inChar))
      {
        //          case 'U': if ( ++useLCD > 1) useLCD = 0; Serial.print("useLCD:"); Serial.println(useLCD); break;
        case '1': delay(100);     break;
        case '2': delay(200);    break;
        case '3': delay(300);   break;
        case '4': delay(400);  break;
        case '5': delay(500); break;
        case 'A': delay(10); break;
        case 'B': delay(20); break;
        case 'C': delay(30); break;
        case 'D': delay(40); break;
        case 'E': delay(50); break;
        case 'F': delay(60); break;
        case 'G': delay(70); break;
        case 'H': delay(80); break;
        case 'I': delay(90); break;
        case 'J': delay(1000); break;
        case 'K': delay(2000); break;
        case 'L': delay(3000); break;

      }


      break;


    case evInString:
      Serial.print("CMD=");
      Serial.println(MyEvent.inputString.length());

      if (MyEvent.inputString.startsWith("TimeStamp=")) {
        MyEvent.inputString.remove(0, 10);
        MyEvent.timestamp = strtoul(MyEvent.inputString.c_str(),NULL,10);
      }
      if (MyEvent.inputString.equals("P")) {
        MyEvent.pushEvent(evUser);
      }
      if (MyEvent.inputString.equals("Q")) {
        Serial.print("TimeStamp:"); Serial.println(MyEvent.timestamp);
      }
      if (MyEvent.inputString.equals("R")) {
        Serial.println("TimeStamp?");
      }
      if (MyEvent.inputString.equals("S")) {
        Serial.print("Ram="); Serial.println(MyEvent.freeRam());
      }

      break;
#endif
  }
  static long N;
  if (N % 1000 == 0) Serial.print('.');
  if (N++ > 60000L ) {
    N = 0;
    Serial.println('.');
  }

}
