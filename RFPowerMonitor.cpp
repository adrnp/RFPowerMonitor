/**
 * RFPowerMonitor.cpp
 * 
 * author     Adrien Perkins <adrienp@stanford.edu>
 * copyright  Stanford University 2017
 */

#include "RFPowerMonitor.h"

// populate the arrays with the slope and intercept data
const float RFPowerMonitor::SLOPE[] =     {19.9,   19.6,  19.0,  18.578, 17.7,  17.6,  18.0};
const float RFPowerMonitor::INTERCEPT[] = {-87.5, -87.3, -88.8, -87.054,  -89, -87.5, -81.4};


RFPowerMonitor::RFPowerMonitor(Frequency frequency, uint8_t pinEnable, uint8_t pinRead) : 
_frequency(frequency),
_slope(SLOPE[static_cast<uint8_t> (frequency)]),
_b(-_slope * INTERCEPT[static_cast<uint8_t> (frequency)]),
_pinEnable(pinEnable),
_pinRead(pinRead),
_measurementRate(10),
_lastMeasurementTime(0),
_signalStrength(0),
_measurementCount(0)
{
	// given the measurement rate (in Hz) calculate the timeout to wait between measurements
	_timeout = 1.0/_measurementRate * 1000.0;

	// set up the pins
	pinMode(_pinEnable, OUTPUT);
	digitalWrite(_pinEnable, LOW);
}


void RFPowerMonitor::setup() {
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
	_measurementCount++;

	// read in the value from the sensor
	//int sensorValue = analogRead(_pinRead);
	//float voltage = sensorValue * (5.0/1023.0) * 1000.0; // mV
	
	// NOTE: other board reads based on 3.3V not 5V
	// TODO: make sure this isn't a problem (i.e. still returns correct results)
	int sensorValue = readRawMeasurement();
	//Serial.println(sensorValue);
	float voltage = sensorValue * (3.3/1023.0) * 1000.0; // mV
	//Serial.println(voltage);

	// convert voltage to dBm and return that value
	_signalStrength = (voltage - _b)/_slope;


	// DEBUG OUTPUT
	Serial1.println("");
	Serial1.print("signal strength: ");
	Serial1.println(_signalStrength);

	return _signalStrength;
}

void RFPowerMonitor::run() {

	// only make a measurement at the desired rate
	if (millis() - _lastMeasurementTime < _timeout) {
		return;
	}

	// make the measurement now
	makeMeasurement();

	// send the measurement along with the azimuth and elevation for this measurement
	sendSignalStrength();

	return;
}

int RFPowerMonitor::readRawMeasurement() {
	//Serial.println("reading from serial3");
	//Serial3.println(Serial3.readString());

	// need the different parse states
	const uint8_t PARSE_SYNC1 = 0;
	const uint8_t PARSE_SYNC2 = 1;
	const uint8_t PARSE_MSG_ID = 2;
	const uint8_t PARSE_MSG = 3;
	const uint8_t PARSE_CHK_A = 4;
	const uint8_t PARSE_CHK_B = 5;

	const unsigned long timeout = 200UL;  // timeout to wait
	unsigned long startTime = millis();

	uint8_t parseMode = PARSE_SYNC1;

	uint8_t msgId = 0;
	uint8_t msgIndex = 0;
	uint8_t buf[16];
	int value = 0;
	uint8_t b;

	uint8_t chkA = 0;
	uint8_t chkB = 0;

	// TODO: somehow limit this to only being available if the compiled target is a mega
	while (millis() - startTime < timeout) {
		if (Serial3.available() > 0) {
			//Serial.println("data available from serial 3");
		
			// read in a byte
			b = Serial3.read();
			
			// DEBUG OUTPUT
			Serial1.print(b, HEX);
			Serial1.print(" ");

			// handle the read char based on the parse mode
			switch (parseMode) {
				case PARSE_SYNC1:
					if (b == SYNC_1) {
						//Serial.println("sync1 match");
						parseMode = PARSE_SYNC2;

						// reset the checksum
						chkA = 0;
						chkB = 0;
					}
					break;

				case PARSE_SYNC2:
					if (b == SYNC_2) {
						//Serial.println("sync2 match");
						parseMode = PARSE_MSG_ID;
					} else {
						//Serial.println("sync2 fail");
						parseMode = PARSE_SYNC1;
					}
					break;

				case PARSE_MSG_ID:
					//Serial.println("parsing message");
					msgId = (uint8_t) b;

					msgIndex = 0;
					parseMode = PARSE_MSG;

					break;

				case PARSE_MSG:
					// add the byte to the buffer
					buf[msgIndex] = b;

					chkA += b;
					chkB += chkA;

					// increment index and see if completed reading
					msgIndex++;
					if (msgIndex >= 5) {
						// DEBUG OUTPUT
						Serial1.println("have msg");
						Serial1.print("calculated checksum: ");
						Serial1.print(chkA, HEX);
						Serial1.print(" ");
						Serial1.println(chkB, HEX);

						// convert the read bytes to the int value they are
						value = (((int) buf[4]) << 8) | ((int) buf[3]);

						// return the value
						// for now treating this as basically calling this function
						// instead of doing analog read
						// TODO: find a more efficient way to do this!!!
						//return value;
						parseMode = PARSE_CHK_A;
					}
					break;

				case PARSE_CHK_A:
					if (b == chkA) {
						// DEBUG OUTPUT
						Serial1.println("chk A match");
						parseMode = PARSE_CHK_B;
					} else {
						// DEBUG OUTPUT
						Serial1.println("chk A FAILED!!!!");
						parseMode = PARSE_SYNC1;
					}
					break;

				case PARSE_CHK_B:
					if (b == chkB) {
						// DEBUG OUTPUT
						Serial1.println("chk B match");
						// all is good, so let's return the value that was received
						// and send a phase message
						sendPhase(buf[0], buf[1], buf[2]);
						return value;
					} else {
						// DEBUG OUTPUT
						Serial1.println("chk B failed!!!!");
					}
					parseMode = PARSE_SYNC1;
					break;
					
				default:
					parseMode = PARSE_SYNC1;
					break;
			}
		}
	}

	// if never got a measurement, return 0 (no measurement)
	// TODO: should figure out a better thing to send to symbolize no measurement....
	return 0;
	

}


/* private functions */

void RFPowerMonitor::sendPhase(uint8_t phase0, uint8_t phase1, uint8_t phase2) {
	PhaseMessage msg;
	msg.timestamp = _lastMeasurementTime;
	msg.phase0 = phase0;
	msg.phase1 = phase1;
	msg.phase2 = phase2;

	Serial.write(SYNC_1);
	Serial.write(SYNC_2);
	Serial.write((uint8_t) 3);  // the message ID for a phase message
	Serial.write((uint8_t*) &msg, sizeof(msg));
}

void RFPowerMonitor::sendSignalStrength() {

	// pack the message
	SignalStrengthMessage msg;
	msg.timestamp = _lastMeasurementTime;
	msg.signalStrength = _signalStrength;


	// send the data over the serial port
	// sync bytes first
	Serial.write(SYNC_1);
	Serial.write(SYNC_2);

	// the actual data
	Serial.write((uint8_t*) &msg, sizeof(msg));

	return;
}

