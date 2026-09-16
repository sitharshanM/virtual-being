#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

namespace VirtualPet {

/// Core emotional, physical, and relationship needs of your companion.
struct PetNeeds {
    float energy{0.85f}; ///< [0.0 (exhausted) - 1.0 (well-rested / glowing)]
    float hunger{0.20f}; ///< [0.0 (satisfied) - 1.0 (craving coffee / boba / sweets)]
    float mood{0.85f};   ///< [0.0 (pouting / sad) - 1.0 (joyful / glowing)]
    float trust{0.18f};  ///< [0.0 (new acquaintance) - 1.0 (deep earned trust)]
    float comfort{0.45f};///< [0.0 (needs space) - 1.0 (fully at ease)]

    [[nodiscard]] float GetAffection() const noexcept { return trust; }
    [[nodiscard]] float GetCoffeeCraving() const noexcept { return hunger; }
    void SetAffection(float val) noexcept { trust = std::clamp(val, 0.0f, 1.0f); }
    void SetCoffeeCraving(float val) noexcept { hunger = std::clamp(val, 0.0f, 1.0f); }
};

/// Personality traits shaping her affection dynamics and behavior.
struct PetPersonality {
    float curiosity{0.7f};   ///< Tendency to inspect windows and watch your cursor.
    float playfulness{0.75f};///< Tendency to toss plushies and tease.
    float laziness{0.25f};   ///< Tendency to take cozy naps by your taskbar.
    float sweetness{0.85f};  ///< Warmth and frequency of supportive encouragement.
    float independence{0.72f}; ///< Willingness to pursue her own interests and ask for space.
};

/// Stable details that make the companion a person rather than a reward meter.
struct CompanionIdentity {
    std::string favoriteDrink{"strawberry iced matcha"};
    std::string favoriteMusic{"dream-pop and lo-fi instrumentals"};
    std::string favoriteActivity{"digital painting after midnight"};
    std::string dislike{"being interrupted repeatedly"};
    std::string imperfection{"overthinks small misunderstandings"};
    std::string personalGoal{"finish a tiny illustrated night-sky journal"};
    float goalProgress{0.08f};
};

/// Interaction and relationship memory recorded across desktop sessions.
struct InteractionHistory {
    int32_t timesPlayed{0};             ///< Plushie play sessions
    int32_t timesFed{0};                ///< Coffee & boba dates shared
    int32_t timesPetted{0};             ///< Headpats given
    int32_t studySessionsTogether{0};   ///< Focus & study sessions together
    int32_t daysTogether{1};            ///< Days spent together
    int32_t boundariesExpressed{0};     ///< Times she honestly asked for space
    int32_t meaningfulMoments{0};       ///< Positive shared experiences
    std::string favoriteScreenArea{"bottom-right"};
};

/// Relationship progression tiers based on affection.
enum class RelationshipTier {
    Acquaintance,
    CloseFriend,
    CrushingOnYou,
    Sweethearts,
    InseparableForever
};

/// Complete serializable companion state representation.
struct PetStateData {
    int32_t version{1};
    std::string name{"Astra"};
    PetNeeds needs;
    PetPersonality personality;
    CompanionIdentity identity;
    InteractionHistory history;
};

/**
 * @brief Manages your companion's emotional state, affection level, and relationship memory.
 */
class Memory {
public:
    explicit Memory(PetStateData initialData = PetStateData{});
    ~Memory() = default;

    /// Slowly decays needs over elapsed time and processes relationship dynamics.
    /// Slowly decays needs over elapsed time; clockHour (0-23) drives time-of-day energy logic.
    void Update(float deltaTime, int32_t clockHour = 12);

    // --- Relationship & Interaction Mutators ---

    /// Shares warm coffee or iced boba, satisfying cravings and boosting affection.
    void ShareCoffee(float amount = 0.35f);

    /// Gives Astra a loving headpat, raising mood and deepening affection.
    bool GiveHeadpat(float amount = 0.04f);

    /// Tosses her favorite heart plushie, playing together and boosting spirits.
    void TossPlushie(float enjoyment = 0.2f);

    /// Enters study/focus mode with you, quietly sitting beside your work.
    void StudyTogether(float duration = 1.0f);

    // Legacy wrappers for subsystem compatibility
    void Feed(float amount = 0.35f) { ShareCoffee(amount); }
    void Play(float enjoyment = 0.2f) { TossPlushie(enjoyment); }
    void Pet(float amount = 0.04f) { GiveHeadpat(amount); }

    /// Updates her favorite hangout area on your desktop.
    void SetFavoriteScreenArea(std::string area) { m_data.history.favoriteScreenArea = std::move(area); }

    // --- State Queries ---

    [[nodiscard]] const PetStateData& GetData() const noexcept { return m_data; }
    [[nodiscard]] PetStateData& GetData() noexcept { return m_data; }
    void SetData(const PetStateData& data) noexcept { m_data = data; }

    [[nodiscard]] const std::string& GetName() const noexcept { return m_data.name; }
    void SetName(std::string name) { m_data.name = std::move(name); }

    [[nodiscard]] const PetNeeds& GetNeeds() const noexcept { return m_data.needs; }
    [[nodiscard]] const PetPersonality& GetPersonality() const noexcept { return m_data.personality; }
    [[nodiscard]] const CompanionIdentity& GetIdentity() const noexcept { return m_data.identity; }
    [[nodiscard]] const InteractionHistory& GetHistory() const noexcept { return m_data.history; }

    [[nodiscard]] float GetAffection() const noexcept { return m_data.needs.trust; }
    [[nodiscard]] RelationshipTier GetRelationshipTier() const noexcept;
    [[nodiscard]] std::string GetRelationshipTitle() const;
    [[nodiscard]] std::string GetMoodTitle() const;
    [[nodiscard]] bool WasLastInteractionAccepted() const noexcept { return m_lastInteractionAccepted; }

    [[nodiscard]] bool IsCravingCoffee() const noexcept { return m_data.needs.hunger >= 0.65f; }
    [[nodiscard]] bool IsHungry() const noexcept { return IsCravingCoffee(); }
    [[nodiscard]] bool IsExhausted() const noexcept { return m_data.needs.energy <= 0.2f; }
    [[nodiscard]] bool IsHappy() const noexcept { return m_data.needs.mood >= 0.6f; }
    [[nodiscard]] bool IsTrusting() const noexcept { return m_data.needs.trust >= 0.6f; }

    // --- Rate Tuning ---

    void SetDecayRates(float energyRate, float hungerRate) noexcept {
        m_energyDepletionRate = energyRate;
        m_hungerIncreaseRate = hungerRate;
    }

private:
    PetStateData m_data;
    float m_energyDepletionRate{0.004f}; // Per second
    float m_hungerIncreaseRate{0.006f};  // Per second
    float m_attentionFatigue{0.0f};
    bool m_lastInteractionAccepted{true};
};

} // namespace VirtualPet
