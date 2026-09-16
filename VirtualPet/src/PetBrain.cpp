#include "PetBrain.h"

#include <cmath>
#include <random>
#include <ctime>

namespace VirtualPet {

PetBrain::PetBrain() {
    BuildBehaviorTree();
}

void PetBrain::RequestAction(PetAction action, float duration) {
    m_currentAction = action;
    m_actionTimer = duration;
    m_manualSleep = (action == PetAction::Sleeping);
    if (action == PetAction::ReactingToClick) {
        m_currentThought = "Blushing happily from your warm touch ❤️";
    }
}

void PetBrain::RequestReaction(std::string thought, float duration) {
    m_currentAction = PetAction::ReactingToClick;
    m_actionTimer = duration;
    m_manualSleep = false;
    m_currentThought = std::move(thought);
}

void PetBrain::StartStudyMode(float duration) {
    m_studyModeTimer = duration;
    m_currentAction = PetAction::StudyMode;
    m_actionTimer = duration;
    m_currentThought = "Focus mode with you! I'll sit quietly cheering you on 📖✨";
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

    // 1. Sleep Utility: scales with exhaustion, real clock hour, and user inactivity.
    //    Sleepiness peaks at ~02:00 and is near-zero at ~10:00, using a cosine curve
    //    so the urge to sleep rises smoothly through the evening and fades by morning.
    float exhaustion = 1.0f - needs.energy;
    const int32_t hour   = system.GetState().currentHour;   // 0-23 from real clock
    const int32_t minute = system.GetState().currentMinute; // 0-59
    // Map clock to a float hour in [0, 24)
    float clockH = static_cast<float>(hour) + static_cast<float>(minute) / 60.0f;
    // Peak sleepiness at 02:00 (hour 2). Shift so peak maps to pi.
    // sleepClock = 0.0 at 02:00 (peak), 1.0 at 14:00 (trough)
    float angleRad = ((clockH - 2.0f) / 24.0f) * (2.0f * 3.14159265f);
    float sleepClock = (1.0f - std::cos(angleRad)) * 0.5f; // [0=peak sleep, 1=most awake]
    float clockMultiplier = 0.4f + (1.0f - sleepClock) * 1.4f; // [0.4 at 14:00, 1.8 at 02:00]
    float idleMultiplier = system.IsUserIdle() ? 1.25f : 1.0f;
    u.sleepScore = std::pow(exhaustion, 1.3f) * clockMultiplier * idleMultiplier * (0.75f + p.laziness * 0.4f);

    // 2. Coffee / Boba Break Utility
    if (toy.active && toy.isTreat) {
        u.eatScore = std::pow(needs.GetCoffeeCraving(), 1.1f) * 2.4f + 0.35f;
    } else {
        u.eatScore = std::pow(needs.GetCoffeeCraving(), 1.4f) * 0.55f;
    }

    // 3. Plushie Play Utility
    if (toy.active && !toy.isTreat) {
        u.toyScore = (p.playfulness * 1.7f + 0.3f) * (0.5f + needs.energy * 0.5f);
    } else {
        u.toyScore = 0.0f;
    }

    // 4. Cursor Follow / Peeking at what you're doing
    if (mouse.IsNear() && !toy.active) {
        u.followScore = (p.playfulness * 0.6f + p.sweetness * 0.5f) * (0.4f + needs.trust * 0.6f);
    } else {
        u.followScore = 0.0f;
    }

    // 5. Desktop Stroll / Window Exploration
    u.wanderScore = p.curiosity * 0.65f * (0.4f + needs.energy * 0.6f);

    // 6. Study / Focus Mode
    if (m_studyModeTimer > 0.0f) {
        u.studyScore = 2.5f;
    } else {
        u.studyScore = 0.0f;
    }

    // 7. Companion Idle (quietly sitting by your windows)
    u.idleScore = 0.25f + (p.laziness * 0.35f);

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
    m_scoreTimer -= deltaTime;
    m_contactCooldown -= deltaTime;
    if (m_studyModeTimer > 0.0f) {
        m_studyModeTimer -= deltaTime;
    }

    const Point petPos = physics.GetPosition();
    const Point cursor = mouse.GetCursorPosition();
    ToyItem& toy = physics.GetToy();

    // 1. High Priority Direct Sensory Overrides
    if (physics.IsDragged()) {
        m_manualSleep = false;
        m_currentAction = PetAction::Dragged;
        m_currentThought = "Kyaaa! Where are you taking me?! Put me down gently, okay? 💕";
        BrainDecision d;
        d.action = PetAction::Dragged;
        d.animState = AnimationState::Reaction;
        d.targetHorizontalSpeed = 0.0f;
        d.facingLeft = m_facingLeft;
        d.thought = m_currentThought;
        return d;
    }

    if (mouse.WasLeftButtonClicked()) {
        m_manualSleep = false;
        const bool accepted = memory.GiveHeadpat(0.04f);
        m_currentAction = PetAction::ReactingToClick;
        float aff = memory.GetAffection();
        if (!accepted) {
            m_currentThought = "Hey, gentle pause please. I like you, but I need a little space right now.";
        } else if (aff < 0.25f) {
            m_currentThought = "Oh! We're still getting to know each other... but that was kind of sweet.";
        } else if (aff < 0.50f) {
            m_currentThought = "H-hey! A warning next time before headpats! ...Thank you though 🌸";
        } else if (aff < 0.85f) {
            m_currentThought = "Hehe, that tickles! Your headpats always make my day brighter ❤️";
        } else {
            m_currentThought = "Mmm... I love your headpats so much. Stay close to me, okay? 💕";
        }
        m_actionTimer = 2.0f;
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
    if (m_manualSleep && memory.GetNeeds().energy < 0.95f) {
        memory.Rest(deltaTime, 0.06f);
        BrainDecision d;
        d.action = PetAction::Sleeping;
        d.animState = AnimationState::Sleep;
        d.facingLeft = m_facingLeft;
        m_currentThought = d.thought = system.IsNight()
            ? "It's late... please get some rest too, sweetheart. Sweet dreams ❤️"
            : "Zzz... taking a cozy little beauty nap by your taskbar.";
        return d;
    }
    m_manualSleep = false;

    if (m_scoreTimer <= 0.0f) {
        m_lastScores = CalculateUtilityScores(memory, mouse, system, physics);
        m_scoreTimer = m_tickInterval;
    }

    // Study mode priority
    if (m_studyModeTimer > 0.0f) {
        m_currentAction = PetAction::StudyMode;
    } else {
        // Pick motivation with highest score
        float maxScore = m_lastScores.idleScore;
        PetAction chosenAction = PetAction::Idle;

        if (m_lastScores.eatScore > maxScore && toy.active && toy.isTreat) {
            maxScore = m_lastScores.eatScore;
            chosenAction = PetAction::EatingTreat;
        }
        if (m_lastScores.toyScore > maxScore && toy.active && !toy.isTreat) {
            maxScore = m_lastScores.toyScore;
            chosenAction = PetAction::ChasingToy;
        }
        if (m_lastScores.sleepScore > maxScore && m_lastScores.sleepScore > 0.52f) {
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

        if (m_actionTimer <= 0.0f || chosenAction == PetAction::EatingTreat || chosenAction == PetAction::ChasingToy) {
            m_currentAction = chosenAction;
            if (m_currentAction == PetAction::Wandering || m_currentAction == PetAction::Idle) {
                m_actionTimer = m_actionDuration;
            }
        }
    }

    BrainDecision decision;
    decision.action = m_currentAction;

    // 3. Execute Selected Behavioral Action
    switch (m_currentAction) {
        case PetAction::StudyMode: {
            decision.animState = AnimationState::Idle;
            decision.targetHorizontalSpeed = 0.0f;
            memory.StudyTogether(deltaTime);
            m_currentThought = "Study session in progress! You're doing amazing, remember to hydrate 📖💧";
            break;
        }

        case PetAction::EatingTreat: {
            float dx = toy.x - (petPos.x + physics.GetWidth() / 2);
            m_facingLeft = (dx < 0.0f);
            m_currentThought = "Ooh, is that coffee for me? Coming right over! ☕✨";

            if (std::abs(dx) > 18.0f || toy.y + toy.radius < petPos.y || toy.y - toy.radius > petPos.y + physics.GetHeight()) {
                decision.animState = AnimationState::Walk;
                decision.targetHorizontalSpeed = m_facingLeft ? -100.0f : 100.0f;
            } else {
                // Reached coffee/boba! Enjoy break together
                decision.animState = AnimationState::Reaction;
                decision.targetHorizontalSpeed = 0.0f;
                memory.ShareCoffee(0.35f);
                physics.ClearToy();
                m_currentThought = "Mmm, delicious iced boba! Thank you for the treat, you're the sweetest 💕";
                m_actionTimer = 2.4f;
                m_currentAction = PetAction::ReactingToClick;
            }
            break;
        }

        case PetAction::ChasingToy: {
            float dx = toy.x - (petPos.x + physics.GetWidth() / 2);
            m_facingLeft = (dx < 0.0f);
            m_currentThought = "Tossing the plushie heart back to you! 🧸";

            if (std::abs(dx) > 22.0f) {
                float speed = (std::abs(dx) > 80.0f) ? 150.0f : 90.0f;
                decision.animState = (speed > 100.0f) ? AnimationState::Run : AnimationState::Walk;
                decision.targetHorizontalSpeed = m_facingLeft ? -speed : speed;
            } else if (toy.y + toy.radius >= petPos.y && toy.y - toy.radius <= petPos.y + physics.GetHeight() && m_contactCooldown <= 0.0f) {
                m_contactCooldown = 0.5f;
                decision.animState = AnimationState::Reaction;
                decision.targetHorizontalSpeed = 0.0f;
                decision.wantJump = true;
                toy.vx = m_facingLeft ? -340.0f : 340.0f;
                toy.vy = -260.0f;
                memory.TossPlushie(0.18f);
                m_currentThought = "Caught the plushie heart! Hehe, playing with you is so much fun 💕";
            }
            break;
        }

        case PetAction::Sleeping:
            decision.animState = AnimationState::Sleep;
            decision.targetHorizontalSpeed = 0.0f;
            m_currentThought = system.IsNight()
                ? "Past bedtime... please don't stay up too late for me. Let's rest soon ❤️"
                : "Zzz... resting softly right beside your taskbar.";
            memory.Rest(deltaTime, 0.06f);
            if (memory.GetNeeds().energy >= 0.95f && !system.IsNight()) {
                m_currentAction = PetAction::Idle;
            }
            break;

        case PetAction::FollowingCursor: {
            float dx = static_cast<float>(cursor.x - (petPos.x + physics.GetWidth() / 2));
            m_currentThought = "Watching your cursor dance around... you look so cool when you code!";
            if (std::abs(dx) > 25.0f) {
                m_facingLeft = (dx < 0.0f);
                float speed = (std::abs(dx) > 100.0f) ? 135.0f : 80.0f;
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
            m_currentThought = "Taking a little stroll along your window frames~";
            if (std::abs(dx) > 12.0f) {
                m_facingLeft = (dx < 0.0f);
                decision.animState = AnimationState::Walk;
                decision.targetHorizontalSpeed = m_facingLeft ? -60.0f : 60.0f;
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
            if (system.IsUserIdle()) {
                m_currentThought = "Did you step away? I'll guard your desktop until you get back! 🌸";
            } else {
                m_currentThought = "Sitting comfortably by your side, watching you work.";
            }
            break;
    }

    decision.facingLeft = m_facingLeft;
    decision.thought = m_currentThought;
    return decision;
}

} // namespace VirtualPet
