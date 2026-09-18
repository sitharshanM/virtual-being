#include "PetBrain.h"
#include "DesktopWatcher.h"

#include <cmath>
#include <random>
#include <ctime>
#include <algorithm>
#include <cctype>

namespace VirtualPet {

PetBrain::PetBrain() {
    BuildBehaviorTree();
}

void PetBrain::RequestAction(PetAction action, float duration) {
    m_currentAction = action;
    m_actionTimer = duration;
    if (action == PetAction::ReactingToClick) {
        m_currentThought = "Blushing happily from your warm touch ❤️";
    } else if (action == PetAction::ClimbingWindow) {
        m_climbTimer = duration;
    } else if (action == PetAction::Daydreaming) {
        m_currentThought = "Out of your focus for now~ daydreaming quietly by your side 💕💭";
    }
}

void PetBrain::RequestReaction(std::string thought, float duration) {
    m_currentAction = PetAction::ReactingToClick;
    m_actionTimer = duration;
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

std::string PetBrain::GenerateAppThought(const std::string& title, const std::string& appClass, bool isOnWindow) {
    std::string titleLower = title;
    std::string classLower = appClass;
    std::transform(titleLower.begin(), titleLower.end(), titleLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::transform(classLower.begin(), classLower.end(), classLower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    bool isCoding = (classLower.find("code") != std::string::npos || classLower.find("vscodium") != std::string::npos ||
                     classLower.find("antigravity") != std::string::npos || classLower.find("sublime") != std::string::npos ||
                     classLower.find("clion") != std::string::npos || classLower.find("pycharm") != std::string::npos ||
                     classLower.find("intellij") != std::string::npos || classLower.find("neovim") != std::string::npos ||
                     classLower.find("vim") != std::string::npos || classLower.find("emacs") != std::string::npos ||
                     classLower.find("geany") != std::string::npos || classLower.find("zed") != std::string::npos ||
                     titleLower.find("code") != std::string::npos || titleLower.find("studio") != std::string::npos ||
                     titleLower.find("antigravity") != std::string::npos || titleLower.find(".cpp") != std::string::npos ||
                     titleLower.find(".h") != std::string::npos || titleLower.find(".py") != std::string::npos ||
                     titleLower.find(".js") != std::string::npos || titleLower.find(".ts") != std::string::npos ||
                     titleLower.find(".rs") != std::string::npos || titleLower.find(".go") != std::string::npos ||
                     titleLower.find(".json") != std::string::npos || titleLower.find(".md") != std::string::npos ||
                     titleLower.find("git") != std::string::npos);

    bool isTerminal = (!isCoding && (classLower.find("kitty") != std::string::npos || classLower.find("alacritty") != std::string::npos ||
                       classLower.find("terminal") != std::string::npos || classLower.find("wezterm") != std::string::npos ||
                       classLower.find("foot") != std::string::npos || classLower.find("konsole") != std::string::npos ||
                       classLower.find("xterm") != std::string::npos || classLower.find("urxvt") != std::string::npos ||
                       classLower.find("cascadia") != std::string::npos || classLower.find("consolewindowclass") != std::string::npos ||
                       classLower.find("mintty") != std::string::npos || classLower.find("powershell") != std::string::npos ||
                       titleLower.find("terminal") != std::string::npos || titleLower.find("bash") != std::string::npos ||
                       titleLower.find("powershell") != std::string::npos || titleLower.find("cmd.exe") != std::string::npos ||
                       titleLower.find("command prompt") != std::string::npos || titleLower.find("git bash") != std::string::npos ||
                       titleLower.find("zsh") != std::string::npos || titleLower.find("fish") != std::string::npos ||
                       titleLower.find("sh") != std::string::npos || titleLower == "~" ||
                       titleLower.find("hemanth@") != std::string::npos || titleLower.find("[run.sh]") != std::string::npos));

    bool isBrowser = (!isCoding && !isTerminal && (classLower.find("zen") != std::string::npos || classLower.find("firefox") != std::string::npos ||
                      classLower.find("chrome") != std::string::npos || classLower.find("chromium") != std::string::npos ||
                      classLower.find("brave") != std::string::npos || classLower.find("edge") != std::string::npos ||
                      titleLower.find("browser") != std::string::npos || titleLower.find("http") != std::string::npos ||
                      titleLower.find("github") != std::string::npos || titleLower.find("google") != std::string::npos ||
                      titleLower.find("youtube") != std::string::npos || titleLower.find("stackoverflow") != std::string::npos));

    bool isMusic = (!isCoding && !isTerminal && !isBrowser && (classLower.find("spotify") != std::string::npos || classLower.find("music") != std::string::npos ||
                    titleLower.find("spotify") != std::string::npos || titleLower.find("music") != std::string::npos));

    bool isGaming = (!isCoding && !isTerminal && !isBrowser && (classLower.find("steam") != std::string::npos || classLower.find("game") != std::string::npos ||
                     titleLower.find("steam") != std::string::npos));

    bool isChat = (!isCoding && !isTerminal && !isBrowser && !isGaming &&
                   (classLower.find("discord") != std::string::npos || classLower.find("telegram") != std::string::npos ||
                    classLower.find("slack") != std::string::npos || classLower.find("teams") != std::string::npos ||
                    classLower.find("whatsapp") != std::string::npos || classLower.find("signal") != std::string::npos ||
                    titleLower.find("discord") != std::string::npos || titleLower.find("telegram") != std::string::npos ||
                    titleLower.find("slack") != std::string::npos || titleLower.find("chat") != std::string::npos));

    if (isCoding) {
        static const std::vector<std::string> kCodingThoughts = {
            "Watching you code~ I'll be your rubber duck! 🦆✨",
            "Tap-tap-tap... love hearing your keyboard rhythm! ⌨️💕",
            "Deep in flow state! Don't forget to take a sip of water~ 💧",
            "Look at those clean functions! You're making awesome progress ✨",
            "Squashing bugs like a pro! I believe in your code 🌸",
            "Ooh, clever logic! What does this part do? 💡",
            "Remember to save and commit your work~ I'm right here cheering! 💾",
            "Coding together is my favorite part of the day ❤️"
        };
        return kCodingThoughts[(m_thoughtCycle++) % kCodingThoughts.size()];
    }

    if (isTerminal) {
        static const std::vector<std::string> kTerminalThoughts = {
            "Terminal magic! What cool command are you running? ✨",
            "Fast terminal typing! Hacker mode activated~ 💻⚡",
            "Compiling and running... fingers crossed for 0 errors! 🤞",
            "Command line wizard at work! Love seeing you in your element 🧙‍♂️",
            "Watching your shell commands fly by... so impressive! 🌸",
            "Tap-tap on the CLI! You make this look so easy 💕"
        };
        return kTerminalThoughts[(m_thoughtCycle++) % kTerminalThoughts.size()];
    }

    if (isBrowser) {
        static const std::vector<std::string> kBrowserThoughts = {
            "Internet surfing! What interesting things are you researching? 🌊",
            "Looking for solutions? You always find the best answers! 🔍",
            "Ooh, interesting page! Let me peek over your shoulder too~ 👀",
            "Learning and exploring together is so much fun 📖✨",
            "Don't get lost in too many open tabs, okay? Hehe~ 📑"
        };
        return kBrowserThoughts[(m_thoughtCycle++) % kBrowserThoughts.size()];
    }

    if (isMusic) {
        static const std::vector<std::string> kMusicThoughts = {
            "I can feel the rhythm from here! Great music taste 🎵✨",
            "Vibing to the playlist with you~ 🎶❤️",
            "Good tunes make any work session feel like a breeze! 🎧"
        };
        return kMusicThoughts[(m_thoughtCycle++) % kMusicThoughts.size()];
    }

    if (isGaming) {
        static const std::vector<std::string> kGamingThoughts = {
            "Gaming time?! Let's go! I'll cheer you on to victory~ 🎮✨",
            "Epic moves! Show them what you've got! 🏆",
            "Rooting for you every match! You've got this 💕"
        };
        return kGamingThoughts[(m_thoughtCycle++) % kGamingThoughts.size()];
    }

    if (isChat) {
        static const std::vector<std::string> kChatThoughts = {
            "Chatting with friends? Say hi for me too! 💕",
            "Typing fast messages! Keep spreading good vibes ✨",
            "Ooh, lively conversation! I'm right here keeping you company 🌸"
        };
        return kChatThoughts[(m_thoughtCycle++) % kChatThoughts.size()];
    }

    if (isOnWindow) {
        static const std::vector<std::string> kWindowThoughts = {
            "Perched right on your window! Keeping you company from up high 🌸",
            "Sitting cozy right here above your work 💕",
            "Enjoying the view from up on your window frame~ ✨"
        };
        return kWindowThoughts[(m_thoughtCycle++) % kWindowThoughts.size()];
    }

    static const std::vector<std::string> kGeneralThoughts = {
        "Watching you work focused and hard... you inspire me! 🌸",
        "Sitting comfortably by your side, cheering you on 💕",
        "You're doing great today! Remember I'm always cheering for you ✨"
    };
    return kGeneralThoughts[(m_thoughtCycle++) % kGeneralThoughts.size()];
}

UtilityScores PetBrain::CalculateUtilityScores(const Memory& memory,
                                               const MouseSensor& mouse,
                                               const SystemSensor& system,
                                               const Physics& physics,
                                               const DesktopWatcher* watcher,
                                               const DesktopWorld* world) const {
    UtilityScores u;
    const auto& needs = memory.GetNeeds();
    const auto& p = memory.GetPersonality();
    const auto& toy = physics.GetToy();
    (void)system;

    // 1. Coffee / Boba Break Utility
    if (toy.active && toy.isTreat) {
        u.eatScore = std::pow(needs.GetCoffeeCraving(), 1.1f) * 2.4f + 0.35f;
    } else {
        u.eatScore = std::pow(needs.GetCoffeeCraving(), 1.4f) * 0.55f;
    }

    // 2. Plushie Play Utility
    if (toy.active && !toy.isTreat) {
        u.toyScore = (p.playfulness * 1.7f + 0.3f) * (0.5f + needs.energy * 0.5f);
    } else {
        u.toyScore = 0.0f;
    }

    // 3. Cursor Follow / Peeking at what you're doing
    // If FollowingCursor action timer has expired, zero out followScore so companion can daydream or wander.
    if (mouse.IsNear() && !toy.active && !(m_currentAction == PetAction::FollowingCursor && m_actionTimer <= 0.0f)) {
        u.followScore = (p.playfulness * 0.6f + p.sweetness * 0.5f) * (0.4f + needs.trust * 0.6f);
    } else {
        u.followScore = 0.0f;
    }

    // 4. Desktop Stroll / Window Exploration
    bool hasClimbableWindows = watcher && world && watcher->HasClimbableSurfaces(*world, physics.GetHeight());
    if (watcher && (watcher->IsFloorOnly() || !hasClimbableWindows)) {
        // Floor stroll mode: enhanced wandering along the screen bottom
        u.wanderScore = p.curiosity * 0.85f * (0.4f + needs.energy * 0.6f);
        u.windowScore = 0.0f;
    } else {
        u.wanderScore = p.curiosity * 0.55f * (0.4f + needs.energy * 0.6f);

        // 5. Window Exploration Utility
        // Only seek window climb if there is at least one open window with headroom to safely climb onto!
        // Prevents endless attempts to climb off-screen on single maximized/top-docked windows.
        if (watcher && !watcher->GetCurrentSurface() && hasClimbableWindows) {
            u.windowScore = p.curiosity * 0.72f * (0.4f + needs.energy * 0.6f);
        } else {
            u.windowScore = 0.0f;
        }
    }

    // 6. Study / Focus Mode
    if (m_studyModeTimer > 0.0f) {
        u.studyScore = 2.5f;
    } else {
        u.studyScore = 0.0f;
    }

    // 7. Companion Idle (quietly sitting by your windows)
    u.idleScore = 0.25f + (p.laziness * 0.35f);

    // 8. Daydreaming / Out-of-focus contemplation
    if (!mouse.IsNear() && !toy.active && m_studyModeTimer <= 0.0f) {
        u.daydreamScore = 0.28f + (p.curiosity * 0.22f);
    } else {
        u.daydreamScore = 0.0f;
    }

    return u;
}

BrainDecision PetBrain::Update(float deltaTime,
                               const MouseSensor& mouse,
                               const SystemSensor& system,
                               Memory& memory,
                               const DesktopWorld& world,
                               Physics& physics,
                               DesktopWatcher* watcher) {
    m_actionTimer -= deltaTime;
    m_scoreTimer -= deltaTime;
    m_contactCooldown -= deltaTime;
    m_thoughtTimer += deltaTime;
    if (m_studyModeTimer > 0.0f) {
        m_studyModeTimer -= deltaTime;
    }

    const Point petPos = physics.GetPosition();
    const Point cursor = mouse.GetCursorPosition();
    ToyItem& toy = physics.GetToy();

    // 0. Active Window & Activity Reaction Context
    const WindowSurface& activeWin = world.GetActiveWindow();
    std::string currentContextTitle = (m_currentAction == PetAction::SittingOnWindow && m_hasTargetSurface)
                                      ? m_targetSurface.title : activeWin.title;
    std::string currentContextClass = (m_currentAction == PetAction::SittingOnWindow && m_hasTargetSurface)
                                      ? m_targetSurface.appClass : activeWin.appClass;

    // React immediately if user switches active window or file in editor/terminal!
    if (!currentContextTitle.empty() && currentContextTitle != m_lastActiveTitle) {
        m_lastActiveTitle = currentContextTitle;
        m_thoughtTimer = 0.0f;
        if (m_currentAction == PetAction::SittingOnWindow || m_currentAction == PetAction::Idle || m_currentAction == PetAction::Wandering) {
            m_currentThought = GenerateAppThought(currentContextTitle, currentContextClass, (m_currentAction == PetAction::SittingOnWindow));
        }
    } else if (m_thoughtTimer >= 9.0f) {
        // Rotate thoughts dynamically every ~9s so Astra stays lively and supportive
        m_thoughtTimer = 0.0f;
        if (m_currentAction == PetAction::SittingOnWindow || m_currentAction == PetAction::Idle || m_currentAction == PetAction::Wandering) {
            if (!currentContextTitle.empty() || !currentContextClass.empty()) {
                m_currentThought = GenerateAppThought(currentContextTitle, currentContextClass, (m_currentAction == PetAction::SittingOnWindow));
            }
        }
    }

    // 1. Right-Click Context Menu Open: freeze in place attentively
    if (m_isMenuOpen) {
        BrainDecision d;
        d.action = (m_currentAction == PetAction::SittingOnWindow) ? PetAction::SittingOnWindow : PetAction::Idle;
        d.animState = (m_currentAction == PetAction::SittingOnWindow) ? AnimationState::SitHang : AnimationState::Idle;
        d.targetHorizontalSpeed = 0.0f;
        d.facingLeft = m_facingLeft;
        d.thought = "I'm all ears! What would you like to do? 💕";
        return d;
    }

    // 2. High Priority Direct Sensory Overrides: Dragging
    if (physics.IsDragged()) {
        m_currentAction = PetAction::Dragged;
        m_currentThought = "Kyaaa! Where are you taking me?! Put me down gently, okay? 💕";
        m_hasTargetSurface = false;
        BrainDecision d;
        d.action = PetAction::Dragged;
        d.animState = AnimationState::Reaction;
        d.targetHorizontalSpeed = 0.0f;
        d.facingLeft = m_facingLeft;
        d.thought = m_currentThought;
        return d;
    } else if (m_currentAction == PetAction::Dragged) {
        // Just released from drag/throw!
        if (!physics.IsGrounded()) {
            // Pet is airborne (thrown or dropped from height)
            m_currentAction = PetAction::Falling;
            m_actionTimer = 0.0f;
            if (std::abs(physics.GetVelocityX()) > 220.0f || std::abs(physics.GetVelocityY()) > 220.0f) {
                m_currentThought = "Wheee~! Flying through the air! 💨✨";
            } else {
                m_currentThought = "Falling gently down~! 🐾";
            }
            BrainDecision d;
            d.action = PetAction::Falling;
            d.animState = AnimationState::Fall;
            d.targetHorizontalSpeed = 0.0f;
            d.facingLeft = (physics.GetVelocityX() < 0.0f);
            d.thought = m_currentThought;
            return d;
        } else {
            // Placed directly onto ground
            m_currentAction = PetAction::Idle;
            m_actionTimer = 0.3f;
            m_currentThought = "Placed back down gently~ thank you 💕";
        }
    }

    // 2. Direct Sensory Overrides: Headpat / Click
    if (mouse.WasLeftButtonClicked()) {
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

    // 3. Desktop Surface Dynamics (Gone or Moved under pet)
    if (watcher) {
        if (watcher->SurfaceJustGone()) {
            m_currentAction = PetAction::Falling;
            m_actionTimer = 1.5f;
            m_currentThought = "Kyaaa! The window vanished?! Falling down~! 💨";
            m_hasTargetSurface = false;
            BrainDecision d;
            d.action = PetAction::Falling;
            d.animState = AnimationState::Fall;
            d.targetHorizontalSpeed = 0.0f;
            d.facingLeft = m_facingLeft;
            d.thought = m_currentThought;
            return d;
        }

        if (watcher->SurfaceJustMoved()) {
            Point delta = watcher->GetSurfaceMoveDelta();
            physics.SetPosition(physics.GetX() + static_cast<float>(delta.x),
                                physics.GetY() + static_cast<float>(delta.y));
            m_currentAction = PetAction::ReactingToMove;
            m_actionTimer = 0.8f;
            m_currentThought = "Whoa! The window moved! Hold on tight~ 🐾";
            BrainDecision d;
            d.action = PetAction::ReactingToMove;
            d.animState = AnimationState::Reaction;
            d.targetHorizontalSpeed = 0.0f;
            d.facingLeft = m_facingLeft;
            d.thought = m_currentThought;
            return d;
        }
    }

    // 4. Handle Ongoing Transient Actions (Move reaction, Falling, Climbing, Sitting)
    if (m_currentAction == PetAction::ReactingToMove && m_actionTimer > 0.0f) {
        BrainDecision d;
        d.action = PetAction::ReactingToMove;
        d.animState = AnimationState::Reaction;
        d.targetHorizontalSpeed = 0.0f;
        d.facingLeft = m_facingLeft;
        d.thought = m_currentThought;
        return d;
    }

    if (m_currentAction == PetAction::Falling) {
        if (physics.IsGrounded()) {
            m_currentAction = PetAction::Idle;
            m_actionTimer = 0.5f;
            m_currentThought = "Oof! Safe landing on my feet! 🌸";
            BrainDecision d;
            d.action = PetAction::Idle;
            d.animState = AnimationState::Land;
            d.targetHorizontalSpeed = 0.0f;
            d.facingLeft = m_facingLeft;
            d.thought = m_currentThought;
            return d;
        } else {
            BrainDecision d;
            d.action = PetAction::Falling;
            d.animState = AnimationState::Fall;
            d.targetHorizontalSpeed = 0.0f;
            d.facingLeft = m_facingLeft;
            d.thought = m_currentThought;
            return d;
        }
    }

    if (m_currentAction == PetAction::ClimbingWindow) {
        m_climbTimer -= deltaTime;
        if (m_climbTimer <= 0.0f) {
            // Finished climbing animation: place pet securely on top of window surface
            if (m_hasTargetSurface) {
                Rect workArea = world.GetPrimaryWorkArea();
                int32_t petW = physics.GetWidth();
                int32_t petH = physics.GetHeight();

                int32_t minX = m_targetSurface.bounds.x + 15;
                int32_t maxX = m_targetSurface.bounds.x + m_targetSurface.bounds.width - petW - 15;
                if (maxX < minX) maxX = minX;

                int32_t targetX = std::clamp(petPos.x, minX, maxX);
                targetX = std::clamp(targetX, workArea.x + 10, workArea.x + workArea.width - petW - 10);

                int32_t targetY = m_targetSurface.bounds.y - petH;
                physics.SetPosition(static_cast<float>(targetX), static_cast<float>(targetY));
                physics.SetVelocity(0.0f, 0.0f);
            }
            m_currentAction = PetAction::SittingOnWindow;
            m_sitTimer = 8.0f + (rand() % 8);
            m_actionTimer = m_sitTimer;
            m_thoughtTimer = 0.0f;
            m_currentThought = GenerateAppThought(m_targetSurface.title, m_targetSurface.appClass, true);

            BrainDecision d;
            d.action = PetAction::SittingOnWindow;
            d.animState = AnimationState::SitHang;
            d.targetHorizontalSpeed = 0.0f;
            d.facingLeft = m_facingLeft;
            d.thought = m_currentThought;
            return d;
        } else {
            BrainDecision d;
            d.action = PetAction::ClimbingWindow;
            d.animState = AnimationState::ClimbUp;
            d.targetHorizontalSpeed = 0.0f;
            d.facingLeft = m_facingLeft;
            d.thought = "Climbing up to your window top! 🐾";
            return d;
        }
    }

    if (m_currentAction == PetAction::SittingOnWindow) {
        m_sitTimer -= deltaTime;
        if (m_sitTimer <= 0.0f) {
            m_currentAction = PetAction::Idle;
            m_actionTimer = 2.5f;
            m_hasTargetSurface = false;
        } else {
            BrainDecision d;
            d.action = PetAction::SittingOnWindow;
            d.animState = AnimationState::SitHang;
            d.targetHorizontalSpeed = 0.0f;
            d.facingLeft = m_facingLeft;
            d.thought = m_currentThought;
            return d;
        }
    }

    if (m_currentAction == PetAction::WalkingToWindow) {
        if (!m_hasTargetSurface || !watcher || !watcher->HasClimbableSurfaces(world, physics.GetHeight())) {
            m_currentAction = PetAction::Idle;
            m_actionTimer = 2.0f;
            m_hasTargetSurface = false;
        } else {
            int32_t petW = physics.GetWidth();
            Rect workArea = world.GetPrimaryWorkArea();
            int32_t minX = m_targetSurface.bounds.x + 15;
            int32_t maxX = m_targetSurface.bounds.x + m_targetSurface.bounds.width - petW - 15;
            if (maxX < minX) maxX = minX;
            int32_t targetX = std::clamp(petPos.x, minX, maxX);
            targetX = std::clamp(targetX, workArea.x + 10, workArea.x + workArea.width - petW - 10);

            float dx = static_cast<float>(targetX - petPos.x);
            m_facingLeft = (dx < 0.0f);
            m_currentThought = "Heading over to climb onto your window frame~ 🐾";

            if (std::abs(dx) <= 20.0f) {
                m_currentAction = PetAction::ClimbingWindow;
                m_climbTimer = 0.8f;
                BrainDecision d;
                d.action = PetAction::ClimbingWindow;
                d.animState = AnimationState::ClimbUp;
                d.targetHorizontalSpeed = 0.0f;
                d.facingLeft = m_facingLeft;
                d.thought = "Climbing up to your window top! 🐾";
                return d;
            } else {
                BrainDecision d;
                d.action = PetAction::WalkingToWindow;
                d.animState = AnimationState::Walk;
                d.targetHorizontalSpeed = m_facingLeft ? -75.0f : 75.0f;
                d.facingLeft = m_facingLeft;
                d.thought = m_currentThought;
                return d;
            }
        }
    }

    // 4b. Cursor Follow Transition: If FollowingCursor timer expires or cursor leaves, enter Daydreaming
    if (m_currentAction == PetAction::FollowingCursor && (m_actionTimer <= 0.0f || !mouse.IsNear())) {
        m_currentAction = PetAction::Daydreaming;
        m_actionTimer = 3.5f + (rand() % 25) / 10.0f;
        m_currentThought = "Out of your focus for now~ daydreaming quietly by your side 💕💭";
    }

    // 5. Evaluate Utility Curves for autonomous behavior
    if (m_scoreTimer <= 0.0f) {
        m_lastScores = CalculateUtilityScores(memory, mouse, system, physics, watcher, &world);
        m_scoreTimer = m_tickInterval;
    }

    // Study mode priority, but user toys and treats can pause or interrupt study for a fun break!
    if (m_studyModeTimer > 0.0f && !toy.active) {
        m_currentAction = PetAction::StudyMode;
    } else {
        // Pick motivation with highest score
        float maxScore = m_lastScores.idleScore;
        PetAction chosenAction = (m_studyModeTimer > 0.0f) ? PetAction::StudyMode : PetAction::Idle;

        if (m_lastScores.eatScore > maxScore && toy.active && toy.isTreat) {
            maxScore = m_lastScores.eatScore;
            chosenAction = PetAction::EatingTreat;
        }
        if (m_lastScores.toyScore > maxScore && toy.active && !toy.isTreat) {
            maxScore = m_lastScores.toyScore;
            chosenAction = PetAction::ChasingToy;
        }
        if (m_studyModeTimer <= 0.0f) {
            if (m_lastScores.followScore > maxScore && m_lastScores.followScore > 0.35f) {
                maxScore = m_lastScores.followScore;
                chosenAction = PetAction::FollowingCursor;
            }
            if (m_lastScores.windowScore > maxScore && watcher && watcher->HasClimbableSurfaces(world, physics.GetHeight())) {
                const WindowSurface* nearest = watcher->GetNearestSurface(petPos, world, physics.GetHeight());
                if (nearest) {
                    maxScore = m_lastScores.windowScore;
                    chosenAction = PetAction::WalkingToWindow;
                    m_targetSurface = *nearest;
                    m_hasTargetSurface = true;
                }
            }
            if (m_lastScores.wanderScore > maxScore && m_actionTimer <= 0.0f) {
                maxScore = m_lastScores.wanderScore;
                chosenAction = PetAction::Wandering;
            }
            if (m_lastScores.daydreamScore > maxScore && m_actionTimer <= 0.0f) {
                maxScore = m_lastScores.daydreamScore;
                chosenAction = PetAction::Daydreaming;
            }
        }

        if (m_actionTimer <= 0.0f || chosenAction == PetAction::EatingTreat || chosenAction == PetAction::ChasingToy || chosenAction == PetAction::WalkingToWindow) {
            m_currentAction = chosenAction;
            if (m_currentAction == PetAction::Wandering || m_currentAction == PetAction::Idle) {
                float baseDuration = (m_actionDuration > 0.0f && m_actionDuration <= 8.0f) ? m_actionDuration : 4.0f;
                m_actionTimer = baseDuration * (0.75f + (rand() % 50) / 100.0f);
            } else if (m_currentAction == PetAction::FollowingCursor) {
                m_actionTimer = 4.0f + (rand() % 25) / 10.0f;
            } else if (m_currentAction == PetAction::Daydreaming) {
                m_actionTimer = 3.5f + (rand() % 25) / 10.0f;
            }
        }
    }

    BrainDecision decision;
    decision.action = m_currentAction;

    // 6. Execute Selected Behavioral Action
    switch (m_currentAction) {
        case PetAction::StudyMode: {
            if (m_studyModeTimer <= 0.0f) {
                m_currentAction = PetAction::Idle;
                m_actionTimer = 2.0f;
                decision.animState = AnimationState::Idle;
                decision.targetHorizontalSpeed = 0.0f;
                m_currentThought = "Study session complete! So proud of your hard work 💕✨";
                break;
            }
            decision.animState = AnimationState::Idle;
            decision.targetHorizontalSpeed = 0.0f;
            memory.StudyTogether(deltaTime);
            m_currentThought = "Study session in progress! You're doing amazing, remember to hydrate 📖💧";
            break;
        }

        case PetAction::EatingTreat: {
            if (!toy.active) {
                m_currentAction = PetAction::Idle;
                m_actionTimer = 1.0f;
                decision.animState = AnimationState::Idle;
                decision.targetHorizontalSpeed = 0.0f;
                break;
            }
            float dx = toy.x - (petPos.x + physics.GetWidth() / 2);
            m_facingLeft = (dx < 0.0f);
            m_currentThought = "Ooh, is that coffee for me? Coming right over! ☕✨";

            if (std::abs(dx) > 18.0f || toy.y + toy.radius < petPos.y || toy.y - toy.radius > petPos.y + physics.GetHeight()) {
                decision.animState = AnimationState::Walk;
                decision.targetHorizontalSpeed = m_facingLeft ? -100.0f : 100.0f;
            } else {
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
            if (!toy.active) {
                m_currentAction = PetAction::Idle;
                m_actionTimer = 1.0f;
                decision.animState = AnimationState::Idle;
                decision.targetHorizontalSpeed = 0.0f;
                break;
            }
            float dx = toy.x - (petPos.x + physics.GetWidth() / 2);
            m_facingLeft = (dx < 0.0f);
            m_currentThought = "Tossing the plushie heart back to you! 🧸";

            if (std::abs(dx) > 22.0f) {
                float speed = (std::abs(dx) > 80.0f) ? 150.0f : 90.0f;
                decision.animState = (speed > 100.0f) ? AnimationState::Run : AnimationState::Walk;
                decision.targetHorizontalSpeed = m_facingLeft ? -speed : speed;
            } else if (toy.y + toy.radius >= petPos.y && toy.y - toy.radius <= petPos.y + physics.GetHeight() && m_contactCooldown <= 0.0f) {
                m_contactCooldown = 0.6f;
                toy.catchesRemaining--;

                if (toy.catchesRemaining <= 0) {
                    // Final catch: companion catches, hugs, and pockets the plushie heart!
                    decision.animState = AnimationState::Reaction;
                    decision.targetHorizontalSpeed = 0.0f;
                    memory.TossPlushie(0.25f);
                    physics.ClearToy();
                    m_currentThought = "Caught it! I'll keep this plushie heart safe with me, thank you! 💕🧸";
                    m_actionTimer = 2.5f;
                    m_currentAction = PetAction::ReactingToClick;
                } else {
                    // Playful rally: bounce it back towards open screen space
                    decision.animState = AnimationState::Reaction;
                    decision.targetHorizontalSpeed = 0.0f;
                    decision.wantJump = true;

                    Rect workArea = world.GetPrimaryWorkArea();
                    float centerX = static_cast<float>(workArea.x + workArea.width / 2);
                    bool tossLeft = (toy.x > centerX) || (toy.x > static_cast<float>(workArea.x + workArea.width - 250));
                    if (toy.x < static_cast<float>(workArea.x + 250)) {
                        tossLeft = false;
                    }

                    toy.vx = tossLeft ? -300.0f : 300.0f;
                    toy.vy = -260.0f;
                    memory.TossPlushie(0.12f);
                    m_currentThought = "Caught the plushie heart! Bouncing it back to you, catch! 🧸✨";
                }
            }
            break;
        }

        case PetAction::FollowingCursor: {
            if (m_actionTimer <= 0.0f || !mouse.IsNear()) {
                m_currentAction = PetAction::Daydreaming;
                m_actionTimer = 3.5f + (rand() % 25) / 10.0f;
                decision.action = PetAction::Daydreaming;
                decision.animState = AnimationState::LookAround;
                decision.targetHorizontalSpeed = 0.0f;
                m_currentThought = "Out of your focus for now~ daydreaming quietly by your side 💕💭";
                break;
            }
            float dx = static_cast<float>(cursor.x - (petPos.x + physics.GetWidth() / 2));
            m_currentThought = "Watching your cursor dance around... you look so cool when you code!";
            if (std::abs(dx) > 25.0f) {
                m_facingLeft = (dx < 0.0f);
                float speed = (std::abs(dx) > 100.0f) ? 135.0f : 80.0f;
                decision.animState = (speed > 100.0f) ? AnimationState::Run : AnimationState::Walk;
                decision.targetHorizontalSpeed = m_facingLeft ? -speed : speed;
            } else {
                decision.animState = AnimationState::LookAround;
                decision.targetHorizontalSpeed = 0.0f;
                m_currentThought = "Peeking right at what you're doing... fascinating!";
            }
            break;
        }

        case PetAction::Daydreaming: {
            if (m_actionTimer <= 0.0f) {
                m_currentAction = PetAction::Wandering;
                float baseDuration = (m_actionDuration > 0.0f && m_actionDuration <= 8.0f) ? m_actionDuration : 4.0f;
                m_actionTimer = baseDuration * (0.75f + (rand() % 50) / 100.0f);
                decision.action = PetAction::Wandering;
                decision.animState = AnimationState::Walk;
                decision.targetHorizontalSpeed = m_facingLeft ? -60.0f : 60.0f;
                m_currentThought = "Taking a little stroll around your desktop~ 🐾";
                break;
            }
            decision.animState = AnimationState::LookAround;
            decision.targetHorizontalSpeed = 0.0f;
            if (m_thoughtTimer >= 4.0f) {
                m_thoughtTimer = 0.0f;
                static const std::string daydreamThoughts[] = {
                    "Out of your focus for now~ daydreaming about little stars and warm tea ✨🍵",
                    "You're in the zone! I'll daydream quietly so I don't distract you 💕💭",
                    "Daydreaming while you focus... you're working so hard today! 🌸",
                    "Lost in my thoughts for a moment... watching the desktop clouds drift by ☁️",
                    "Doing my own little daydreams while you conquer your tasks! 🐾✨"
                };
                m_currentThought = daydreamThoughts[(++m_thoughtCycle) % 5];
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
            if (!currentContextTitle.empty() || !currentContextClass.empty()) {
                // Active app context thought managed & refreshed periodically
            } else if (watcher && watcher->IsFloorOnly()) {
                m_currentThought = "Exploring your desktop floor! I'm right here by your side 🌸";
            } else {
                m_currentThought = "Taking a little stroll along your window frames~";
            }

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

        case PetAction::WalkingToWindow: {
            decision.animState = AnimationState::Walk;
            decision.targetHorizontalSpeed = m_facingLeft ? -75.0f : 75.0f;
            decision.thought = "Heading over to climb onto your window frame~ 🐾";
            break;
        }

        case PetAction::ClimbingWindow: {
            decision.animState = AnimationState::ClimbUp;
            decision.targetHorizontalSpeed = 0.0f;
            decision.thought = "Climbing up to your window top! 🐾";
            break;
        }

        case PetAction::SittingOnWindow: {
            decision.animState = AnimationState::SitHang;
            decision.targetHorizontalSpeed = 0.0f;
            decision.thought = m_currentThought;
            break;
        }

        case PetAction::Idle:
        default:
            decision.animState = AnimationState::Idle;
            decision.targetHorizontalSpeed = 0.0f;
            if (system.IsUserIdle()) {
                m_currentThought = "Did you step away? I'll guard your desktop until you get back! 🌸";
            } else if (!currentContextTitle.empty() || !currentContextClass.empty()) {
                // Active app context thought managed & refreshed periodically
            } else if (watcher && watcher->IsFloorOnly()) {
                m_currentThought = "Sitting comfortably on your screen floor, cheering you on! 🌸";
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
