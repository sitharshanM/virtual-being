#pragma once

#include "Memory.h"

#include <string>

namespace VirtualPet {

/// Complete parsed application configuration settings matching pet_config.json.
struct AppConfigData {
    int32_t version{1};
    std::string title{"Virtual Pet"};
    int32_t targetFps{60};
    bool alwaysOnTop{true};
    bool transparentBackground{true};
    bool startWithWindows{false};

    int32_t windowWidth{128};
    int32_t windowHeight{128};
    float windowScale{2.0f};
    std::string initialPosition{"tray_bottom_right"};

    float gravity{980.0f};
    int32_t groundMargin{10};
    float edgeBounce{0.2f};
    float dragSmoothing{0.15f};

    float tickRateHz{10.0f};
    float idleTimeoutSeconds{30.0f};
    float energyDepletionRate{0.005f};
    float hungerIncreaseRate{0.008f};

    float mouseProximityDistancePx{150.0f};
    bool detectCursorDrag{true};
    float idleSystemThresholdSeconds{120.0f};

    std::string assetsDir{"assets"};
    std::string dataDir{"data"};
    std::string saveFile{"data/pet_state.json"};
};

/**
 * @brief Handles JSON persistence, configuration parsing, and Windows system registry auto-start.
 */
class SaveManager {
public:
    explicit SaveManager(std::string saveFilePath = "data/pet_state.json");
    ~SaveManager() = default;

    /// Loads pet state data from disk. Falls back to defaults if missing or corrupted.
    bool Load(PetStateData& outData);

    /// Saves pet state data to disk in JSON format.
    bool Save(const PetStateData& data);

    /// Loads runtime application configuration from pet_config.json.
    bool LoadConfig(const std::string& configPath, AppConfigData& outConfig);

    /// Validates fields and bounds within PetStateData.
    [[nodiscard]] bool Validate(const PetStateData& data) const noexcept;

    /// Returns clean baseline default pet state.
    [[nodiscard]] PetStateData GetDefaultState() const noexcept;

    /// Returns clean default application configuration.
    [[nodiscard]] static AppConfigData GetDefaultConfig() noexcept;

    // --- Windows Auto-Start Registry Utilities ---

    /// Checks if VirtualPet is registered in HKCU Run key.
    [[nodiscard]] static bool IsStartWithWindowsEnabled();

    /// Adds or removes VirtualPet from HKCU Run key.
    static bool SetStartWithWindows(bool enable);

    [[nodiscard]] const std::string& GetFilePath() const noexcept { return m_filePath; }
    void SetFilePath(std::string path) { m_filePath = std::move(path); }

private:
    std::string m_filePath;

    [[nodiscard]] std::string SerializeToJson(const PetStateData& data) const;
    [[nodiscard]] bool DeserializeFromJson(const std::string& json, PetStateData& outData) const;
    [[nodiscard]] bool DeserializeConfigFromJson(const std::string& json, AppConfigData& outConfig) const;
};

} // namespace VirtualPet
