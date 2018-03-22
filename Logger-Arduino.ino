#include <OneWire.h>
#include <DallasTemperature.h>

#define PP2 2   // TempSensors (DS18B20) 1-wire Interface connected to Port Pins D2..D5
#define PP3 3
#define PP4 4
#define PP5 5
#define NoIFs 4          // Set number of 1-wire Interfaces
#define NoSensors_IF 10  // Set number of sensors per 1-wire Interface. Can be a maximum of 20 devices
#define ROMsize 8        // DS18B20 sensor ROM size
#define AMOUNT_OF_ONEWIRE_INSTANCES 4

//OneWire oneWire(ONE_WIRE_BUS);  // Setup a oneWire instance to communicate with any OneWire devices (Original aus Internet)


//function declarations
byte discoverOneWireDevices(OneWire oneWire_X, byte allAddr_X[NoSensors_IF][ROMsize]);
void setSensorResolution(byte totalDev_X, DallasTemperature sensor_X, byte allAddr_X[NoSensors_IF][ROMsize]);
void Temp_X(DeviceAddress addr, DallasTemperature sensor_X, int index, byte allAddr_X[NoSensors_IF][ROMsize]);
void setupSensorTemperature(DallasTemperature sensor_X, byte totalDev_X, byte allAddr_X[NoSensors_IF][ROMsize], int index, byte* ii);



OneWire oneWires[AMOUNT_OF_ONEWIRE_INSTANCES] = {
    OneWire(PP2),
    OneWire(PP3),
    OneWire(PP4),
    OneWire(PP5)
};

//DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature. (Original aus Internet)

DallasTemperature sensors[AMOUNT_OF_ONEWIRE_INSTANCES];

byte allAddrses [AMOUNT_OF_ONEWIRE_INSTANCES][NoSensors_IF][ROMsize];  // DS18B20 ROM size byte arrays.
 // we need one for each of our DS18B20 sensors.


byte totalDevs[AMOUNT_OF_ONEWIRE_INSTANCES]; // total number of One Wire devices connected to a port pin

int   datarate = 9600;  // serial port data rate
byte  resolution = 12;  // Sensor resolution - 12=0,0625°C; 11=0,125°C; 10=0,25°C; 9=0,5°C
char  add = 'a';        // Zusatz bei mehrerem Arduinos anpassen
int   TempA[NoIFs*NoSensors_IF];    // last temperature values
int   SenDiff[NoIFs*NoSensors_IF];
float tempC;
byte  S_totalCnt = 0;   // Sensor total counter
long  lastTime;
byte  i,j,k,ii = 0; // index
int   cycle_time;    // time in milliseconds between measurements
int   NoCycles = 0;
int   time_left;
int   daempf;       //Faktor Dämpfung
long  milltempo;    // Intervall
byte  tempDFF = 100;  // Temperaturänderung für Datenaufzeichnung * 100
byte  tempAbz = 10;  // Abzug für Berechnung  Beispiel : 10 - 25%
byte  Pause = 15;    // sekunden, min 5 Pause zwischen den Messungen
byte  Fehlmess = 2;  // Fehler bei Messungen, erneuter Messversuch


void setup()
{
  
  for (int i = 0; i < AMOUNT_OF_ONEWIRE_INSTANCES; i++) {
      sensors[i] = DallasTemperature(&oneWires[i]);
  }
    
  for (int i=0; i<(NoIFs*NoSensors_IF); i++)
  {
    SenDiff [i]=tempDFF;
    TempA [i]=(-2000);   // Speicherbereiche initialisieren    
  }
  
  Serial.begin(datarate);  // start serial port
  for (int i = 0; i < AMOUNT_OF_ONEWIRE_INSTANCES; i++)
      sensors[i].begin(); // start sensors data transfer

  Serial.print("............Arduino Nano...........\n");  // Zeile nicht verändern wegen Arduno-Erkennung!!!!!!!!!!!!!!!!!!!!
  Serial.print("...................................\n");
  Serial.print(".. Version 2017-10-29 1.20 MN-LW ..\n");
  Serial.print(".Temperatursensoren Dallas DS18B20.\n");
  Serial.print("...  Max 10 Sensoren pro Pin    ...\n");
  Serial.print("................................TH.\n");
  S_totalCnt = 0;
  
   for (int i = 0; i < AMOUNT_OF_ONEWIRE_INSTANCES; i++) {
      Serial.print("\n sensors at interface ");
      Serial.print(i+1);
      Serial.print(": ");
      totalDevs[i] = discoverOneWireDevices(oneWires[i], allAddrses[i]);
      setSensorResolution(totalDevs[i], sensors[i], allAddrses[i]);
   }
   
  Serial.print("\n ... A total number of ");Serial.print(S_totalCnt, DEC); Serial.print(" sensors found\n\n");
  if (S_totalCnt != 0)
    cycle_time = (S_totalCnt*750); // maximum sensor-temperature-conversion time is 750 msec
  else
      cycle_time = 1000; // one second default cycle time for main loop in case no sensors where detected
  // Serial.print(cycle_time); Serial.print(" msec cycle time\n");
} //void setup() - end

byte discoverOneWireDevices(OneWire oneWire_X, byte allAddr_X[NoSensors_IF][ROMsize]){ 
  j=0; 
  while((j < NoSensors_IF)&&(oneWire_X.search(allAddr_X[j]))){
    j++;
  }
  for(i=0; i < j; i++){
    Serial.print(" ...");
    print_shortAddr(allAddr_X[i]);
    S_totalCnt++;
  }
  return j;
}

void setSensorResolution(byte totalDev_X, DallasTemperature sensor_X, byte allAddr_X[NoSensors_IF][ROMsize]) {
    for (i=0; i < totalDev_X; i++){
    sensor_X.setResolution(allAddr_X[i], resolution);
  }
}


// device serial number = ROM Byte 1-6
// ROM Byte 1-3,7 wird ausgegeben (warum ???)  Byte0 = Family code(0x28), Byte1-6 = DS18B20 serial number, Byte7 = CRC
//void print_shortAddr(DeviceAddress addr){for(k=1; k < ROMsize; k++){if (addr[k] < 16) {Serial.print('0');} Serial.print(addr[k], HEX); if(k>2){k=k+3;}}}
void print_shortAddr(DeviceAddress addr){
  for(k=2; k < (ROMsize-1); k++){
    if (addr[k] < 16) {
      Serial.print('0');
    } 
    Serial.print(addr[k], HEX); 
    if(k>2){
      k=k+3;
    }
  }
}

// print sensor values                                                           -127
void Temp_X(DeviceAddress addr, DallasTemperature sensor_X, int index, byte allAddr_X[NoSensors_IF][ROMsize]) {
  tempC = (sensor_X.getTempC(addr)); 
  if (tempC == DEVICE_DISCONNECTED_C || tempC == 85  || tempC == 0) {
    Serial.print("Error-");
    Serial.print(add);
    Serial.print(index + ",sensor=");
    print_shortAddr(allAddr_X[i]);
    Serial.print(" value=");
    Serial.print(tempC,1);
    Serial.print("\n");
    tempC=-2000;
  } 
  else {
    tempC = tempC*100;
  }
}

void Daempfung()
{
  delay(300);
  if(TempA[ii]>(-2000))
  {
    if(tempC == -1270 || tempC == 850 || tempC == 0){
      Serial.println("");
      Serial.print("Fehler_");
      Serial.print(tempC);
      Serial.println("-");
    }
    else
    {
      //TempTemp =((tempC)-TempA[ii]);  // Differenz zwischen altem und neuen Wert
      //TempTemp = abs(TempTemp);
      //Serial.print(" .x. ");
      //Serial.print(tempC);
      daempf=((abs((tempC)-TempA[ii]))/100)+3;
      tempC=((tempC*daempf)+TempA[ii])/(1+daempf);
      ///Serial.print(" ");
      //Serial.print(tempC);
    }
    if((abs((tempC)-TempA[ii]))>1000) //Aktion bei Temperaturänderung > 10 Grad 
    {
      //Serial.print("  ");
      //Serial.print(tempC);
      //Serial.print("  ");
      if (Fehlmess > 0)  // Zwei weitere Versuche bei Fehlmessungen
      {
        Fehlmess = Fehlmess - 1;
        tempC = -2000;
        i = i - 1;
        delay (750);
      } 
    }
  }
  else
  {
    //TempTemp =0;Serial.print("\n Interface 2:\n");
    TempA[ii]= tempC;
    SenDiff[ii] =0;
  }
  //Serial.println("   TempTemp");
  //Serial.println(TempTemp); 
  //Serial.println(TempA[ii]); 
}


void setupSensorTemperature(DallasTemperature sensor_X, byte totalDev_X, byte allAddr_X[NoSensors_IF][ROMsize], int index, byte* ii) {
  sensor_X.requestTemperatures();
  for (i=0; i < totalDev_X; i++){
    Temp_X(allAddr_X[i], sensor_X, index, allAddr_X);
    Daempfung();
    if(abs(tempC-TempA[*ii])+5>(SenDiff[*ii])) {
      if(tempC > -2000){
        Fehlmess = 2;
        Serial.print("tempT");
        Serial.print(add);
        Serial.print(index);
        Serial.print(" sensor ");
        print_shortAddr(allAddr_X[i]);
        //Serial.print(" value=");
        Serial.print(" ");
        Serial.print(tempC/100,1);
        Serial.print("\n");
      };
      SenDiff[*ii]=tempDFF;
      TempA[*ii]=tempC;
    } 
    else {
      SenDiff[*ii]=SenDiff[*ii]*(100-tempAbz)/100;
    } 
    (*ii)++;
  }
}

// main loop - scanning sensors, cycle time depends on the number of detected sensors
void loop()
{
  //Serial.println("Start");
  ii = 0;

  for (int i = 0; i < AMOUNT_OF_ONEWIRE_INSTANCES; i++)
      setupSensorTemperature(sensors[i], totalDevs[i], allAddrses[i], i+1, &ii);
  
  
  if(Pause>30){
    delay((Pause-30)*1000);
  }
  milltempo=millis();
  if(milltempo>=0){
    //milltempo=Pause*1000-(milltempo-(long(milltempo/Pause/1000))*Pause*1000);
    milltempo=Pause*1000-(milltempo-((milltempo/Pause/1000))*Pause*1000);
  }
  else {
    //milltempo=(long(milltempo/Pause/1000))*Pause*1000-milltempo;
    milltempo=((milltempo/Pause/1000))*Pause*1000-milltempo;
  }
  delay(milltempo);

} // void loop() - end
