#include "commandParser.h"
#include "config.h"
#include "eventReporter.h"
#include <Arduino.h>

void handleCommands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        if (cmd == "START") {
            masterEnabled = true;
            Serial.println("STATUS:RUNNING");
        } else if (cmd == "STOP") {
            masterEnabled = false;
            Serial.println("STOPPED");
        } else if (cmd.startsWith("ENABLE:")) {
            int station = cmd.substring(7).toInt();
            if (station < STATION_COUNT) stationEnabled[station] = true;
        } else if (cmd.startsWith("DISABLE:")) {
            int station = cmd.substring(8).toInt();
            if (station < STATION_COUNT) stationEnabled[station] = false;
        } else if (cmd == "RESET") {
            for (int i = 0; i < STATION_COUNT; i++) switchFailCount[i] = 0;
            Serial.println("RESET_DONE");
        } else if (cmd == "STATUS") {
            reportStatus();
        } else if (cmd == "COUNTS") {
            for (int i = 0; i < STATION_COUNT; i++) {
                Serial.print("FAIL_COUNT:");
                Serial.print(i);
                Serial.print(":");
                Serial.println(switchFailCount[i]);
            }
        }
    }
}