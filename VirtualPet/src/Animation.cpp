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
    m_currentState = StringToAnimationState(clipName);
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
        AnimationClip clip(folderName, isLooping);

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

    return loadedCount;
}

#ifdef _WIN32
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
