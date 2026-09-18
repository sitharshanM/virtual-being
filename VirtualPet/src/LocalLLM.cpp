#include "LocalLLM.h"

namespace VirtualPet {

void LocalLLM::Configure(bool enabled,
                         const std::string& endpoint,
                         const std::string& model,
                         float intervalSeconds) {
    m_enabled = enabled;
    m_endpoint = endpoint;
    m_model = model;
    m_intervalSeconds = intervalSeconds > 0.0f ? intervalSeconds : 30.0f;
}

void LocalLLM::Update(float deltaTime, const std::string& /*context*/) {
    if (!m_enabled) return;
    m_timer += deltaTime;
    // In background mode, could periodically poll or generate contextual remarks if needed
}

bool LocalLLM::RequestNow(std::string prompt) {
    if (!m_enabled) {
        // Fallback response when LLM is not configured/disabled
        m_lastReply = "I'm right here with you! (" + prompt.substr(0, 30) + "...)";
        m_replyVersion++;
        return true;
    }
    // Simple placeholder when enabled
    m_lastReply = "I hear you! Let's keep hanging out.";
    m_replyVersion++;
    return true;
}

std::string LocalLLM::GetStatus() const {
    if (!m_enabled) return "Disabled (local companion heuristics active)";
    return "Ready (" + m_model + ")";
}

} // namespace VirtualPet
