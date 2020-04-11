#include <stdlib.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <U8g2lib.h>
//OLED
//U8G2_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);  // I2C / TWI 
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

//WS2812 LED
#define PIXEL_PIN 8
Adafruit_NeoPixel strip = Adafruit_NeoPixel(6, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Midi(24bit) : command | note | velocity
// velocity must be between 0 and 127, higher louder
 int velocity = 100;
 int noteON = 144;//144 = 1001 0000 in binary, note on command
 int noteOFF = 128;//128 = 1000 0000 , note off
 int afterTouch = 160;//160 = 1010 0000
 int pitchWheel = 224;//1110 0000
 
// note 1-6 for D2-7
 int notes[6];
 int noteStat = 0;
 int noteMidi = 0;
 int lastNoteStat = 0;
 int lastNoteMidi = 0;
 int lastVelocity = 0;
 int lastOnPlay =0;
 int onPlay = 0; //if blowing
 
// keySelect 0-11: C~B
 int keySelect = 0;
 int onKeySelect = 0;

// 8va 8vb
 int vavb = 0;
 boolean on8va = false;
 boolean on8vb = false;
 boolean tmp8va = false;
 boolean tmp8vb = false;
 int a6;
 
// flat
 int onFlat = 0;
 int onSharp = 0;

 boolean onPress = false;

//breath
 float breathLOW = 0.9;
 float breathHIGH = 3.8;
 float breath8VA = 4.5;
 float k,k2;
 
//pitch wheel
 int wheel;
 
void setup() {
  Wire.begin();
  Serial.begin(9600);
  
  u8g2.begin();  
  
  //WS2812 LEDs
  strip.begin();
  strip.show();

  //  listen port 2-9
  pinMode(2,INPUT_PULLUP);
  pinMode(3,INPUT_PULLUP);
  pinMode(4,INPUT_PULLUP);
  pinMode(5,INPUT_PULLUP);
  pinMode(6,INPUT_PULLUP);
  pinMode(7,INPUT_PULLUP);
  //LED control
  pinMode(8,OUTPUT);
  //sharp,flat
  pinMode(9,INPUT_PULLUP);
  pinMode(10,INPUT_PULLUP);
  //vavb
  pinMode(11,INPUT_PULLUP);
  pinMode(12,INPUT_PULLUP);
  //key select
  pinMode(13, INPUT_PULLUP);

  //breath
  k = 127.0/(breathHIGH-breathLOW);
  k2 = 127.0/(breath8VA-breathHIGH);

  u8g2.clear();
  u8g2.clearBuffer();
  
  u8g2.firstPage();  
  do {
  draw();
  } while( u8g2.nextPage() );
}

void loop() {
  delay(10);
  velocity = getVelocity();
  velocity = expFliter(velocity,150.0,25.0);
  //Serial.write(analogRead(A3));
  //return;
  getButtonStats();

  //note stat
  if(!digitalRead(5)){notes[0] = 1;}
  if(!digitalRead(6)){notes[1] = 1;} 
  if(!digitalRead(7)){notes[2] = 1;} 
  if(!digitalRead(2)){notes[3] = 1;} 
  if(!digitalRead(3)){notes[4] = 1;} 
  if(!digitalRead(4)){notes[5] = 1;} 
  noteStat = getNoteStat(notes);
  noteMidi = getNoteMidi(noteStat,vavb,onFlat,onSharp,keySelect);
  
  //lights
  colorWipeRandom(50,notes,false); 
  //for(int i = 0;i<6;i++) notes[i] = 1;
  //colorWipe(strip.Color(0,0,7),50,notes,false);
  
  if(onPlay){
    if(onKeySelect){
      // select key
      keySelect = selectKey(noteStat,onFlat);
    } else {
      if(lastOnPlay){
        if(lastNoteMidi != noteMidi){
          //last note ends but we do have a new note
          MIDImessage(noteOFF,lastNoteMidi,0);
          MIDImessage(noteON,noteMidi,75);
        } else{
          // last note still playing, but needs to send a new velocity
          //MIDImessage_ctrl(208,velocity);
          //wheel = pitchMap(analogRead(A2));
          //MIDImessage(pitchWheel,0,wheel);
          MIDImessage(afterTouch,lastNoteMidi,75);
        }
      } else{
        // new note begins, last noet ends
        MIDImessage(noteOFF,lastNoteMidi,0);
        MIDImessage(noteON,noteMidi,75);
      }
    }
  } else{
    if(lastOnPlay){
      //last note ends  
      MIDImessage(noteOFF,lastNoteMidi,0);
    } else{
      //not playing,send nothing
    }  
  }

  lastNoteStat = noteStat;
  lastNoteMidi = noteMidi;
  lastOnPlay = onPlay;
  lastVelocity = velocity;
  
  noteReset();
}
void draw(void) {
  // graphic commands to redraw the complete screen should be placed here  
  u8g2.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb21);
  u8g2.drawStr( 0, 10, "Midi saxphone");
  u8g2.drawStr( 0, 30, "Key select: C");
  u8g2.drawStr( 0, 50, "Ottava: 0");
}
void drawStat(int _key, int _ottava){
  u8g2.clearDisplay();
  u8g2.setFont(u8g_font_unifont);
  u8g2.drawStr( 0, 10, "Midi saxphone");
  //TODO
  u8g2.drawStr( 0, 30, "Key select: C");
  u8g2.drawStr( 0, 50, "Ottava: 0");
}
int pitchMap(int input){
  int output;
  if(input > 550){
    output = (int)((63/473*(input-550))+64);
    output = 127;
  } else if(input<490){
    output = (int)(63/490*input);
  } else output = 64;
  if(output > 127) output = 127;
  if(output < 0) output = 0;
  return output;
}
//press to velocity
int getVelocity(){
  int xgzpPin = analogRead(A3);
  float _press = xgzpPin * (5.0 / 1023.0);
  int _velocity = 0;
  _velocity = press2Velo(_press);
  return _velocity;
}
int press2Velo(float _press){
  int _velocity;
  if(_press<=breathLOW){
    _velocity = 0;  
    onPlay = 0;
  } 
  else if(_press<breathHIGH){
    _velocity = (int)(_press * k - k * breathLOW);
     onPlay = 1;
  }
  else {
    _velocity = 127;
    onPlay = 1;
  }
  if (_velocity <0) {_velocity = 0;onPlay = 0;}
  if (_velocity >127) _velocity = 127;
  return _velocity;
}

// helper function to enhance the velocity 
//(sometimes the sensor may get a low input value, which we should magnify it depends on how we design the mouthpiece)
// a and b controls how easy we can reach the max output
int expFliter(int _velocity,float a,float b){
  _velocity = (int)(a*_velocity/(b+_velocity));
  if (_velocity <0) {_velocity = 0;onPlay = 0;}
  if (_velocity >127) _velocity = 127;
  return _velocity;
}

//send MIDI message
void MIDImessage(int command, int MIDInote, int MIDIvelocity) {
  //debug
  //Serial.println("midiINFO");
  //Serial.print(command);Serial.print(" ");Serial.print(MIDInote);Serial.print(" ");Serial.print(MIDIvelocity);delay(100);
  //return;
  Serial.write(command);//send note on or note off command 
  Serial.write(MIDInote);//send pitch data
  Serial.write(MIDIvelocity);//send velocity data
}
void MIDImessage_ctrl(int command, int info){
  Serial.write(command);
  Serial.write(info);
}

//LED control
void colorWipe(uint32_t c, uint8_t wait,int* _notes,bool _hold) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    if(_notes[i]==1) {
      if(i>2) strip.setPixelColor(i, c); else strip.setPixelColor(2-i, c); // i made a mistake on hardware connection.. 
    } else{
      if(i>2) strip.setPixelColor(i, strip.Color(0, 0, 0)); else strip.setPixelColor(2-i, strip.Color(0, 0, 0));
    }
  }
    
  strip.show();
  if(_hold) delay(wait);
}

//random color
void colorWipeRandom(uint8_t wait,int* _notes,bool _hold) {
  int g,b,r;
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    if(_notes[i]==1) {
      int g = random(15);int b = random(15);int r = random(15);
      uint32_t c = strip.Color(r, g, b);
      if(i>2) strip.setPixelColor(i, c); else strip.setPixelColor(2-i, c); // i made a mistake on hardware connection.. 
    } else{
      if(i>2) strip.setPixelColor(i, strip.Color(0, 0, 0)); else strip.setPixelColor(2-i, strip.Color(0, 0, 0));
    }
  } 
  strip.show();
  if(_hold) delay(wait);
}

//note stat
int getNoteStat(int* _notes){
  int stat = _notes[0]+2*_notes[1]+4*_notes[2]+8*_notes[3]+16*_notes[4]+32*_notes[5];
  if(stat!=31){
    stat = 0;
    for(int i=5;i>=0;i--){
      if(_notes[i]!=0) stat += notes[i]*pow(2,i);
      else break;
    }
  }
  
  switch(stat){
    case 31:stat=12;break; //i
    case 0 :stat=11;break; //7
    case 32:stat=9;break;  //...
    case 48:stat=7;break;
    case 56:stat=5;break;
    case 60:stat=4;break;
    case 62:stat=2;break;
    case 63:stat=0;break;  //1
    default:stat=11;
  }
  return stat;
}
//helper function
int pow(int _a,int _b){
  int _s = 1;
  for (;_b>0;_b--) _s*=_a;
  return _s;
}
int getNoteMidi(int _stat,int _vavb,int _onFlat,int _onSharp,int _keySelect){
  int stat = _stat;
  if(_onFlat) stat-=1; //if flat, 1 key lower
  if(_onSharp) stat+=1;
  stat +=60; //C4 by default
  stat += _vavb*12; //8va or 8vb
  stat += _keySelect; //which key to use
  return stat;
}
void noteReset(){
  for(int i=0;i<6;i++){notes[i]=0;}
}
int selectKey(int _stat,int _onFLat){
  if(_onFLat) _stat-=1;
  if(_stat == -1) _stat = 11;
  return _stat;
}

void getButtonStats(){
  if(!digitalRead(12)){onKeySelect = 1;} else onKeySelect = 0;
  if(!digitalRead(9)){onSharp = 1;} else onSharp = 0;
  if(!digitalRead(10)){onFlat = 1;} else onFlat = 0;
  /*if(!digitalRead(11)){
    if(!on8va){
      if(vavb<1) vavb+=1;
      on8va = true;
    }
  } else on8va = false;

  if(!digitalRead(12)){
    if(!on8vb){
      if(vavb>-1) vavb-=1;
      on8vb = true;
    }
  } else on8vb = false;*/
  
  int a2 = analogRead(A2);
  int a1 = analogRead(A1);
  //Serial.write(a1);
  if(a1<200){
    //if(vavb>-1 && !tmp8vb) vavb-=1;
    //tmp8vb = true;
    vavb=-1;
  } else if(a2>900){
    //if(vavb<1 && !tmp8va) vavb+=1;
    //tmp8va = true;
    vavb=1;
  } else{
    //if(vavb>-1 && tmp8va) vavb-=1;
    //if(vavb<1 && tmp8vb) vavb+=1;
    //tmp8va = false;
    //tmp8vb = false;
    vavb=0;
  }
}
