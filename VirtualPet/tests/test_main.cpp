#include "../include/BehaviorTree.h"
#include "../include/DesktopWorld.h"
#include "../include/Memory.h"
#include "../include/Physics.h"
#include "../include/SaveManager.h"
#include "../include/CompanionLife.h"

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
    original.identity.personalGoal = "paint a tiny comet journal";
    original.identity.goalProgress = 0.37f;

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
    EXPECT(loaded.identity.personalGoal == original.identity.personalGoal &&
           std::abs(loaded.identity.goalProgress - 0.37f) < 0.001f,
           "Independent goal should persist");

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
    std::cout << "Running TestMemory (Girlfriend Relationship)...\n";
    Memory mem;

    // Test initial bounds
    EXPECT(mem.GetNeeds().energy >= 0.0f && mem.GetNeeds().energy <= 1.0f, "Energy in range");
    EXPECT(mem.GetNeeds().hunger >= 0.0f && mem.GetNeeds().hunger <= 1.0f, "Coffee craving in range");
    EXPECT(mem.GetAffection() >= 0.0f && mem.GetAffection() <= 1.0f, "Affection in range");

    // Test relationship tier progression
    EXPECT(mem.GetRelationshipTier() == RelationshipTier::Acquaintance, "Relationship starts naturally as acquaintances");
    EXPECT(!mem.GetRelationshipTitle().empty(), "Relationship tier title is formatted");

    // Test headpats
    float prevMood = mem.GetNeeds().mood;
    float prevAffection = mem.GetAffection();
    int32_t prevHeadpats = mem.GetHistory().timesPetted;
    mem.GiveHeadpat(0.04f);
    EXPECT(mem.GetNeeds().mood >= prevMood, "Mood increases with headpats");
    EXPECT(mem.GetAffection() >= prevAffection, "Affection deepens with headpats");
    EXPECT(mem.GetHistory().timesPetted == prevHeadpats + 1, "Headpats counter increments");
    EXPECT(!mem.GetIdentity().personalGoal.empty(), "Companion has her own persistent goal");
    EXPECT(!mem.GetMoodTitle().empty(), "Changing emotional state has a readable mood");

    // Repetitive attention eventually triggers an honest boundary instead of infinite rewards.
    for (int i = 0; i < 8; ++i) mem.GiveHeadpat(0.01f);
    EXPECT(!mem.WasLastInteractionAccepted(), "Repeated attention can trigger a boundary");
    EXPECT(mem.GetHistory().boundariesExpressed > 0, "Expressed boundaries are remembered");

    // Test coffee & boba dates
    float prevCraving = mem.GetNeeds().GetCoffeeCraving();
    int32_t prevDates = mem.GetHistory().timesFed;
    mem.ShareCoffee(0.2f);
    EXPECT(mem.GetNeeds().GetCoffeeCraving() < prevCraving, "Craving decreases after coffee break");
    EXPECT(mem.GetHistory().timesFed == prevDates + 1, "Coffee dates counter increments");

    // Test study sessions
    int32_t prevStudy = mem.GetHistory().studySessionsTogether;
    mem.StudyTogether(1.0f);
    EXPECT(mem.GetHistory().studySessionsTogether == prevStudy + 1, "Study sessions counter increments");

    // Test clamping
    mem.ShareCoffee(10.0f);
    EXPECT(mem.GetNeeds().GetCoffeeCraving() == 0.0f, "Coffee craving clamps at 0.0 minimum");
}

void TestCompanionLife() {
    CompanionLife life; life.SetPath("life_test.json");
    life.AddTurn("user","remember I like jazz"); life.RememberFact("user likes jazz");
    life.AddDiaryEntry("We shared a quiet test moment."); life.SetUnresolvedIssue("a small misunderstanding");
    EXPECT(life.Save(), "Companion memory saves");
    CompanionLife loaded; loaded.SetPath("life_test.json");
    EXPECT(loaded.Load(), "Companion memory loads");
    EXPECT(loaded.BuildMemoryContext().find("jazz") != std::string::npos, "Remembered facts survive restart");
    EXPECT(loaded.GetUnresolvedIssue() == "a small misunderstanding", "Unresolved issues survive restart");
    EXPECT(!std::filesystem::exists("life_test.json.tmp"), "Atomic save leaves no temporary file");
    { std::ofstream oversized("life_oversized.json", std::ios::binary); oversized.seekp(1024 * 1024); oversized.put('x'); }
    CompanionLife rejected; rejected.SetPath("life_oversized.json");
    EXPECT(!rejected.Load(), "Oversized companion history is rejected");
    std::filesystem::remove("life_test.json");
    std::filesystem::remove("life_oversized.json");
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
    EXPECT(!cfg.localLlmUseWindowTitle && !cfg.localLlmUseClipboard,
           "Sensitive context capture is opt-in by default");
    { std::ofstream f("regression_config.json"); f << R"({"window":{"scale":-5}})"; }
    EXPECT(!sm.LoadConfig("regression_config.json", cfg) && cfg.windowScale == 2, "Invalid config returns clean defaults");
    fs::remove("regression_config.json");

    { std::ofstream f("pet_integration_config.json"); f << R"({"window":{"width":40,"height":50,"scale":1.5},"sensors":{"detect_cursor_drag":false,"idle_system_threshold_seconds":42}})"; }
    {
        Pet pet;
        EXPECT(pet.Init("pet_integration_config.json", "pet_integration_state.json"), "Pet initializes with explicit paths");
        EXPECT(pet.GetWidth() == 60 && pet.GetHeight() == 75 && !pet.GetMouseSensor().GetConfig().detectCursorDrag && pet.GetSystemSensor().GetIdleThreshold() == 42, "Pet applies loaded configuration");
        const float hunger = pet.GetMemory().GetNeeds().hunger;
        pet.Update(0.2f);
        EXPECT(std::abs(pet.GetMemory().GetNeeds().hunger - hunger - 0.0016f) < 0.00001f, "Low frame rate preserves elapsed simulation time");
        pet.Feed();
        pet.Update(0.016f);
        EXPECT(pet.GetAnimation().GetCurrentState() == AnimationState::Reaction, "Pet feeding reaction survives full update");

        // Test girlfriend companion interactions
        pet.GiveHeadpat();
        pet.Update(0.016f);
        EXPECT(pet.GetAnimation().GetCurrentState() == AnimationState::Reaction, "Headpat triggers heart reaction");

        pet.StartStudyMode(60.0f);
        EXPECT(pet.GetBrain().IsInStudyMode(), "Study mode activates successfully");

        pet.TossPlushieHeart();
        EXPECT(pet.GetPhysics().GetToy().active && !pet.GetPhysics().GetToy().isTreat, "Plushie heart spawned in physics world");
    }
    fs::remove("pet_integration_config.json");
    fs::remove("pet_integration_state.json");
    fs::remove("companion_life.json");

    MouseSensor mouse;
    mouse.OnButtonDown(true, Point(10,10), Rect(0,0,100,100));
    mouse.OnButtonUp(true, Point(10,10));
    mouse.Update(0.016f, Rect(0,0,100,100));
    EXPECT(mouse.WasLeftButtonClicked(), "Click survives sensor update");
    mouse.ResetFrameState();
    EXPECT(!mouse.WasLeftButtonClicked(), "Consumed click clears");

    // Dragging tests
    MouseSensor dragMouse;
    dragMouse.OnButtonDown(true, Point(20, 20), Rect(0, 0, 100, 100));
    dragMouse.OnMouseMove(Point(50, 50), Rect(0, 0, 100, 100));
    EXPECT(dragMouse.IsDragging(), "Drag state triggers after movement exceeds threshold");
    EXPECT(!dragMouse.WasLeftButtonClicked(), "Drag does not falsely trigger click");
    dragMouse.OnButtonUp(true, Point(50, 50));
    EXPECT(!dragMouse.IsDragging(), "Releasing mouse button ends drag");
    EXPECT(!dragMouse.WasLeftButtonClicked(), "Release after dragging does not trigger click");

    DesktopWorld world;
    SystemSensor system;
    Physics physics;
    Memory memory;
    PetBrain brain;
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
    {
        Animation sprites;
        AnimationFrame frame;
        frame.width = 2;
        frame.height = 1;
        frame.bitmap = std::make_shared<Gdiplus::Bitmap>(2, 1, PixelFormat32bppARGB);
        frame.bitmap->SetPixel(0, 0, Gdiplus::Color(128, 200, 0, 0));
        frame.bitmap->SetPixel(1, 0, Gdiplus::Color(0, 0, 0, 0));
        AnimationClip clip("alpha", true);
        clip.AddFrame(frame);
        sprites.RegisterClip("alpha", std::move(clip));
        sprites.PlayClip("alpha");
        BYTE pixels[8]{};
        EXPECT(sprites.RenderAlpha(pixels, 2, 1), "Alpha surface renders");
        EXPECT(pixels[0] == 0 && pixels[1] == 0 && std::abs(int(pixels[2]) - 100) <= 1
            && pixels[3] == 128, "Soft edges retain premultiplied alpha without magenta");
        EXPECT(pixels[4] == 0 && pixels[5] == 0 && pixels[6] == 0 && pixels[7] == 0,
            "Transparent background remains clear");
        sprites.SetFacingLeft(true);
        EXPECT(sprites.RenderAlpha(pixels, 2, 1) && pixels[3] == 0 && pixels[7] == 128,
            "Horizontal flip preserves alpha");
    }
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
    TestCompanionLife();
    TestRegressions();

    std::cout << "\n----------------------------------------\n";
    std::cout << "Passed: " << g_testsPassed << " | Failed: " << g_testsFailed << "\n";
    std::cout << "----------------------------------------\n";

    fs::current_path(previousDirectory);
    fs::remove(scratch);
    return (g_testsFailed == 0) ? 0 : 1;
}
