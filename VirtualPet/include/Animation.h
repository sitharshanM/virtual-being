#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#endif

namespace VirtualPet {

/// Primary pet animation states matching asset folder categories.
enum class AnimationState {
    Idle,
    Walk,
    Run,
    Sleep,
    Reaction,
    Custom
};

/// Convert AnimationState enum to folder name string.
[[nodiscard]] const char* AnimationStateToString(AnimationState state) noexcept;

/// Convert folder name string to AnimationState enum.
[[nodiscard]] AnimationState StringToAnimationState(const std::string& name) noexcept;

/// Represents a single frame in an animation sequence.
struct AnimationFrame {
    std::string filePath;               ///< Relative or absolute path to sprite image.
    float durationSeconds{0.1f};        ///< Duration to display this frame in seconds.
    int32_t width{0};                   ///< Pixel width of frame.
    int32_t height{0};                  ///< Pixel height of frame.
#ifdef _WIN32
    std::shared_ptr<Gdiplus::Bitmap> bitmap; ///< Shared ownership of decoded image data.
#endif
};

/// Represents a sequence of frames forming an animation clip.
class AnimationClip {
public:
    AnimationClip() = default;
    explicit AnimationClip(std::string name, bool isLooping = true, float defaultFrameDuration = 0.1f);

    void AddFrame(const AnimationFrame& frame);
    void AddFrame(const std::string& filePath, float durationSeconds = -1.0f);
    void Clear();

    [[nodiscard]] const std::string& GetName() const noexcept { return m_name; }
    [[nodiscard]] bool IsLooping() const noexcept { return m_isLooping; }
    void SetLooping(bool looping) noexcept { m_isLooping = looping; }

    [[nodiscard]] size_t GetFrameCount() const noexcept { return m_frames.size(); }
    [[nodiscard]] bool IsEmpty() const noexcept { return m_frames.empty(); }

    [[nodiscard]] const AnimationFrame* GetFrame(size_t index) const noexcept;
    [[nodiscard]] float GetTotalDuration() const noexcept { return m_totalDuration; }

private:
    std::string m_name;
    std::vector<AnimationFrame> m_frames;
    bool m_isLooping{true};
    float m_totalDuration{0.0f};
    float m_defaultFrameDuration{0.1f};
};

/// Callback invoked when a non-looping animation reaches its end.
using AnimationCompleteCallback = std::function<void(AnimationState state)>;

/**
 * @brief Manages animation states, clip playback, timing, and frame rendering.
 */
class Animation {
public:
    Animation();
    ~Animation();

    // Non-copyable due to optional native image handles
    Animation(const Animation&) = delete;
    Animation& operator=(const Animation&) = delete;
    Animation(Animation&&) = delete;
    Animation& operator=(Animation&&) = delete;

    /// Advances playback timer and frame index based on elapsed seconds.
    void Update(float deltaTime);

    /// Switches current animation clip to state.
    /// @param state Target animation state.
    /// @param restartIfSame If false, calling Play with already playing state continues playback.
    void Play(AnimationState state, bool restartIfSame = false);

    /// Switches playback to a registered clip by name.
    void PlayClip(const std::string& clipName, bool restartIfSame = false);

    /// Registers a custom or loaded clip into the animation controller.
    void RegisterClip(AnimationState state, AnimationClip clip);
    void RegisterClip(const std::string& name, AnimationClip clip);

    /// Loads animation frames from the disk assets directory (e.g. assets/idle, assets/walk).
    /// @param assetsDirectory Root assets folder path containing idle/, walk/, run/, sleep/, reactions/.
    /// @return Number of clips loaded.
    size_t LoadFromDirectory(const std::string& assetsDirectory);

    // --- State & Playback Queries ---

    [[nodiscard]] AnimationState GetCurrentState() const noexcept { return m_currentState; }
    [[nodiscard]] const std::string& GetCurrentClipName() const noexcept { return m_currentClipName; }
    [[nodiscard]] size_t GetCurrentFrameIndex() const noexcept { return m_currentFrameIndex; }
    [[nodiscard]] const AnimationFrame* GetCurrentFrame() const noexcept;

    [[nodiscard]] bool IsFinished() const noexcept { return m_isFinished; }
    [[nodiscard]] bool IsFacingLeft() const noexcept { return m_facingLeft; }
    void SetFacingLeft(bool facingLeft) noexcept { m_facingLeft = facingLeft; }

    [[nodiscard]] float GetPlaybackSpeed() const noexcept { return m_playbackSpeed; }
    void SetPlaybackSpeed(float speed) noexcept { m_playbackSpeed = (speed > 0.0f) ? speed : 1.0f; }

    /// Register a callback to fire when a non-looping clip completes.
    void SetOnCompleteCallback(AnimationCompleteCallback callback) { m_onComplete = std::move(callback); }

#ifdef _WIN32
    /// Renders current animation frame to the specified Windows device context.
    /// Supports alpha blending and horizontal flipping based on facing direction.
    bool Render(HDC hdc, int32_t destX, int32_t destY, int32_t destWidth, int32_t destHeight);
#endif

private:
#ifdef _WIN32
    ULONG_PTR m_gdiToken{0};
#endif
    std::unordered_map<std::string, AnimationClip> m_clips;
    std::string m_currentClipName;
    AnimationState m_currentState{AnimationState::Idle};

    size_t m_currentFrameIndex{0};
    float m_frameTimer{0.0f};
    float m_playbackSpeed{1.0f};
    bool m_facingLeft{false};
    bool m_isFinished{false};

    AnimationCompleteCallback m_onComplete;
};

} // namespace VirtualPet
