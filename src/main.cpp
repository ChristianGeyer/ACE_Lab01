#include <Arduino.h>
#include <NeoPixelConnect.h> 

enum state_fsm{

    NORMAL,
    CONFIG,
    DT1,
    EFFECT,
    COLOR,
    START,
    COUNT_DOWN,
    BLINK,
    RUNNING,
    FREEZE
};

enum dT_mode{

  DT_1,
  DT_2,
  DT_5,
  DT_10
};

enum effect_mode{

  SWITCH_OFF,
  FAST_BLINK,
  FADE_OUT
};

enum color_mode{

  WHITE_,
  GREEN_,
  BLUE_,
  YELLOW_
};

enum color_mode cm, new_cm;
enum effect_mode em, new_em;
enum dT_mode dm, new_dm;
enum color_mode cm_config;

void print_state(int state){

  switch(state){

    case 0:
      Serial.print("NORMAL");
      break;
    case 1:
      Serial.print("CONFIG");
      break;
    case 2:
      Serial.print("DT1");
      break;
    case 3:
      Serial.print("EFFECT");
      break;
    case 4:
      Serial.print("COLOR");
      break;
    case 5:
      Serial.print("START");
      break;
    case 6:
      Serial.print("COUNT_DOWN");
      break;
    case 7:
      Serial.print("BLINK");
      break;
    case 8:
      Serial.print("RUNNING");
      break;
    case 9:
      Serial.print("FREEZE");
      break;
  } 
}

void print_dm(int dm){

  switch(dm){

    case 0:
      Serial.print("DT_1");
      break;
    case 1:
      Serial.print("DT_2");
      break;
    case 2:
      Serial.print("DT_5");
      break;
    case 3:
      Serial.print("DT_10");
      break;
  }
}

void print_em(int em){

  switch(em){

    case 0:
      Serial.print("SWITCH_OFF");
      break;
    case 1:
      Serial.print("FAST_BLINK");
      break;
    case 2:
      Serial.print("FADE_OUT");
      break;
  }
}

void print_cm(int cm){

  switch(cm){

    case 0:
      Serial.print("WHITE");
      break;
    case 1:
      Serial.print("GREEN");
      break;
    case 2:
      Serial.print("BLUE");
      break;
    case 3:
      Serial.print("YELLOW");
      break;
  }
}

typedef struct{

    enum state_fsm state;
    enum state_fsm new_state;
    uint16_t tis;
    uint16_t tes;
} fsm_t;

fsm_t fsm1, fsm2, fsm3, fsm4;

bool set_state(fsm_t& fsm){

    if(fsm.state != fsm.new_state){

        fsm.state = fsm.new_state;
        fsm.tis = 0;
        fsm.tes = millis();
        return true;
    }
    return false;
}

bool blink(uint16_t time, double blink_frequency, bool condition){

    if(condition == false) return false;

    double half_blink_period = 1000/(2*blink_frequency);
    return (((int)(time/half_blink_period))%2) == 0;
}

uint16_t prev_time;
uint16_t curr_time;
uint16_t delta_time;
uint16_t control_period = 10; // ms

uint16_t dT1; // time between leds going out in COUNT_DOWN state
uint16_t dT2; // time in BLINK state
uint16_t dT_config; // time to press Smore to go in and out of CONFIG state
uint16_t Smore_t; // time Smore has been pressed
double f_config; // blinking frequency in CONFIG state
double f_blink; // blinking frequency in BLINK state
double f_freeze; // blinking frequency in FREEZE state
double f_fast; // fast blinking in COUNT_DOWN

int Sgo_pin = 2;
int Sesc_pin = 3;
int Smore_pin = 4;

bool Sgo;
bool Sesc;
bool Smore;

bool Sgo_prev;
bool Sesc_prev;
bool Smore_prev;

bool Sgo_RE;
bool Sesc_RE;
bool Smore_RE;

bool led1, led2, led3, led4, led5;
int led1r, led1g, led1b;
int led2r, led2g, led2b;
int led3r, led3g, led3b;
int led4r, led4g, led4b;
int led5r, led5g, led5b;

char Serial_in;

bool state_changed;

#define MAXIMUM_NUM_NEOPIXELS 5
#define LED_PIN 6 // strip input

// Create an instance of NeoPixelConnect and initialize it
// to use GPIO pin 22 as the control pin, for a string
// of 8 neopixels. Name the instance p
NeoPixelConnect strip(LED_PIN, MAXIMUM_NUM_NEOPIXELS, pio0, 0);

void setup(){

    Serial.begin(9600);

    pinMode(Sgo_pin, INPUT_PULLUP);
    pinMode(Sesc_pin, INPUT_PULLUP);
    pinMode(Smore_pin, INPUT_PULLUP);

    // inicializar fsm
    fsm1.new_state = NORMAL;
    set_state(fsm1);
    fsm2.new_state = DT1;
    set_state(fsm2);
    fsm3.new_state = START;
    set_state(fsm3);
    fsm4.new_state = RUNNING;
    set_state(fsm4);
    
    // inicializar variaveis
    prev_time = 0;
    curr_time = 0;
    delta_time = 0;
    
    dm = DT_2;
    em = SWITCH_OFF;
    cm = GREEN_;

    new_dm = dm;
    new_em = em;
    new_cm = cm;

    dT2 = 10000;
    dT_config = 3000;
    Smore_t = 0;

    f_config = 1;
    f_blink = 2;
    f_freeze = 2;
    f_fast = 5;

    Serial_in = '-';
}

void loop(){

  
    // obter tempo atual
    curr_time = millis();
    delta_time = curr_time - prev_time;
    // testar se ja passou o periodo de controlo
    if(delta_time >= control_period){

        // Serial.print("millis():");
        // Serial.println(millis());
        // atualizar prev_time
        prev_time = curr_time;

        // atualizar tempo das fsm
        fsm1.tis += delta_time;
        fsm2.tis += delta_time;
        if(!((fsm3.state == COUNT_DOWN) && (fsm4.state == FREEZE))) fsm3.tis += delta_time;
        fsm4.tis += delta_time;

        // ler entrada Serial
        if(Serial.available()){

          Serial_in = Serial.read();
        }
        else{

          Serial_in = '-';
        }

        // ler entradas
        Sgo_prev = Sgo;
        Sesc_prev = Sesc;
        Smore_prev = Smore;

        Sgo = !digitalRead(Sgo_pin);
        Sesc = !digitalRead(Sesc_pin);
        Smore = !digitalRead(Smore_pin);

        // detetar flancos
        Sgo_RE = !Sgo_prev && Sgo;
        Sesc_RE = !Sesc_prev && Sesc;
        Smore_RE = !Smore_prev && Smore;

        // atualizar Smore_t
        if(Smore) Smore_t += delta_time;
        else Smore_t = 0;

        // atualizar estados
        // fsm1: {NORMAL, CONFIG}
        if((fsm1.state == NORMAL) && ((Smore_t >= dT_config) || (Serial_in == 'c'))){

            Smore_t = 0;
            fsm1.new_state = CONFIG;
            fsm2.new_state = DT1;
            cm_config = WHITE_;
            new_dm = dm;
            new_em = em;
            new_cm = cm;
        }
        else if((fsm1.state == CONFIG) && ((Smore_t >= dT_config) || (Serial_in == 's'))){

            // save configuration
            em = new_em;
            cm = new_cm;
            dm = new_dm;

            // reset Smore_t
            Smore_t = 0;

            fsm1.new_state = NORMAL;
            fsm3.new_state = START;
        }
        else if((fsm1.state == CONFIG) && (Sesc_RE || (Serial_in == 'e'))){

            // discard configuration

            // reset Smore_t
            Smore_t = 0;

            fsm1.new_state = NORMAL;
            fsm3.new_state = START;
        }

        // fsm2: {DT1, EFFECT, COLOR}
        if(fsm1.state == CONFIG){

            if((fsm2.state == DT1) && (Smore_RE || (Serial_in == 'n'))){

                fsm2.new_state = EFFECT;
                cm_config = GREEN_;
            }
            else if((fsm2.state == EFFECT) && (Smore_RE || (Serial_in == 'n'))){

                fsm2.new_state = COLOR;
                cm_config = BLUE_;
            }
            else if((fsm2.state == COLOR) && (Smore_RE || (Serial_in == 'n'))){

                fsm2.new_state = DT1;
                cm_config = WHITE_;
            }
            else if((fsm2.state == DT1) && (Serial_in == '1')){

              new_dm = DT_1;
            }
            else if((fsm2.state == DT1) && (Serial_in == '2')){

              new_dm = DT_2;
            }
            else if((fsm2.state == DT1) && (Serial_in == '5')){

              new_dm = DT_5;
            }
            else if((fsm2.state == DT1) && Sgo_RE){

              switch(new_dm){

                case DT_1:
                  new_dm = DT_2;
                  break;
                case DT_2:
                  new_dm = DT_5;
                  break;
                case DT_5:
                  new_dm = DT_10;
                  break;
                case DT_10:
                  new_dm = DT_1;
                  break;
              }
            }
            else if((fsm2.state == EFFECT) && (Serial_in == 'o')){

              new_em = SWITCH_OFF;
            }
            else if((fsm2.state == EFFECT) && (Serial_in == 'b')){

              new_em = FAST_BLINK;
            }
            else if((fsm2.state == EFFECT) && (Serial_in == 'f')){

              new_em = FADE_OUT;
            }
            else if((fsm2.state == EFFECT) && Sgo_RE){

              switch(new_em){

                case SWITCH_OFF:
                  new_em = FAST_BLINK;
                  break;
                case FAST_BLINK:
                  new_em = FADE_OUT;
                  break;
                case FADE_OUT:
                  new_em = SWITCH_OFF;
                  break;
              }
            }
            else if((fsm2.state == COLOR) && (Serial_in == 'b')){

              new_cm = BLUE_;
            }
            else if((fsm2.state == COLOR) && (Serial_in == 'g')){

              new_cm = GREEN_;
            }
            else if((fsm2.state == COLOR) && (Serial_in == 'y')){

              new_cm = YELLOW_;
            }
            else if((fsm2.state == COLOR) && (Serial_in == 'w')){

              new_cm = WHITE_;
            }
            else if((fsm2.state == COLOR) && Sgo_RE){

              switch(new_cm){

                case WHITE_:
                  new_cm = GREEN_;
                  break;
                case GREEN_:
                  new_cm = BLUE_;
                  break;
                case BLUE_:
                  new_cm = YELLOW_;
                  break;
                case YELLOW_:
                  new_cm = WHITE_;
                  break;
              }
            }
        }

        // fsm3: {START, COUNT_DOWN, BLINK}
        if(fsm1.state == NORMAL){

          // diminuir tis de COUNT_DOWN
          if((fsm3.state == COUNT_DOWN) && (Smore_RE || (Serial_in == 'm'))){

            if(fsm3.tis < dT1) fsm3.tis = 0;
            else fsm3.tis -= dT1;
          }

          if((fsm3.state == START) && (Sgo_RE || (Serial_in == 'g'))){

            fsm3.new_state = COUNT_DOWN;
            fsm4.new_state = RUNNING;
          }
          else if((fsm3.state == COUNT_DOWN) && (Sgo_RE || (Serial_in == 'g'))){

            fsm3.new_state = START;
          }
          else if((fsm3.state == COUNT_DOWN) && (fsm3.tis >= 5*dT1)){

            fsm3.new_state = BLINK;
          }
          else if((fsm3.state == BLINK) && ((fsm3.tis >= dT2) || Sgo_RE || (Serial_in == 'g'))){

            fsm3.new_state = START;
          }

          // fsm4: {RUNNING, FREEZE}
          if(fsm3.state == COUNT_DOWN){

            if((fsm4.state == RUNNING) && (Sesc_RE || (Serial_in == 'p'))){

                fsm4.new_state = FREEZE;
            }
            else if((fsm4.state == FREEZE) && (Sesc_RE || (Serial_in == 'r'))){

                fsm4.new_state = RUNNING;
            }
          }
        }

        // atualizar estado das fsm
        state_changed = false;
        state_changed = set_state(fsm1) || state_changed;
        state_changed = set_state(fsm2) || state_changed;
        state_changed = set_state(fsm3) || state_changed;
        state_changed =  set_state(fsm4) || state_changed;

        // calcular saidas
        if(fsm1.state == NORMAL){

          if(fsm3.state == START){

            led1 = false;
            led2 = false;
            led3 = false;
            led4 = false;
            led5 = false;
          }
          else if(fsm3.state == COUNT_DOWN){
            
            switch(cm){

              case WHITE_:
                led1r = 100; led1g = 100; led1b = 100;
                led2r = 100; led2g = 100; led2b = 100;
                led3r = 100; led3g = 100; led3b = 100;
                led4r = 100; led4g = 100; led4b = 100;
                led5r = 100; led5g = 100; led5b = 100;
                break;
              case GREEN_:
                led1r = 0; led1g = 100; led1b = 0;
                led2r = 0; led2g = 100; led2b = 0;
                led3r = 0; led3g = 100; led3b = 0;
                led4r = 0; led4g = 100; led4b = 0;
                led5r = 0; led5g = 100; led5b = 0;
                break;
              case YELLOW_:
                led1r = 100; led1g = 100; led1b = 0;
                led2r = 100; led2g = 100; led2b = 0;
                led3r = 100; led3g = 100; led3b = 0;
                led4r = 100; led4g = 100; led4b = 0;
                led5r = 100; led5g = 100; led5b = 0;
                break;
              case BLUE_:
                led1r = 0; led1g = 0; led1b = 100;
                led2r = 0; led2g = 0; led2b = 100;
                led3r = 0; led3g = 0; led3b = 100;
                led4r = 0; led4g = 0; led4b = 100;
                led5r = 0; led5g = 0; led5b = 100;
                break;
            }

            if(fsm4.state == RUNNING){

              switch(dm){

                case DT_1:
                  dT1 = 1000;
                  break;
                case DT_2:
                  dT1 = 2000;
                  break;
                case DT_5:
                  dT1 = 5000;
                  break;
                case DT_10:
                  dT1 = 10000;
                  break;
              }
              switch(em){

                case FADE_OUT:
                  
                  if(fsm3.tis < dT1){

                    led1r = (int)(led1r - led1r * fsm3.tis/dT1);
                    led1g = (int)(led1g - led1g * fsm3.tis/dT1);
                    led1b = (int)(led1b - led1b * fsm3.tis/dT1);
                  }
                  else if(fsm3.tis < 2*dT1){

                    led2r = (int)(led2r - led2r * (fsm3.tis-dT1)/dT1);
                    led2g = (int)(led2g - led2g * (fsm3.tis-dT1)/dT1);
                    led2b = (int)(led2b - led2b * (fsm3.tis-dT1)/dT1);
                  }
                  else if(fsm3.tis < 3*dT1){

                    led3r = (int)(led3r - led3r * (fsm3.tis-2*dT1)/dT1);
                    led3g = (int)(led3g - led3g * (fsm3.tis-2*dT1)/dT1);
                    led3b = (int)(led3b - led3b * (fsm3.tis-2*dT1)/dT1);
                  }
                  else if(fsm3.tis < 4*dT1){

                    led4r = (int)(led4r - led4r * (fsm3.tis-3*dT1)/dT1);
                    led4g = (int)(led4g - led4g * (fsm3.tis-3*dT1)/dT1);
                    led4b = (int)(led4b - led4b * (fsm3.tis-3*dT1)/dT1);
                  }
                  else if(fsm3.tis < 5*dT1){

                    led5r = (int)(led5r - led5r * (fsm3.tis-4*dT1)/dT1);
                    led5g = (int)(led5g - led5g * (fsm3.tis-4*dT1)/dT1);
                    led5b = (int)(led5b - led5b * (fsm3.tis-4*dT1)/dT1);
                  }

                case SWITCH_OFF:
                  led1 = fsm3.tis < dT1;
                  led2 = fsm3.tis < 2*dT1;
                  led3 = fsm3.tis < 3*dT1;
                  led4 = fsm3.tis < 4*dT1;
                  led5 = fsm3.tis < 5*dT1;
                  break;
                case FAST_BLINK:
                  led1 = fsm3.tis < dT1;
                  led2 = fsm3.tis < 2*dT1;
                  led3 = fsm3.tis < 3*dT1;
                  led4 = fsm3.tis < 4*dT1;
                  led5 = fsm3.tis < 5*dT1;
                  if(!led1) {led1r = 100; led1g = 0; led1b = 0;}
                  if(!led2) {led2r = 100; led2g = 0; led2b = 0;}
                  if(!led3) {led3r = 100; led3g = 0; led3b = 0;}
                  if(!led4) {led4r = 100; led4g = 0; led4b = 0;}
                  if(!led5) {led5r = 100; led5g = 0; led5b = 0;}
                  led1 = (fsm3.tis < dT1) || blink(fsm3.tis, f_fast, fsm3.tis > dT1);
                  led2 = (fsm3.tis < 2*dT1) || blink(fsm3.tis, f_fast, fsm3.tis > 2*dT1);
                  led3 = (fsm3.tis < 3*dT1) || blink(fsm3.tis, f_fast, fsm3.tis > 3*dT1);
                  led4 = (fsm3.tis < 4*dT1) || blink(fsm3.tis, f_fast, fsm3.tis > 4*dT1);
                  led5 = (fsm3.tis < 5*dT1);
                  break;
              }
            }  
            else if(fsm4.state == FREEZE){

              led1 = blink(fsm4.tis, f_freeze, fsm3.tis < dT1);
              led2 = blink(fsm4.tis, f_freeze, fsm3.tis < 2*dT1);
              led3 = blink(fsm4.tis, f_freeze, fsm3.tis < 3*dT1);
              led4 = blink(fsm4.tis, f_freeze, fsm3.tis < 4*dT1);
              led5 = blink(fsm4.tis, f_freeze, fsm3.tis < 5*dT1);
            }          
          }
          else if(fsm3.state == BLINK){

            led1r = 100; led1g = 0; led1b = 0;
            led2r = 100; led2g = 0; led2b = 0;
            led3r = 100; led3g = 0; led3b = 0;
            led4r = 100; led4g = 0; led4b = 0;
            led5r = 100; led5g = 0; led5b = 0;
            led1 = blink(fsm3.tis, f_blink, true);
            led2 = blink(fsm3.tis, f_blink, true);
            led3 = blink(fsm3.tis, f_blink, true);
            led4 = blink(fsm3.tis, f_blink, true);
            led5 = blink(fsm3.tis, f_blink, true);
          }
        }
        else if(fsm1.state == CONFIG){
          
          led1 = false;
          led2 = false;
          led3 = false;
          led4 = false;
          led5 = blink(fsm1.tis, f_config, true);
          if(fsm2.state == DT1){

            led1r = 100; led1g = 0; led1b = 0;
            led2r = 100; led2g = 0; led2b = 0;
            led3r = 100; led3g = 0; led3b = 0;
            led4r = 100; led4g = 0; led4b = 0;
            led5r = 100; led5g = 0; led5b = 0;
            switch(new_dm){

              case DT_10:
                led4 = true;
              case DT_5:
                led3 = true;
              case DT_2:
                led2 = true;
              case DT_1:
                led1 = true;
            }
          }
          else if(fsm2.state == EFFECT){

            led1r = 0; led1g = 100; led1b = 0;
            led2r = 0; led2g = 100; led2b = 0;
            led3r = 0; led3g = 100; led3b = 0;
            led4r = 0; led4g = 100; led4b = 0;
            led5r = 0; led5g = 100; led5b = 0;
            switch(new_em){

              case FADE_OUT:
                led3 = true;
              case FAST_BLINK:
                led2 = true;
              case SWITCH_OFF:
                led1 = true;
            }
          }
          else if(fsm2.state == COLOR){

            led1r = 0; led1g = 0; led1b = 100;
            led2r = 0; led2g = 0; led2b = 100;
            led3r = 0; led3g = 0; led3b = 100;
            led4r = 0; led4g = 0; led4b = 100;
            led5r = 0; led5g = 0; led5b = 100;
            switch(new_cm){

              case YELLOW_:
                led4 = true;
              case BLUE_:
                led3 = true;
              case GREEN_:
                led2 = true;
              case WHITE_:
                led1 = true;
            }
          }
        }

        // control led strip
        for(int i = 0 ; i < MAXIMUM_NUM_NEOPIXELS ; i++){

          strip.neoPixelSetValue(i, 0, 0, 0);
        }

        if(led1) strip.neoPixelSetValue(4, led1r, led1g, led1b);
        if(led2) strip.neoPixelSetValue(3, led2r, led2g, led2b);
        if(led3) strip.neoPixelSetValue(2, led3r, led3g, led3b);
        if(led4) strip.neoPixelSetValue(1, led4r, led4g, led4b);
        if(led5) strip.neoPixelSetValue(0, led5r, led5g, led5b);

        strip.neoPixelShow();

        if((Serial_in != '-') || state_changed){

          Serial.println(Serial_in);
          Serial.print("fsm1:");
          print_state(fsm1.state);
          Serial.print("  fsm2:");
          print_state(fsm2.state);
          Serial.print("  fsm3:");
          print_state(fsm3.state);
          Serial.print("  fsm4:");
          print_state(fsm4.state);
          Serial.println();
          Serial.print("CONFIG:     dT1:");
          print_dm(dm);
          Serial.print("  EFFECT:");
          print_em(em);
          Serial.print("  COLOR:");
          print_cm(cm);
          Serial.println();
          Serial.print("NEW CONFIG: dT1:");
          print_dm(new_dm);
          Serial.print("  EFFECT:");
          print_em(new_em);
          Serial.print("  COLOR:");
          print_cm(new_cm);
          Serial.println();
        }
        // debug
        
        // Serial.print("led1:");
        // Serial.print(led1);
        // Serial.print("led2:");
        // Serial.print(led2);
        // Serial.print("led3:");
        // Serial.print(led3);
        // Serial.print("led4:");
        // Serial.print(led4);
        // Serial.print("led5:");
        // Serial.println(led5);
        // Serial.print("fsm1:");
        // print_state(fsm1.state);
        // Serial.print(":");
        // Serial.print(fsm1.tis);
        // Serial.print("   fsm2:");
        // print_state(fsm2.state);

        // Serial.print("   fsm3:");
        // print_state(fsm3.state);
        // Serial.print(":");
        // Serial.print(fsm3.tis);
        // Serial.print("   fsm4:");
        // print_state(fsm4.state);
        // Serial.println("");
        // Serial.print("color:");
        // Serial.print(new_cm);
        // Serial.print(" ");
        // Serial.println(cm);
    }

  
  // control led strip
}