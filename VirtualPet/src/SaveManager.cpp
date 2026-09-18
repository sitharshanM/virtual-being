#include "SaveManager.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <cmath>
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
    def.history = InteractionHistory{};
    return def;
}

AppConfigData SaveManager::GetDefaultConfig() noexcept {
    return AppConfigData{};
}

bool SaveManager::Validate(const PetStateData& data) const noexcept {
    if (data.version != 1 || data.name.empty()) {
        return false;
    }
    for (float value : {data.needs.energy, data.needs.hunger, data.needs.mood, data.needs.trust,
                        data.personality.curiosity, data.personality.playfulness, data.personality.laziness})
        if (!std::isfinite(value)) return false;
    if (data.history.timesPlayed < 0 || data.history.timesFed < 0 || data.history.timesPetted < 0) return false;
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

using Json = nlohmann::json;

std::string SaveManager::SerializeToJson(const PetStateData& d) const {
    return Json{{"version", d.version}, {"name", d.name},
        {"energy", d.needs.energy}, {"mood", d.needs.mood},
        {"hunger", d.needs.hunger}, {"trust", d.needs.trust},
        {"personality", {{"curiosity", d.personality.curiosity}, {"playfulness", d.personality.playfulness}, {"laziness", d.personality.laziness}}},
        {"identity", {{"personal_goal", d.identity.personalGoal}, {"goal_progress", d.identity.goalProgress},
            {"favorite_drink", d.identity.favoriteDrink}, {"favorite_music", d.identity.favoriteMusic},
            {"favorite_activity", d.identity.favoriteActivity}, {"dislike", d.identity.dislike}, {"imperfection", d.identity.imperfection}}},
        {"memory", {{"times_played", d.history.timesPlayed}, {"times_fed", d.history.timesFed},
            {"times_petted", d.history.timesPetted}, {"favorite_screen_area", d.history.favoriteScreenArea}}}}.dump(2);
}

bool SaveManager::DeserializeFromJson(const std::string& text, PetStateData& outData) const {
    try {
        const auto j = Json::parse(text);
        PetStateData d = GetDefaultState();
        d.version = j.at("version").get<int32_t>();
        d.name = j.at("name").get<std::string>();
        d.needs.energy = j.at("energy").get<float>();
        d.needs.mood = j.at("mood").get<float>();
        d.needs.hunger = j.at("hunger").get<float>();
        d.needs.trust = j.at("trust").get<float>();
        const auto& p = j.at("personality");
        d.personality = {p.at("curiosity").get<float>(), p.at("playfulness").get<float>(), p.at("laziness").get<float>()};
        const auto& h = j.at("memory");
        d.history.timesPlayed = h.value("times_played", 0);
        d.history.timesFed = h.value("times_fed", 0);
        d.history.timesPetted = h.value("times_petted", 0);
        d.history.favoriteScreenArea = h.value("favorite_screen_area", std::string("bottom-right"));
        if (j.contains("identity") && j["identity"].is_object()) {
            const auto& id = j["identity"];
            d.identity.personalGoal = id.value("personal_goal", d.identity.personalGoal);
            d.identity.goalProgress = id.value("goal_progress", d.identity.goalProgress);
            d.identity.favoriteDrink = id.value("favorite_drink", d.identity.favoriteDrink);
            d.identity.favoriteMusic = id.value("favorite_music", d.identity.favoriteMusic);
            d.identity.favoriteActivity = id.value("favorite_activity", d.identity.favoriteActivity);
            d.identity.dislike = id.value("dislike", d.identity.dislike);
            d.identity.imperfection = id.value("imperfection", d.identity.imperfection);
        }
        if (!Validate(d)) return false;
        outData = std::move(d);
        return true;
    } catch (const Json::exception&) { return false; }
}

bool SaveManager::DeserializeConfigFromJson(const std::string& text, AppConfigData& outConfig) const {
    try {
        const auto j = Json::parse(text);
        if (!j.is_object()) return false;
        AppConfigData c;
        c.version = j.value("version", 1);
        const auto app = j.value("app", Json::object());
        c.title = app.value("title", c.title);
        c.targetFps = app.value("target_fps", c.targetFps);
        c.alwaysOnTop = app.value("always_on_top", c.alwaysOnTop);
        c.transparentBackground = app.value("transparent_background", c.transparentBackground);
        c.startWithWindows = app.value("start_with_windows", c.startWithWindows);
        const auto window = j.value("window", Json::object());
        c.windowWidth = window.value("width", c.windowWidth);
        c.windowHeight = window.value("height", c.windowHeight);
        c.windowScale = window.value("scale", c.windowScale);
        c.initialPosition = window.value("initial_position", c.initialPosition);
        const auto physics = j.value("physics", Json::object());
        c.gravity = physics.value("gravity", c.gravity);
        c.groundMargin = physics.value("ground_margin", c.groundMargin);
        c.edgeBounce = physics.value("edge_bounce", c.edgeBounce);
        c.dragSmoothing = physics.value("drag_smoothing", c.dragSmoothing);
        const auto behavior = j.value("behavior", Json::object());
        c.tickRateHz = behavior.value("tick_rate_hz", c.tickRateHz);
        c.idleTimeoutSeconds = behavior.value("idle_timeout_seconds", c.idleTimeoutSeconds);
        c.energyDepletionRate = behavior.value("energy_depletion_rate", c.energyDepletionRate);
        c.hungerIncreaseRate = behavior.value("hunger_increase_rate", c.hungerIncreaseRate);
        const auto sensors = j.value("sensors", Json::object());
        c.mouseProximityDistancePx = sensors.value("mouse_proximity_distance_px", c.mouseProximityDistancePx);
        c.detectCursorDrag = sensors.value("detect_cursor_drag", c.detectCursorDrag);
        c.idleSystemThresholdSeconds = sensors.value("idle_system_threshold_seconds", c.idleSystemThresholdSeconds);
        const auto paths = j.value("paths", Json::object());
        c.assetsDir = paths.value("assets_dir", c.assetsDir);
        c.dataDir = paths.value("data_dir", c.dataDir);
        c.saveFile = paths.value("save_file", (std::filesystem::path(c.dataDir) / "pet_state.json").string());
        const auto llm = j.value("llm", Json::object());
        c.localLlmEnabled = llm.value("enabled", c.localLlmEnabled);
        c.localLlmEndpoint = llm.value("endpoint", c.localLlmEndpoint);
        c.localLlmModel = llm.value("model", c.localLlmModel);
        c.localLlmIntervalSeconds = llm.value("interval_seconds", c.localLlmIntervalSeconds);
        c.localLlmUseWindowTitle = llm.value("use_window_title", c.localLlmUseWindowTitle);
        c.localLlmUseClipboard = llm.value("use_clipboard", c.localLlmUseClipboard);
        c.localLlmBlockSensitiveWindows = llm.value("block_sensitive_windows", c.localLlmBlockSensitiveWindows);
        const auto range = [](float v, float lo, float hi) { return std::isfinite(v) && v >= lo && v <= hi; };
        if (c.version != 1 || c.title.empty() || c.targetFps < 1 || c.targetFps > 240 ||
            c.windowWidth < 1 || c.windowWidth > 2048 || c.windowHeight < 1 || c.windowHeight > 2048 ||
            !range(c.windowScale, 0.1f, 8.0f) || c.windowWidth*c.windowScale > 4096 || c.windowHeight*c.windowScale > 4096 ||
            !range(c.gravity, 0, 10000) || c.groundMargin < 0 || c.groundMargin > 1000 ||
            !range(c.edgeBounce, 0, 1) || !range(c.dragSmoothing, 0.01f, 10) ||
            !range(c.tickRateHz, 1, 240) || !range(c.idleTimeoutSeconds, 0.1f, 3600) ||
            !range(c.energyDepletionRate, 0, 1) || !range(c.hungerIncreaseRate, 0, 1) ||
            !range(c.mouseProximityDistancePx, 0, 10000) || !range(c.idleSystemThresholdSeconds, 1, 86400) ||
            c.assetsDir.empty() || c.dataDir.empty() || c.saveFile.empty() ||
            (c.initialPosition != "tray_bottom_right" && c.initialPosition != "bottom-right" && c.initialPosition != "bottom-left" && c.initialPosition != "center")) return false;
        outConfig = std::move(c);
        return true;
    } catch (const Json::exception&) { return false; }
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
    if (!Validate(data) || m_filePath.empty()) return false;
    std::error_code ec;
    const fs::path targetPath(m_filePath);
    if (targetPath.has_parent_path()) {
        fs::create_directories(targetPath.parent_path(), ec);
        if (ec) return false;
    }
    fs::path tempPath = targetPath;
    tempPath += ".tmp";
    std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    try { out << SerializeToJson(data); }
    catch (const Json::exception&) { out.close(); fs::remove(tempPath, ec); return false; }
    out.flush();
    bool written = out.good();
    out.close();
    if (!written || out.fail()) { fs::remove(tempPath, ec); return false; }
#ifdef _WIN32
    // Replace the destination without a non-atomic copy fallback.
    if (!::MoveFileExW(tempPath.c_str(), targetPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        fs::remove(tempPath, ec);
        return false;
    }
#else
    fs::rename(tempPath, targetPath, ec);
    if (ec) { std::error_code cleanup; fs::remove(tempPath, cleanup); return false; }
#endif
    return true;
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

    outConfig = GetDefaultConfig();
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
            LONG result = ::RegDeleteValueW(hKey, L"VirtualPet");
            ::RegCloseKey(hKey);
            return result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
        }
        ::RegCloseKey(hKey);
    }
#else
    (void)enable;
#endif
    return false;
}

} // namespace VirtualPet
