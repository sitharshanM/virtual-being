#include "Memory.h"

namespace VirtualPet {

Memory::Memory(PetStateData initialData)
    : m_data(std::move(initialData)) {}

void Memory::Update(float deltaTime, int32_t clockHour) {
    if (deltaTime <= 0.0f) return;

#if 0 // SLEEP_MODULE_DISABLED: Clock-aware energy model
    // --- Clock-aware energy model ---
    // Sleep hours (22:00 - 06:00): passive energy recovery, no decay.
    // Active daytime (07:00 - 17:59): normal decay.
    // Evening (18:00 - 21:59): slower decay as she winds down.
    bool isSleepHour = (clockHour >= 22 || clockHour < 6);
    bool isEvening   = (clockHour >= 18 && clockHour < 22);

    if (isSleepHour) {
        // Passive recharge during real night hours (gentle, not as fast as Sleep action)
        m_data.needs.energy = std::clamp(m_data.needs.energy + 0.015f * deltaTime, 0.0f, 1.0f);
    } else {
        float timeOfDayModifier = isEvening ? 0.5f : 1.0f; // Half decay rate in the evening
        float energyModifier = 1.0f + (m_data.personality.playfulness * 0.2f) - (m_data.personality.laziness * 0.2f);
        m_data.needs.energy -= (m_energyDepletionRate * energyModifier * timeOfDayModifier * deltaTime);
    }
#else
    (void)clockHour;
    // With sleep disabled, keep energy at full so pet never suffers exhaustion or gets sleepy
    m_data.needs.energy = 1.0f;
#endif

    // Coffee / boba cravings build gradually over time
    m_data.needs.hunger += (m_hungerIncreaseRate * deltaTime);

    // Mood suffers slightly if overtired or craving coffee for too long
    if (m_data.needs.hunger > 0.80f) {
        m_data.needs.mood -= 0.005f * deltaTime;
    }
    if (m_data.needs.energy < 0.15f) {
        m_data.needs.mood -= 0.004f * deltaTime;
    }

    // Deep bond slowly elevates mood passively
    if (m_data.needs.trust > 0.70f) {
        m_data.needs.mood += 0.002f * deltaTime;
    }

    // Clamp all needs to [0.0, 1.0] range
    m_data.needs.energy = std::clamp(m_data.needs.energy, 0.0f, 1.0f);
    m_data.needs.hunger = std::clamp(m_data.needs.hunger, 0.0f, 1.0f);
    m_data.needs.mood = std::clamp(m_data.needs.mood, 0.0f, 1.0f);
    m_data.needs.trust = std::clamp(m_data.needs.trust, 0.0f, 1.0f);
    m_data.needs.comfort = std::clamp(m_data.needs.comfort, 0.0f, 1.0f);
    m_attentionFatigue = std::max(0.0f, m_attentionFatigue - deltaTime * 0.025f);
}

std::string Memory::GetMoodTitle() const {
#if 0 // SLEEP_MODULE_DISABLED: "sleepy" mood title
    if (m_data.needs.energy < 0.22f) return "sleepy";
#endif
    if (m_data.needs.hunger > 0.78f) return "craving a quiet break";
    if (m_data.needs.comfort < 0.28f) return "guarded";
    if (m_data.needs.mood < 0.35f) return "a little withdrawn";
    if (m_data.needs.mood > 0.82f && m_data.needs.energy > 0.55f) return "bright and playful";
    return "calm";
}

RelationshipTier Memory::GetRelationshipTier() const noexcept {
    float aff = m_data.needs.trust;
    if (aff < 0.25f) return RelationshipTier::Acquaintance;
    if (aff < 0.50f) return RelationshipTier::CloseFriend;
    if (aff < 0.75f) return RelationshipTier::CrushingOnYou;
    if (aff < 0.90f) return RelationshipTier::Sweethearts;
    return RelationshipTier::InseparableForever;
}

std::string Memory::GetRelationshipTitle() const {
    switch (GetRelationshipTier()) {
        case RelationshipTier::Acquaintance:
            return "Shy Acquaintance 🌸";
        case RelationshipTier::CloseFriend:
            return "Close Companion ✨";
        case RelationshipTier::CrushingOnYou:
            return "Crushing on You 💕";
        case RelationshipTier::Sweethearts:
            return "Sweethearts ❤️";
        case RelationshipTier::InseparableForever:
        default:
            return "Inseparable Soulmates 💖";
    }
}

void Memory::ShareCoffee(float amount) {
    m_data.needs.hunger = std::clamp(m_data.needs.hunger - amount, 0.0f, 1.0f);
    m_data.needs.mood = std::clamp(m_data.needs.mood + 0.18f, 0.0f, 1.0f);
    m_data.needs.trust = std::clamp(m_data.needs.trust + 0.035f, 0.0f, 1.0f);
    m_data.needs.comfort = std::clamp(m_data.needs.comfort + 0.04f, 0.0f, 1.0f);
    m_data.history.timesFed++;
    m_data.history.meaningfulMoments++;
    m_attentionFatigue = std::max(0.0f, m_attentionFatigue - 0.2f);
}

bool Memory::GiveHeadpat(float amount) {
    if (m_attentionFatigue > 0.82f || m_data.needs.comfort < 0.20f) {
        m_data.needs.mood = std::clamp(m_data.needs.mood - 0.025f, 0.0f, 1.0f);
        m_data.history.boundariesExpressed++;
        m_lastInteractionAccepted = false;
        return false;
    }
    m_data.needs.mood = std::clamp(m_data.needs.mood + amount * 2.5f, 0.0f, 1.0f);
    m_data.needs.trust = std::clamp(m_data.needs.trust + amount, 0.0f, 1.0f);
    m_data.needs.comfort = std::clamp(m_data.needs.comfort + amount * 0.6f, 0.0f, 1.0f);
    m_data.history.timesPetted++;
    m_attentionFatigue = std::clamp(m_attentionFatigue + 0.18f, 0.0f, 1.0f);
    m_lastInteractionAccepted = true;
    return true;
}

void Memory::TossPlushie(float enjoyment) {
    m_data.needs.mood = std::clamp(m_data.needs.mood + enjoyment, 0.0f, 1.0f);
    m_data.needs.energy = std::clamp(m_data.needs.energy - 0.06f, 0.0f, 1.0f);
    m_data.needs.hunger = std::clamp(m_data.needs.hunger + 0.03f, 0.0f, 1.0f);
    m_data.needs.trust = std::clamp(m_data.needs.trust + 0.02f, 0.0f, 1.0f);
    m_data.history.timesPlayed++;
    m_data.history.meaningfulMoments++;
    m_data.identity.goalProgress = std::clamp(m_data.identity.goalProgress + 0.006f, 0.0f, 1.0f);
}

void Memory::StudyTogether(float duration) {
    m_data.needs.trust = std::clamp(m_data.needs.trust + 0.01f * duration, 0.0f, 1.0f);
    m_data.history.studySessionsTogether++;
    m_data.identity.goalProgress = std::clamp(m_data.identity.goalProgress + 0.002f * duration, 0.0f, 1.0f);
}

void Memory::Rest(float deltaTime, float recoveryRate) {
    m_data.needs.energy = std::clamp(m_data.needs.energy + (recoveryRate * deltaTime), 0.0f, 1.0f);
    // Resting together brings quiet contentment
    m_data.needs.mood = std::clamp(m_data.needs.mood + (0.01f * deltaTime), 0.0f, 1.0f);
}

} // namespace VirtualPet
