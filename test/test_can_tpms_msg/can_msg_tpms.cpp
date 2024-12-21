#include <unity.h>
#include <CAN_CheryTiggo2.h>
#include <CAN_CheryTiggo2_Types.h>
#include <functional>

void check_struct_size()
{
    int nBytes = sizeof(Ct2CanMsgTpms);
    TEST_ASSERT_EQUAL_INT(8, nBytes);
}

using f_getter_f = float (Ct2CanMsgTpms::*)();
using f_setter_f = void (Ct2CanMsgTpms::*)(float);

void check_pressure(f_getter_f getter, f_setter_f setter, size_t nOffsetBytes)
{
    float fPressure = 2.0;
    float fExpected = PRESSURE_CONVERT_FROM(PRESSURE_CONVERT_TO(fPressure));
    float fActual;

    Ct2CanMsgTpms msg;
    (msg.*setter)(fExpected);

    uint8_t abExpected[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    abExpected[nOffsetBytes] = PRESSURE_CONVERT_TO(fPressure);
    uint8_t *pbActual = (uint8_t *)&msg;
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(abExpected, pbActual, sizeof(abExpected), "Buffer not valid.");

    fActual = (msg.*getter)();
    TEST_ASSERT_EQUAL_FLOAT_MESSAGE(fExpected, fActual, "Readed data mismatch.");
}

void check_pressure_FL() { check_pressure(&Ct2CanMsgTpms::pressureFL, &Ct2CanMsgTpms::setPressureFL, 4); }
void check_pressure_FR() { check_pressure(&Ct2CanMsgTpms::pressureFR, &Ct2CanMsgTpms::setPressureFR, 5); }
void check_pressure_RL() { check_pressure(&Ct2CanMsgTpms::pressureRL, &Ct2CanMsgTpms::setPressureRL, 6); }
void check_pressure_RR() { check_pressure(&Ct2CanMsgTpms::pressureRR, &Ct2CanMsgTpms::setPressureRR, 7); }

using x_getter_f = bool (Ct2CanMsgTpms::*)();
using x_setter_f = void (Ct2CanMsgTpms::*)(bool);

void check_bit(x_getter_f getter, x_setter_f setter, uint16_t wMask)
{
    Ct2CanMsgTpms msg;
    (msg.*setter)(true);

    uint8_t abExpected[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t *pbActual = (uint8_t *)&msg;
    abExpected[0] |= *((uint8_t *)&wMask + 0);
    abExpected[1] |= *((uint8_t *)&wMask + 1);
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(abExpected, pbActual, sizeof(abExpected), "Buffer not valid.");

    bool xActual = (msg.*getter)();
    TEST_ASSERT_TRUE(xActual);
}

void check_bit_tire_alarm_FL() { check_bit(&Ct2CanMsgTpms::tireAlarmFL, &Ct2CanMsgTpms::setTireAlarmFL, TPMS_ALARM_TIRE_FL); }
void check_bit_tire_alarm_FR() { check_bit(&Ct2CanMsgTpms::tireAlarmFR, &Ct2CanMsgTpms::setTireAlarmFR, TPMS_ALARM_TIRE_FR); }
void check_bit_tire_alarm_RL() { check_bit(&Ct2CanMsgTpms::tireAlarmRL, &Ct2CanMsgTpms::setTireAlarmRL, TPMS_ALARM_TIRE_RL); }
void check_bit_tire_alarm_RR() { check_bit(&Ct2CanMsgTpms::tireAlarmRR, &Ct2CanMsgTpms::setTireAlarmRR, TPMS_ALARM_TIRE_RR); }
void check_bit_alarm_continuous() { check_bit(&Ct2CanMsgTpms::alarmContinuous, &Ct2CanMsgTpms::setAlarmContinuous, TPMS_ALARM_CONTINUOIS); }
void check_bit_alarm_strobe() { check_bit(&Ct2CanMsgTpms::alarmStrobe, &Ct2CanMsgTpms::setAlarmStrobe, TPMS_ALARM_STROBE); }

void setUp() {}

void tearDown() {}

int main(int argc, char **argv)
{
    UNITY_BEGIN();

    RUN_TEST(check_struct_size);
    RUN_TEST(check_pressure_FL);
    RUN_TEST(check_pressure_FR);
    RUN_TEST(check_pressure_RL);
    RUN_TEST(check_pressure_RR);
    RUN_TEST(check_bit_tire_alarm_FL);
    RUN_TEST(check_bit_tire_alarm_FR);
    RUN_TEST(check_bit_tire_alarm_RL);
    RUN_TEST(check_bit_tire_alarm_RR);
    RUN_TEST(check_bit_alarm_continuous);
    RUN_TEST(check_bit_alarm_strobe);

    UNITY_END();
}
