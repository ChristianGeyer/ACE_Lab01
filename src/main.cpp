#include <Arduino.h>
#include <NeoPixelConnect.h> 

// ### ACE 2024/2025 ###
// 
// ### Lab1 ###
//
// ### Description ###
// -Finite state machine to control led strip
// -Count Down of up to N=5 leds acting as a timer
// -Navigate the configurations using buttons or Serial_in
// 
// ### Authors ###
// Christian Geyer
// Maria Costa

#define MAXIMUM_NUM_NEOPIXELS 5
#define LED_PIN 6
int N = 5; // number of leds in countdown

NeoPixelConnect strip(LED_PIN, MAXIMUM_NUM_NEOPIXELS, pio0, 0);

// struct for a finite state machine
typedef struct{

  uint16_t state;
  uint16_t new_state;
  uint16_t tis; // time in state
  uint16_t tes; // time entering state
} fsm_t;

fsm_t fsm;

uint16_t prev_tis; // to save tis from COUNTDOWN going to FREEZE

enum state_fsm{

  START,
  COUNTDOWN,
  FREEZE,
  BLINK,
  CONFIG
};

// update state of a fsm
// update tes and tis in case the state changes
bool set_state(fsm_t &fsm1){

  if(fsm1.new_state != fsm1.state){

    fsm1.state = fsm1.new_state;
    fsm1.tes = millis();
    fsm1.tis = 0;
  }
}

// update tis of a fsm
void update_tis(fsm_t &fsm1){

  fsm1.tis = millis()-fsm1.tes;
}

// inputs
bool Sgo, Sesc, Smore;
bool Sgo_prev, Sesc_prev, Smore_prev;
bool Sgo_RE, Sesc_RE, Smore_RE, Smore_FE;
int Sgo_pin = 2, Sesc_pin = 3, Smore_pin = 4;

// struct for a timer 
// for measuring how long a boolean expression has been true
typedef struct{

  bool on;
  uint16_t tit; // time in timer
  uint16_t tet; // time entering timer
} tmr_t;

tmr_t timer_Smore, timer_control;

uint16_t T_control = 50; // time interval between control cycles
uint16_t T_countdown = 2000; // time between leds turning off during COUNTDOWN
uint16_t T_blink = 10000; // time to stay in BLINK state
uint16_t T_Smore = 3000;

// update on, tit and tet based on boolean expression (val)
void update_timer(tmr_t &tmr, bool val){

  if(val){

    if(!tmr.on){

      // start timer
      tmr.on = true;
      // set tet and tit
      tmr.tet = millis();
      tmr.tit = 0;
    }
    else{

      // update tit
      tmr.tit = millis()-tmr.tet;
    }
  }
  else{

    // reset on and tit
    tmr.on = false;
    tmr.tit = 0;
  }
}

char Serial_in = '-';

void setup(){

  Serial.begin(9600);
  pinMode(Sgo_pin, INPUT_PULLUP);
  pinMode(Sesc_pin, INPUT_PULLUP);
  pinMode(Smore_pin, INPUT_PULLUP);

  N = min(N, MAXIMUM_NUM_NEOPIXELS); // upper limit for N: MAXIMUM_NUM_NEOPIXELS

  fsm.new_state = START;
  set_state(fsm);

  timer_control.on = false;
  timer_Smore.on = false;
}

void loop(){

  // update timer_control
  update_timer(timer_control, true);
  if(timer_control.tit >= T_control){

    // reset timer_control
    update_timer(timer_control, false);
    
    // read inputs
    Sgo_prev = Sgo;
    Sesc_prev = Sesc;
    Smore_prev = Smore;
    
    Sgo = !digitalRead(Sgo_pin);
    Sesc = !digitalRead(Sesc_pin);
    Smore = !digitalRead(Smore_pin);
    
    Sgo_RE = !Sgo_prev && Sgo;
    Sesc_RE = !Sesc_prev && Sesc;
    Smore_RE = !Smore_prev && Smore;
    Smore_FE = Smore_prev && !Smore;

    // update timers
    update_timer(timer_Smore, Smore);
    update_tis(fsm);

    // Serial input
    if(Serial.available()){

      Serial_in = Serial.read();
    }
    else{

      Serial_in = '-';
    }

    // finite state machine
    if((fsm.state == START) && (Sgo_RE || (Serial_in == 'g'))){

      fsm.new_state = COUNTDOWN;
    }
    else if((fsm.state == COUNTDOWN) && (Sesc_RE || (Serial_in == 'p'))){

      prev_tis = fsm.tis;
      fsm.new_state = FREEZE;
    }
    else if((fsm.state == COUNTDOWN) && (Sgo_RE || (Serial_in == 'g') || (fsm.tis >= N*T_countdown))){

      fsm.new_state = BLINK;
    }
    else if((fsm.state == FREEZE) && (Sesc_RE || (Serial_in == 'r'))){

      fsm.tes = millis()-prev_tis;
      fsm.new_state = COUNTDOWN;
    }
    else if((fsm.state == BLINK) && (Sgo_RE || (Serial_in == 'g') || (fsm.tis >= T_blink))){

      fsm.new_state = START;
    }
    else if((fsm.state == CONFIG) && (Sesc_RE || (Serial_in == 'e'))){

      fsm.new_state = START;
      // TODO - discard new config
    }
    else if((fsm.state == CONFIG) && ((timer_Smore.tit >= T_Smore) || (Serial_in == 's'))){

      fsm.new_state = START;
      // TODO - save new config
    }
    else if((fsm.state != CONFIG) && ((timer_Smore.tit >= T_Smore) || (Serial_in == 'c'))){

      fsm.new_state = CONFIG;
    }

    // update fsm state
    set_state(fsm);

    // print state for debugging

    for(int i = 0 ; i < N ; i++){

      strip.neoPixelSetValue(i, 0, 0, 0);
    }
    strip.neoPixelSetValue(fsm.state, 50, 0, 0);
    strip.neoPixelShow();
    Serial.println(fsm.state);
  }
}