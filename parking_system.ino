#include <Servo.h>
const int analogInPin1 = A0;
const int analogInPin2 = A1;
const int analogInPin3 = A2;
const int analogInPin4 = A3;
const int analogInPin5 = A4;
const int analogInPin6 = A5;


int led = 2;
int led1 = 3;
int led2 = 4;
int led3 = 5;



int sensorValue = 0;
int sensorrValue = 0;

int servoPin = 9;
int servooPin = 8;
int i = 0;
int j = 0;

Servo servo;
Servo servoo;



void setup() 
{
servo.attach(servoPin = 9);
servoo.attach(servooPin = 8);

  pinMode (led,OUTPUT);
  pinMode (led1,OUTPUT);
  pinMode (led2,OUTPUT);
  pinMode (led3,OUTPUT);
}

void loop() 
{
  
 {
    sensorValue = analogRead(analogInPin1);
    
      if (sensorValue <600) // entrance Gate
        {
          for (int i=0;i<=90;i++)
        {
          servo.write(i);
          delay(30);
        }
          delay(3000);
        }
     if(sensorValue >600)
        {
          for (int i=90;i>0;i--)
          servo.write(i);
          delay(60);
        }
 }
 { 
   sensorrValue = analogRead(analogInPin6);
   
    if (sensorrValue <600) // entrance Gate
      {
        for (int j=0;j<=90;j++)
          {
            servoo.write(j);
            delay(30);
          }
        delay(3000);
      }
 }   
 if(sensorrValue >600)
  {
  for (int j=90;j>0;j--)
    servoo.write(j);
    delay(60);
  }
   
  if (analogRead(A1)<500)       
       {
        digitalWrite(led,LOW);
       }
     else
        {
          digitalWrite(led,HIGH);
        }
          
   if (analogRead(A2)<500)       
       {
        digitalWrite(led1,LOW);
       }
     else
        {
          digitalWrite(led1,HIGH);
        }
          
    if (analogRead(A3)<500)       
       {
        digitalWrite(led2,LOW);
       }
     else
       {
         digitalWrite(led2,HIGH);
       }
          
    if (analogRead(A4)<500)       
       {
        digitalWrite(led3,LOW);
       }
     else
       {
         digitalWrite(led3,HIGH);
       }
       

}

