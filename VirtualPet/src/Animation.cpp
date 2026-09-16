#include "Animation.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <cctype>

#ifdef _WIN32
#include <wingdi.h>
#endif

namespace VirtualPet {

const char* AnimationStateToString(AnimationState state) noexcept {
    switch (state) {
        case AnimationState::Idle:     return "idle";
        case AnimationState::Walk:     return "walk";
        case AnimationState::Run:      return "run";
        case AnimationState::Sleep:    return "sleep";
        case AnimationState::Reaction: return "reactions";
        case AnimationState::Custom:   return "custom";
        default:                       return "idle";
    }
}

AnimationState StringToAnimationState(const std::string& name) noexcept {
    if (name == "idle")      return AnimationState::Idle;
    if (name == "walk")      return AnimationState::Walk;
    if (name == "run")       return AnimationState::Run;
    if (name == "sleep")     return AnimationState::Sleep;
    if (name == "reactions" || name == "reaction") return AnimationState::Reaction;
    return AnimationState::Custom;
}

// ============================================================================
// AnimationClip Implementation
// ============================================================================

AnimationClip::AnimationClip(std::string name, bool isLooping, float defaultFrameDuration)
    : m_name(std::move(name)), m_isLooping(isLooping) {
    if (std::isfinite(defaultFrameDuration) && defaultFrameDuration > 0)
        m_defaultFrameDuration = defaultFrameDuration;
}

void AnimationClip::AddFrame(const AnimationFrame& frame) {
    if (!std::isfinite(frame.durationSeconds) || frame.durationSeconds <= 0) return;
    m_frames.push_back(frame);
    m_totalDuration += frame.durationSeconds;
}

void AnimationClip::AddFrame(const std::string& filePath, float durationSeconds) {
    AnimationFrame frame;
    frame.filePath = filePath;
    frame.durationSeconds = durationSeconds == -1.0f ? m_defaultFrameDuration : durationSeconds;
    AddFrame(frame);
}

void AnimationClip::Clear() {
    m_frames.clear();
    m_totalDuration = 0.0f;
}

const AnimationFrame* AnimationClip::GetFrame(size_t index) const noexcept {
    if (index < m_frames.size()) {
        return &m_frames[index];
    }
    return nullptr;
}

// ============================================================================
// Animation Controller Implementation
// ============================================================================

Animation::Animation() {
#ifdef _WIN32
    Gdiplus::GdiplusStartupInput input;
    Gdiplus::GdiplusStartup(&m_gdiToken, &input, nullptr);
#endif
    // Initialize default clips for core states
    RegisterClip(AnimationState::Idle, AnimationClip("idle", true));
    RegisterClip(AnimationState::Walk, AnimationClip("walk", true));
    RegisterClip(AnimationState::Run, AnimationClip("run", true));
    RegisterClip(AnimationState::Sleep, AnimationClip("sleep", true));
    RegisterClip(AnimationState::Reaction, AnimationClip("reactions", false));

    m_currentClipName = "idle";
    m_currentState = AnimationState::Idle;
}

Animation::~Animation() {
    m_clips.clear();
#ifdef _WIN32
    if (m_gdiToken) Gdiplus::GdiplusShutdown(m_gdiToken);
#endif
}

void Animation::RegisterClip(AnimationState state, AnimationClip clip) {
    const std::string name = AnimationStateToString(state);
    RegisterClip(name, std::move(clip));
}

void Animation::RegisterClip(const std::string& name, AnimationClip clip) {
    m_clips[name] = std::move(clip);
    if (m_currentClipName == name) PlayClip(name, true);
}

void Animation::Play(AnimationState state, bool restartIfSame) {
    m_currentState = state;
    PlayClip(AnimationStateToString(state), restartIfSame);
}

void Animation::PlayClip(const std::string& clipName, bool restartIfSame) {
    if (!restartIfSame && m_currentClipName == clipName && !m_isFinished) {
        return;
    }

    m_currentClipName = clipName;
    m_currentFrameIndex = 0;
    m_frameTimer = 0.0f;
    m_isFinished = false;
    if (clipName == "idle" || clipName == "walk" || clipName == "run" ||
        clipName == "sleep" || clipName == "reaction") {
        m_currentState = StringToAnimationState(clipName);
    }
}

const AnimationFrame* Animation::GetCurrentFrame() const noexcept {
    auto it = m_clips.find(m_currentClipName);
    if (it != m_clips.end()) {
        return it->second.GetFrame(m_currentFrameIndex);
    }
    return nullptr;
}

void Animation::Update(float deltaTime) {
    if (!std::isfinite(deltaTime) || deltaTime <= 0) return;
    auto it = m_clips.find(m_currentClipName);
    if (it == m_clips.end()) {
        return;
    }

    AnimationClip& clip = it->second;
    const size_t frameCount = clip.GetFrameCount();
    if (frameCount <= 1 && clip.IsLooping()) {
        return;
    }

    if (m_isFinished) {
        return;
    }

    if (frameCount == 0) return;
    m_frameTimer += deltaTime * m_playbackSpeed;
    if (clip.IsLooping() && clip.GetTotalDuration() > 0)
        m_frameTimer = std::fmod(m_frameTimer, clip.GetTotalDuration());
    while (m_frameTimer >= clip.GetFrame(m_currentFrameIndex)->durationSeconds) {
        m_frameTimer -= clip.GetFrame(m_currentFrameIndex)->durationSeconds;
        if (++m_currentFrameIndex >= frameCount) {
            if (clip.IsLooping()) m_currentFrameIndex = 0;
            else {
                m_currentFrameIndex = frameCount - 1;
                m_isFinished = true;
                if (m_onComplete) m_onComplete(m_currentState);
                return;
            }
        }
    }
}

size_t Animation::LoadFromDirectory(const std::string& assetsDirectory) {
    namespace fs = std::filesystem;
    size_t loadedCount = 0;

    std::error_code ec;
    if (!fs::exists(assetsDirectory, ec) || !fs::is_directory(assetsDirectory, ec)) {
        return 0;
    }

    const std::vector<std::pair<AnimationState, std::string>> categories = {
        {AnimationState::Idle, "idle"},
        {AnimationState::Walk, "walk"},
        {AnimationState::Run, "run"},
        {AnimationState::Sleep, "sleep"},
        {AnimationState::Reaction, "reactions"}
    };

    for (const auto& [state, folderName] : categories) {
        fs::path folderPath = fs::path(assetsDirectory) / folderName;
        if (!fs::exists(folderPath, ec) || !fs::is_directory(folderPath, ec)) {
            continue;
        }

        bool isLooping = (state != AnimationState::Reaction);
        // Resting illustrations need longer holds than locomotion frames.
        const float frameDuration = state == AnimationState::Idle ? 0.25f
                                  : state == AnimationState::Sleep ? 0.35f
                                  : state == AnimationState::Reaction ? 0.18f
                                  : state == AnimationState::Run ? 0.09f : 0.12f;
        AnimationClip clip(folderName, isLooping, frameDuration);

        std::vector<fs::path> imageFiles;
        for (const auto& entry : fs::directory_iterator(folderPath, ec)) {
            if (entry.is_regular_file(ec)) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (ext == ".png" || ext == ".bmp" || ext == ".jpg" || ext == ".jpeg") {
                    imageFiles.push_back(entry.path());
                }
            }
        }

        std::sort(imageFiles.begin(), imageFiles.end());

        for (const auto& filePath : imageFiles) {
            AnimationFrame frame;
            frame.filePath = filePath.string();
            frame.durationSeconds = frameDuration;
#ifdef _WIN32
            if (!m_gdiToken) continue;
            frame.bitmap = std::make_shared<Gdiplus::Bitmap>(filePath.c_str());
            if (frame.bitmap->GetLastStatus() != Gdiplus::Ok) continue;
            frame.width = static_cast<int32_t>(frame.bitmap->GetWidth());
            frame.height = static_cast<int32_t>(frame.bitmap->GetHeight());
#endif
            clip.AddFrame(frame);
        }

        if (!clip.IsEmpty()) {
            RegisterClip(state, std::move(clip));
            loadedCount++;
        }
    }

    // -----------------------------------------------------------------------
    // Per-clip timing + looping table.
    // Each entry: { frameDurationSeconds, shouldLoop }
    // Clips not listed fall back to { 0.14f, false }.
    // -----------------------------------------------------------------------
    struct ClipConfig { float frameDuration; bool looping; };
    static const std::unordered_map<std::string, ClipConfig> kClipTimings = {
        // ── Sleep cycle ────────────────────────────────────────────────────
        // Sheet 1: slow, easing-in drift into slumber
        { "falling-asleep",       { 0.28f, false } },
        // Sheet 2: single languid yawn — plays once, then sleep loop takes over
        { "sleepy-yawning",       { 0.22f, false } },
        // Reverse of falling-asleep: gentle stretch before resuming idle
        { "waking-up",            { 0.22f, false } },
        { "morning-stretch",      { 0.22f, false } },
        { "evening-wind-down",    { 0.24f, false } },

        // ── Study / focus ──────────────────────────────────────────────────
        // Sheet 3: calm page-turn rhythm — loops for the whole study session
        { "reading",              { 0.20f, true  } },
        { "studying",             { 0.20f, true  } },
        { "digital-drawing",      { 0.18f, true  } },

        // ── Conversation ───────────────────────────────────────────────────
        // Sheet 4 (left half → thinking/pointing): attentive listener pace
        { "listening",            { 0.18f, false } },
        // Sheet 4 (right half → smiling): speech-paced, slightly quicker
        { "talking",              { 0.16f, false } },

        // ── Positive reactions ─────────────────────────────────────────────
        // Sheet 5: upbeat celebratory pop
        { "celebrating",          { 0.13f, false } },
        { "goal-completed",       { 0.13f, false } },
        { "happy-reunion",        { 0.13f, false } },

        // ── Affection / warmth ─────────────────────────────────────────────
        { "shy-affection",        { 0.17f, false } },
        { "comforting",           { 0.17f, false } },
        { "matcha-drinking",      { 0.19f, false } },
        { "apology-repair",       { 0.17f, false } },

        // ── Negative / boundary states ─────────────────────────────────────
        { "asking-for-space",     { 0.19f, false } },
        { "pouting",              { 0.21f, false } },
        { "creative-frustration", { 0.19f, false } },
        { "tired-encouragement",  { 0.22f, false } },

        // ── Playful ────────────────────────────────────────────────────────
        { "playful-teasing",      { 0.14f, false } },
    };

    // Discover optional personality/action clips without requiring enum changes.
    for (const auto& entry : fs::directory_iterator(assetsDirectory, ec)) {
        if (!entry.is_directory(ec)) {
            continue;
        }
        const std::string name = entry.path().filename().string();
        if (name == "idle" || name == "walk" || name == "run" ||
            name == "sleep" || name == "reactions") {
            continue;
        }

        // Resolve timing config — fall back gracefully for unknown clip names.
        float frameDuration = 0.14f;
        bool shouldLoop = false;
        auto timingIt = kClipTimings.find(name);
        if (timingIt != kClipTimings.end()) {
            frameDuration = timingIt->second.frameDuration;
            shouldLoop    = timingIt->second.looping;
        }

        AnimationClip clip(name, shouldLoop, frameDuration);
        std::vector<fs::path> files;
        for (const auto& image : fs::directory_iterator(entry.path(), ec)) {
            std::string ext = image.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            if (image.is_regular_file(ec) &&
                (ext == ".png" || ext == ".bmp" || ext == ".jpg" || ext == ".jpeg")) {
                files.push_back(image.path());
            }
        }
        std::sort(files.begin(), files.end());
        for (const auto& path : files) {
            AnimationFrame frame;
            frame.filePath = path.string();
            frame.durationSeconds = frameDuration;
#ifdef _WIN32
            if (!m_gdiToken) {
                continue;
            }
            frame.bitmap = std::make_shared<Gdiplus::Bitmap>(path.c_str());
            if (frame.bitmap->GetLastStatus() != Gdiplus::Ok) {
                continue;
            }
            frame.width  = static_cast<int32_t>(frame.bitmap->GetWidth());
            frame.height = static_cast<int32_t>(frame.bitmap->GetHeight());
#endif
            clip.AddFrame(frame);
        }
        if (!clip.IsEmpty()) {
            RegisterClip(name, std::move(clip));
            loadedCount++;
        }
    }

    return loadedCount;
}

#ifdef _WIN32
size_t Animation::LoadFromSpriteSheet(const std::string& sheetPath,
                                      int32_t cols, int32_t rows,
                                      const std::vector<SpriteSheetClipDef>& clips) {
    if (cols <= 0 || rows <= 0 || clips.empty()) return 0;

    // Convert narrow path to wide for GDI+
    std::wstring widePath(sheetPath.begin(), sheetPath.end());
    auto sheet = std::make_shared<Gdiplus::Bitmap>(widePath.c_str());
    if (!sheet || sheet->GetLastStatus() != Gdiplus::Ok) {
        std::cerr << "[Animation] Failed to load sprite sheet: " << sheetPath << "\n";
        return 0;
    }

    const int32_t sheetW = static_cast<int32_t>(sheet->GetWidth());
    const int32_t sheetH = static_cast<int32_t>(sheet->GetHeight());
    const int32_t cellW  = sheetW / cols;
    const int32_t cellH  = sheetH / rows;
    if (cellW <= 0 || cellH <= 0) return 0;

    size_t loadedCount = 0;

    for (const auto& def : clips) {
        if (def.row < 0 || def.row >= rows || def.frameCount <= 0) continue;

        AnimationClip clip(def.clipName, def.looping, def.frameDuration);

        for (int32_t col = 0; col < def.frameCount && col < cols; ++col) {
            // Crop this cell out of the sheet into a new Bitmap
            auto cell = std::make_shared<Gdiplus::Bitmap>(cellW, cellH, PixelFormat32bppPARGB);
            if (!cell || cell->GetLastStatus() != Gdiplus::Ok) continue;

            Gdiplus::Graphics g(cell.get());
            g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
            // Blit the source cell into the destination bitmap
            g.DrawImage(sheet.get(),
                Gdiplus::Rect(0, 0, cellW, cellH),        // dest: entire cell bitmap
                col * cellW, def.row * cellH, cellW, cellH, // src: one grid cell
                Gdiplus::UnitPixel);

            AnimationFrame frame;
            frame.durationSeconds = def.frameDuration;
            frame.bitmap          = cell;
            frame.width           = cellW;
            frame.height          = cellH;
            clip.AddFrame(frame);
        }

        if (!clip.IsEmpty()) {
            RegisterClip(def.clipName, std::move(clip));
            loadedCount++;
        }
    }

    return loadedCount;
}
#endif

#ifdef _WIN32
bool Animation::RenderAlpha(BYTE* pixels, int32_t width, int32_t height) const {
    const auto* frame = GetCurrentFrame();
    if (!pixels || width <= 0 || height <= 0 || !frame || !frame->bitmap) return false;
    Gdiplus::Bitmap surface(width, height, width * 4, PixelFormat32bppPARGB, pixels);
    Gdiplus::Graphics graphics(&surface);
    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
    graphics.SetCompositingMode(Gdiplus::CompositingModeSourceCopy);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    if (m_facingLeft) {
        graphics.TranslateTransform(static_cast<float>(width), 0);
        graphics.ScaleTransform(-1.0f, 1.0f);
    }
    Gdiplus::ImageAttributes attributes;
    attributes.SetWrapMode(Gdiplus::WrapModeTileFlipXY);
    // Always draw at the configured render dimensions (m_renderWidth x m_renderHeight).
    // Using frame->width/height as the draw size caused sleep frames with larger native
    // PNG dimensions to appear visually bigger than idle/walk frames.
    int32_t drawW = (m_renderWidth  > 0) ? m_renderWidth  : width;
    int32_t drawH = (m_renderHeight > 0) ? m_renderHeight : height;
    return graphics.DrawImage(frame->bitmap.get(), Gdiplus::Rect(0, 0, drawW, drawH),
        0, 0, frame->width, frame->height, Gdiplus::UnitPixel, &attributes) == Gdiplus::Ok;
}

bool Animation::Render(HDC hdc, int32_t destX, int32_t destY, int32_t destWidth, int32_t destHeight) {
    if (!hdc) return false;

    const AnimationFrame* frame = GetCurrentFrame();

    if (frame && frame->bitmap) {
        Gdiplus::Graphics graphics(hdc);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeNearestNeighbor);
        graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
        if (m_facingLeft) {
            graphics.TranslateTransform(static_cast<float>(destX + destWidth), static_cast<float>(destY));
            graphics.ScaleTransform(-1.0f, 1.0f);
            destX = destY = 0;
        }
        return graphics.DrawImage(frame->bitmap.get(), destX, destY, destWidth, destHeight) == Gdiplus::Ok;
    }

    // Fallback: procedural rendering if sprite assets are not loaded yet
    HBRUSH bodyBrush = ::CreateSolidBrush(RGB(255, 180, 100)); // Warm orange pet
    HBRUSH eyeBrush = ::CreateSolidBrush(RGB(30, 30, 30));
    HGDIOBJ oldBrush = ::SelectObject(hdc, bodyBrush);

    // Body ellipse
    ::Ellipse(hdc, destX, destY + destHeight / 4, destX + destWidth, destY + destHeight);

    // Head / Ears
    ::Ellipse(hdc, destX + destWidth / 8, destY, destX + destWidth * 3 / 8, destY + destHeight / 3);
    ::Ellipse(hdc, destX + destWidth * 5 / 8, destY, destX + destWidth * 7 / 8, destY + destHeight / 3);

    // Eyes
    ::SelectObject(hdc, eyeBrush);
    int eyeOffsetX = m_facingLeft ? -4 : 4;
    ::Ellipse(hdc, destX + destWidth / 3 + eyeOffsetX, destY + destHeight * 4 / 10,
                   destX + destWidth * 4 / 10 + eyeOffsetX, destY + destHeight * 5 / 10);
    ::Ellipse(hdc, destX + destWidth * 6 / 10 + eyeOffsetX, destY + destHeight * 4 / 10,
                   destX + destWidth * 7 / 10 + eyeOffsetX, destY + destHeight * 5 / 10);

    ::SelectObject(hdc, oldBrush);
    ::DeleteObject(bodyBrush);
    ::DeleteObject(eyeBrush);

    return true;
}
#endif

} // namespace VirtualPet
