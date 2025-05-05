#include "commandParser.h"
#include "config.h"
#include "eventReporter.h"
#include "stateMachine.h"
#include <Arduino.h>

void handleCommands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        if (cmd == "START") {
            startSystem();
        } else if (cmd == "STOP") {
            stopSystem();
        } else if (cmd.startsWith("ENABLE:")) {
            enableStation(cmd.substring(7).toInt());
        } else if (cmd.startsWith("DISABLE:")) {
            disableStation(cmd.substring(8).toInt());
        }
    }
}