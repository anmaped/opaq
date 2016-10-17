void setup() {
  
  Serial.begin(9600);

  pinMode(4,OUTPUT);
  
  pinMode(8,OUTPUT);
  pinMode(9,OUTPUT);

  pinMode(4,LOW);
  digitalWrite(8,HIGH);
  digitalWrite(9,HIGH);

}

bool togle = false;

void loop() {
  
  delay(100);

  if(togle)
    digitalWrite(9,LOW);
  else
    digitalWrite(9,HIGH);

 togle = !togle;

}
