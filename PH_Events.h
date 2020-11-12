/*************************************************
 *************************************************
    PH_Events.h  librairie pour les Events
    Pierre HENRY  V1.3.1 P.henry 23/04/2020
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
     V1.3.1  P.Henry 14/04/2020
   refonte du pushEvent
 *************************************************/





#pragma once
#include "Arduino.h"

#define   USE_SERIALEVENT       // remove this if you need standard Serial.read 
//#define   USE_TimerOne          // remove this if you need TimerOne on your side (work only with AVR)
//const int  FrequenceTimer = 100;  //  frequence d'interuption en Hz

#define   MAX_WAITING_EVENT      10    // max event who can be pusshed at once

#define   MAX_WAITING_DELAYEVENT  10   // max delayed event

#ifndef	LED_BUILTIN
#define LED_BUILTIN 13
#endif
// without timerone without Serial event
// P:4836 R:305  N:225.500
// without timerone with Serial event
// P:7866 R:383  N:136 400
// with  timerone without Serial event
// P:4760 R:300  N:288.500
// with timerone  with serial event
// P:7792 R:378  N:184 500










enum  TypeCodeEvent {
  evNill = 0,   // Rien (on sort d'un sleep)
  ev100Hz,      // un tick 100HZ pas de cumul en cas de retard (le nombre de 100Hz passé est dans paramEvent)
  ev10Hz,       // un tick 100Z  pas de cumul en cas de retard (le nombre de 10Hz passé est dans paramEvent)
  ev1Hz,        // un tick 1HZ   il y aura un tick pour chaque seconde
  ev24H,        // 24H depuis le boot  (gerer le rolover de timestamp si necessaire)
  evDepassement1HZ,
  evInChar,
  evInString,
  evUser = 99
};




// 5 byte structure for event
struct __attribute__((__packed__)) stdEvent  {
	byte codeEvent = evNill;
  union {
    struct {
      unsigned int   paramEvent : 16;
      unsigned int   extendEvent: 16;
    };
    struct {
      byte  byte0 : 8;
      byte  byte1 : 8;
      byte  byte2 : 8;
      byte  byte3 : 8;
    };
    struct {
      int   int0 : 16;
      int   int1 : 16;
    };
    struct {
      long   longEvent : 32;
    };
    struct {
      unsigned long   ULEvent : 32;
    };
    struct {
      float   floatEvent;
    };
  };
};

struct delayEvent : stdEvent {
  long delay;
};

class Event
{
  public:
    Event(const byte aPinNumber = LED_BUILTIN, const byte inputStringSizeMax = 30) {  // constructeur
      currentEvent.codeEvent = evNill;
      _waitingEventIndex = 0;
      //     Event:Event();
      _LedPulse = aPinNumber;
#ifdef USE_SERIALEVENT
      _inputStringSizeMax = inputStringSizeMax;
#endif
    }
    void   begin();
    byte   GetEvent(const bool sleep = true);
    void   HandleEvent();
    bool   removeDelayEvent(const byte codeevent);
    bool   pushEvent(const byte codeevent, const int paramevent = 0, const int extendevent = 0);
    bool   pushEvent(const byte codeevent, const long longEvent);
    bool   pushEvent(stdEvent* aevent);
    bool   pushEventMillisec(const long delayMillisec, stdEvent* aevent );
    bool   pushEventMillisec(const long delayMillisec, const byte codeevent,const int paramevent = 0, const int extendevent = 0 );
    void   SetPulsePercent(byte);  // durée du pulse de 0 a 100%
    int    syncroSeconde(const int millisec = 0);
    byte   Second() const;
    byte   Minute() const;
    byte   Hour()   const;
    stdEvent currentEvent;

#ifdef  USE_SERIALEVENT
    char  inChar = '\0';
    String inputString = "";
#endif
    int freeRam();
    unsigned long   timestamp = 0;   //timestamp en seconde  (environ 49 jours)

  protected:
    long      _nillEventCompteur = 0;
    byte      _LedPulse;        // Pin de la led de vie
    byte       _pulse10Hz = 1;        // 10% du temps par default
    int        _paramEvent;
    //    unsigned int   _;       // Evenement regroupés (pour ev100Hz et ev10Hz)
    byte       _waitingEventIndex = 0;
    stdEvent  _waitingEvent[MAX_WAITING_EVENT];
    byte       _waitingDelayEventIndex = 0;
    delayEvent _waitingDelayEvent[MAX_WAITING_DELAYEVENT];
#ifdef  USE_SERIALEVENT
    byte _inputStringSizeMax = 1;
    bool _stringComplete = false;
    bool _stringErase = false;
#endif

};


class EventTrack : public Event
{
  public:
	EventTrack(const byte aPinNumber = LED_BUILTIN, const byte inputStringSizeMax = 30) : Event{aPinNumber,inputStringSizeMax}  {};
    void HandleEvent();
  protected:
    byte _trackTime = 0;
    int _ev100HzMissed = 0;
    int _ev10HzMissed = 0;
};
