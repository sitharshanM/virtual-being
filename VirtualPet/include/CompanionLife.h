#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace VirtualPet {

struct ChatTurn {
    std::string role;
    std::string content;
};

class CompanionLife {
public:
    CompanionLife() = default;
    explicit CompanionLife(std::string path);

    void SetPath(std::string path);
    [[nodiscard]] const std::string& GetPath() const noexcept { return m_path; }

    bool Load();
    bool Save();

    bool BeginDay(const std::string& day);
    void AddTurn(const std::string& role, const std::string& content);
    void RememberFact(const std::string& fact);
    void AddDiaryEntry(const std::string& entry);

    void SetUnresolvedIssue(const std::string& issue) { m_unresolvedIssue = issue; }
    [[nodiscard]] const std::string& GetUnresolvedIssue() const noexcept { return m_unresolvedIssue; }
    void ResolveIssue() { m_unresolvedIssue.clear(); }

    [[nodiscard]] int GetMilestoneLevel() const noexcept { return m_milestoneLevel; }
    void SetMilestoneLevel(int level) noexcept { m_milestoneLevel = level; }

    [[nodiscard]] std::string BuildMemoryContext() const;
    [[nodiscard]] std::string ConversationText() const;
    [[nodiscard]] std::string DiaryText() const;

private:
    std::string m_path{"companion_life.json"};
    std::string m_lastDay;
    std::string m_unresolvedIssue;
    int m_milestoneLevel{0};
    std::vector<ChatTurn> m_conversation;
    std::vector<std::string> m_rememberedFacts;
    std::vector<std::string> m_diaryEntries;
};

} // namespace VirtualPet
