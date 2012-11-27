#include <Serial.h>

#define DEBUG

int semicycle=250;
int cycle=semicycle*2;
int jitter=5;

int statusLED=4;
int sensorPin=2; //we need an interrupt
int signalLED=3;
int switchPin=7;
unsigned long int t0; //previous transition
unsigned long int t=0;    //current transition

unsigned char incoming=0;
boolean charAvailable=false;

void setup(){
  Serial.begin(57600);
  pinMode(statusLED,OUTPUT);
  pinMode(signalLED,OUTPUT);
  pinMode(switchPin,INPUT);
  digitalWrite(switchPin,HIGH);
  pinMode(sensorPin,INPUT);
  selfTest();
  attachInterrupt(0,crossover,CHANGE);
}

void selfTest(){
  for (int i=0;i<=8;i++){
    digitalWrite(statusLED,i%2);
    digitalWrite(signalLED,i%2);
    delay(50);
  }
}

void crossover(){ //ISR for sensor pin change
  static unsigned char state=0;
  static unsigned int bitcount=0;
  unsigned long int deltaT; //watch out for the datatype size

  int tmp;

  t0=t;
  t=millis();
  deltaT=t-t0;
  if (deltaT>cycle*2) {
    state=0;
  } //autoreset after a long silence: assume sender has died
  Serial.print(deltaT);
  Serial.print("  ");
  Serial.print(state);
  Serial.print("  ");
  switch(state) {

  case 0:
    bitcount=0;
    state=1;
    break;

  case 1:
    //another short pulse
    if (abs(deltaT-semicycle)<jitter){
      state=2;
    }
    break;

  case 2:
    tmp=deltaT-semicycle;
    Serial.print(" ");
    if(abs(tmp)<jitter){ //semiperiod, we have read a 1
      Serial.print(1);
      incoming = (incoming<<1) | 1;
      bitcount++;
      state=3;
    }
    else if (abs(tmp-semicycle)<jitter) { //full period, we have read a 0, we are ready for the next symbol
      Serial.print(0);
      incoming = (incoming<<1) | 0; //hahahahahahahahahahaha
      bitcount++;
      state=2;
    }
    else{
      state=0; //decoding failed, go back to zero
    }
    break;

  case 3: //if we have read a one, there will be one more transition inside the symbol
    if(abs(tmp)<jitter){ //semiperiod
      state=2;
    }
    else{
      state=0;
    } //decoding failed
    break;

  default:
    Serial.print("Oy vey!");
    break;
  } 
  if (bitcount==8){
    Serial.println();
    Serial.print("yaba yaba = ");
    Serial.print(incoming,DEC);
    Serial.println();
    bitcount=0;
    //state=2;
    incoming=0;
  }
  Serial.println();
}

void signal(boolean value){
  // static unsigned long int t=millis();
  digitalWrite(signalLED,value);
  /*
    Serial.print(" ");
  Serial.print(millis()-t,DEC);
  Serial.print(" ");
  Serial.println(value?"1":"0");
  */
}

void send_start(){
  boolean lampstate=true;
  digitalWrite(statusLED,HIGH);

  //state 0
  digitalWrite(signalLED,lampstate); //HIGH
  delay(semicycle); //state 1
  lampstate = !lampstate;
  signal(lampstate); //LOW
  delay(semicycle);
}

void send1(unsigned char c){
  send_start();
  send(c);
}

void sends(unsigned char *s){
  send_start();
  for (int i = 0; s[i] != 0; i++){
    send(s[i]);}
}


  
void send(unsigned char c){
  static boolean lampstate=false;
  Serial.print(c,DEC);
  Serial.print(" ");
  Serial.print(c,BIN);
  Serial.print(" ");
  Serial.print(" ");
  for(int i=7;i>=0;i--){
    if(c & (1<<i)){ //the ith bit is set we need to flip at the beginning of the symbol
      lampstate = !lampstate;
      signal(lampstate);
      Serial.print(1);
    } 
    else {
      Serial.print(0);
    }

    delay(semicycle); //wait another half cycle
    lampstate = !lampstate;
    signal(lampstate);
    delay(semicycle);
    //state 2;

  }
  Serial.println();

  //digitalWrite(signalLED,LOW); //conclude the character
  //digitalWrite(statusLED,LOW);

}

void sendPulseTDM(char c, boolean wait_for_channel){
  for(int i=7;i>=0;i--){
    if(c & (1<<i)){ //the ith bit is set
      digitalWrite(signalPin,HIGH);
      delayMicroseconds(PULSEWIDTH);
      digitalWrite(signalPin,LOW);
      #ifdef DEBUG
      Serial.print(1);
      #endif
    } 
    else {
      delayMicroseconds(PULSEWIDTH+PULSEDELAY);
      #ifdef DEBUG
      Serial.print(0);
      #endif
    }
    delay(TOTALWIDTH);
  }
}


boolean switchDown(){
  boolean state=digitalRead(switchPin);
  static boolean oldstate=state;
  if (state!=oldstate){
    oldstate=state;
    return (state==HIGH);
  }
  return false;
}



void loop(){
  unsigned char st="Hello, world!";
  if (switchDown()){
    sends(st);

  }
  /*
  delay(25);
   Serial.print(t0);
   Serial.print(" ");
   Serial.println(t);
   */
}













