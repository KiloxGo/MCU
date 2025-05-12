const int buttonPin=PUSH2;
const int ledPin=RED_LED;
int buttonState=0;
void setup() {
  pinMode(ledPin,OUTPUT);
  pinMode(buttonState,INPUT_PULLUP);
}

void loop() {
  buttonState=digitalRead(buttonPin);
  if(buttonState==LOW){
    digitalWrite(ledPin,HIGH);
    delay(4000);
    digitalWrite(ledPin,LOW);
    delay(4000);
  }
  else{
    digitalWrite(ledPin,HIGH);
    delay(200);
    digitalWrite(ledPin,LOW);
    delay(200);
  }  
}
