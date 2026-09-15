#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

namespace VirtualPet {

/// Core psychological and physiological needs of the pet.
struct PetNeeds {
    float energy{0.8f}; ///< [0.0 (exhausted) - 1.0 (energetic)]
    float hunger{0.2f}; ///< [0.0 (full) - 1.0 (starving)]
    float mood{0.8f};   ///< [0.0 (unhappy) - 1.0 (ecstatic)]
    float trust{0.5f};  ///< [0.0 (fearful) - 1.0 (devoted)]
};

/// Personality traits shaping decision biases and reaction frequencies.
struct PetPersonality {
    float curiosity{0.7f};   ///< Tendency to inspect screen areas and follow cursor.
    float playfulness{0.7f}; ///< Tendency to trigger games and seek interaction.
    float laziness{0.3f};    ///< Preference for rest and sitting idle.
};

/// Interaction and habit memory recorded across sessions.
struct InteractionHistory {
    int32_t timesPlayed{0};
    int32_t timesFed{0};
    int32_t timesPetted{0};
    std::string favoriteScreenArea{"bottom-right"};
};

/// Complete serializable pet state data representation.
struct PetStateData {
    int32_t version{1};
    std::string name{"Astra"};
    PetNeeds needs;
    PetPersonality personality;
    InteractionHistory history;
};

/**
 * @brief Manages the pet's dynamic needs, personality traits, and interaction memories.
 */
class Memory {
public:
    explicit Memory(PetStateData initialData = PetStateData{});
    ~Memory() = default;

    /// Slowly decays needs over elapsed time based on personality traits.
    void Update(float deltaTime);

    // --- Interaction Mutators ---

    /// Feeds the pet, decreasing hunger and boosting mood.
    void Feed(float amount = 0.35f);

    /// Engages pet in play, increasing timesPlayed, boosting mood, but expending energy.
    void Play(float enjoyment = 0.2f);

    /// Pets the companion, increasing trust and mood.
    void Pet(float amount = 0.05f);

    /// Allows the pet to rest or sleep, recovering energy.
    void Rest(float deltaTime, float recoveryRate = 0.05f);

    /// Updates the pet's favorite hangout quadrant on the desktop.
    void SetFavoriteScreenArea(std::string area) { m_data.history.favoriteScreenArea = std::move(area); }

    // --- State Queries ---

    [[nodiscard]] const PetStateData& GetData() const noexcept { return m_data; }
    [[nodiscard]] PetStateData& GetData() noexcept { return m_data; }
    void SetData(const PetStateData& data) noexcept { m_data = data; }

    [[nodiscard]] const std::string& GetName() const noexcept { return m_data.name; }
    void SetName(std::string name) { m_data.name = std::move(name); }

    [[nodiscard]] const PetNeeds& GetNeeds() const noexcept { return m_data.needs; }
    [[nodiscard]] const PetPersonality& GetPersonality() const noexcept { return m_data.personality; }
    [[nodiscard]] const InteractionHistory& GetHistory() const noexcept { return m_data.history; }

    [[nodiscard]] bool IsHungry() const noexcept { return m_data.needs.hunger >= 0.7f; }
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
    float m_energyDepletionRate{0.005f}; // Per second
    float m_hungerIncreaseRate{0.008f};  // Per second
};

} // namespace VirtualPet
