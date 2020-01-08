#!/usr/bin/env python

import sys
from datetime import datetime
import time
import re
import requests
import serial
import serial.tools.list_ports
from influxdb import client as influxdb


class Bootlogger:
    def __init__(self, database, name, password):
        self.current_amount_of_ports = 0
        self.arduino_ports = []
        self.db = influxdb.InfluxDBClient('localhost', 8086, database, password, database)
        print "database " + database + " is set up"

    def setSerialPorts(self):
        time.sleep(5) #give Arduino some time to connect properly
        ports = []
        arduino_port_names = [arduino_port.name for arduino_port in self.arduino_ports]
        for port in serial.tools.list_ports.comports():
            ports.append(port)

        self.current_amount_of_ports = self.get_amount_of_ports()

        for port in ports:
            if port.device not in arduino_port_names:
                serial_port = serial.Serial(port.device, 9600, timeout=5)
                serial_port.setDTR(False)  # Restart Arduino
                time.sleep(0.022)  # Restart Arduino
                serial_port.setDTR(True)  # Restart Arduino (necessary for identification)
                line = serial_port.readline()
                if "Arduino" in line:
                    self.arduino_ports.append(serial_port)
                    self.displayArduinoInformation(serial_port)


    def get_amount_of_ports(self):
        return len(serial.tools.list_ports.comports())

    def isValidTemperature(self, line):
        return re.match("^tempT[a-z][0-9] sensor [a-zA-Z0-9]{4} -?[0-9]{1,4}(\.[0-9]+)$", line)

    def parseAndWriteToDB(self, temperature_string):
        wert = float(temperature_string.split(' ')[3])
        current_time = datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')
        json_body = [
            {
                "measurement": temperature_string.split(' ')[0],
                "tags": {
                    temperature_string.split(' ')[1]: temperature_string.split(' ')[2]
                },
                "time": current_time,
                "fields": {
                    "value": wert
                }
            }
        ]

        self.db.write_points(json_body)

    def displayArduinoInformation(self, arduino_port):
        print "-" * 20
        print "-" * 20
        print "Arduino" + str(self.arduino_ports.index(arduino_port)+1)
        print "-" * 20
        ap_info = arduino_port.readlines()
        while self.isValidTemperature(ap_info[-1]):
            self.parseAndWriteToDB(ap_info[-1])
            ap_info = ap_info[:-1]

        print '.'.join(ap_info)
        print "-" * 20
        sys.stdout.flush()

    def change_serial_port_amount(self):
        old_length = len(self.arduino_ports)
        self.setSerialPorts()
        new_length = len(self.arduino_ports)

        if (new_length > old_length):
            print "Found %d additional Arduino, currently connected to %d Arduino" % (
            new_length - old_length, new_length)
        else:
            print "error in change_serial_port_amount function"

        print "Found Arduino port(s): %s" % (
            None if new_length == 0 else ", ".join([serial_port.name for serial_port in self.arduino_ports]))
        print "---"
        sys.stdout.flush()

    def storeToDatabase(self):
        self.current_amount_of_ports = self.get_amount_of_ports()
        self.change_serial_port_amount()
        if len(self.arduino_ports) == 0:
            print "Couldn't find an Arduino, please plug it in to continue"
        else:
            print "Writing to Database"
        while True:
            if self.current_amount_of_ports != self.get_amount_of_ports():
                self.current_amount_of_ports = self.get_amount_of_ports()
                self.change_serial_port_amount()

            for arduino_port in self.arduino_ports:
                line = arduino_port.readline()

                if self.isValidTemperature(line):
                    self.parseAndWriteToDB(line)
                else:
                    if line != '': print line

            sys.stdout.flush()


def main():
    time.sleep(30)  # wait 30 seconds for an internet connection at boot
    database = "rasp4"
    name = "rasp4"
    password = "rasp4"
    bootlogger = Bootlogger(database, name, password)

    while True:
        try:
            bootlogger.storeToDatabase()
        except Exception as e:
            print "Lost connection to at least one Arduino -> necessary restart..."
            bootlogger.arduino_ports = []
            for t in range(15, 0, -5):
                print "Restarting in %d seconds" % (t)
                sys.stdout.flush()
                time.sleep(5)

            print "Restarting..."


if __name__ == "__main__":
    print "start"
    main()
    
