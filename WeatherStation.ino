#include "DHT.h"
#include <SoftwareSerial.h>
#include "RTClib.h"

#define PI_NUMBER 3.14159265
#define ANEMOMETER_RADIUS 147
#define ANEMOMETER_PIN 2
#define PERIOD 5000
#define RAINGAUGE_PIN 3
#define DHT_PIN A2 
#define DHT_TYPE DHT11
#define USB_PIN_10 10
#define USB_PIN_11 11


DHT dht(DHT_PIN, DHT_TYPE);
RTC_Millis rtc;
SoftwareSerial USB(USB_PIN_10, USB_PIN_11);

unsigned int anemometerCounter = 0;
byte computerByte;           //used to store data coming from the computer
byte USB_Byte;               //used to store data coming from the USB stick
int timeOut = 2000;
String data;
unsigned long lastMillis;

double rain = 0.0;
bool bucketPosition = false;
const double bucketAmount = 0.01610595;

void setup() {
  Serial.begin(9600);
  rtc.begin(DateTime(__DATE__, __TIME__));
  dht.begin();
  pinMode(ANEMOMETER_PIN, INPUT);
  digitalWrite(ANEMOMETER_PIN, HIGH);
  pinMode(RAINGAUGE_PIN, INPUT);
  USB.begin(9600);
}

void loop() {
  calcRainGauge();
  writeDataEvent();
}

void writeDataEvent() {
  if (millis() - lastMillis >= 10 * 60 * 1000UL)
  {
    lastMillis = millis();  //get ready for the next iteration
    data = convertFloatToString(getTemperature()) + ";" + convertFloatToString(getHumidity()) + ";" + convertFloatToString(getWindSpeed()) + ";" + convertFloatToString(0) + ";" + convertFloatToString(getRainGauge()) + ";" + getDate() + ";" + getHour() + ";\n";
    writeData();
    //Serial.println(data);
  }

}

String convertFloatToString(float value)
{
  char temp[10];
  String tempAsString;
  dtostrf(value, 1, 2, temp);
  tempAsString = String(temp);
  return tempAsString;
}

void writeData() {
  if (!verificaSeExisteArquivo("WEATHER.CSV")) {
    writeFile("WEATHER.CSV", "Temperatura;Umidade;Vel. Ventos;Dir. Vento;Pluviometro;Data;Hora\n");
  }
  appendFile("WEATHER.CSV", data);
}

bool verificaSeExisteArquivo(String fileName) {
  resetALL();                     //Reset the module
  set_USB_Mode(0x06);             //Set to USB Mode
  diskConnectionStatus();         //Check that communication with the USB device is possible
  USBdiskMount();                 //Prepare the USB for reading/writing - you need to mount the USB disk for proper read/write operations.
  setFileName(fileName);          //Set File name
  Serial.println("Opening file.");
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x32);
  if (waitForResponse("file Open")) {               //wait for a response from the CH376S. If CH376S responds, it will be true. If it times out, it will be false.
    if (getResponseFromUSB() == 0x14) {             //CH376S will send 0x14 if this command was successful
      Serial.println(">File opened successfully.");
      fileClose(0x00);
      return true;
    } else {
      //fileClose(0x00);
      return false;
    }
  }
}

String getHour() {
  DateTime now = rtc.now();
  char hour[16];
  sprintf(hour, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
  return String(hour);
}

String getDate() {
  DateTime now = rtc.now();
  char date[16];
  sprintf(date, "%02d/%02d/%02d", now.day(), now.month(), now.year());
  return String(date);
}

float getWindSpeed() {
  WindVelocity();
  return WindSpeed();
  delay(2000);
}

void WindVelocity() {
  anemometerCounter = 0;
  attachInterrupt(0, addcount, RISING);
  unsigned long millis();
  long startTime = millis();
  while (millis() < startTime + PERIOD) {}
}



float getTemperature() {
  return dht.readTemperature();
}

float getHumidity() {
  return dht.readHumidity();
}

int RPM() {
  return ((anemometerCounter) * 60) / (PERIOD / 1000);
}

float WindSpeed() {
  return ((4 * PI_NUMBER * ANEMOMETER_RADIUS * RPM()) / 60) / 1000;
}

void addcount() {
  anemometerCounter++;
}

float getRainGauge() {
  float rainGaugeValue = rain;
  resetRainGauge();
  return rainGaugeValue;
}

void calcRainGauge() {
  if ((bucketPosition == false) && (digitalRead(RAINGAUGE_PIN) == HIGH)) {
    //Serial.println("GAUGE HIGH");
    bucketPosition = true;
    rain += bucketAmount;
  }

  if ((bucketPosition == true) && (digitalRead(RAINGAUGE_PIN) == LOW)) {
    bucketPosition = false;
  }
}

void resetRainGauge() {
  rain = 0.0;
}




























// --> USB CONTENT START ==============================================================================

//checkConnection==================================================================================
//This function is used to check for successful communication with the CH376S module. This is not dependant of the presence of a USB stick.
//Send any value between 0 to 255, and the CH376S module will return a number = 255 - value.
void checkConnection(byte value) {
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x06);
  USB.write(value);

  if (waitForResponse("checking connection")) {     //wait for a response from the CH376S. If CH376S responds, it will be true. If it times out, it will be false.
    if (getResponseFromUSB() == (255 - value)) {
      Serial.println(">Connection to CH376S was successful.");
    } else {
      Serial.print(">Connection to CH376S - FAILED.");
    }
  }
}

//set_USB_Mode=====================================================================================
//Make sure that the USB is inserted when using 0x06 as the value in this specific code sequence
void set_USB_Mode (byte value) {
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x15);
  USB.write(value);

  delay(20);

  if (USB.available()) {
    USB_Byte = USB.read();
    //Check to see if the command has been successfully transmitted and acknowledged.
    if (USB_Byte == 0x51) {                               // If true - the CH376S has acknowledged the command.
      Serial.println("set_USB_Mode command acknowledged"); //The CH376S will now check and monitor the USB port
      USB_Byte = USB.read();

      //Check to see if the USB stick is connected or not.
      if (USB_Byte == 0x15) {                           // If true - there is a USB stick connected
        Serial.println("USB is present");
      } else {
        Serial.print("USB Not present. Error code:");   // If the USB is not connected - it should return an Error code = FFH
        Serial.print(USB_Byte, HEX);
        Serial.println("H");
      }

    } else {
      Serial.print("CH3765 error!   Error code:");
      Serial.print(USB_Byte, HEX);
      Serial.println("H");
    }
  }
  delay(20);
}

//resetALL=========================================================================================
//This will perform a hardware reset of the CH376S module - which usually takes about 35 msecs =====
void resetALL() {
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x05);
  Serial.println("The CH376S module has been reset !");
  delay(200);
}

//readFile=====================================================================================
//This will send a series of commands to read data from a specific file (defined by fileName)
void readFile(String fileName) {
  resetALL();                     //Reset the module
  set_USB_Mode(0x06);             //Set to USB Mode
  diskConnectionStatus();         //Check that communication with the USB device is possible
  USBdiskMount();                 //Prepare the USB for reading/writing - you need to mount the USB disk for proper read/write operations.
  setFileName(fileName);          //Set File name
  fileOpen();                     //Open the file for reading
  int fs = getFileSize();         //Get the size of the file
  fileRead();                     //***** Send the command to read the file ***
  fileClose(0x00);                //Close the file
}

//writeFile========================================================================================
//is used to create a new file and then write data to that file. "fileName" is a variable used to hold the name of the file (e.g TEST.TXT). "data" should not be greater than 255 bytes long.
void writeFile(String fileName, String data) {
  resetALL();                     //Reset the module
  set_USB_Mode(0x06);             //Set to USB Mode
  diskConnectionStatus();         //Check that communication with the USB device is possible
  USBdiskMount();                 //Prepare the USB for reading/writing - you need to mount the USB disk for proper read/write operations.
  setFileName(fileName);          //Set File name
  if (fileCreate()) {             //Try to create a new file. If file creation is successful
    fileWrite(data);              //write data to the file.
  } else {
    Serial.println("File could not be created, or it already exists");
  }
  fileClose(0x01);
}

//appendFile()====================================================================================
//is used to write data to the end of the file, without erasing the contents of the file.
void appendFile(String fileName, String data) {
  resetALL();                     //Reset the module
  set_USB_Mode(0x06);             //Set to USB Mode
  diskConnectionStatus();         //Check that communication with the USB device is possible
  USBdiskMount();                 //Prepare the USB for reading/writing - you need to mount the USB disk for proper read/write operations.
  setFileName(fileName);          //Set File name
  fileOpen();                     //Open the file
  filePointer(false);             //filePointer(false) is to set the pointer at the end of the file.  filePointer(true) will set the pointer to the beginning.
  fileWrite(data);                //Write data to the end of the file
  fileClose(0x01);                //Close the file using 0x01 - which means to update the size of the file on close.
}

//setFileName======================================================================================
//This sets the name of the file to work with
void setFileName(String fileName) {
  Serial.print("Setting filename to:");
  Serial.println(fileName);
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x2F);
  USB.write(0x2F);         // Every filename must have this byte to indicate the start of the file name.
  USB.print(fileName);     // "fileName" is a variable that holds the name of the file.  eg. TEST.TXT
  USB.write((byte)0x00);   // you need to cast as a byte - otherwise it will not compile.  The null byte indicates the end of the file name.
  delay(20);
}

//diskConnectionStatus================================================================================
//Check the disk connection status
void diskConnectionStatus() {
  Serial.println("Checking USB disk connection status");
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x30);

  if (waitForResponse("Connecting to USB disk")) {     //wait for a response from the CH376S. If CH376S responds, it will be true. If it times out, it will be false.
    if (getResponseFromUSB() == 0x14) {           //CH376S will send 0x14 if this command was successful
      Serial.println(">Connection to USB OK");
    } else {
      Serial.print(">Connection to USB - FAILED.");
    }
  }
}

//USBdiskMount========================================================================================
//initialise the USB disk and check that it is ready - this process is required if you want to find the manufacturing information of the USB disk
void USBdiskMount() {
  Serial.println("Mounting USB disk");
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x31);

  if (waitForResponse("mounting USB disk")) {     //wait for a response from the CH376S. If CH376S responds, it will be true. If it times out, it will be false.
    if (getResponseFromUSB() == 0x14) {           //CH376S will send 0x14 if this command was successful
      Serial.println(">USB Mounted - OK");
    } else {
      Serial.print(">Failed to Mount USB disk.");
    }
  }
}

//fileOpen========================================================================================
//opens the file for reading or writing
void fileOpen() {
  Serial.println("Opening file.");
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x32);
  if (waitForResponse("file Open")) {               //wait for a response from the CH376S. If CH376S responds, it will be true. If it times out, it will be false.
    if (getResponseFromUSB() == 0x14) {             //CH376S will send 0x14 if this command was successful
      Serial.println(">File opened successfully.");
    } else {
      Serial.print(">Failed to open file.");
    }
  }
}

//setByteRead=====================================================================================
//This function is required if you want to read data from the file.
boolean setByteRead(byte numBytes) {
  boolean bytesToRead = false;
  int timeCounter = 0;
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x3A);
  USB.write((byte)numBytes);   //tells the CH376S how many bytes to read at a time
  USB.write((byte)0x00);
  if (waitForResponse("setByteRead")) {     //wait for a response from the CH376S. If CH376S responds, it will be true. If it times out, it will be false.
    if (getResponseFromUSB() == 0x1D) {     //read the CH376S message. If equal to 0x1D, data is present, so return true. Will return 0x14 if no data is present.
      bytesToRead = true;
    }
  }
  return (bytesToRead);
}

//getFileSize()===================================================================================
//writes the file size to the serial Monitor.
int getFileSize() {
  int fileSize = 0;
  Serial.println("Getting File Size");
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x0C);
  USB.write(0x68);
  delay(100);
  Serial.print("FileSize =");
  if (USB.available()) {
    fileSize = fileSize + USB.read();
  }
  if (USB.available()) {
    fileSize = fileSize + (USB.read() * 255);
  }
  if (USB.available()) {
    fileSize = fileSize + (USB.read() * 255 * 255);
  }
  if (USB.available()) {
    fileSize = fileSize + (USB.read() * 255 * 255 * 255);
  }
  Serial.println(fileSize);
  delay(10);
  return (fileSize);
}


//fileRead========================================================================================
//read the contents of the file
void fileRead() {
  Serial.println("Reading file:");
  byte firstByte = 0x00;                     //Variable to hold the firstByte from every transmission.  Can be used as a checkSum if required.
  byte numBytes = 0x40;                      //The maximum value is 0x40  =  64 bytes

  while (setByteRead(numBytes)) {            //This tells the CH376S module how many bytes to read on the next reading step. In this example, we will read 0x10 bytes at a time. Returns true if there are bytes to read, false if there are no more bytes to read.
    USB.write(0x57);
    USB.write(0xAB);
    USB.write(0x27);                          //Command to read ALL of the bytes (allocated by setByteRead(x))
    if (waitForResponse("reading data")) {    //Wait for the CH376S module to return data. TimeOut will return false. If data is being transmitted, it will return true.
      firstByte = USB.read();               //Read the first byte
      while (USB.available()) {
        Serial.write(USB.read());           //Send the data from the USB disk to the Serial monitor
        delay(1);                           //This delay is necessary for successful Serial transmission
      }
    }
    if (!continueRead()) {                     //prepares the module for further reading. If false, stop reading.
      break;                                   //You need the continueRead() method if the data to be read from the USB device is greater than numBytes.
    }
  }
  Serial.println();
  Serial.println("NO MORE DATA");
}

//fileWrite=======================================================================================
//are the commands used to write to the file
void fileWrite(String data) {
  Serial.println("Writing to file:");
  byte dataLength = (byte) data.length();         // This variable holds the length of the data to be written (in bytes)
  Serial.println(data);
  Serial.print("Data Length:");
  Serial.println(dataLength);
  delay(100);
  // This set of commands tells the CH376S module how many bytes to expect from the Arduino.  (defined by the "dataLength" variable)
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x3C);
  USB.write((byte) dataLength);
  USB.write((byte) 0x00);
  if (waitForResponse("setting data Length")) {    // Wait for an acknowledgement from the CH376S module before trying to send data to it
    if (getResponseFromUSB() == 0x1E) {            // 0x1E indicates that the USB device is in write mode.
      USB.write(0x57);
      USB.write(0xAB);
      USB.write(0x2D);
      USB.print(data);                             // write the data to the file

      if (waitForResponse("writing data to file")) { // wait for an acknowledgement from the CH376S module
      }
      Serial.print("Write code (normally FF and 14): ");
      Serial.print(USB.read(), HEX);               // code is normally 0xFF
      Serial.print(",");
      USB.write(0x57);
      USB.write(0xAB);
      USB.write(0x3D);                             // This is used to update the file size. Not sure if this is necessary for successful writing.
      if (waitForResponse("updating file size")) { // wait for an acknowledgement from the CH376S module
      }
      Serial.println(USB.read(), HEX);             //code is normally 0x14
    }
  }
}

//continueRead()==================================================================================
//continue to read the file : I could not get this function to work as intended.
boolean continueRead() {
  boolean readAgain = false;
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x3B);
  if (waitForResponse("continueRead")) {     //wait for a response from the CH376S. If CH376S responds, it will be true. If it times out, it will be false.
    if (getResponseFromUSB() == 0x14) {     //CH376S will send 0x14 if this command was successful
      readAgain = true;
    }
  }
  return (readAgain);
}

//fileCreate()========================================================================================
//the command sequence to create a file
boolean fileCreate() {
  boolean createdFile = false;
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x34);
  if (waitForResponse("creating file")) {     //wait for a response from the CH376S. If file has been created successfully, it will return true.
    if (getResponseFromUSB() == 0x14) {      //CH376S will send 0x14 if this command was successful
      createdFile = true;
    }
  }
  return (createdFile);
}


//fileDelete()========================================================================================
//the command sequence to delete a file
void fileDelete(String fileName) {
  setFileName(fileName);
  delay(20);
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x35);
  if (waitForResponse("deleting file")) {     //wait for a response from the CH376S. If file has been created successfully, it will return true.
    if (getResponseFromUSB() == 0x14) {      //CH376S will send 0x14 if this command was successful
      Serial.println("Successfully deleted file");
    }
  }
}


//filePointer========================================================================================
//is used to set the file pointer position. true for beginning of file, false for the end of the file.
void filePointer(boolean fileBeginning) {
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x39);
  if (fileBeginning) {
    USB.write((byte)0x00);             //beginning of file
    USB.write((byte)0x00);
    USB.write((byte)0x00);
    USB.write((byte)0x00);
  } else {
    USB.write((byte)0xFF);             //end of file
    USB.write((byte)0xFF);
    USB.write((byte)0xFF);
    USB.write((byte)0xFF);
  }
  if (waitForResponse("setting file pointer")) {     //wait for a response from the CH376S.
    if (getResponseFromUSB() == 0x14) {             //CH376S will send 0x14 if this command was successful
      Serial.println("Pointer successfully applied");
    }
  }
}


//fileClose=======================================================================================
//closes the file
void fileClose(byte closeCmd) {
  Serial.println("Closing file:");
  USB.write(0x57);
  USB.write(0xAB);
  USB.write(0x36);
  USB.write((byte)closeCmd);                                // closeCmd = 0x00 = close without updating file Size, 0x01 = close and update file Size

  if (waitForResponse("closing file")) {                    // wait for a response from the CH376S.
    byte resp = getResponseFromUSB();
    if (resp == 0x14) {                                    // CH376S will send 0x14 if this command was successful
      Serial.println(">File closed successfully.");
    } else {
      Serial.print(">Failed to close file. Error code:");
      Serial.println(resp, HEX);
    }
  }
}

//waitForResponse===================================================================================
//is used to wait for a response from USB. Returns true when bytes become available, false if it times out.
boolean waitForResponse(String errorMsg) {
  boolean bytesAvailable = true;
  int counter = 0;
  while (!USB.available()) {   //wait for CH376S to verify command
    delay(1);
    counter++;
    if (counter > timeOut) {
      Serial.print("TimeOut waiting for response: Error while: ");
      Serial.println(errorMsg);
      bytesAvailable = false;
      break;
    }
  }
  delay(1);
  return (bytesAvailable);
}

//getResponseFromUSB================================================================================
//is used to get any error codes or messages from the CH376S module (in response to certain commands)
byte getResponseFromUSB() {
  byte response = byte(0x00);
  if (USB.available()) {
    response = USB.read();
  }
  return (response);
}
//USB CONTENT END ======================================================================
