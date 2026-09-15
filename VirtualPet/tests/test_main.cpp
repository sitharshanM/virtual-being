#include "../include/BehaviorTree.h"
#include "../include/DesktopWorld.h"
#include "../include/Memory.h"
#include "../include/Physics.h"
#include "../include/SaveManager.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <limits>
#include <chrono>
#include "PetBrain.h"
#include "Pet.h"

using namespace VirtualPet;

int g_testsPassed = 0;
int g_testsFailed = 0;

#define EXPECT(cond, msg) \
    do { \
        if (cond) { \
            g_testsPassed++; \
        } else { \
            g_testsFailed++; \
            std::cerr << "[FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
        } \
    } while(0)

void TestSaveManager() {
    std::cout << "Running TestSaveManager...\n";
    SaveManager sm("test_temp_state.json");

    PetStateData original = sm.GetDefaultState();
    original.name = "TestPet";
    original.needs.energy = 0.42f;
    original.needs.hunger = 0.88f;

    // Test Validation
    EXPECT(sm.Validate(original), "Default state should be valid");

    PetStateData invalid = original;
    invalid.needs.energy = 1.5f; // Out of range
    EXPECT(!sm.Validate(invalid), "Invalid state with energy > 1.0 should fail validation");

    // Test Save & Load
    EXPECT(sm.Save(original), "Save should succeed");
    PetStateData loaded;
    EXPECT(sm.Load(loaded), "Load should succeed");
    EXPECT(loaded.name == "TestPet", "Name should match saved value");
    EXPECT(std::abs(loaded.needs.energy - 0.42f) < 0.05f, "Energy should match saved value");
    EXPECT(std::abs(loaded.needs.hunger - 0.88f) < 0.05f, "Hunger should match saved value");

    // Clean up temporary test file
    std::remove("test_temp_state.json");
}

void TestBehaviorTree() {
    std::cout << "Running TestBehaviorTree...\n";
    BehaviorTree tree;
    Blackboard& bb = tree.GetBlackboard();

    // 1. Test Blackboard
    bb.SetBool("is_hungry", true);
    bb.SetFloat("mood", 0.75f);
    bb.SetString("favorite", "window_top");

    EXPECT(bb.GetBool("is_hungry") == true, "Blackboard bool match");
    EXPECT(std::abs(bb.GetFloat("mood") - 0.75f) < 0.001f, "Blackboard float match");
    EXPECT(bb.GetString("favorite") == "window_top", "Blackboard string match");

    // 2. Test SequenceNode (all must succeed)
    auto seq = std::make_shared<SequenceNode>("TestSeq");
    bool action1Ran = false;
    bool action2Ran = false;

    seq->AddChild(std::make_shared<ActionNode>("A1", [&](float, Blackboard&) {
        action1Ran = true;
        return NodeStatus::Success;
    }));
    seq->AddChild(std::make_shared<ActionNode>("A2", [&](float, Blackboard&) {
        action2Ran = true;
        return NodeStatus::Success;
    }));

    tree.SetRoot(seq);
    NodeStatus status = tree.Tick(0.1f);
    EXPECT(status == NodeStatus::Success, "Sequence should succeed when all children succeed");
    EXPECT(action1Ran && action2Ran, "Both sequence actions should have executed");

    // 3. Test SelectorNode (first success stops)
    auto sel = std::make_shared<SelectorNode>("TestSel");
    bool sel1Ran = false;
    bool sel2Ran = false;

    sel->AddChild(std::make_shared<ActionNode>("S1", [&](float, Blackboard&) {
        sel1Ran = true;
        return NodeStatus::Success;
    }));
    sel->AddChild(std::make_shared<ActionNode>("S2", [&](float, Blackboard&) {
        sel2Ran = true;
        return NodeStatus::Success;
    }));

    tree.SetRoot(sel);
    status = tree.Tick(0.1f);
    EXPECT(status == NodeStatus::Success, "Selector should succeed on first child success");
    EXPECT(sel1Ran, "First selector action ran");
    EXPECT(!sel2Ran, "Second selector action was bypassed");
}

void TestPhysics() {
    std::cout << "Running TestPhysics...\n";
    DesktopWorld world(Rect(0,0,1920,1080), {});
    PhysicsConfig pc;
    pc.gravity = 500.0f;
    pc.groundMargin = 10;
    pc.edgeBounce = 0.4f;

    Physics physics(pc);
    physics.SetDimensions(100, 100);
    physics.SetPosition(200.0f, 100.0f);
    physics.SetVelocity(0.0f, 0.0f);

    // Test falling under gravity
    physics.Update(0.1f, world);
    EXPECT(physics.GetY() > 100.0f, "Y position should increase under gravity");
    EXPECT(physics.GetVelocityY() > 0.0f, "Vertical velocity should increase downwards");

    // Test dragging
    physics.StartDragging(Point(300, 400), Point(50, 50));
    EXPECT(physics.IsDragged(), "Physics state should be dragged");
    physics.Update(0.1f, world);
    physics.StopDragging();
    EXPECT(!physics.IsDragged(), "Physics state should no longer be dragged");
}

void TestMemory() {
    std::cout << "Running TestMemory...\n";
    Memory mem;

    // Test initial bounds
    EXPECT(mem.GetNeeds().energy >= 0.0f && mem.GetNeeds().energy <= 1.0f, "Energy in range");
    EXPECT(mem.GetNeeds().hunger >= 0.0f && mem.GetNeeds().hunger <= 1.0f, "Hunger in range");

    // Test feeding
    float prevHunger = mem.GetNeeds().hunger;
    mem.Feed(0.2f);
    EXPECT(mem.GetNeeds().hunger < prevHunger, "Hunger should decrease after feeding");

    // Test play
    float prevMood = mem.GetNeeds().mood;
    mem.Play(0.1f);
    EXPECT(mem.GetNeeds().mood >= prevMood, "Mood should increase after playing");

    // Test clamping
    mem.Feed(10.0f);
    EXPECT(mem.GetNeeds().hunger == 0.0f, "Hunger should clamp at 0.0 minimum");
}

void TestRegressions() {
    namespace fs = std::filesystem;
    SaveManager sm("regression_state.json");
    auto state = sm.GetDefaultState();
    state.name = "Astra \"star\"\\\n";
    state.history.timesFed = 7;
    state.history.timesPetted = 9;
    EXPECT(sm.Save(state), "Escaped state saves");
    PetStateData loaded;
    EXPECT(sm.Load(loaded) && loaded.name == state.name, "Escaped name round trips");
    EXPECT(loaded.history.timesFed == 7 && loaded.history.timesPetted == 9, "All counters persist");
    state.needs.energy = 0.123456f;
    EXPECT(sm.Save(state), "Existing save is replaced");
    EXPECT(sm.Load(loaded) && std::abs(loaded.needs.energy-state.needs.energy) < 0.000001f, "Float precision survives saves");
    state.needs.energy = std::numeric_limits<float>::quiet_NaN();
    EXPECT(!sm.Save(state), "Nonfinite needs cannot overwrite valid state");
    EXPECT(sm.Load(loaded), "Rejected save preserves previous state");
    { std::ofstream f("regression_state.json"); f << "broken {"; }
    EXPECT(!sm.Load(loaded), "Malformed JSON rejected");
    { std::ofstream f("regression_state.json"); f << "{}"; }
    EXPECT(!sm.Load(loaded), "Incomplete state rejected");
    fs::create_directory("save_destination_directory");
    SaveManager blocked("save_destination_directory");
    EXPECT(!blocked.Save(sm.GetDefaultState()), "Replacing directory returns failure");
    EXPECT(fs::is_directory("save_destination_directory"), "Failed save leaves destination intact");
    fs::remove("save_destination_directory");
    fs::remove("regression_state.json");

    { std::ofstream f("regression_config.json"); f << R"({"version":1,"app":{"target_fps":30,"always_on_top":false},"window":{"initial_position":"center"},"behavior":{"tick_rate_hz":20},"sensors":{"detect_cursor_drag":false,"idle_system_threshold_seconds":42},"paths":{"assets_dir":"art","data_dir":"progress"}})"; }
    AppConfigData cfg;
    EXPECT(sm.LoadConfig("regression_config.json", cfg), "Partial config accepts defaults");
    EXPECT(cfg.targetFps == 30 && !cfg.alwaysOnTop && !cfg.detectCursorDrag && cfg.tickRateHz == 20 && cfg.idleSystemThresholdSeconds == 42 && cfg.assetsDir == "art", "Config fields parsed");
    EXPECT(fs::path(cfg.saveFile) == fs::path("progress") / "pet_state.json", "Data directory sets default save path");
    { std::ofstream f("regression_config.json"); f << R"({"window":{"scale":-5}})"; }
    EXPECT(!sm.LoadConfig("regression_config.json", cfg) && cfg.windowScale == 2, "Invalid config returns clean defaults");
    fs::remove("regression_config.json");

    { std::ofstream f("pet_integration_config.json"); f << R"({"window":{"width":40,"height":50,"scale":1.5},"sensors":{"detect_cursor_drag":false,"idle_system_threshold_seconds":42}})"; }
    {
        Pet pet;
        EXPECT(pet.Init("pet_integration_config.json", "pet_integration_state.json"), "Pet initializes with explicit paths");
        EXPECT(pet.GetWidth() == 60 && pet.GetHeight() == 75 && !pet.GetMouseSensor().GetConfig().detectCursorDrag && pet.GetSystemSensor().GetIdleThreshold() == 42, "Pet applies loaded configuration");
        const float hunger = pet.GetMemory().GetNeeds().hunger;
        pet.Sleep();
        pet.Update(0.2f);
        EXPECT(pet.GetAnimation().GetCurrentState() == AnimationState::Sleep, "Pet sleep survives full update");
        EXPECT(std::abs(pet.GetMemory().GetNeeds().hunger - hunger - 0.0016f) < 0.00001f, "Low frame rate preserves elapsed simulation time");
        pet.Feed();
        pet.Update(0.016f);
        EXPECT(pet.GetAnimation().GetCurrentState() == AnimationState::Reaction, "Pet feeding reaction survives full update");
    }
    fs::remove("pet_integration_config.json");
    fs::remove("pet_integration_state.json");

    MouseSensor mouse;
    mouse.OnButtonDown(true, Point(10,10), Rect(0,0,100,100));
    mouse.OnButtonUp(true, Point(10,10));
    mouse.Update(0.016f, Rect(0,0,100,100));
    EXPECT(mouse.WasLeftButtonClicked(), "Click survives sensor update");
    mouse.ResetFrameState();
    EXPECT(!mouse.WasLeftButtonClicked(), "Consumed click clears");

    DesktopWorld world;
    SystemSensor system;
    Physics physics;
    Memory memory;
    PetBrain brain;
    brain.RequestAction(PetAction::Sleeping);
    const float before = memory.GetNeeds().energy;
    auto decision = brain.Update(0.1f, mouse, system, memory, world, physics);
    EXPECT(decision.animState == AnimationState::Sleep && memory.GetNeeds().energy > before, "Manual sleep persists and restores energy");
    brain.RequestAction(PetAction::ReactingToClick);
    EXPECT(brain.Update(0.1f, mouse, system, memory, world, physics).animState == AnimationState::Reaction, "Manual reaction persists");
    physics.SetPosition(100,100);
    physics.SpawnToy(164,-1000,0,0,true);
    brain.RequestAction(PetAction::Idle, 0);
    brain.Update(0.1f, mouse, system, memory, world, physics);
    EXPECT(physics.GetToy().active, "Treat cannot be eaten remotely along vertical axis");

    Animation animation;
    AnimationClip clip("test", true);
    clip.AddFrame("a", 0.1f); clip.AddFrame("b", 0.1f); clip.AddFrame("c", 0.1f);
    animation.RegisterClip("test", std::move(clip));
    animation.PlayClip("test");
    animation.Update(0.25f);
    EXPECT(animation.GetCurrentFrameIndex() == 2, "Animation advances multiple frames");
    animation.Update(0.1f);
    EXPECT(animation.GetCurrentFrameIndex() == 0, "Animation wraps correctly");
    AnimationClip bad("invalid", true);
    bad.AddFrame("bad", 0);
    EXPECT(bad.IsEmpty(), "Zero-duration frame rejected");
    AnimationClip defaults("defaults", true, 0.25f);
    defaults.AddFrame("a");
    EXPECT(defaults.GetTotalDuration() == 0.25f, "Configured frame duration is used");

    WindowSurface surface;
    surface.bounds = Rect(0,300,800,200);
    DesktopWorld fixedWorld(Rect(0,0,1000,800), {surface});
    Physics falling;
    falling.SetDimensions(100,100);
    falling.SetPosition(100,195);
    falling.SetVelocity(0,1000);
    falling.Update(0.05f, fixedWorld);
    EXPECT(falling.GetY() == 200, "Fast fall lands on crossed window edge");
    falling.SetPosition(100,195);
    falling.SetVelocity(0,-300);
    falling.Update(0.01f, fixedWorld);
    EXPECT(falling.GetY() < 195, "Upward motion does not snap onto a platform");

#ifdef _WIN32
    fs::create_directories("sprite_fixture/idle");
    // One opaque red pixel in a 24-bit BMP, padded to a four-byte scanline.
    const unsigned char bmp[] = {
        'B','M',58,0,0,0,0,0,0,0,54,0,0,0,
        40,0,0,0,1,0,0,0,1,0,0,0,1,0,24,0,
        0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,255,0};
    { std::ofstream f("sprite_fixture/idle/01.BMP", std::ios::binary); f.write(reinterpret_cast<const char*>(bmp), sizeof(bmp)); }
    { std::ofstream f("sprite_fixture/idle/02.png"); f << "invalid image"; }
    {
        Animation sprites;
        EXPECT(sprites.LoadFromDirectory("sprite_fixture") == 1, "Valid sprite folder loads");
        const auto* frame = sprites.GetCurrentFrame();
        EXPECT(frame && frame->bitmap && frame->width == 1 && frame->height == 1, "Sprite image is decoded with dimensions");
        HDC dc = CreateCompatibleDC(nullptr);
        HBITMAP bitmap = CreateBitmap(8,8,1,32,nullptr);
        auto previous = SelectObject(dc, bitmap);
        EXPECT(sprites.Render(dc,0,0,8,8), "Decoded sprite renders");
        EXPECT(GetRValue(GetPixel(dc,4,4)) > 200, "Rendered sprite uses image pixels");
        SelectObject(dc,previous); DeleteObject(bitmap); DeleteDC(dc);
    }
    fs::remove("sprite_fixture/idle/01.BMP");
    fs::remove("sprite_fixture/idle/02.png");
    fs::remove("sprite_fixture/idle");
    fs::remove("sprite_fixture");
#endif
}

int main() {
    namespace fs = std::filesystem;
    const auto previousDirectory = fs::current_path();
    const auto scratch = fs::temp_directory_path() / ("virtualpet-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directory(scratch);
    fs::current_path(scratch);
    std::cout << "========================================\n";
    std::cout << "  VirtualPet Autonomous Test Suite      \n";
    std::cout << "========================================\n\n";

    TestSaveManager();
    TestBehaviorTree();
    TestPhysics();
    TestMemory();
    TestRegressions();

    std::cout << "\n----------------------------------------\n";
    std::cout << "Passed: " << g_testsPassed << " | Failed: " << g_testsFailed << "\n";
    std::cout << "----------------------------------------\n";

    fs::current_path(previousDirectory);
    fs::remove(scratch);
    return (g_testsFailed == 0) ? 0 : 1;
}
