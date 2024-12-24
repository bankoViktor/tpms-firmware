/**
 ********************************************************************************
 * @file    tpms.h
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    22.12.2024
 * @brief   Header file of TPMS.
 ********************************************************************************
 */

#include <Ticker.h>

//-----------------------------------------------------------------------------

/// @brief Tire sensor identifier type.
using sensor_id_t = uint32_t;

//-----------------------------------------------------------------------------

/// @brief Tire sensor values structure.
struct sensor_values_t
{
    /// @brief Pressure in PSI.
    float fPressure;

    /// @brief Temperature in C.
    float fTemperature;

    /// @brief Battery state.
    uint8_t bBatteryState;

    /// @brief Signal level.
    uint8_t bSignalLevel;
};

//-----------------------------------------------------------------------------

/// @brief Flags of states of the TPM system.
enum tpms_state_t
{
    TPMS_STATE_TIRE_FRONT_LEFT_WARN = (1 << 0),
    TPMS_STATE_TIRE_FRONT_LEFT_ERROR = (1 << 1),
    TPMS_STATE_TIRE_FRONT_RIGHT_WARN = (1 << 2),
    TPMS_STATE_TIRE_FRONT_RIGHT_ERROR = (1 << 3),
    TPMS_STATE_TIRE_REAR_LEFT_WARN = (1 << 4),
    TPMS_STATE_TIRE_REAR_LEFT_ERROR = (1 << 5),
    TPMS_STATE_TIRE_REAR_RIGHT_WARN = (1 << 6),
    TPMS_STATE_TIRE_REAR_RIGHT_ERROR = (1 << 7),
    TPMS_STATE_TIRE_SPARE_WARN = (1 << 8),
    TPMS_STATE_TIRE_SPARE_ERROR = (1 << 9),
    TPMS_STATE_WARN = (1 << 10),
    TPMS_STATE_ERROR = (1 << 11),
    TPMS_STATE_CAN_ERROR = (1 << 12),
    TPMS_STATE_UHF_ERROR = (1 << 13),
};

//-----------------------------------------------------------------------------

/// @brief Tire indexes.
enum tire_index_t
{
    TIRE_INDEX_FRONT_LEFT,
    TIRE_INDEX_FRONT_RIGHT,
    TIRE_INDEX_REAR_LEFT,
    TIRE_INDEX_REAR_RIGHT,
    TIRE_INDEX_SPARE,
    TIRE_COUNT,
};

//-----------------------------------------------------------------------------

/// @brief Time in seconds for reset of tire sensor values.
constexpr float SENSOR_WATCHDOG_INTERVAL_SEC = 20;

/// @brief Tire sensor class of the TPMS.
class TireSensor
{
private:
    /// @brief Watch-Dog timer.
    Ticker m_watchDogTimer;

    /// @brief Unique sensor identifier.
    sensor_id_t m_nSensorId;

    /// @brief Sensor values.
    sensor_values_t m_values;

private:
    /// @brief Default constructor.
    void resetValues();

    /// @brief Reset Watch-Dog timer of sensor and start it if xEnable equal TRUE.
    void resetWatchDog();

public:
    /// @brief Default constructor.
    TireSensor();

    /// @brief Define sensor identifier and init instance.
    /// @param dwSensorId Sensor identifier.
    void registerSensor(sensor_id_t nSensorId);

    /// @brief Reset sensor and clear identifier.
    void unregisterSensor();

    /// @brief Return sensor registration status.
    bool isRegistered() const;

    /// @brief Return sensor identifier.
    sensor_id_t getSensorId() const;

    /// @brief Return registration status.
    /// @param pValues Sensor values structure.
    void updateValues(const sensor_values_t *pValues);

    /// @brief Return pressure value.
    /// @return Pressure value in PSI.
    float getPressure() const;

    /// @brief Return temperature value.
    /// @return Temperature value in C.
    float getTemperature() const;

    /// @brief Return signal level.
    /// @return Signal level in dB.
    uint8_t getSignalLevel() const;
};

//-----------------------------------------------------------------------------

class Tpms;

//-----------------------------------------------------------------------------

/// @brief Pusher random data tire sensor.
class RandPusher
{
private:
    /// @brief Pointer to TPMS instance.
    Tpms *m_pTpms;

    /// @brief Ticker for periodical events.
    Ticker m_ticker;

    /// @brief Time counter.
    float m_fTime;

private:
    /// @brief Ticker callback function.
    void tickerCallback();

public:
    /// @brief Default constructor.
    /// @param pTpms Pointer to TPMS instance.
    RandPusher(Tpms *pTpms);

    /// @brief Start working of the pusher.
    /// @param fIntervalSec Ticker interval in seconds.
    void run(float fIntervalSec = 10);

    /// @brief Stop working of the pusher.
    void stop();
};

//-----------------------------------------------------------------------------

/// @brief Puller to CAN bus of Chery Tiggo 2.
class Ct2CanPuller
{
private:
    /// @brief Pointer to TPMS instance.
    Tpms *m_pTpms;

    /// @brief Ticker for periodical events.
    Ticker m_ticker;

private:
    /// @brief Ticker callback function.
    void tickerCallback();

public:
    /// @brief Default constructor.
    /// @param pTpms Pointer to TPMS instance.
    Ct2CanPuller(Tpms *pTpms);

    /// @brief Start working of the puller.
    /// @param fIntervalSec Ticker interval in seconds.
    void run(float fIntervalSec = 1);

    /// @brief Stop working of the puller.
    void stop();
};

//-----------------------------------------------------------------------------

/// @brief UHF transiver.
class UhfTransiver
{
private:
    /// @brief Pointer to TPMS instance.
    Tpms *m_pTpms;

public:
    /// @brief Default constructor.
    /// @param pTpms Pointer to TPMS instance.
    UhfTransiver(Tpms *pTpms);

    /// @brief Wake-Up sensor
    /// @param pSensor Tire sensor identifier.
    void wakeUpSensor(sensor_id_t nSensorId) const;
};

//-----------------------------------------------------------------------------

/// @brief Configuration class.
class Configuration
{
public:
    /// @brief Tire sensor identifiers.
    sensor_id_t nSensorIds[TIRE_COUNT];

    /// @brief Threshold of max tire temperature in C.
    float fMaxTemperatureThreshold;

    /// @brief Threshold of high tire temperature in C.
    float fHighTemperatureThreshold;

    /// @brief Threshold of max tire pressure in PSI.
    float fMaxPressureThreshold;

    /// @brief Threshold of high tire pressure in PSI.
    float fHighPressureThreshold;

    /// @brief Threshold of low tire pressure in PSI.
    float fLowPressureThreshold;

    /// @brief Threshold of min tire pressure in PSI.
    float fMinPressureThreshold;

    /// @brief Indicating units of pressure in KPa (TRUE) or PSI (FALSE).
    bool m_xPressureUnitsKPa;

    /// @brief Indicating units of temperature in F (TRUE) or C (FALSE).
    bool m_xTemperatureUnitsF;

    /// @brief Freeze tire sensor values.
    bool m_xFreezeValues;

public:
    /// @brief Load settings.
    void restore();

    /// @brief Save settings.
    void store();
};

//-----------------------------------------------------------------------------

/// @brief TPMS class.
class Tpms
{
private:
    /// @brief Tire sensors.
    TireSensor m_sensors[TIRE_COUNT];

    /// @brief States of system.
    uint16_t m_bmSysStates;

    /// @brief Settings of system.
    Configuration m_config;

    /// @brief Rand pusher.
    RandPusher m_randPusher;

private:
    /// @brief Update system state (errors and warns flags).
    void updateSystemState();

    /// @brief Update system state for tire (error and warn flags).
    /// @param pSensor Pointer to the tire sensor.
    /// @param wErrorMask Error mask of the tire.
    /// @param wWarnMask Warning mask of the tire.
    void updateTireState(const TireSensor *pSensor, uint16_t nErrorMask, uint16_t nWarnMask);

public:
    /// @brief Default constructor.
    Tpms();

    /// @brief Initialization of TPMS.
    void init();

    /// @brief Update sensor values.
    /// @param nSensorId Tire sensor identifier.
    /// @param pValues Pointer to sensor values structure.
    void updateSensorValues(sensor_id_t nSensorId, const sensor_values_t *pValues);

    /// @brief Set failure flag of CAN transmitter and update system state.
    void setCanTransmitterFailure();

    /// @brief Clear failure flag of CAN transmitter and update system state.
    void clearCanTransmitterFailure();

    /// @brief Set failure flag of UHF transmitter and update system state.
    void setUhfTransmitterFailure();

    /// @brief Clear failure flag of UHF transmitter and update system state.
    void clearUhfTransmitterFailure();
};
