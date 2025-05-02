#include "commandParser.h"
#include "config.h"
#include "eventReporter.h"
#include <Arduino.h>

void handleCommands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        if (cmd == "START") {
            masterEnabled = true;
            digitalWrite(LED_BUILTIN, HIGH);
            Serial.println("STATUS:RUNNING");
        } else if (cmd == "STOP") {
            masterEnabled = false;
            digitalWrite(LED_BUILTIN, LOW);
            Serial.println("STATUS:STOPPED");
        } else if (cmd.startsWith("ENABLE:")) {
            int station = cmd.substring(7).toInt();
            if (station < STATION_COUNT) stationEnabled[station] = true;
        } else if (cmd.startsWith("DISABLE:")) {
            int station = cmd.substring(8).toInt();
            if (station < STATION_COUNT) stationEnabled[station] = false;
        } else if (cmd == "STATUS") {
            reportStatus();
        }
    }
}