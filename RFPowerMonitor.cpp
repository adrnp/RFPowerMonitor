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
_measurementRate(10),
_lastMeasurementTime(0),
_signalStrength(0),
_azimuth(-1),
_elevation(-1)
{
	// given the measurement rate (in Hz) calculate the timeout to wait between measurements
	_timeout = 1.0/_measurementRate * 1000.0;

	// set up the pins
	pinMode(_pinEnable, OUTPUT);
	digitalWrite(_pinEnable, LOW);

	// start the serial connection
	Serial.begin(115200);
}

void RFPowerMonitor::enable() {
	digitalWrite(_pinEnable, HIGH);
}

void RFPowerMonitor::disable() {
	digitalWrite(_pinEnable, LOW);
}

float RFPowerMonitor::makeMeasurement() {

	// get the timestamp for this measurement
	_lastMeasurementTime = millis();

	// read in the value from the sensor
	int sensorValue = analogRead(_pinRead);
	float voltage = sensorValue * (5.0/1023.0) * 1000.0; // mV
	
	// convert voltage to dBm and return that value
	_signalStrength = (voltage - _b)/_slope;
	return _signalStrength;
}

void RFPowerMonitor::run() {

	// only make a measurement at the desired rate
	if (millis() - _lastMeasurementTime < _timeout) {
		return;
	}

	// make the measurement now
	// NOTE: this updates _lastMeasurementTime
	makeMeasurement();

	// send the measurement along with the azimuth and elevation for this measurement
	sendData();

	return;
}


/* private functions */

void RFPowerMonitor::sendData() {

	// pack the message
	MeasurementMessage msg;
	msg.timestamp = _lastMeasurementTime;
	msg.signalStrength = _signalStrength;
	msg.azimuth = _azimuth;
	msg.elevation = _elevation;


	// send the data over the serial port
	// sync bytes first
	Serial.write(SYNC_1);
	Serial.write(SYNC_2);

	// the actual data
	Serial.write((uint8_t*) &msg, sizeof(msg));

	return;
}

