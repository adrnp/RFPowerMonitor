/**
 * RFPowerMonitor.cpp
 * 
 * author     Adrien Perkins <adrienp@stanford.edu>
 * copyright  Stanford University 2017
 */

#include "RFPowerMonitor.h"


RFPowerMonitor::RFPowerMonitor(uint8_t frequency, uint8_t pinEnable, uint8_t pinRead) : 
_frequency(frequency),
_slope(SLOPE[frequency]),
_b(-_slope * INTERCEPT[_frequency]),
_pinEnable(pinEnable),
_pinRead(pinRead),
_measurementRate(10)
{
	// set up the pins
	pinMode(_pinEnable, OUTPUT);
	digitalWrite(_pinEnable, LOW);
}

void RFPowerMonitor::enable() {
	digitalWrite(_pinEnable, HIGH);
}

void RFPowerMonitor::disable() {
	digitalWrite(_pinEnable, LOW);
}

float RFPowerMonitor::makeMeasurement() {

	// read in the value from the sensor
	int sensorValue = analogRead(_pinRead);
	float voltage = sensorValue * (5.0/1023.0) * 1000.0; // mV
	
	// convert voltage to dBm and return that value
	float dBm = (voltage - _b)/_slope;
	return dBm;
}
