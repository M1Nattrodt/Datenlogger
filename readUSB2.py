#!/usr/bin/env python3

import serial.tools.list_ports
import serial
import time
from datetime import datetime
import re
# from influxdb import client as influxdb
import sys
import requests

host = 'localhost'  # Nur veraendern,
port = '8086'  # au eienem anderen Rechner liegt
user = 'rasp4'  # Benutzer und Passwort der InfluxDB Datenbank, evtl. anpassen
password = 'rasp4'
dbname = 'rasp4'
# db = influxdb.InfluxDBClient(host, port, user, password, dbname)
portUSB = '/dev/ttyUSB2'  # evtl. anpassen, je nach Arduino z. B. ttyACM0
portsa = 0   # Ab hier nichts mehr veraendern!
url_string = 'http://localhost:8086/write?db=rasp4'

ports = list(serial.tools.list_ports.comports())
for p in ports:
    print(p)
    if portUSB in p:
        print(p)
        print("Arduino gefunden")
        portsa = 1
    print('--------------------')

if portsa == 0:
    print('Kein Arduino an USB0 gefunden! Abbruch!')
    # Wenn an diesem Port kein Arduino gefunden wird,
    # dann wird das Programm beendet
    sys.exit(0)

def main():
    time.sleep(1)
    ser = serial.Serial(portUSB, 9600)
    url_string = 'http://localhost:8086/write?db=rasp4'
    ser.close()
    time.sleep(0.05)
    ser.open()
    # Falls der Arduino beim booten nicht gefunden
    # wird, evtl. 4 oder 6 einstellen.
    time.sleep(5)

    while 1:
        # Lesen, konvertieren nach utf-8
        serial_line = ser.readline().decode('utf-8')
        # Validierung, kann angepasst werden,
        # nur Daten, die diesem Muster entsprechen werden in Die Datenbank geschrieben.
        # Beispiel:    tempTa1 sensor ABCD 15.0   -> Arduino Ausgabe
        # tempT kleiner Buchstabe a-z Zahl von 0-9
        # sensor
        # 4 Buchstaben oder Zahlen
        # Gleitkommazahl kann auch negativ sein
        # print(serial_line + current_time)
        m = re.match("^tempT[a-z][0-9] sensor [a-zA-Z0-9]{4} -?[0-9]{1,5}(\.[0-9]+)*$", serial_line)
        if m:
            # wert = float(serial_line.split(' ')[3])
            wert = (serial_line.split(' ')[3])
            sensor = serial_line.split(' ')[1]
            # bei den Dallas DS18b20 Sensoren ist -127 eine Fehlmessung
            # Fehlmessungen werden sepperat in die Datenbank geschrieben
            if wert == (-127):
                sensor = 'Fehler-' & serial_line.split(' ')[1]
        
            data_string = serial_line.split(' ')[0] + ',' + serial_line.split(' ')[1] + '=' + serial_line.split(' ')[2] + ' value=' + wert
            print(data_string)
            r = requests.post(url_string, data=data_string)
      
    ser.close()  # Beim Beenden wird die serielle Schnittstelle geschlossen

if __name__ == "__main__":
    print("start")
    main()