#include "BehaviorTree.h"

namespace VirtualPet {

// ============================================================================
// Blackboard Implementation
// ============================================================================

void Blackboard::SetBool(const std::string& key, bool value) {
    m_bools[key] = value;
}

bool Blackboard::GetBool(const std::string& key, bool defaultValue) const {
    auto it = m_bools.find(key);
    return (it != m_bools.end()) ? it->second : defaultValue;
}

void Blackboard::SetFloat(const std::string& key, float value) {
    m_floats[key] = value;
}

float Blackboard::GetFloat(const std::string& key, float defaultValue) const {
    auto it = m_floats.find(key);
    return (it != m_floats.end()) ? it->second : defaultValue;
}

void Blackboard::SetInt(const std::string& key, int32_t value) {
    m_ints[key] = value;
}

int32_t Blackboard::GetInt(const std::string& key, int32_t defaultValue) const {
    auto it = m_ints.find(key);
    return (it != m_ints.end()) ? it->second : defaultValue;
}

void Blackboard::SetString(const std::string& key, const std::string& value) {
    m_strings[key] = value;
}

std::string Blackboard::GetString(const std::string& key, const std::string& defaultValue) const {
    auto it = m_strings.find(key);
    return (it != m_strings.end()) ? it->second : defaultValue;
}

bool Blackboard::HasKey(const std::string& key) const {
    return (m_bools.find(key) != m_bools.end()) ||
           (m_floats.find(key) != m_floats.end()) ||
           (m_ints.find(key) != m_ints.end()) ||
           (m_strings.find(key) != m_strings.end());
}

void Blackboard::Remove(const std::string& key) {
    m_bools.erase(key);
    m_floats.erase(key);
    m_ints.erase(key);
    m_strings.erase(key);
}

void Blackboard::Clear() {
    m_bools.clear();
    m_floats.clear();
    m_ints.clear();
    m_strings.clear();
}

// ============================================================================
// SequenceNode Implementation
// ============================================================================

NodeStatus SequenceNode::Tick(float deltaTime, Blackboard& blackboard) {
    while (m_currentChildIndex < m_children.size()) {
        NodeStatus status = m_children[m_currentChildIndex]->Tick(deltaTime, blackboard);

        if (status == NodeStatus::Running) {
            return NodeStatus::Running;
        }

        if (status == NodeStatus::Failure) {
            Reset();
            return NodeStatus::Failure;
        }

        // Child succeeded, advance to next
        m_currentChildIndex++;
    }

    // All children succeeded
    Reset();
    return NodeStatus::Success;
}

// ============================================================================
// SelectorNode Implementation
// ============================================================================

NodeStatus SelectorNode::Tick(float deltaTime, Blackboard& blackboard) {
    while (m_currentChildIndex < m_children.size()) {
        NodeStatus status = m_children[m_currentChildIndex]->Tick(deltaTime, blackboard);

        if (status == NodeStatus::Running) {
            return NodeStatus::Running;
        }

        if (status == NodeStatus::Success) {
            Reset();
            return NodeStatus::Success;
        }

        // Child failed, advance to try next selector branch
        m_currentChildIndex++;
    }

    // All children failed
    Reset();
    return NodeStatus::Failure;
}

// ============================================================================
// BehaviorTree Implementation
// ============================================================================

NodeStatus BehaviorTree::Tick(float deltaTime) {
    if (m_root) {
        return m_root->Tick(deltaTime, m_blackboard);
    }
    return NodeStatus::Failure;
}

void BehaviorTree::Reset() {
    if (m_root) {
        m_root->Reset();
    }
}

} // namespace VirtualPet
