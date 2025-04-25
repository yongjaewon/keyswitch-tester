//eventReporter.h
#pragma once

void reportCurrent(int station, float current);
void reportComplete(int station);
void reportFailure(int station, int failCount);
void reportStatus();