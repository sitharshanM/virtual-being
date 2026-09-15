#include "Memory.h"

namespace VirtualPet {

Memory::Memory(PetStateData initialData)
    : m_data(std::move(initialData)) {}

void Memory::Update(float deltaTime) {
    if (deltaTime <= 0.0f) return;

    // Lazier pets burn energy slower; playful pets burn it faster
    float energyModifier = 1.0f + (m_data.personality.playfulness * 0.3f) - (m_data.personality.laziness * 0.3f);
    m_data.needs.energy -= (m_energyDepletionRate * energyModifier * deltaTime);

    // Hunger builds over time
    m_data.needs.hunger += (m_hungerIncreaseRate * deltaTime);

    // Mood suffers if neglected, starving, or overtired
    if (m_data.needs.hunger > 0.75f) {
        m_data.needs.mood -= 0.01f * deltaTime;
    }
    if (m_data.needs.energy < 0.2f) {
        m_data.needs.mood -= 0.005f * deltaTime;
    }

    // Clamp all needs to [0.0, 1.0] range
    m_data.needs.energy = std::clamp(m_data.needs.energy, 0.0f, 1.0f);
    m_data.needs.hunger = std::clamp(m_data.needs.hunger, 0.0f, 1.0f);
    m_data.needs.mood = std::clamp(m_data.needs.mood, 0.0f, 1.0f);
    m_data.needs.trust = std::clamp(m_data.needs.trust, 0.0f, 1.0f);
}

void Memory::Feed(float amount) {
    m_data.needs.hunger = std::clamp(m_data.needs.hunger - amount, 0.0f, 1.0f);
    m_data.needs.mood = std::clamp(m_data.needs.mood + 0.15f, 0.0f, 1.0f);
    m_data.needs.trust = std::clamp(m_data.needs.trust + 0.03f, 0.0f, 1.0f);
    m_data.history.timesFed++;
}

void Memory::Play(float enjoyment) {
    m_data.needs.mood = std::clamp(m_data.needs.mood + enjoyment, 0.0f, 1.0f);
    m_data.needs.energy = std::clamp(m_data.needs.energy - 0.08f, 0.0f, 1.0f);
    m_data.needs.hunger = std::clamp(m_data.needs.hunger + 0.04f, 0.0f, 1.0f);
    m_data.needs.trust = std::clamp(m_data.needs.trust + 0.02f, 0.0f, 1.0f);
    m_data.history.timesPlayed++;
}

void Memory::Pet(float amount) {
    m_data.needs.mood = std::clamp(m_data.needs.mood + amount, 0.0f, 1.0f);
    m_data.needs.trust = std::clamp(m_data.needs.trust + 0.015f, 0.0f, 1.0f);
    m_data.history.timesPetted++;
}

void Memory::Rest(float deltaTime, float recoveryRate) {
    m_data.needs.energy = std::clamp(m_data.needs.energy + (recoveryRate * deltaTime), 0.0f, 1.0f);
}

} // namespace VirtualPet
