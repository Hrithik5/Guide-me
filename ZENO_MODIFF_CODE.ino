#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
void setup() {
lcd.begin(16, 2); 

}
void loop() 
{
lcd.print(" PEWDIEPIE "); 
lcd.setCursor(0, 1);
delay(1500);
lcd.clear();
lcd.print("KI ");
lcd.setCursor(0, 1);
delay(1500);
lcd.setCursor(0, 1);
lcd.print("MAA ");
delay(1500);
lcd.clear();
lcd.print("KA");
lcd.setCursor(0, 1);
delay(1500);
lcd.print("BHOSDA ");
delay(1500);
lcd.clear();
}
