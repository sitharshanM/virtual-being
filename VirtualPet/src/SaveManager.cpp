#include "SaveManager.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace VirtualPet {

SaveManager::SaveManager(std::string saveFilePath)
    : m_filePath(std::move(saveFilePath)) {}

PetStateData SaveManager::GetDefaultState() const noexcept {
    PetStateData def;
    def.version = 1;
    def.name = "Astra";
    def.needs = {0.8f, 0.2f, 0.8f, 0.5f};
    def.personality = {0.7f, 0.7f, 0.3f};
    def.history = {0, 0, 0, "bottom-right"};
    return def;
}

AppConfigData SaveManager::GetDefaultConfig() noexcept {
    return AppConfigData{};
}

bool SaveManager::Validate(const PetStateData& data) const noexcept {
    if (data.version <= 0 || data.name.empty()) {
        return false;
    }
    const auto& n = data.needs;
    if (n.energy < 0.0f || n.energy > 1.0f ||
        n.hunger < 0.0f || n.hunger > 1.0f ||
        n.mood < 0.0f || n.mood > 1.0f ||
        n.trust < 0.0f || n.trust > 1.0f) {
        return false;
    }
    const auto& p = data.personality;
    if (p.curiosity < 0.0f || p.curiosity > 1.0f ||
        p.playfulness < 0.0f || p.playfulness > 1.0f ||
        p.laziness < 0.0f || p.laziness > 1.0f) {
        return false;
    }
    return true;
}

std::string SaveManager::SerializeToJson(const PetStateData& data) const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{\n";
    ss << "  \"version\": " << data.version << ",\n";
    ss << "  \"name\": \"" << data.name << "\",\n";
    ss << "  \"energy\": " << data.needs.energy << ",\n";
    ss << "  \"mood\": " << data.needs.mood << ",\n";
    ss << "  \"hunger\": " << data.needs.hunger << ",\n";
    ss << "  \"trust\": " << data.needs.trust << ",\n";
    ss << "  \"personality\": {\n";
    ss << "    \"curiosity\": " << data.personality.curiosity << ",\n";
    ss << "    \"playfulness\": " << data.personality.playfulness << ",\n";
    ss << "    \"laziness\": " << data.personality.laziness << "\n";
    ss << "  },\n";
    ss << "  \"memory\": {\n";
    ss << "    \"times_played\": " << data.history.timesPlayed << ",\n";
    ss << "    \"favorite_screen_area\": \"" << data.history.favoriteScreenArea << "\"\n";
    ss << "  }\n";
    ss << "}\n";
    return ss.str();
}

bool SaveManager::DeserializeFromJson(const std::string& json, PetStateData& outData) const {
    outData = GetDefaultState();

    auto extractString = [&](const std::string& key) -> std::string {
        std::regex re("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
        std::smatch m;
        if (std::regex_search(json, m, re) && m.size() > 1) {
            return m[1].str();
        }
        return "";
    };

    auto extractFloat = [&](const std::string& key, float defVal) -> float {
        std::regex re("\"" + key + "\"\\s*:\\s*([0-9.-]+)");
        std::smatch m;
        if (std::regex_search(json, m, re) && m.size() > 1) {
            try {
                return std::stof(m[1].str());
            } catch (...) {}
        }
        return defVal;
    };

    auto extractInt = [&](const std::string& key, int32_t defVal) -> int32_t {
        std::regex re("\"" + key + "\"\\s*:\\s*([0-9-]+)");
        std::smatch m;
        if (std::regex_search(json, m, re) && m.size() > 1) {
            try {
                return std::stoi(m[1].str());
            } catch (...) {}
        }
        return defVal;
    };

    outData.version = extractInt("version", 1);
    std::string parsedName = extractString("name");
    if (!parsedName.empty()) {
        outData.name = parsedName;
    }

    outData.needs.energy = extractFloat("energy", 0.8f);
    outData.needs.mood = extractFloat("mood", 0.8f);
    outData.needs.hunger = extractFloat("hunger", 0.2f);
    outData.needs.trust = extractFloat("trust", 0.5f);

    outData.personality.curiosity = extractFloat("curiosity", 0.7f);
    outData.personality.playfulness = extractFloat("playfulness", 0.7f);
    outData.personality.laziness = extractFloat("laziness", 0.3f);

    outData.history.timesPlayed = extractInt("times_played", 0);
    std::string parsedArea = extractString("favorite_screen_area");
    if (!parsedArea.empty()) {
        outData.history.favoriteScreenArea = parsedArea;
    }

    return Validate(outData);
}

bool SaveManager::DeserializeConfigFromJson(const std::string& json, AppConfigData& outConfig) const {
    outConfig = GetDefaultConfig();

    auto extractString = [&](const std::string& key) -> std::string {
        std::regex re("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
        std::smatch m;
        if (std::regex_search(json, m, re) && m.size() > 1) {
            return m[1].str();
        }
        return "";
    };

    auto extractFloat = [&](const std::string& key, float defVal) -> float {
        std::regex re("\"" + key + "\"\\s*:\\s*([0-9.-]+)");
        std::smatch m;
        if (std::regex_search(json, m, re) && m.size() > 1) {
            try {
                return std::stof(m[1].str());
            } catch (...) {}
        }
        return defVal;
    };

    auto extractInt = [&](const std::string& key, int32_t defVal) -> int32_t {
        std::regex re("\"" + key + "\"\\s*:\\s*([0-9-]+)");
        std::smatch m;
        if (std::regex_search(json, m, re) && m.size() > 1) {
            try {
                return std::stoi(m[1].str());
            } catch (...) {}
        }
        return defVal;
    };

    auto extractBool = [&](const std::string& key, bool defVal) -> bool {
        std::regex re("\"" + key + "\"\\s*:\\s*(true|false)");
        std::smatch m;
        if (std::regex_search(json, m, re) && m.size() > 1) {
            return m[1].str() == "true";
        }
        return defVal;
    };

    std::string title = extractString("title");
    if (!title.empty()) outConfig.title = title;

    outConfig.targetFps = extractInt("target_fps", 60);
    outConfig.alwaysOnTop = extractBool("always_on_top", true);
    outConfig.transparentBackground = extractBool("transparent_background", true);
    outConfig.startWithWindows = extractBool("start_with_windows", false);

    outConfig.windowWidth = extractInt("width", 128);
    outConfig.windowHeight = extractInt("height", 128);
    outConfig.windowScale = extractFloat("scale", 2.0f);

    outConfig.gravity = extractFloat("gravity", 980.0f);
    outConfig.groundMargin = extractInt("ground_margin", 10);
    outConfig.edgeBounce = extractFloat("edge_bounce", 0.2f);
    outConfig.dragSmoothing = extractFloat("drag_smoothing", 0.15f);

    outConfig.idleTimeoutSeconds = extractFloat("idle_timeout_seconds", 30.0f);
    outConfig.energyDepletionRate = extractFloat("energy_depletion_rate", 0.005f);
    outConfig.hungerIncreaseRate = extractFloat("hunger_increase_rate", 0.008f);
    outConfig.mouseProximityDistancePx = extractFloat("mouse_proximity_distance_px", 150.0f);

    return true;
}

bool SaveManager::Load(PetStateData& outData) {
    namespace fs = std::filesystem;
    std::error_code ec;

    if (!fs::exists(m_filePath, ec)) {
        outData = GetDefaultState();
        Save(outData);
        return false;
    }

    std::ifstream file(m_filePath);
    if (!file.is_open()) {
        outData = GetDefaultState();
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    if (!DeserializeFromJson(buffer.str(), outData)) {
        outData = GetDefaultState();
        return false;
    }

    return true;
}

bool SaveManager::Save(const PetStateData& data) {
    namespace fs = std::filesystem;
    std::error_code ec;

    fs::path targetPath(m_filePath);
    if (targetPath.has_parent_path()) {
        fs::create_directories(targetPath.parent_path(), ec);
    }

    fs::path tempPath = targetPath;
    tempPath += ".tmp";

    std::ofstream out(tempPath, std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << SerializeToJson(data);
    out.flush();
    out.close();

    fs::rename(tempPath, targetPath, ec);
    if (ec) {
        fs::copy_file(tempPath, targetPath, fs::copy_options::overwrite_existing, ec);
        fs::remove(tempPath, ec);
    }

    return !ec;
}

bool SaveManager::LoadConfig(const std::string& configPath, AppConfigData& outConfig) {
    namespace fs = std::filesystem;
    std::error_code ec;

    if (!fs::exists(configPath, ec)) {
        outConfig = GetDefaultConfig();
        return false;
    }

    std::ifstream file(configPath);
    if (!file.is_open()) {
        outConfig = GetDefaultConfig();
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return DeserializeConfigFromJson(buffer.str(), outConfig);
}

bool SaveManager::IsStartWithWindowsEnabled() {
#ifdef _WIN32
    HKEY hKey;
    if (::RegOpenKeyExW(HKEY_CURRENT_USER,
                        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type = 0;
        DWORD size = 0;
        LONG res = ::RegQueryValueExW(hKey, L"VirtualPet", nullptr, &type, nullptr, &size);
        ::RegCloseKey(hKey);
        return (res == ERROR_SUCCESS);
    }
#endif
    return false;
}

bool SaveManager::SetStartWithWindows(bool enable) {
#ifdef _WIN32
    HKEY hKey;
    if (::RegOpenKeyExW(HKEY_CURRENT_USER,
                        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                        0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t exePath[MAX_PATH] = {0};
            if (::GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0) {
                std::wstring quoted = L"\"" + std::wstring(exePath) + L"\"";
                LONG res = ::RegSetValueExW(hKey, L"VirtualPet", 0, REG_SZ,
                                            reinterpret_cast<const BYTE*>(quoted.c_str()),
                                            static_cast<DWORD>((quoted.length() + 1) * sizeof(wchar_t)));
                ::RegCloseKey(hKey);
                return (res == ERROR_SUCCESS);
            }
        } else {
            ::RegDeleteValueW(hKey, L"VirtualPet");
            ::RegCloseKey(hKey);
            return true;
        }
        ::RegCloseKey(hKey);
    }
#else
    (void)enable;
#endif
    return false;
}

} // namespace VirtualPet
