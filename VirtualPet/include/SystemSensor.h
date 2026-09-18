#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace VirtualPet {

/// High-level time periods in a 24-hour day.
enum class TimeOfDay {
    Morning,    // 06:00 - 11:59
    Afternoon,  // 12:00 - 17:59
    Evening,    // 18:00 - 21:59
    Night       // 22:00 - 05:59
};

/// System electrical power sources and battery levels.
enum class PowerState {
    ACPower,
    BatteryNormal,
    BatteryLow,
    Unknown
};

/// Snapshot of system environmental observations.
struct SystemState {
    float userIdleTimeSeconds{0.0f};    ///< Seconds since user last moved mouse or pressed key.
    bool isUserIdle{false};             ///< True if userIdleTime exceeds threshold.
    PowerState powerState{PowerState::ACPower};
    float batteryPercent{-1.0f};        ///< Battery remaining percentage [0.0 - 100.0], or -1.0 if desktop/AC.
    bool isBatterySaverOn{false};
    TimeOfDay timeOfDay{TimeOfDay::Afternoon};
    int32_t currentHour{12};
    int32_t currentMinute{0};

    std::string activeWindowTitle;
    std::string applicationKind;
    float activityIntensity{0.0f};
    float stressEstimate{0.0f};
    bool isWeekend{false};
    std::string clipboardText;
};

/**
 * @brief Senses system-level context: user inactivity, day/night cycles, and battery charge.
 */
class SystemSensor {
public:
    explicit SystemSensor(float idleThresholdSeconds = 120.0f);
    ~SystemSensor() = default;

    /// Refreshes system observations for the current frame.
    void Update(float deltaTime);

    [[nodiscard]] const SystemState& GetState() const noexcept { return m_state; }
    [[nodiscard]] float GetUserIdleTime() const noexcept { return m_state.userIdleTimeSeconds; }
    [[nodiscard]] bool IsUserIdle() const noexcept { return m_state.isUserIdle; }
    [[nodiscard]] PowerState GetPowerState() const noexcept { return m_state.powerState; }
    [[nodiscard]] float GetBatteryPercent() const noexcept { return m_state.batteryPercent; }
    [[nodiscard]] TimeOfDay GetTimeOfDay() const noexcept { return m_state.timeOfDay; }
    [[nodiscard]] bool IsNight() const noexcept { return m_state.timeOfDay == TimeOfDay::Night; }

    [[nodiscard]] float GetIdleThreshold() const noexcept { return m_idleThresholdSeconds; }
    void SetIdleThreshold(float seconds) noexcept { m_idleThresholdSeconds = seconds; }

    void SetContextCapture(bool useWindowTitle, bool useClipboard) noexcept {
        m_useWindowTitle = useWindowTitle;
        m_useClipboard = useClipboard;
    }
    void SetPrivacyFilter(bool blockSensitive, const std::vector<std::string>& excludedTerms) {
        m_blockSensitive = blockSensitive;
        m_excludedTerms = excludedTerms;
    }

private:
    float m_idleThresholdSeconds{120.0f};
    SystemState m_state;
    float m_pollTimer{0.0f};

    bool m_useWindowTitle{false};
    bool m_useClipboard{false};
    bool m_blockSensitive{true};
    std::vector<std::string> m_excludedTerms;

    void PollSystemState();
};

} // namespace VirtualPet
