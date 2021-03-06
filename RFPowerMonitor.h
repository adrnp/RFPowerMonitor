/**
 * RFPowerMonitor.h
 * 
 * Class to handle making RF power (signal strength) measurements from an LT5538 chip.
 * Also handles sending the measurements over the serial connection with a specifc binary protocol.
 * Means that this keeps track of the current azimuth and elevation of the antenna.
 * 
 * author      Adrien Perkins <adrienp@stanford.edu>
 * copyright  Stanford University 2017
 */

#ifndef RFPowerMonitor_h
#define RFPowerMonitor_h


#include <Arduino.h>

class RFPowerMonitor {

public:

	// for now limiting the frequency to only the options that have known data
	enum class Frequency : uint8_t {
		F_40_MHz = 0,
		F_450_MHz,
		F_880_MHz,
		F_1575_MHz,
		F_2140_MHz,
		F_2700_MHz,
		F_3600_MHz
	};

	enum class Type : uint8_t {
		SERIAL_DETECTOR = 0,
		ANALOG_DETECTOR
	};


	/* constructor for reading the power monitor input over serial (connected to difference device) */
	RFPowerMonitor(Frequency frequency);

	/* constructor for directly using the RF power monitor */
	RFPowerMonitor(Frequency frequency, uint8_t pinEnable, uint8_t pinRead);


	void setMeasurementRate(uint8_t rate) { _measurementRate = rate; };

	void setType(Type type) { _type = type; };

	void resetMeasurementCount() { _measurementCount = 0; };

	int getMeasurementCount() { return _measurementCount; };

	void setup();

	void enable();

	void disable();

	float makeMeasurement();

	void run();

	/* function to call to be able to continually parse the incoming serial data */
	int readRawMeasurement();

private:

	// NOTE: these are populated in the .cpp file
	static const float SLOPE[7]; // =     {19.9,   19.6,  19.0,  18.578, 17.7,  17.6,  18.0};
	static const float INTERCEPT[7]; // = {-87.5, -87.3, -88.8, -87.054,  -89, -87.5, -81.4};

	static const byte SYNC_1 = 0xA0;
	static const byte SYNC_2 = 0xB1;

	const Frequency _frequency;

	const float _slope;
	const float _b;
	
	const uint8_t _pinEnable;
	const uint8_t _pinRead;

	uint8_t _measurementRate;
	float _timeout;

	unsigned long _lastMeasurementTime;
	float _signalStrength;

	int _measurementCount;

	Type _type;


	// stuff needed for the sending of the binary messages
	struct __attribute__((__packed__)) SignalStrengthMessage {
		unsigned long timestamp;
		float signalStrength;
	};

	struct __attribute__((__packed__)) PhaseMessage {
		unsigned long timestamp;
		uint8_t phase0;
		uint8_t phase1;
		uint8_t phase2;
	};

	void sendPhase(uint8_t phase0, uint8_t phase1, uint8_t phase2);

	void sendSignalStrength();
	
};

#endif /* RFPowerMonitor_h */