#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace VirtualPet {

/// Execution status returned by behavior tree nodes.
enum class NodeStatus {
    Success,
    Failure,
    Running
};

/**
 * @brief Key-value blackboard for sharing state across behavior tree nodes and brain subsystems.
 */
class Blackboard {
public:
    Blackboard() = default;

    void SetBool(const std::string& key, bool value);
    [[nodiscard]] bool GetBool(const std::string& key, bool defaultValue = false) const;

    void SetFloat(const std::string& key, float value);
    [[nodiscard]] float GetFloat(const std::string& key, float defaultValue = 0.0f) const;

    void SetInt(const std::string& key, int32_t value);
    [[nodiscard]] int32_t GetInt(const std::string& key, int32_t defaultValue = 0) const;

    void SetString(const std::string& key, const std::string& value);
    [[nodiscard]] std::string GetString(const std::string& key, const std::string& defaultValue = "") const;

    [[nodiscard]] bool HasKey(const std::string& key) const;
    void Remove(const std::string& key);
    void Clear();

private:
    std::unordered_map<std::string, bool> m_bools;
    std::unordered_map<std::string, float> m_floats;
    std::unordered_map<std::string, int32_t> m_ints;
    std::unordered_map<std::string, std::string> m_strings;
};

/**
 * @brief Abstract base class for all nodes in the behavior tree.
 */
class BehaviorNode {
public:
    explicit BehaviorNode(std::string name = "BehaviorNode")
        : m_name(std::move(name)) {}
    virtual ~BehaviorNode() = default;

    virtual NodeStatus Tick(float deltaTime, Blackboard& blackboard) = 0;
    virtual void Reset() = 0;

    [[nodiscard]] const std::string& GetName() const noexcept { return m_name; }

protected:
    std::string m_name;
};

/**
 * @brief Base class for composite nodes that manage a list of children.
 */
class CompositeNode : public BehaviorNode {
public:
    explicit CompositeNode(std::string name = "CompositeNode")
        : BehaviorNode(std::move(name)) {}

    void AddChild(std::shared_ptr<BehaviorNode> child) {
        if (child) {
            m_children.push_back(std::move(child));
        }
    }

    void Reset() override {
        m_currentChildIndex = 0;
        for (auto& child : m_children) {
            child->Reset();
        }
    }

protected:
    std::vector<std::shared_ptr<BehaviorNode>> m_children;
    size_t m_currentChildIndex{0};
};

/**
 * @brief Executes children in order. Succeeds if ALL children succeed.
 */
class SequenceNode : public CompositeNode {
public:
    explicit SequenceNode(std::string name = "Sequence")
        : CompositeNode(std::move(name)) {}

    NodeStatus Tick(float deltaTime, Blackboard& blackboard) override;
};

/**
 * @brief Executes children in order. Succeeds if ANY child succeeds.
 */
class SelectorNode : public CompositeNode {
public:
    explicit SelectorNode(std::string name = "Selector")
        : CompositeNode(std::move(name)) {}

    NodeStatus Tick(float deltaTime, Blackboard& blackboard) override;
};

/**
 * @brief Leaf node that executes a functional action.
 */
class ActionNode : public BehaviorNode {
public:
    using ActionFunc = std::function<NodeStatus(float, Blackboard&)>;

    ActionNode(std::string name, ActionFunc action)
        : BehaviorNode(std::move(name)), m_action(std::move(action)) {}

    NodeStatus Tick(float deltaTime, Blackboard& blackboard) override {
        if (m_action) {
            return m_action(deltaTime, blackboard);
        }
        return NodeStatus::Failure;
    }

    void Reset() override {}

private:
    ActionFunc m_action;
};

/**
 * @brief Leaf node that checks a boolean condition.
 */
class ConditionNode : public BehaviorNode {
public:
    using ConditionFunc = std::function<bool(const Blackboard&)>;

    ConditionNode(std::string name, ConditionFunc condition)
        : BehaviorNode(std::move(name)), m_condition(std::move(condition)) {}

    NodeStatus Tick(float deltaTime, Blackboard& blackboard) override {
        (void)deltaTime;
        if (m_condition && m_condition(blackboard)) {
            return NodeStatus::Success;
        }
        return NodeStatus::Failure;
    }

    void Reset() override {}

private:
    ConditionFunc m_condition;
};

/**
 * @brief Root controller managing the behavior tree and its associated blackboard.
 */
class BehaviorTree {
public:
    BehaviorTree() = default;
    explicit BehaviorTree(std::shared_ptr<BehaviorNode> root)
        : m_root(std::move(root)) {}

    void SetRoot(std::shared_ptr<BehaviorNode> root) { m_root = std::move(root); }
    [[nodiscard]] std::shared_ptr<BehaviorNode> GetRoot() const noexcept { return m_root; }

    NodeStatus Tick(float deltaTime);
    void Reset();

    [[nodiscard]] Blackboard& GetBlackboard() noexcept { return m_blackboard; }
    [[nodiscard]] const Blackboard& GetBlackboard() const noexcept { return m_blackboard; }

private:
    std::shared_ptr<BehaviorNode> m_root;
    Blackboard m_blackboard;
};

} // namespace VirtualPet
