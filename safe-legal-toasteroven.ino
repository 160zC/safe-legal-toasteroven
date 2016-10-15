#define SOAKMIN 100
#define SOAKTIME 100
#define SOAKMAX 150
#define LIQUIDUS 183
#define LIQUIDTIME 100
#define RAMPUP 2
#define RAMPDOWN 4
#define PEAK 220
#define DECAYWEIGHTING 0.8 //this is used to smooth the calculated rate must be less than 1 . 
                          // It performs a weighted average with recent measurements and exponential decay. smaller numbers are quicker decay.


#include "max6675.h"// www.ladyada.net/learn/sensors/thermocouple
//for the thermocouple
byte thermoDO = 6; //SPI serial
byte thermoCS = 5;  //SPI chip select
byte thermoCLK = 4; //SPI clock
byte vccPin = 3;//these pins power the thermocouple chip
byte gndPin = 2;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

//for the remote
byte onPin= A0;//connect to the on button on the remote
byte offPin=A1;//connect to the off button on the remote

//for the potentiometer
byte potPin=A4;//three pins are used for the potentiometer so we can plug it straight in
byte potHigh=A3;//5v
byte potLow=A5;//0v

byte buzzPin=12;

//
int minTime=500;// there is a limit to how quickly the remote can switch the power
int buttonTime=10;//how long for the arduino to activate the button on the remote
int heatknob; //store the level of the pot to control the heat in phase 2
float ontime;
float offtime;
float temp;     //these could be optimised to avoid floats by storing temperature in millidegrees
float lastTemp;
long millisecs;
long lastMillisecs;
float rate=1; //rate of temperature rise
int heatlevel=0;
int timethisphase=0;
int timephasestart=0;
byte phase=1;


  
void setup() {
  Serial.begin(9600);
  pinMode(vccPin, OUTPUT); digitalWrite(vccPin, HIGH);
  pinMode(gndPin, OUTPUT); digitalWrite(gndPin, LOW);
  pinMode(potHigh, OUTPUT); digitalWrite(potHigh, HIGH);
  pinMode(potLow, OUTPUT); digitalWrite(potLow, LOW);  
  pinMode(potPin, INPUT);
  pinMode(onPin, OUTPUT);
  pinMode(offPin, OUTPUT);
  pinMode(buzzPin, OUTPUT); digitalWrite(buzzPin, LOW);
   millisecs=millis(); 
   temp = thermocouple.readCelsius();//initial measurements
  // wait for MAX chip to stabilize
  delay(1000);
 
   
}

void loop() {
    logTemp();  
    heat(heatlevel);
    if (phase==1){
      heatlevel=100;  //maximum heat for the first part of the ramp
      if (temp>100){ phase = 2; timephasestart = millisecs/1000;  }//end phase 1
        }
    else if (phase==2){ //use the knob to control the heat for the slower part of the ramp for 90 seconds
      heatknob = analogRead(potPin);
      if(temp > 160){heatlevel=0; }
      else {     heatlevel = map(heatknob, 0, 1023,0,100); }
      if (timethisphase > SOAKTIME) { phase=3; timephasestart=millisecs/1000;}//end phase after 90 seconds
      }
    else if (phase==3) {
      heatlevel=100;//maximum heat for peak temperature
      if (temp>LIQUIDUS) {phase=4; timephasestart=millisecs/1000;}//end phase when we get hot 
      }
    else if (phase==4) {
       if (temp<PEAK) {heatlevel=100;}//maximum heat for peak temperature
       else {heatlevel=50;}
      if (timethisphase>60) {
          heatlevel=0; // turn off heat 
          if ((timethisphase>90) && (int(millisecs/1000) %2 == 1)) { tone(buzzPin,500); }// beep and flash LED to open the door
          else {noTone(buzzPin);} 
          if (temp<180) {phase=5; timephasestart=millisecs/1000; noTone(buzzPin);}
        }
      }
      else{
        heatlevel=0;
      }
  }

   
  

void logTemp() {
   lastMillisecs=millisecs;
   millisecs=millis();
   Serial.println();
   Serial.print(millisecs/1000);
   Serial.print(", T= ,");
   lastTemp=temp;
   temp = thermocouple.readCelsius();
   Serial.print(temp);
   Serial.print(",rate= ,");
   rate=DECAYWEIGHTING*rate + (1-DECAYWEIGHTING)*(temp-lastTemp)/(millisecs-lastMillisecs);//rate of temperature change this is smoothed by a weighted average with the previous rate
   Serial.print(rate);
   Serial.print(" ,phase= ,");
   Serial.print(phase);
   Serial.print(" ,");
   timethisphase=millisecs/1000;
   timethisphase=timethisphase-timephasestart;//how long ago did phase start?
   Serial.print(timethisphase);
   Serial.print(",heat= ,");
   Serial.print(heatlevel); 
  
}
int heat(float level){
   if (level>90) {on(990); }
  else  if (level<10) {off(990); }
  else if (level>50) {off(minTime); ontime=(minTime+buttonTime)*level/(100-level);  on(ontime);}
  else if (level<=50) {on(minTime); offtime=(minTime+buttonTime)*(100-level)/level;off(offtime);}
  
  
}

int on(int timeOn) {
  pinMode(offPin,INPUT);//this is just a precaution so we don't activate both buttons on the remote at once
  pinMode(onPin,OUTPUT);
  digitalWrite(onPin,0);
  delay(buttonTime);
  pinMode(onPin,INPUT);
  delay(timeOn);
}

int off(int timeOff) {
  pinMode(onPin,INPUT);//this is just a precaution so we don't activate both buttons on the remote at once
  pinMode(offPin,OUTPUT);
  delay(buttonTime);
  pinMode(offPin,INPUT);
  delay(timeOff); 
}
