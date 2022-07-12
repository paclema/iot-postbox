#include "PowerManagement.h"

PowerManagement::PowerManagement(void) : 
  vBusSense(VBUS_SENSE_PIN, VBUS_ADC_CHANNEL, (float)VBUS_VOLTAGE_DIVIDER_COEFICIENT, "VBUS"),
  vBatSense(VBAT_SENSE_PIN, VBAT_ADC_CHANNEL, (float)VBAT_VOLTAGE_DIVIDER_COEFICIENT, "VBAT")
{


}


PowerManagement::~PowerManagement(void) {
}


void PowerManagement::setup(void){
	vBusSense.setup(VBUS_ADC_ATTEN);
	vBatSense.setup(VBAT_ADC_ATTEN);
	pinMode(VBAT_STAT_SENSE_PIN, INPUT);
	attachInterrupt(VBAT_STAT_SENSE_PIN, std::bind(&PowerManagement::isrCharging,this), CHANGE);
}


void PowerManagement::update(void){
	//Read ADCSense pins
  	vBusSense.updatedADC();
	vBatSense.updatedADC();
	vBatStat = digitalRead(VBAT_STAT_SENSE_PIN);

	lastPowerStatus = powerStatus;
	// Charging always happens if vBus > 3,75v for MCP73831/2, 4v for TP4056
	// if (vBusSense.mV >= VBUS_MIN){
	if (vBusSense.mV >= 4000){
	if (vBatStat == LOW) {
		chargingStatus = ChargingStatus::Charging;
		lastChargingStatus = chargingStatus;
		powerStatus = PowerStatus::BatteryAndUSBPowered;
	} else if (vBatStat == HIGH) {
		if (lastChargingStatus ==  ChargingStatus::Charging) {
		chargingStatus = ChargingStatus::Charged;
		lastChargingStatus = chargingStatus;
		powerStatus = PowerStatus::BatteryAndUSBPowered;
		} else if (lastChargingStatus !=  ChargingStatus::Charging) {
		chargingStatus = ChargingStatus::NotCharging;
		powerStatus = PowerStatus::USBPowered;
		}
	} else chargingStatus = ChargingStatus::Unknown;

	// } else if (vBusSense.mV <= VBUS_MIN-1) {
	} else if (vBusSense.mV <= 3999) {
	chargingStatus = ChargingStatus::NotCharging;
	lastChargingStatus = chargingStatus;
	// if (vBatSense.mV >= VBAT_MIN){
	if (vBatSense.mV >= 2700){
		powerStatus = PowerStatus::BatteryPowered;
	} else if (vBatSense.mV <= 100) powerStatus = PowerStatus::USBPowered;
	}

}
