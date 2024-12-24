/**
 ********************************************************************************
 * @file    tpms.cpp
 * @author  Viktor Banko S. (bankviktor14@mail.com)
 * @date    22.12.2024
 * @brief   Source file of TPMS.
 ********************************************************************************
 */

#include "tpms.h"
#include <esp_log.h>
#include <math.h>
#include <cstring>

static const char *TIRE_SENSOR_LOG_TAG = "TireSensor";
static const char *RAND_PUSHER_LOG_TAG = "RandPusher";
static const char *CAN_PULLER_LOG_TAG = "CanPuller";
static const char *UHF_TRANSIVER_LOG_TAG = "UhfTransiver";
static const char *CONFIG_LOG_TAG = "Config";
static const char *TPMS_LOG_TAG = "TPMS";

//-----------------------------------------------------------------------------

constexpr float SENSOR_PRESSURE_NOT_VALUE = -1;
constexpr float SENSOR_TEMPERATURE_NOT_VALUE = -100;

TireSensor::TireSensor() : m_nSensorId(0)
{
    m_watchDogTimer.detach();
    resetValues();
}

void TireSensor::resetValues()
{
    m_values.fPressure = SENSOR_PRESSURE_NOT_VALUE;
    m_values.fTemperature = SENSOR_TEMPERATURE_NOT_VALUE;
    m_values.bBatteryState = 0;
    m_values.bSignalLevel = 0;
}

void TireSensor::resetWatchDog()
{
    m_watchDogTimer.detach();
    m_watchDogTimer.once<TireSensor *>(
        SENSOR_WATCHDOG_INTERVAL_SEC,
        [](TireSensor *pSensor)
        { pSensor->resetValues(); }, this);
}

void TireSensor::registerSensor(sensor_id_t nSensorId)
{
    m_watchDogTimer.detach();
    resetValues();
    m_nSensorId = nSensorId;

    ESP_LOGI(TIRE_SENSOR_LOG_TAG, "%08X : Registered", m_nSensorId);
}

void TireSensor::unregisterSensor()
{
    m_watchDogTimer.detach();
    resetValues();
    m_nSensorId = 0;

    ESP_LOGI(TIRE_SENSOR_LOG_TAG, "%08X : Unregistered", m_nSensorId);
}

bool TireSensor::isRegistered() const
{
    return m_nSensorId != 0;
}

sensor_id_t TireSensor::getSensorId() const
{
    return m_nSensorId;
}

void TireSensor::updateValues(const sensor_values_t *pValues)
{
    if (!isRegistered())
    {
        return;
    }

    memcpy(&m_values, pValues, sizeof(sensor_values_t));

    resetWatchDog();

    ESP_LOGD(TIRE_SENSOR_LOG_TAG,
             "%08X : Updated values: P=%.2f PSI, T=%.2f C, B=%u, S=%u",
             m_nSensorId,
             m_values.fPressure,
             m_values.fTemperature,
             m_values.bBatteryState,
             m_values.bSignalLevel);
}

float TireSensor::getPressure() const
{
    return m_values.fPressure;
}

float TireSensor::getTemperature() const
{
    return m_values.fTemperature;
}

uint8_t TireSensor::getSignalLevel() const
{
    return m_values.bSignalLevel;
}

//-----------------------------------------------------------------------------

constexpr float MAX_TEMPERATURE_C = 80.0;
constexpr float MIN_TEMPERATURE_C = -40.0;
constexpr float MAX_PRESSURE_PSI = 4.0;
constexpr float MIN_PRESSURE_PSI = 0.0;

RandPusher::RandPusher(Tpms *pTpms) : m_pTpms(pTpms)
{
}

static float calculateTemperature(float fTime)
{
    float fAmplitude = (MAX_TEMPERATURE_C - MIN_TEMPERATURE_C) / 2.0;
    float fOffset = (MAX_TEMPERATURE_C + MIN_TEMPERATURE_C) / 2.0;
    return fAmplitude * sin(fTime) + fOffset;
}

static float calculatePressure(float fTime)
{
    float fAmplitude = (MAX_PRESSURE_PSI - MIN_PRESSURE_PSI) / 2.0;
    float fOffset = (MAX_PRESSURE_PSI + MIN_PRESSURE_PSI) / 2.0;
    return fAmplitude * cos(fTime) + fOffset;
}

void RandPusher::tickerCallback()
{
    if (m_fTime >= 2 * M_PI)
    {
        m_fTime = 0;
    }

    bool xSkip =
        m_fTime >= 0.4 && m_fTime <= 0.6 ||
        m_fTime >= 2.4 && m_fTime <= 2.6 ||
        m_fTime >= 4.4 && m_fTime <= 4.6;
    if (!xSkip)
    {
        sensor_values_t values = {0};
        values.fPressure = calculatePressure(m_fTime);
        values.fTemperature = calculateTemperature(m_fTime);
        values.bSignalLevel = 12;
        m_pTpms->updateSensorValues(0x00000001, &values);
    }

    m_fTime += 0.1;
}

void RandPusher::run(float fIntervalSec)
{
    m_ticker.detach();

    if (m_pTpms == nullptr)
    {
        ESP_LOGE(RAND_PUSHER_LOG_TAG, "Null pointer to TPMS core");
        return;
    }

    m_ticker.attach<RandPusher *>(
        fIntervalSec,
        [](RandPusher *pPusher)
        { pPusher->tickerCallback(); }, this);

    ESP_LOGI(RAND_PUSHER_LOG_TAG, "Started");
}

void RandPusher::stop()
{
    m_ticker.detach();

    ESP_LOGI(RAND_PUSHER_LOG_TAG, "Stopped");
}

//-----------------------------------------------------------------------------

Ct2CanPuller::Ct2CanPuller(Tpms *pTpms) : m_pTpms(pTpms)
{
}

void Ct2CanPuller::tickerCallback()
{
    ESP_LOGD(CAN_PULLER_LOG_TAG, "Puller");
}

void Ct2CanPuller::run(float fIntervalSec)
{
    m_ticker.detach();

    if (m_pTpms == nullptr)
    {
        ESP_LOGE(CAN_PULLER_LOG_TAG, "Null pointer to TPMS core");
        return;
    }

    m_ticker.attach<Ct2CanPuller *>(
        fIntervalSec,
        [](Ct2CanPuller *pPusher)
        { pPusher->tickerCallback(); }, this);

    ESP_LOGI(CAN_PULLER_LOG_TAG, "Started");
}

void Ct2CanPuller::stop()
{
    m_ticker.detach();

    ESP_LOGI(CAN_PULLER_LOG_TAG, "Stopped");
}

//-----------------------------------------------------------------------------

UhfTransiver::UhfTransiver(Tpms *pTpms) : m_pTpms(pTpms)
{
}

void UhfTransiver::wakeUpSensor(sensor_id_t nSensorId) const
{
    ESP_LOGI(UHF_TRANSIVER_LOG_TAG, "Wake-Up Sensor");
}

//-----------------------------------------------------------------------------

void Configuration::restore()
{
    // Tire sernsor Ids
    nSensorIds[TIRE_INDEX_FRONT_LEFT] = 0x00000001;
    nSensorIds[TIRE_INDEX_FRONT_RIGHT] = 0x00000002;
    nSensorIds[TIRE_INDEX_REAR_LEFT] = 0x00000003;
    nSensorIds[TIRE_INDEX_REAR_RIGHT] = 0x00000004;

    // Pressure thresholds
    fMaxTemperatureThreshold = 45;
    fHighTemperatureThreshold = 30;

    // Temperature thresholds
    fMaxPressureThreshold = 2.8;
    fHighPressureThreshold = 2.3;
    fLowPressureThreshold = 2.1;
    fMinPressureThreshold = 1.8;

    // Units
    m_xPressureUnitsKPa = false;
    m_xTemperatureUnitsF = false;

    // Misc
    m_xFreezeValues = true;

    ESP_LOGI(CONFIG_LOG_TAG, "Loaded");
}

void Configuration::store()
{
    ESP_LOGE(CONFIG_LOG_TAG, "store() method not implementd");
}

//-----------------------------------------------------------------------------

Tpms::Tpms() : m_randPusher(this)
{
}

void Tpms::init()
{
    // Load configuration.
    m_config.restore();

    // Register tire sensors
    for (int i = 0; i < TIRE_COUNT; i++)
    {
        sensor_id_t nSensorId = m_config.nSensorIds[i];
        if (nSensorId != 0)
        {
            m_sensors[i].registerSensor(nSensorId);
        }
    }

    // Start system
    m_randPusher.run();
}

void Tpms::updateTireState(const TireSensor *pSensor, uint16_t nErrorMask, uint16_t nWarnMask)
{
    // Clear bit
    m_bmSysStates &= ~(nErrorMask | nWarnMask);

    // Get current sensor values
    if (!pSensor->isRegistered())
    {
        return;
    }

    float fPressure = pSensor->getPressure();
    float fTemperature = pSensor->getTemperature();

    // Check pressure error thresholds
    bool xPressureError =
        fPressure != SENSOR_PRESSURE_NOT_VALUE &&
        (fPressure < m_config.fMinPressureThreshold || fPressure > m_config.fMaxPressureThreshold);

    // Check temperature error thresholds
    bool xTemperatureError =
        fTemperature != SENSOR_TEMPERATURE_NOT_VALUE &&
        fTemperature > m_config.fMaxTemperatureThreshold;

    // Set bit if required
    bool xError = xPressureError || xTemperatureError;
    if (xError)
    {
        m_bmSysStates |= nErrorMask;
    }

    // Check pressure warn thresholds
    bool xPressureWarn =
        fPressure != SENSOR_PRESSURE_NOT_VALUE &&
        (fPressure < m_config.fLowPressureThreshold || fPressure > m_config.fHighPressureThreshold);

    // Check temperature warn thresholds
    bool xTemperatureWarn =
        fTemperature != SENSOR_TEMPERATURE_NOT_VALUE &&
        fTemperature > m_config.fHighTemperatureThreshold;

    // Set bit if required
    bool xWarn = xPressureWarn || xTemperatureWarn;
    if (!xError && xWarn)
    {
        m_bmSysStates |= nWarnMask;
    }
}

void Tpms::updateSystemState()
{
    // Tires Errors and Warns
    uint anErrorMasks[TIRE_COUNT] =
        {
            TPMS_STATE_TIRE_FRONT_LEFT_ERROR,
            TPMS_STATE_TIRE_FRONT_RIGHT_ERROR,
            TPMS_STATE_TIRE_REAR_LEFT_ERROR,
            TPMS_STATE_TIRE_REAR_RIGHT_ERROR,
            TPMS_STATE_TIRE_SPARE_ERROR};
    uint anWarnMasks[TIRE_COUNT] =
        {
            TPMS_STATE_TIRE_FRONT_LEFT_WARN,
            TPMS_STATE_TIRE_FRONT_RIGHT_WARN,
            TPMS_STATE_TIRE_REAR_LEFT_WARN,
            TPMS_STATE_TIRE_REAR_RIGHT_WARN,
            TPMS_STATE_TIRE_SPARE_WARN};
    for (int i = 0; i < TIRE_COUNT; i++)
    {
        updateTireState(&m_sensors[i], anErrorMasks[i], anWarnMasks[i]);
    }

    // Main Error (continuous light)
    m_bmSysStates &= ~TPMS_STATE_ERROR;
    bool xError =
        m_bmSysStates & TPMS_STATE_TIRE_FRONT_LEFT_ERROR ||
        m_bmSysStates & TPMS_STATE_TIRE_FRONT_RIGHT_ERROR ||
        m_bmSysStates & TPMS_STATE_TIRE_REAR_LEFT_ERROR ||
        m_bmSysStates & TPMS_STATE_TIRE_REAR_RIGHT_ERROR ||
        m_bmSysStates & TPMS_STATE_CAN_ERROR ||
        m_bmSysStates & TPMS_STATE_UHF_ERROR;
    if (xError)
    {
        m_bmSysStates |= TPMS_STATE_ERROR;
    }

    // Main Waring (blinking light)
    m_bmSysStates &= ~TPMS_STATE_WARN;
    bool xWarn =
        m_bmSysStates & TPMS_STATE_TIRE_FRONT_LEFT_WARN ||
        m_bmSysStates & TPMS_STATE_TIRE_FRONT_RIGHT_WARN ||
        m_bmSysStates & TPMS_STATE_TIRE_REAR_LEFT_WARN ||
        m_bmSysStates & TPMS_STATE_TIRE_REAR_RIGHT_WARN ||
        m_bmSysStates & TPMS_STATE_TIRE_SPARE_WARN ||
        m_bmSysStates & TPMS_STATE_TIRE_SPARE_ERROR;
    if (!(m_bmSysStates & TPMS_STATE_ERROR) && xWarn)
    {
        m_bmSysStates |= TPMS_STATE_WARN;
    }

    ESP_LOGI(CONFIG_LOG_TAG, "CAN%i  UHF%i  FL-E%i-W%i  FR-E%i-W%i  RL-E%i-W%i  RR-E%i-W%i  SPR-E%i-W%i",
             m_bmSysStates & TPMS_STATE_CAN_ERROR,
             m_bmSysStates & TPMS_STATE_UHF_ERROR,
             m_bmSysStates & TPMS_STATE_TIRE_FRONT_LEFT_ERROR,
             m_bmSysStates & TPMS_STATE_TIRE_FRONT_LEFT_WARN,
             m_bmSysStates & TPMS_STATE_TIRE_FRONT_RIGHT_ERROR,
             m_bmSysStates & TPMS_STATE_TIRE_FRONT_RIGHT_WARN,
             m_bmSysStates & TPMS_STATE_TIRE_REAR_LEFT_ERROR,
             m_bmSysStates & TPMS_STATE_TIRE_REAR_LEFT_WARN,
             m_bmSysStates & TPMS_STATE_TIRE_REAR_RIGHT_ERROR,
             m_bmSysStates & TPMS_STATE_TIRE_REAR_RIGHT_WARN,
             m_bmSysStates & TPMS_STATE_TIRE_SPARE_ERROR,
             m_bmSysStates & TPMS_STATE_TIRE_SPARE_WARN);
}

void Tpms::updateSensorValues(sensor_id_t nSensorId, const sensor_values_t *pValues)
{
    // Update sensor values
    for (int i = 0; i < TIRE_COUNT; i++)
    {
        TireSensor *pSensor = &m_sensors[i];

        if (pSensor->isRegistered() &&
            pSensor->getSensorId() != nSensorId)
        {
            pSensor->updateValues(pValues);
            break;
        }
    }

    // Check system state
    updateSystemState();
}

void Tpms::setCanTransmitterFailure()
{
    m_bmSysStates |= TPMS_STATE_CAN_ERROR;
    updateSystemState();
}

void Tpms::clearCanTransmitterFailure()
{
    m_bmSysStates &= ~TPMS_STATE_CAN_ERROR;
    updateSystemState();
}

void Tpms::setUhfTransmitterFailure()
{
    m_bmSysStates |= TPMS_STATE_UHF_ERROR;
    updateSystemState();
}

void Tpms::clearUhfTransmitterFailure()
{
    m_bmSysStates &= ~TPMS_STATE_UHF_ERROR;
    updateSystemState();
}
