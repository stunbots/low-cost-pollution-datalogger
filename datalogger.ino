/*
                      +-----+
         +------------| USB |------------+
         |            +-----+            |
    B5   | [ ]D13/SCK        MISO/D12[ ] |   B4
         | [ ]3.3V           MOSI/D11[ ]~|   B3
         | [ ]V.ref     ___    SS/D10[ ]~|   B2
    C0   | [ ]A0       / N \       D9[ ]~|   B1
    C1   | [ ]A1      /  A  \      D8[ ] |   B0
    C2   | [ ]A2      \  N  /      D7[ ] |   D7
    C3   | [ ]A3       \_0_/       D6[ ]~|   D6
    C4   | [ ]A4/SDA               D5[ ]~|   D5
    C5   | [ ]A5/SCL               D4[ ] |   D4
         | [ ]A6              INT1/D3[ ]~|   D3
         | [ ]A7              INT0/D2[ ] |   D2
         | [ ]5V                  GND[ ] |     
    C6   | [ ]RST                 RST[ ] |   C6
         | [ ]GND   5V MOSI GND   TX1[ ] |   D0
         | [ ]Vin   [ ] [ ] [ ]   RX1[ ] |   D1
         |          [ ] [ ] [ ]          |
         |          MISO SCK RST         |
         | NANO-V3                       |
         +-------------------------------+
       
      http://busyducks.com/ascii-art-arduinos
*/

#include <MQ131.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include <virtuabotixRTC.h>


// Creation of the Real Time Clock Object
virtuabotixRTC myRTC(6, 7, 8);

LiquidCrystal_I2C lcd(0x27, 16, 2);
File myFile;

float m = -0.683; //Slope
float b = 2.131; //Y-Intercept
float R0 = 3.45; //Sensor Resistance in fresh air from previous code
double ppm_log;

void setup() {
  Serial.begin(9600); //Baud rate
  lcd.begin();
  lcd.backlight();
  // Set the current date, and time in the following format:
  // seconds, minutes, hours, day of the week, day of the month, month, year
  //myRTC.setDS1302Time(15, 14, 20, 06, 16, 12, 2022);
  //---------------------------------------------------------------
  Serial.print("Initializing SD card...");
  lcd.print("Initializing");
  lcd.setCursor(0, 1);
  lcd.print("SD Card...");
  delay(5000);
  lcd.clear();

  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    lcd.print("No SD Card Found");
    while (1);
  }
  Serial.println("initialization done.");
  lcd.print("Initialization");
  lcd.setCursor(0, 1);
  lcd.print("Successful");
  delay(3000);
  myFile = SD.open("Logs.txt", FILE_WRITE);

  myFile.close();
  //----------------------------------------------------------------

  pinMode(A2, INPUT); //Set gas sensor connection as input
  lcd.clear();
  Serial.println("Calibration in progress...");

  lcd.print("Calibrating...");
  MQ131.begin(2, A0, LOW_CONCENTRATION, 1000000);
  MQ131.calibrate();

  Serial.println("Calibration done!");
  lcd.setCursor(0, 1);
  lcd.print("Calibration Done");
  delay(5000);
  lcd.clear();

  delay(500);

  lcd.print("Starting Sensors");
  lcd.setCursor(0, 1);
  lcd.print("Please Wait...");
}


//------------------------------LOOP--------------------------------


void loop() {
  float CO, O3, CO_raw, O3_raw;

  MQ131.sample();
  O3 = MQ131.getO3(PPB);

  O3_raw = analogRead(A0);


  //  Serial.print(analogRead(A0));
  //  Serial.print("\t");

  Serial.print("Concentration O3 : ");
  Serial.print(O3);
  Serial.print(" ppb ");

  CO = calculateCO();
  CO_raw = analogRead(A2);
  //  myFile.print(analogRead(A2));
  //  myFile.print("\t");

  Serial.print(analogRead(A2));
  Serial.print("\t");

  Serial.print("Concentration CO : ");
  Serial.print(CO);
  Serial.print(" ppm\t");

  writeToFile(O3, CO, O3_raw, CO_raw);

  lcd.clear();
  lcd.print("OZONE:");
  lcd.print(O3);
  lcd.print(" PPB");
  lcd.setCursor(0, 1);
  lcd.print("CO:");
  lcd.print(CO);
  lcd.print(" PPM");

  delay(30000);
}




float calculateCO() {

  float sensor_volt; //Define variable for sensor voltage
  float RS_gas; //Define variable for sensor resistance
  float ratio; //Define variable for ratio
  float sensorValue = analogRead(A2); //Read analog values of sensor



  //Serial.print("Sensor Value: ");
  //Serial.print(sensorValue);
  //Serial.print(" Sensor Volt: ");

  sensor_volt = sensorValue * (5.0 / 1023.0); //Convert analog values to voltage

  //Serial.print(sensor_volt);

  RS_gas = ((5.0 * 10.0) / sensor_volt) - 10.0; //Get value of RS in a gas

  //Serial.print(" RS: ");
  //Serial.print(RS_gas);

  ratio = RS_gas / R0; // Get ratio RS_gas/RS_air

  //Serial.print(" Ratio: ");
  //Serial.print(ratio);

  ppm_log = ((log10(ratio)) - b) / m; //Get ppm value in linear scale according to the the ratio value

  //Serial.print(" PPM_Log: ");
  //Serial.print(ppm_log);
  //Serial.print("  ");

  float ppm = pow(10, ppm_log); //Convert ppm value to log scale
  //float percentage = ppm/10000; //Convert to percentage (uncomment to get value in %)
  return ppm;
}


void writeToFile(float o3, float co, int o3_raw, int co_raw) {

  myFile = SD.open("Logs.txt", FILE_WRITE);
  if (myFile) {

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }

    myRTC.updateTime();
    myFile.print(o3_raw);
    myFile.print("\t");
    myFile.print(co_raw);
    myFile.print("\t");
    myFile.print(o3, 7);
    myFile.print("\t");
    myFile.print(co, 7);
    myFile.print("\t");
    myFile.print(myRTC.dayofmonth);
    myFile.print("/");
    myFile.print(myRTC.month);
    myFile.print("/");
    myFile.print(myRTC.year);
    myFile.print("\t");
    myFile.print(myRTC.hours);
    myFile.print(":");
    myFile.print(myRTC.minutes);
    myFile.print(":");
    myFile.println(myRTC.seconds);

    Serial.print(o3_raw);
    Serial.print("\t");
    Serial.print(co_raw);
    Serial.print("\t");
    Serial.print(o3, 7);
    Serial.print("\t");
    Serial.print(co, 7);
    Serial.print("\t");
    Serial.print(myRTC.dayofmonth);
    Serial.print("/");
    Serial.print(myRTC.month);
    Serial.print("/");
    Serial.print(myRTC.year);
    Serial.print("\t");
    Serial.print(myRTC.hours);
    Serial.print(":");
    Serial.print(myRTC.minutes);
    Serial.print(":");
    Serial.println(myRTC.seconds);

    //close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
  delay(1000);
}
