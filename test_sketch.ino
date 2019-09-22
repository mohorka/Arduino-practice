#include <Servo.h>
#include <NewPing.h>

NewPing sonar_first(7,6,100);
NewPing sonar_second(12,11,100);
Servo servo;
void setup() {
Serial.begin(9600); //delete it after debug
pinMode(14,OUTPUT);//for sonar_first
pinMode(15,OUTPUT);//for sonar_second
servo.attach(9);
}

void loop() {
  digitalWrite(14,HIGH);
  digitalWrite(15,HIGH);
  if(servo.attached())
  {
    for(int i=0;i<=90;++i)
  {
    servo.write(i*2);
    delay(70);
    Serial.print("Ping from first: ");
    Serial.print(sonar_first.ping_cm());
    Serial.println("cm");
    Serial.print("Ping from second: ");
    Serial.print(sonar_second.ping_cm());
    Serial.println("cm");
  }
  }

}
