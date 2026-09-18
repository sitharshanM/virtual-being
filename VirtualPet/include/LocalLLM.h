#pragma once

#include <string>
#include <cstdint>

namespace VirtualPet {

class LocalLLM {
public:
    LocalLLM() = default;

    void Configure(bool enabled,
                   const std::string& endpoint,
                   const std::string& model,
                   float intervalSeconds);

    void Update(float deltaTime, const std::string& context);
    bool RequestNow(std::string prompt);

    [[nodiscard]] bool IsBusy() const noexcept { return m_isBusy; }
    [[nodiscard]] bool HasReply() const noexcept { return !m_lastReply.empty(); }
    [[nodiscard]] const std::string& GetReply() const noexcept { return m_lastReply; }
    [[nodiscard]] uint64_t GetReplyVersion() const noexcept { return m_replyVersion; }
    [[nodiscard]] std::string GetStatus() const;

private:
    bool m_enabled{false};
    std::string m_endpoint;
    std::string m_model;
    float m_intervalSeconds{30.0f};
    float m_timer{0.0f};

    bool m_isBusy{false};
    std::string m_lastReply;
    uint64_t m_replyVersion{0};
};

} // namespace VirtualPet
