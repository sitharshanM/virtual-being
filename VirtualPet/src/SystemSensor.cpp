#include "SystemSensor.h"

#include <ctime>
#include <fstream>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace VirtualPet {

SystemSensor::SystemSensor(float idleThresholdSeconds)
    : m_idleThresholdSeconds(idleThresholdSeconds) {
    PollSystemState();
}

void SystemSensor::Update(float deltaTime) {
    m_pollTimer += deltaTime;
    // Refresh system status every 0.5 seconds to minimize OS overhead
    if (m_pollTimer >= 0.5f) {
        m_pollTimer = 0.0f;
        PollSystemState();
    } else {
        // Accumulate idle time locally between polls
        m_state.userIdleTimeSeconds += deltaTime;
        m_state.isUserIdle = (m_state.userIdleTimeSeconds >= m_idleThresholdSeconds);
    }
}

void SystemSensor::PollSystemState() {
#ifdef _WIN32
    // 1. User Inactivity / Idle Time
    LASTINPUTINFO lii;
    lii.cbSize = sizeof(LASTINPUTINFO);
    if (::GetLastInputInfo(&lii)) {
        DWORD currentTicks = ::GetTickCount();
        DWORD elapsedMs = currentTicks - lii.dwTime;
        m_state.userIdleTimeSeconds = static_cast<float>(elapsedMs) / 1000.0f;
    }
    m_state.isUserIdle = (m_state.userIdleTimeSeconds >= m_idleThresholdSeconds);

    // 2. Battery & Power Status
    SYSTEM_POWER_STATUS sps;
    if (::GetSystemPowerStatus(&sps)) {
        if (sps.ACLineStatus == 1) {
            m_state.powerState = PowerState::ACPower;
        } else if (sps.ACLineStatus == 0) {
            if (sps.BatteryLifePercent <= 20) {
                m_state.powerState = PowerState::BatteryLow;
            } else {
                m_state.powerState = PowerState::BatteryNormal;
            }
        } else {
            m_state.powerState = PowerState::Unknown;
        }

        if (sps.BatteryLifePercent != 255) {
            m_state.batteryPercent = static_cast<float>(sps.BatteryLifePercent);
        } else {
            m_state.batteryPercent = -1.0f;
        }
        m_state.isBatterySaverOn = (sps.SystemStatusFlag == 1);
    }

    // 3. System Time / Day-Night Cycle
    SYSTEMTIME st;
    ::GetLocalTime(&st);
    m_state.currentHour = static_cast<int32_t>(st.wHour);
    m_state.currentMinute = static_cast<int32_t>(st.wMinute);

    if (st.wHour >= 6 && st.wHour < 12) {
        m_state.timeOfDay = TimeOfDay::Morning;
    } else if (st.wHour >= 12 && st.wHour < 18) {
        m_state.timeOfDay = TimeOfDay::Afternoon;
    } else if (st.wHour >= 18 && st.wHour < 22) {
        m_state.timeOfDay = TimeOfDay::Evening;
    } else {
        m_state.timeOfDay = TimeOfDay::Night;
    }
#else
    // 1. System Time / Day-Night Cycle
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_r(&now, &local);
    m_state.currentHour = local.tm_hour;
    m_state.currentMinute = local.tm_min;

    if (local.tm_hour >= 6 && local.tm_hour < 12) {
        m_state.timeOfDay = TimeOfDay::Morning;
    } else if (local.tm_hour >= 12 && local.tm_hour < 18) {
        m_state.timeOfDay = TimeOfDay::Afternoon;
    } else if (local.tm_hour >= 18 && local.tm_hour < 22) {
        m_state.timeOfDay = TimeOfDay::Evening;
    } else {
        m_state.timeOfDay = TimeOfDay::Night;
    }

    // 2. Battery & Power Status
    m_state.powerState = PowerState::ACPower;
    m_state.batteryPercent = -1.0f;
    m_state.isBatterySaverOn = false;

    // Check Linux sysfs power supply
    std::ifstream statusFile("/sys/class/power_supply/BAT0/status");
    if (!statusFile.is_open()) statusFile.open("/sys/class/power_supply/BAT1/status");
    if (statusFile.is_open()) {
        std::string status;
        statusFile >> status;
        statusFile.close();

        std::ifstream capFile("/sys/class/power_supply/BAT0/capacity");
        if (!capFile.is_open()) capFile.open("/sys/class/power_supply/BAT1/capacity");
        if (capFile.is_open()) {
            int cap = 100;
            if (capFile >> cap) {
                m_state.batteryPercent = static_cast<float>(cap);
                if (status == "Discharging") {
                    m_state.powerState = (cap <= 20) ? PowerState::BatteryLow : PowerState::BatteryNormal;
                } else {
                    m_state.powerState = PowerState::ACPower;
                }
            }
            capFile.close();
        }
    }

    // 3. User idle state
    m_state.isUserIdle = (m_state.userIdleTimeSeconds >= m_idleThresholdSeconds);
#endif
}

} // namespace VirtualPet
