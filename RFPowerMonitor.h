/**
 * RFPowerMonitor.h
 * 
 * Class to handle making RF power (signal strength) measurements from an LT5538 chip.
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
	enum Frequency : uint8_t {
		F_40 = 0,
		F_450,
		F_880,
		F_1575,
		F_2140,
		F_2700,
		F_3600
	};


	RFPowerMonitor(uint8_t frequency, uint8_t pinEnable, uint8_t pinRead);


	void setMeasurementRate(uint8_t rate) { _measurementRate = rate; };

	void enable();

	float makeMeasurement();

	// TODO: add the following functions for allowing this monitor to run continuously
	// 	- sendData()
	// 	- calculateChecksum()
	// 	- run()


private:

	static constexpr float SLOPE[] =     {19.9,   19.6,  19.0,  18.578, 17.7,  17.6,  18.0};
	static constexpr float INTERCEPT[] = {-87.5, -87.3, -88.8, -87.054,  -89, -87.5, -81.4};


	const uint8_t _frequency;

	const float _slope;
	const float _b;
	
	const uint8_t _pinEnable;
	const uint8_t _pinRead;

	uint8_t _measurementRate;


};

#endif /* RFPowerMonitor_h */