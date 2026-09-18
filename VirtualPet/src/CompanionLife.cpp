#include "CompanionLife.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

namespace VirtualPet {

using json = nlohmann::json;

CompanionLife::CompanionLife(std::string path)
    : m_path(std::move(path)) {}

void CompanionLife::SetPath(std::string path) {
    m_path = std::move(path);
}

bool CompanionLife::Load() {
    namespace fs = std::filesystem;
    std::error_code ec;

    if (!fs::exists(m_path, ec) || !fs::is_regular_file(m_path, ec)) {
        return false;
    }

    auto fileSize = fs::file_size(m_path, ec);
    if (ec || fileSize >= 1024 * 1024) { // Reject files >= 1MB
        return false;
    }

    std::ifstream file(m_path);
    if (!file.is_open()) return false;

    try {
        json j = json::parse(file);
        m_lastDay = j.value("last_day", "");
        m_unresolvedIssue = j.value("unresolved_issue", "");
        m_milestoneLevel = j.value("milestone_level", 0);

        m_rememberedFacts.clear();
        if (j.contains("remembered_facts") && j["remembered_facts"].is_array()) {
            for (const auto& item : j["remembered_facts"]) {
                if (item.is_string()) m_rememberedFacts.push_back(item.get<std::string>());
            }
        }

        m_diaryEntries.clear();
        if (j.contains("diary_entries") && j["diary_entries"].is_array()) {
            for (const auto& item : j["diary_entries"]) {
                if (item.is_string()) m_diaryEntries.push_back(item.get<std::string>());
            }
        }

        m_conversation.clear();
        if (j.contains("conversation") && j["conversation"].is_array()) {
            for (const auto& item : j["conversation"]) {
                if (item.is_object() && item.contains("role") && item.contains("content")) {
                    m_conversation.push_back({item["role"].get<std::string>(), item["content"].get<std::string>()});
                }
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool CompanionLife::Save() {
    namespace fs = std::filesystem;
    std::error_code ec;

    fs::path targetPath(m_path);
    if (targetPath.has_parent_path()) {
        fs::create_directories(targetPath.parent_path(), ec);
    }

    std::string tempPath = m_path + ".tmp";
    std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return false;

    try {
        json j;
        j["last_day"] = m_lastDay;
        j["unresolved_issue"] = m_unresolvedIssue;
        j["milestone_level"] = m_milestoneLevel;
        j["remembered_facts"] = m_rememberedFacts;
        j["diary_entries"] = m_diaryEntries;

        json convoArray = json::array();
        for (const auto& turn : m_conversation) {
            convoArray.push_back({{"role", turn.role}, {"content", turn.content}});
        }
        j["conversation"] = convoArray;

        out << j.dump(2);
        out.flush();
        bool ok = out.good();
        out.close();
        if (!ok) {
            fs::remove(tempPath, ec);
            return false;
        }

        fs::rename(tempPath, m_path, ec);
        if (ec) {
            fs::remove(tempPath, ec);
            return false;
        }
        return true;
    } catch (...) {
        out.close();
        fs::remove(tempPath, ec);
        return false;
    }
}

bool CompanionLife::BeginDay(const std::string& day) {
    if (m_lastDay != day) {
        m_lastDay = day;
        return true;
    }
    return false;
}

void CompanionLife::AddTurn(const std::string& role, const std::string& content) {
    m_conversation.push_back({role, content});
    if (m_conversation.size() > 100) {
        m_conversation.erase(m_conversation.begin(), m_conversation.begin() + 20);
    }
}

void CompanionLife::RememberFact(const std::string& fact) {
    if (fact.empty()) return;
    for (const auto& f : m_rememberedFacts) {
        if (f == fact) return;
    }
    m_rememberedFacts.push_back(fact);
}

void CompanionLife::AddDiaryEntry(const std::string& entry) {
    if (!entry.empty()) {
        m_diaryEntries.push_back(entry);
    }
}

std::string CompanionLife::BuildMemoryContext() const {
    std::string context;
    if (!m_rememberedFacts.empty()) {
        context += ". Remembered facts:";
        for (const auto& fact : m_rememberedFacts) {
            context += " [Fact: " + fact + "]";
        }
    }
    if (!m_unresolvedIssue.empty()) {
        context += ". Unresolved issue: " + m_unresolvedIssue;
    }
    return context;
}

std::string CompanionLife::ConversationText() const {
    std::ostringstream ss;
    for (const auto& turn : m_conversation) {
        if (turn.role == "user") {
            ss << "You: " << turn.content << "\n\n";
        } else {
            ss << "Astra: " << turn.content << "\n\n";
        }
    }
    return ss.str();
}

std::string CompanionLife::DiaryText() const {
    std::ostringstream ss;
    ss << "=== Astra's Relationship Diary ===\n\n";
    if (m_diaryEntries.empty()) {
        ss << "No entries recorded yet.\n";
    } else {
        for (const auto& entry : m_diaryEntries) {
            ss << "• " << entry << "\n";
        }
    }
    if (!m_unresolvedIssue.empty()) {
        ss << "\n[Note: " << m_unresolvedIssue << "]\n";
    }
    return ss.str();
}

} // namespace VirtualPet
