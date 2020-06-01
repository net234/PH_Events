/*************************************************
  /*************************************************
 *************************************************
    PH_Events  librairie pour les Events
    Pierre HENRY  V1.3 P.henry 23/04/2020
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
   integration eventnill
   economie memoire pour inputString
     V1.3  P.Henry 14/04/2020
   compatibilité avec les ESP
   ajout d'un buffer pour les push event avec un delay
   gestion du pushevent en 100' de seconde
 *************************************************/



#include "PH_Events.h"
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__)  //LEONARDO
#define LED_PULSE_ON LOW
#else
#ifdef  __AVR__
#define LED_PULSE_ON HIGH

#else
// Pour ESP c'est l'inverse
#define LED_PULSE_ON LOW
#endif
#endif
// todo replacer timerOne par ticker ?
#ifdef  USE_TimerOne
#include <TimerOne.h>
#endif

#ifdef  __AVR__
#include <avr/sleep.h>
#endif

volatile byte     __div10Hz   = 0;    // diviseur pour avoir du 10Hz
volatile byte     __div1Hz    = 0;    // diviseur pour avoir du 1Hz



#ifdef  USE_TimerOne

// Flag pour signaler le passage du timer
// volatile est important pour les interuptions :)
volatile byte     __cnt100Hz  = 0;    // compteur de pulse 100Hz (max 199)
volatile byte     __cnt10Hz   = 0;    // compteur de pulse 10Hz  (max 199)
volatile byte     __cnt1Hz    = 0;    // compteur de pulse 1Hz   (max 199)



//====== Timer1 interuption
void __callback_Timer1(void) {
  if (__cnt100Hz != 199U)  __cnt100Hz++;   // Signale les evenements 100HZ

  if (++__div10Hz == 10U) {
    __div10Hz = 0;
    if (__cnt10Hz != 199U)  __cnt10Hz++;   // Signale les evenements 10HZ

    if (++__div1Hz == 10U) {
      __div1Hz = 0;
      if (__cnt1Hz != 199U)  __cnt1Hz++;   // Signale l'evenement 1HZ
    }
  }
}

#endif


void  Event::begin() {

#ifdef USE_TimerOne
  Timer1.initialize(1000000 / FrequenceTimer); // Timer reglé en microsecondes
  Timer1.attachInterrupt(__callback_Timer1);    // attaches callback() pour gerer l'interuption
#endif
#ifdef  USE_SERIALEVENT
  inputString.reserve(_inputStringSizeMax);

#endif
  pinMode(_LedPulse, OUTPUT);

#ifdef  __AVR__
  /*
    Atmega328 seul et en sleep mode:  // 22 mA
    SLEEP_MODE_IDLE:       15 mA      //15 mA
    SLEEP_MODE_ADC:         6,5 mA    //30 mA  no timer1
    SLEEP_MODE_PWR_SAVE:    1,62 mA   //22 mA  no timer1
    SLEEP_MODE_EXT_STANDBY: 1,62 mA   //22 mA  no timer1
    SLEEP_MODE_STANDBY :    0,84 mA   //21 mA  no timer1
    SLEEP_MODE_PWR_DOWN :   0,36 mA   //21 mA  no timer1
  */
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
#endif
}

// todo ajouter une frequence autre que 1HZ pour la led
void  Event::SetPulsePercent(byte percent) {
  _pulse10Hz = percent / 10;
}

byte  Event::Second() const {
  return ( timestamp % 60);
}
byte  Event::Minute() const {
  return ( (timestamp / 60) % 60);
}
byte  Event::Hour() const {
  return ( (timestamp / 3600) % 24);
}

static unsigned long milliSeconds = 0;
static unsigned int delta1Hz = 0;
static unsigned int delta10Hz = 0;
static unsigned int delta100Hz = 0;

int Event::syncroSeconde(const int millisec) {
  int result =  millisec - delta1Hz;
  if (result != 0) {
    delta1Hz = millisec;
    delta10Hz = millisec;
    delta100Hz = millisec;
  }
  return result;
}


byte Event::GetEvent(const bool sleep ) {  //  sleep = true;

#ifdef USE_TimerOne
  noInterrupts();

  if (__cnt100Hz)
  {
    _evMissed = __cnt100Hz - 1;
    __cnt100Hz = 0U;
    interrupts();
    return (_codeEvent = ev100Hz);
  }


  // les ev10Hz ne sont pas tous restitués
  if (__cnt10Hz)
  {
    _evMissed = __cnt10Hz - 1;
    __cnt10Hz = 0;
    interrupts();
    return (_codeEvent = ev10Hz);
  }
  // les ev1HZ sont comptés ET restitués un par un (meme en retard)
  // TODO: generer un evDepassement1H si
  if (__cnt1Hz)
  {
    __cnt1Hz--;
    interrupts();
    return (_codeEvent = ev1Hz);
  }


  interrupts();
#else  // without timerOne we use mills()

  noInterrupts();
  //  unsigned long newmilliSeconds = millis();
  unsigned long delta = millis() - milliSeconds;
  interrupts();
  //  unsigned long delta = newmilliSeconds - milliSeconds;
  if (delta) {
    milliSeconds += delta;
    delta1Hz += delta;
    delta10Hz += delta;
    delta100Hz += delta;

    // les ev100Hz ne sont pas tous restitués
    // il sont utilisé pour les DelayedEvent
    if (delta100Hz >= 10)
    {
      if ( ++__div10Hz == 10) __div10Hz = 0;
      _paramEvent = (delta100Hz / 10);  // nombre d'ev100Hz d'un coup
      delta100Hz -= (_paramEvent) * 10;
      return (currentEvent.codeEvent = ev100Hz);
    }


    // les ev10Hz ne sont pas tous restitués
    if (delta10Hz >= 100)
    {
      if ( ++__div1Hz == 10) __div1Hz = 0;
      _paramEvent = (delta10Hz / 100);  // nombre d'ev10Hz d'un coup
      delta10Hz -= (_paramEvent) * 100;
      return (currentEvent.codeEvent = ev10Hz);
    }
    // par contre les ev1Hz sont tous restirués meme avec du retard
    if (delta1Hz >= 1000)
    {
      //    __cnt1Hz--;
      delta1Hz -= 1000;
      return (currentEvent.codeEvent = ev1Hz);
    }
  }

#ifdef  USE_SERIALEVENT
  if (_stringComplete)
  {
    _stringComplete = false;
    _stringErase = true;      // la chaine ser  a effacee au prochain caractere recu
    return (currentEvent.codeEvent = evInString);
  }

  if (Serial.available())
  {
    inChar = (char)Serial.read();
    return (currentEvent.codeEvent = evInChar);
  }

#endif

  // les evenements sans delay sont geré ici
  // les delais sont gere via ev100HZ
  if (_waitingEventIndex != 0) {
    currentEvent = _waitingEvent[0];

    for (byte N = 0; N < _waitingEventIndex; N++) {
      _waitingEvent[N] = _waitingEvent[N + 1];
    }
    _waitingEvent[--_waitingEventIndex].codeEvent = evNill;
    return (currentEvent.codeEvent);
  }

  _nillEventCompteur++;
  if (sleep) {
#ifdef  __AVR__
    sleep_mode();
#else
    // pour l'ESP le sleep mode est activé par un delay
    delay(1);
    //ESP.deepSleep(5000000);
    //   esp_sleep_enable_timer_wakeup(5000000); //5 seconds
    //   yield();
#endif
  }

  return (currentEvent.codeEvent = evNill);
#endif
}






void  Event::HandleEvent() {
  //   Serial.print(_codeEvent);
  switch (currentEvent.codeEvent)
  {


    // gestion des evenement avec delay
    case ev100Hz: {
        //      Serial.print("waitingdelay : ");
        //          Serial.println(_waitingDelayEventIndex);
        // on scan les _waitintDelayEvent pour les passer en _waitintEvent
        byte N = 0;
        while (N < _waitingDelayEventIndex) {
          //        Serial.print("delay : ");
          //        Serial.println(_waitingDelayEvent[N].delay);
          if (_waitingDelayEvent[N].delay > _paramEvent) {
            _waitingDelayEvent[N].delay -= _paramEvent;
            N++;
          } else {
            //                      Serial.print("Exec delay Event ");
            //                     Serial.println(_waitingDelayEvent[N].codeEvent);
            pushEvent(&_waitingDelayEvent[N]);
            removeDelayEvent(_waitingDelayEvent[N].codeEvent);
          }
        }
      }

      break;



    case ev10Hz:
      //todo Gerer le clignotement de la led de vie une frequence ajustable
      if ( __div1Hz >= _pulse10Hz)
      {
        digitalWrite(_LedPulse, !LED_PULSE_ON);   // led off
      }
      break;

    case ev1Hz: {
        if (_pulse10Hz > 0) {
          digitalWrite(_LedPulse, LED_PULSE_ON);                     // allume la led
        }
        timestamp++;          // !!  les secondes sont incrementés hors interuptions
        if (timestamp % 86400L == 0) {
          //       timestamp = 0;
          pushEvent(ev24H);  // la gestion de l'overflow timestamp est a gerer par l'appli maitre si c'est utile
        }
      }
      break;
#ifdef  USE_SERIALEVENT
    case  evInChar:
      if (_stringErase) {
        inputString = "";
        _stringErase = false;
      }
      if (isPrintable(inChar) && (inputString.length() < _inputStringSizeMax)) {
        inputString += inChar;
      };
      if (inChar == '\n' || inChar == '\r') {
        _stringComplete = (inputString.length() > 0);
      }
      break;
#endif
  }
}

bool   Event::removeDelayEvent(const byte codeevent) {
  byte N = 0;
  while (N < _waitingDelayEventIndex) {
    if (_waitingDelayEvent[N].codeEvent == codeevent ) {
      //     Serial.print("Remove Delay Event ");
      //     Serial.println(codeevent);

      for (byte N2 = N; N2 < _waitingDelayEventIndex; N2++) {
        _waitingDelayEvent[N2] = _waitingDelayEvent[N2 + 1];
      }
      _waitingDelayEvent[--_waitingDelayEventIndex].codeEvent = evNill;
      _waitingDelayEvent[_waitingDelayEventIndex].longEvent = 0;
    } else {
      N++;
    }
  }
}


bool  Event::pushEvent(stdEvent* aevent) {
  if (_waitingEventIndex >= MAX_WAITING_EVENT) {
    return (false);
  }
  _waitingEvent[_waitingEventIndex++] = *aevent;
  return (true);

}

bool   Event::pushEvent(const byte codeevent, const long longEvent) {
  stdEvent aEvent;
  aEvent.codeEvent = (TypeCodeEvent)codeevent;
  aEvent.longEvent = longEvent;
  return ( pushEvent(&aEvent) );
}


bool   Event::pushEvent(const byte codeevent, const int paramevent, const int extendevent) {
  stdEvent aEvent;
  aEvent.codeEvent = (TypeCodeEvent)codeevent;
  aEvent.paramEvent = paramevent;
  aEvent.extendEvent = extendevent;
  return ( pushEvent(&aEvent) );
}

bool   Event::pushEventMillisec(const long delayMillisec, const byte codeevent, const int paramevent, const int extendevent ) {
  delayEvent aEvent;
  aEvent.codeEvent = (TypeCodeEvent)codeevent;
  aEvent.paramEvent = paramevent;
  aEvent.extendEvent = extendevent;

  removeDelayEvent(codeevent);

  if (delayMillisec == 0) {
    return ( pushEvent(&aEvent) );
  }



  if (_waitingDelayEventIndex >= MAX_WAITING_DELAYEVENT) {
    return (false);
  }

  aEvent.delay = delayMillisec / 10;
  if (aEvent.delay == 0 )  aEvent.delay = 1;


  _waitingDelayEvent[_waitingDelayEventIndex++] = aEvent;
  //  Serial.print("Pushdelay ");
  //  Serial.print(aEvent.codeEvent);
  //  Serial.print(" dans ");
  //  Serial.print(aEvent.delay);
  //  Serial.println(" msec.");

  return (true);
}


//====== Sram dispo =========
#ifndef __AVR__
int Event::freeRam () {
  return ESP.getFreeHeap();
}
#else
int Event::freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
#endif

//================ trackEvent =====================


void EventTrack::HandleEvent() {
  Event::HandleEvent();
  switch (currentEvent.codeEvent) {
    case ev1Hz:

      if (_trackTime) {
        char aBuffer[40];

#ifdef  USE_TimerOne
        snprintf(aBuffer, 40 , " %02d:%02d:%02d (%ld%%,%d) %lu", Hour(), Minute(), Second(), 101L - ((_nillEventCompteur - 100) / 10), freeRam(), _nillEventCompteur );
#else
        snprintf(aBuffer, 40 , " %02d:%02d:%02d (%ld%%,%d) %lu", Hour(), Minute(), Second(), 101L - (_nillEventCompteur / 10), freeRam(), _nillEventCompteur );
#endif
        Serial.print(aBuffer);

        if (_ev100HzMissed + _ev10HzMissed) {
          sprintf(aBuffer, " Miss:%d/%d", _ev100HzMissed, _ev10HzMissed);
          Serial.print(aBuffer);
          _ev100HzMissed = 0;
          _ev10HzMissed = 0;
        }
        Serial.println();
      }
      _nillEventCompteur = 0;
      break;

    case ev10Hz:

      _ev10HzMissed += _paramEvent - 1;
      if (_trackTime > 1 ) {

        if (_paramEvent > 1) {
          Serial.print('X');
          Serial.print(_paramEvent - 1);
        } else {
          Serial.print('|');
        }
      }
      break;

    case ev100Hz:
      _ev100HzMissed += _paramEvent - 1;

      if (_trackTime > 2)
      {

        if (_paramEvent > 1) {
          Serial.print('x');
          Serial.print(_paramEvent - 1);
        } else {
          Serial.print('_');
        }
      }
      break;
    case evInString:
      if (inputString.equals("T")) {

        if ( ++_trackTime > 3 ) {
          _trackTime = 0;
        }
        Serial.print("\nTrackTime=");
        Serial.println(_trackTime);
      }

      break;
  }


};
