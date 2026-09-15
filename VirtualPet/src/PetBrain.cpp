#include "PetBrain.h"

#include <cmath>
#include <random>

namespace VirtualPet {

PetBrain::PetBrain() {
    BuildBehaviorTree();
}

void PetBrain::BuildBehaviorTree() {
    m_tree = std::make_unique<BehaviorTree>();
}

UtilityScores PetBrain::CalculateUtilityScores(const Memory& memory,
                                               const MouseSensor& mouse,
                                               const SystemSensor& system,
                                               const Physics& physics) const {
    UtilityScores u;
    const auto& needs = memory.GetNeeds();
    const auto& p = memory.GetPersonality();
    const auto& toy = physics.GetToy();

    // 1. Sleep Utility: exponentially increases with exhaustion, night, and user inactivity
    float exhaustion = 1.0f - needs.energy;
    float nightMultiplier = system.IsNight() ? 1.4f : 1.0f;
    float idleMultiplier = system.IsUserIdle() ? 1.3f : 1.0f;
    u.sleepScore = std::pow(exhaustion, 1.3f) * nightMultiplier * idleMultiplier * (0.8f + p.laziness * 0.4f);

    // 2. Treat Eating Utility
    if (toy.active && toy.isTreat) {
        u.eatScore = std::pow(needs.hunger, 1.1f) * 2.2f + 0.3f;
    } else {
        u.eatScore = std::pow(needs.hunger, 1.4f) * 0.6f;
    }

    // 3. Toy Play Utility
    if (toy.active && !toy.isTreat) {
        u.toyScore = (p.playfulness * 1.6f + 0.4f) * (0.5f + needs.energy * 0.5f);
    } else {
        u.toyScore = 0.0f;
    }

    // 4. Cursor Follow Utility
    if (mouse.IsNear() && !toy.active) {
        u.followScore = p.playfulness * 0.9f * (0.4f + needs.mood * 0.6f);
    } else {
        u.followScore = 0.0f;
    }

    // 5. Desktop Exploration / Wander Utility
    u.wanderScore = p.curiosity * 0.75f * (0.4f + needs.energy * 0.6f);

    // 6. Idle Utility: resting quietly
    u.idleScore = 0.20f + (p.laziness * 0.45f);

    return u;
}

BrainDecision PetBrain::Update(float deltaTime,
                               const MouseSensor& mouse,
                               const SystemSensor& system,
                               Memory& memory,
                               const DesktopWorld& world,
                               Physics& physics) {
    (void)world;
    m_actionTimer -= deltaTime;

    const Point petPos = physics.GetPosition();
    const Point cursor = mouse.GetCursorPosition();
    ToyItem& toy = physics.GetToy();

    // 1. High Priority Direct Sensory Overrides
    if (physics.IsDragged()) {
        m_currentAction = PetAction::Dragged;
        m_currentThought = "Wheee! Flying across the desktop windows!";
        BrainDecision d;
        d.action = PetAction::Dragged;
        d.animState = AnimationState::Reaction;
        d.targetHorizontalSpeed = 0.0f;
        d.facingLeft = m_facingLeft;
        d.thought = m_currentThought;
        return d;
    }

    if (mouse.WasLeftButtonClicked()) {
        memory.Pet(0.08f);
        m_currentAction = PetAction::ReactingToClick;
        m_currentThought = "Purring with happiness from your pets!";
        m_actionTimer = 1.8f;
    }

    if (m_currentAction == PetAction::ReactingToClick && m_actionTimer > 0.0f) {
        BrainDecision d;
        d.action = PetAction::ReactingToClick;
        d.animState = AnimationState::Reaction;
        d.targetHorizontalSpeed = 0.0f;
        d.facingLeft = m_facingLeft;
        d.thought = m_currentThought;
        return d;
    }

    // 2. Evaluate Utility Curves for autonomous behavior
    m_lastScores = CalculateUtilityScores(memory, mouse, system, physics);

    // Pick motivation with highest score
    float maxScore = -1.0f;
    PetAction chosenAction = PetAction::Idle;

    if (m_lastScores.eatScore > maxScore && toy.active && toy.isTreat) {
        maxScore = m_lastScores.eatScore;
        chosenAction = PetAction::EatingTreat;
    }
    if (m_lastScores.toyScore > maxScore && toy.active && !toy.isTreat) {
        maxScore = m_lastScores.toyScore;
        chosenAction = PetAction::ChasingToy;
    }
    if (m_lastScores.sleepScore > maxScore && m_lastScores.sleepScore > 0.55f) {
        maxScore = m_lastScores.sleepScore;
        chosenAction = PetAction::Sleeping;
    }
    if (m_lastScores.followScore > maxScore && m_lastScores.followScore > 0.35f) {
        maxScore = m_lastScores.followScore;
        chosenAction = PetAction::FollowingCursor;
    }
    if (m_lastScores.wanderScore > maxScore && m_actionTimer <= 0.0f) {
        maxScore = m_lastScores.wanderScore;
        chosenAction = PetAction::Wandering;
    }

    // Default to Idle if no strong motivation
    if (maxScore < 0.0f && m_actionTimer <= 0.0f) {
        chosenAction = PetAction::Idle;
    }

    if (m_actionTimer <= 0.0f || chosenAction == PetAction::EatingTreat || chosenAction == PetAction::ChasingToy) {
        m_currentAction = chosenAction;
        if (m_currentAction == PetAction::Wandering || m_currentAction == PetAction::Idle) {
            m_actionTimer = 3.0f + (static_cast<float>(rand() % 100) / 100.0f) * 4.0f;
        }
    }

    BrainDecision decision;
    decision.action = m_currentAction;

    // 3. Execute Selected Behavioral Action
    switch (m_currentAction) {
        case PetAction::EatingTreat: {
            float dx = toy.x - (petPos.x + physics.GetWidth() / 2);
            m_facingLeft = (dx < 0.0f);
            m_currentThought = "Sniffing out a yummy desktop treat!";

            if (std::abs(dx) > 18.0f) {
                decision.animState = AnimationState::Walk;
                decision.targetHorizontalSpeed = m_facingLeft ? -100.0f : 100.0f;
            } else {
                // Reached treat! Eat snack
                decision.animState = AnimationState::Reaction;
                decision.targetHorizontalSpeed = 0.0f;
                memory.Feed(0.35f);
                physics.ClearToy();
                m_currentThought = "Munch munch! Delicious treat!";
                m_actionTimer = 2.0f;
                m_currentAction = PetAction::ReactingToClick;
            }
            break;
        }

        case PetAction::ChasingToy: {
            float dx = toy.x - (petPos.x + physics.GetWidth() / 2);
            m_facingLeft = (dx < 0.0f);
            m_currentThought = "Pouncing after the toy!";

            if (std::abs(dx) > 22.0f) {
                float speed = (std::abs(dx) > 80.0f) ? 150.0f : 90.0f;
                decision.animState = (speed > 100.0f) ? AnimationState::Run : AnimationState::Walk;
                decision.targetHorizontalSpeed = m_facingLeft ? -speed : speed;
            } else {
                // Reached toy! Bat and play
                decision.animState = AnimationState::Reaction;
                decision.targetHorizontalSpeed = 0.0f;
                decision.wantJump = true;
                toy.vx = m_facingLeft ? -350.0f : 350.0f;
                toy.vy = -280.0f;
                memory.Play(0.15f);
                m_currentThought = "Swatted the yarn ball!";
            }
            break;
        }

        case PetAction::Sleeping:
            decision.animState = AnimationState::Sleep;
            decision.targetHorizontalSpeed = 0.0f;
            m_currentThought = "Zzz... having sweet desktop dreams.";
            memory.Rest(deltaTime, 0.06f);
            if (memory.GetNeeds().energy >= 0.95f && !system.IsNight()) {
                m_currentAction = PetAction::Idle;
            }
            break;

        case PetAction::FollowingCursor: {
            float dx = static_cast<float>(cursor.x - (petPos.x + physics.GetWidth() / 2));
            m_currentThought = "Chasing after your mouse pointer!";
            if (std::abs(dx) > 25.0f) {
                m_facingLeft = (dx < 0.0f);
                float speed = (std::abs(dx) > 100.0f) ? 140.0f : 85.0f;
                decision.animState = (speed > 100.0f) ? AnimationState::Run : AnimationState::Walk;
                decision.targetHorizontalSpeed = m_facingLeft ? -speed : speed;
            } else {
                decision.animState = AnimationState::Idle;
                decision.targetHorizontalSpeed = 0.0f;
            }
            break;
        }

        case PetAction::Wandering: {
            if (m_wanderTargetX == 0 || std::abs(petPos.x - m_wanderTargetX) < 15) {
                Rect area = world.GetPrimaryWorkArea();
                int32_t span = area.width - physics.GetWidth() - 60;
                m_wanderTargetX = area.x + 30 + (span > 0 ? (rand() % span) : 0);
            }

            float dx = static_cast<float>(m_wanderTargetX - petPos.x);
            m_currentThought = "Wandering along window borders.";
            if (std::abs(dx) > 12.0f) {
                m_facingLeft = (dx < 0.0f);
                decision.animState = AnimationState::Walk;
                decision.targetHorizontalSpeed = m_facingLeft ? -65.0f : 65.0f;
            } else {
                decision.animState = AnimationState::Idle;
                decision.targetHorizontalSpeed = 0.0f;
            }
            break;
        }

        case PetAction::Idle:
        default:
            decision.animState = AnimationState::Idle;
            decision.targetHorizontalSpeed = 0.0f;
            m_currentThought = "Sitting comfortably watching you work.";
            break;
    }

    decision.facingLeft = m_facingLeft;
    decision.thought = m_currentThought;
    return decision;
}

} // namespace VirtualPet
