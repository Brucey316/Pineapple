#ifndef SCANNING_DATA_H
#define SCANNING_DATA_H

#include "wifi.h"
#include "common_dependancies.h"

void readCSV( char* fileName);
void read_Station_Data(char* line);
void read_AP_Data(char* line);

#endif