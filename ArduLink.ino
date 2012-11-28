
// 0.0.1 early functionality


#include <Serial.h>



#define DEBUG 
#define DEBUG0

#define PULSEWIDTH 100
#define PULSEGAP 100
#define PULSEFRAME 1000


int semicycle=32;
int cycle=semicycle*2;
int jitter=semicycle/4;

int statusLED=4;
int sensorPin=2; //we need an interrupt
int signalLED=3;
int switchPin=7;
unsigned long int t0; //previous transition
unsigned long int t=0;    //current transition

unsigned char incoming=0;
boolean packetAvailable=false;
char incomingBuffer[128];

void setup(){
  Serial.begin(57600);
  pinMode(statusLED,OUTPUT);
  pinMode(signalLED,OUTPUT);
  pinMode(switchPin,INPUT);
  digitalWrite(switchPin,HIGH);
  pinMode(sensorPin,INPUT);
#ifdef DEBUG
  selfTest();
#endif
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
#ifdef DEBUG0
  Serial.print(deltaT);
  Serial.print("  ");
  Serial.print(state);
  Serial.print("  ");
#endif
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
#ifdef DEBUG0
    Serial.print(" ");
#endif
    if(abs(tmp)<jitter){ //semiperiod, we have read a 1
#ifdef DEBUG0
      Serial.print(1);
#endif
      incoming = (incoming<<1) | 1;
      bitcount++;
      state=3;
    }
    else if (abs(tmp-semicycle)<jitter) { //full period, we have read a 0, we are ready for the next symbol
#ifdef DEBUG0
      Serial.print(0);
#endif
      incoming = (incoming<<1); 
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
#ifdef DEBUG
    Serial.println();
    Serial.print("read = ");
    Serial.print(incoming,DEC);
    Serial.println();
#endif
    bitcount=0;
    //state=2;
    processCharacter(incoming);
    incoming=0;
  }
#ifdef DEBUG
  Serial.println();
#endif
}

void processCharacter(char c){
  static unsigned char state=0;
  static unsigned char checksum;
  static unsigned char ctr; //characters to read
  static unsigned char incomingCharIdx;

#ifdef DEBUG
  Serial.print(state);
  Serial.print(" ");
  Serial.print(c);
  Serial.print(" ");
  Serial.print(ctr);
  Serial.print(" ");
  Serial.print(checksum);
  Serial.println();
#endif 
  switch (state){

  case 0:
    packetAvailable=false;
    incomingCharIdx=0;
    ctr=c;
    state=1;
    break;

  case 1:
    state=2;
    break;

  case 2:
    ctr--;
    incomingBuffer[incomingCharIdx++]=incoming;
    checksum+=incoming;
    if (ctr==0) {
      if (checksum==0){
        packetAvailable=true;
        incomingBuffer[++incomingCharIdx]=0; //zero terminate, a valid string
        state=0;
        #ifdef DEBUG
        Serial.print(incomingBuffer);
        Serial.print("    parity=");
        Serial.println(checksum);
        #endif
      }
      else{
        //checksum failed!
        #ifdef DEBUG
        Serial.println("Checksum error in received packet");
        Serial.print("    parity=");
        Serial.println(checksum);
        #endif
        state=0;
      }
    }
  }
}



void status(unsigned char c){
  unsigned int t=millis();
  unsigned short int t1=t%128;
  if((1<<((t/128)%8))&c){ //true if this timeslot is a "true" one
    if (t1>16 && t1<248){
      digitalWrite(statusLED,HIGH);
    }
  } 
  else if (t1>112 && t1<144){
    digitalWrite(statusLED,HIGH);
  }
  else {
    digitalWrite(statusLED,LOW);
  }
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

void sends(char *s){
    digitalWrite(statusLED,HIGH);
  for (int i = 0; s[i] != 0; i++){
    send(s[i]);
  }
  delay(cycle);
  signal(LOW);
    digitalWrite(statusLED,LOW);
}

/*
Packet structure
 
 [LEN][PAR][DATA][DATA][DATA...]
 
 LEN is the number of bytes in the data. It does not include LEN and PAR
 PAR is the parity of the whole packet including LEN. A valid packet must XOR to zero
 DATA is up to 256 data bytes
 
 [003 [   ] [00
 */

void sendPacket(char *s){
  unsigned char len=0;
  unsigned char checksum=0;
  //count the characters in s and accumulate checksum by XORing values
  while(s[len]!=0){
    checksum += s[len];
    len++;
  }
  checksum |=len; //make sure length is in the checksum too
  send_start();
  send(len);
  send(checksum);
  sends(s);
#ifdef DEBUG
  Serial.print("Packet length ");
  Serial.println(len,DEC);
  Serial.print("Checksum ");
  Serial.println(checksum,DEC);
#endif
}

void send(unsigned char c){
  static boolean lampstate=false;
#ifdef DEBUG
  Serial.print(c,DEC);
  Serial.print(" ");
  Serial.print(c,BIN);
  Serial.print(" ");
  Serial.print(" ");
#endif
  for(int i=7;i>=0;i--){
    if(c & (1<<i)){ //the ith bit is set we need to flip at the beginning of the symbol
      lampstate = !lampstate;
      signal(lampstate);
#ifdef DEBUG
      Serial.print(1);
#endif
    } 
    else {
#ifdef DEBUG
      Serial.print(0);
#endif
    }

    delay(semicycle); //wait another half cycle
    lampstate = !lampstate;
    signal(lampstate);
    delay(semicycle);
  }
#ifdef DEBUG
  Serial.println();
#endif

  //digitalWrite(signalLED,LOW); //conclude the character
  //digitalWrite(statusLED,LOW);

}

void sendPulseTDM(char c, boolean wait_for_channel){
  for(int i=7;i>=0;i--){
    if(c & (1<<i)){ //the ith bit is set
      digitalWrite(signalLED,HIGH);
      delayMicroseconds(PULSEWIDTH);
      digitalWrite(signalLED,LOW);
#ifdef DEBUG
      Serial.print(1);
#endif
    } 
    else {
      delayMicroseconds(PULSEWIDTH+PULSEGAP);
#ifdef DEBUG
      Serial.print(0);
#endif
    }
    delay(PULSEFRAME);
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
  char st[]="Suca!!!!";
  for (int i=0;i<254;i++){
    st[i]=i;}
    st[255]=0;
  if (switchDown()){
    sendPacket(st);
  }

  delay(10);
  if (packetAvailable){
    Serial.println("========================================");
    Serial.println(incomingBuffer);
    Serial.println("========================================");
  }
  /*
  delay(25);
   Serial.print(t0);
   Serial.print(" ");
   Serial.println(t);
   */
}



















