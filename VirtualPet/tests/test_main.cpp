#include "../include/BehaviorTree.h"
#include "../include/DesktopWorld.h"
#include "../include/Memory.h"
#include "../include/Physics.h"
#include "../include/SaveManager.h"
#include "../include/CompanionLife.h"
#include "../include/DesktopWatcher.h"

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

        // Test right-click context menu freezing
        pet.OnRButtonDown(Point(20, 20));
        EXPECT(pet.IsMenuOpen(), "Right click opens menu state on Pet");
        pet.Update(0.016f);
        EXPECT(pet.GetPhysics().GetVelocityX() == 0.0f, "Pet velocity is zeroed while menu is open");
        EXPECT(pet.GetBrain().IsMenuOpen(), "Pet brain registers menu open");

        pet.SetMenuOpen(false);
        EXPECT(!pet.IsMenuOpen(), "Dismissing menu clears open state on Pet");
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

void TestDesktopInteraction() {
    std::cout << "Running TestDesktopInteraction (Milestone 1)...\n";

    // 1. Test Animation States and Graceful Fallbacks
    Animation anim;
    anim.Play(AnimationState::ClimbUp);
    EXPECT(anim.GetCurrentState() == AnimationState::ClimbUp, "ClimbUp animation state activates");
    EXPECT(anim.GetCurrentClipName() == "walk", "ClimbUp gracefully fakes walk clip when dedicated sprite is absent");

    anim.Play(AnimationState::SitHang);
    EXPECT(anim.GetCurrentState() == AnimationState::SitHang, "SitHang animation state activates");
    EXPECT(anim.GetCurrentClipName() == "idle" || anim.GetCurrentClipName() == "sitting-idle", "SitHang gracefully fakes idle clip");

    anim.Play(AnimationState::Fall);
    EXPECT(anim.GetCurrentState() == AnimationState::Fall, "Fall animation state activates");

    anim.Play(AnimationState::Land);
    EXPECT(anim.GetCurrentState() == AnimationState::Land, "Land animation state activates");

    // 2. Test DesktopWatcher Surface Tracking & Nearest Surface
    WindowSurface ws1;
    ws1.bounds = Rect(200, 400, 600, 300);
    ws1.title = "Visual Studio Code - main.cpp";
#ifdef _WIN32
    ws1.hwnd = (HWND)0x1234;
#else
    ws1.xwin = 0x1234;
#endif

    WindowSurface ws2;
    ws2.bounds = Rect(900, 500, 500, 400);
    ws2.title = "Alacritty - bash terminal";
#ifdef _WIN32
    ws2.hwnd = (HWND)0x5678;
#else
    ws2.xwin = 0x5678;
#endif

    DesktopWorld testWorld(Rect(0, 0, 1920, 1080), {ws1, ws2});
    DesktopWatcher watcher;
    watcher.SetRefreshInterval(0.1f);

    // Nearest surface selection
    const WindowSurface* nearest = watcher.GetNearestSurface(Point(250, 600), testWorld);
    EXPECT(nearest != nullptr && nearest->title == ws1.title, "Nearest surface found correctly");

    const WindowSurface* nearest2 = watcher.GetNearestSurface(Point(1000, 600), testWorld);
    EXPECT(nearest2 != nullptr && nearest2->title == ws2.title, "Nearest surface switches based on proximity");

    // 3. Test Pet Standing on Window Top
    Rect petOnWindow(250, 300, 100, 100);
    watcher.Update(0.016f, petOnWindow, testWorld);
    EXPECT(watcher.GetCurrentSurface() != nullptr && watcher.GetCurrentSurface()->title == ws1.title,
           "Pet correctly detected as resting on window top surface");

    // 4. Test Window Movement Diff
    WindowSurface ws1Moved = ws1;
    ws1Moved.bounds = Rect(250, 420, 600, 300);
    DesktopWorld movedWorld(Rect(0, 0, 1920, 1080), {ws1Moved, ws2});
    watcher.Update(0.15f, petOnWindow, movedWorld);
    EXPECT(watcher.SurfaceJustMoved(), "Surface movement detected by watcher");
    EXPECT(watcher.GetSurfaceMoveDelta().x == 50 && watcher.GetSurfaceMoveDelta().y == 20,
           "Watcher calculated exact surface translation delta");

    // 5. Test Window Disappearance (Surface Gone)
    DesktopWorld closedWorld(Rect(0, 0, 1920, 1080), {ws2});
    watcher.Update(0.15f, petOnWindow, closedWorld);
    EXPECT(watcher.SurfaceJustGone(), "Watcher detected closed/vanished window surface");

    // 6. Test PetBrain Reactions to Window Dynamics
    MouseSensor mouse;
    SystemSensor system;
    Physics physics;
    physics.SetDimensions(100, 100);
    physics.SetPosition(250.0f, 300.0f);
    Memory memory;
    PetBrain brain;

    BrainDecision fallDecision = brain.Update(0.016f, mouse, system, memory, closedWorld, physics, &watcher);
    EXPECT(fallDecision.action == PetAction::Falling, "Pet enters Falling action when window vanishes");
    EXPECT(fallDecision.animState == AnimationState::Fall, "Pet plays Fall animation");
    watcher.ResetFrameFlags();

    // Pet landing on ground after fall
    physics.SetPosition(250.0f, static_cast<float>(closedWorld.GetGroundY(250, 100, 0)));
    physics.SetVelocity(0.0f, 0.0f);
    physics.Update(0.016f, closedWorld);
    BrainDecision landDecision = brain.Update(0.016f, mouse, system, memory, closedWorld, physics, &watcher);
    EXPECT(landDecision.action == PetAction::Idle, "Pet returns to Idle upon landing");
    EXPECT(landDecision.animState == AnimationState::Land, "Pet plays Land animation upon touchdown");

    // 7. Test Keyword Matching on Window Sit
    DesktopWatcher climbWatcher;
    climbWatcher.SetRefreshInterval(0.1f);
    DesktopWorld codeWorld(Rect(0, 0, 1920, 1080), {ws1});
    climbWatcher.Update(0.15f, Rect(200, 700, 100, 100), codeWorld);

    brain.RequestAction(PetAction::ClimbingWindow, 0.8f);
    BrainDecision climbStep = brain.Update(0.016f, mouse, system, memory, codeWorld, physics, &climbWatcher);
    EXPECT(climbStep.action == PetAction::ClimbingWindow && climbStep.animState == AnimationState::ClimbUp,
           "Pet climbs window with ClimbUp animation");

    BrainDecision sitDecision = brain.Update(0.85f, mouse, system, memory, codeWorld, physics, &climbWatcher);
    EXPECT(sitDecision.action == PetAction::SittingOnWindow, "Pet sits on window after climb completion");
    EXPECT(sitDecision.animState == AnimationState::SitHang, "Pet sits with SitHang animation state");

    // 8. Test Headroom Clearance Edge Case (Maximized / Ceiling-Docked Windows)
    WindowSurface topDockedWindow;
    topDockedWindow.bounds = Rect(0, 47, 1920, 1033); // e.g. Hyprland single window tiled below Waybar
    topDockedWindow.title = "virtual-being - Antigravity IDE";
    topDockedWindow.appClass = "antigravity-ide";

    DesktopWorld singleMaximizedWorld(Rect(0, 0, 1920, 1080), {topDockedWindow});
    DesktopWatcher headroomWatcher;

    // Nearest surface must reject top-docked window because bounds.y (47) - petHeight (120) = -73 < 0 + 10
    const WindowSurface* noSurface = headroomWatcher.GetNearestSurface(Point(500, 500), singleMaximizedWorld, 120);
    EXPECT(noSurface == nullptr, "Watcher rejects top-docked window with insufficient ceiling headroom");
    EXPECT(!headroomWatcher.HasClimbableSurfaces(singleMaximizedWorld, 120), "HasClimbableSurfaces returns false for zero headroom");

    // Supporting surface check ignores surface that forces pet off-screen, falling back to desktop floor
    int32_t groundY = singleMaximizedWorld.GetGroundY(500, 120, 0);
    EXPECT(singleMaximizedWorld.GetSupportingSurfaceY(500, 47, 100, 120, 0) == groundY,
           "DesktopWorld rejects supporting surface that pushes pet off top screen edge and uses floor");

    // Utility scores must not attempt climb (windowScore == 0) on single maximized screen
    UtilityScores scores = brain.CalculateUtilityScores(memory, mouse, system, physics, &headroomWatcher, &singleMaximizedWorld);
    EXPECT(scores.windowScore == 0.0f, "Pet utility engine gives 0 windowScore when no climbable windows exist");

    // 9. Test Dynamic Contextual Thought Generation & Active App Awareness
    std::string kittyThought1 = brain.GenerateAppThought("~", "kitty", false);
    EXPECT(!kittyThought1.empty(), "Kitty terminal with '~' title generates valid thought");
    EXPECT(kittyThought1.find("Terminal") != std::string::npos || kittyThought1.find("terminal") != std::string::npos ||
           kittyThought1.find("CLI") != std::string::npos || kittyThought1.find("command") != std::string::npos,
           "Terminal class triggers terminal-specific reaction");

    std::string kittyThought2 = brain.GenerateAppThought("~", "kitty", false);
    EXPECT(kittyThought1 != kittyThought2, "Successive thoughts rotate rather than repeating static text");

    std::string winTermThought = brain.GenerateAppThought("Windows PowerShell", "CASCADIA_HOSTING_WINDOW_CLASS", false);
    EXPECT(winTermThought.find("Terminal") != std::string::npos || winTermThought.find("terminal") != std::string::npos ||
           winTermThought.find("CLI") != std::string::npos || winTermThought.find("Command") != std::string::npos ||
           winTermThought.find("command") != std::string::npos || winTermThought.find("Compiling") != std::string::npos ||
           winTermThought.find("shell") != std::string::npos,
           "Windows Terminal class triggers terminal reaction dynamically");

    std::string cmdThought = brain.GenerateAppThought("Command Prompt - ping 127.0.0.1", "ConsoleWindowClass", false);
    EXPECT(cmdThought.find("Terminal") != std::string::npos || cmdThought.find("terminal") != std::string::npos ||
           cmdThought.find("CLI") != std::string::npos || cmdThought.find("command") != std::string::npos ||
           cmdThought.find("Compiling") != std::string::npos || cmdThought.find("shell") != std::string::npos,
           "Windows ConsoleWindowClass cmd triggers terminal reaction dynamically");

    std::string codeThought1 = brain.GenerateAppThought("virtual-being - Antigravity IDE", "antigravity-ide", false);
    EXPECT(!codeThought1.empty(), "Antigravity IDE generates valid thought");
    EXPECT(codeThought1.find("code") != std::string::npos || codeThought1.find("Coding") != std::string::npos ||
           codeThought1.find("keyboard") != std::string::npos || codeThought1.find("rubber duck") != std::string::npos ||
           codeThought1.find("bugs") != std::string::npos || codeThought1.find("flow state") != std::string::npos ||
           codeThought1.find("functions") != std::string::npos || codeThought1.find("commit") != std::string::npos ||
           codeThought1.find("logic") != std::string::npos,
           "IDE triggers coding-specific reaction");

    std::string chatThought = brain.GenerateAppThought("general", "discord", false);
    EXPECT(chatThought.find("Chat") != std::string::npos || chatThought.find("messages") != std::string::npos ||
           chatThought.find("conversation") != std::string::npos,
           "Discord app class triggers chat-specific reaction");

    // 10. Test Active Window Context Reflection in Brain Update
    WindowSurface activeWs;
    activeWs.bounds = Rect(0, 47, 1920, 1033);
    activeWs.title = "test_main.cpp - Antigravity IDE";
    activeWs.appClass = "antigravity-ide";
    singleMaximizedWorld.SetActiveWindow(activeWs);

    BrainDecision activeDecision = brain.Update(0.016f, mouse, system, memory, singleMaximizedWorld, physics, &headroomWatcher);
    EXPECT(!activeDecision.thought.empty(), "Brain produces non-empty thought reflecting active window");

    // 11. Test Drag Release & Airborne Throwing Transitions
    physics.StartDragging(Point(500, 300), Point(50, 50));
    BrainDecision dragDecision = brain.Update(0.016f, mouse, system, memory, singleMaximizedWorld, physics, &headroomWatcher);
    EXPECT(dragDecision.action == PetAction::Dragged, "Pet enters Dragged state when mouse dragged");

    // Release mouse while in mid-air (simulate throw/drop)
    physics.StopDragging();
    physics.SetPosition(500.0f, 300.0f); // mid-air, not grounded
    physics.SetVelocity(300.0f, -150.0f);
    BrainDecision throwDecision = brain.Update(0.016f, mouse, system, memory, singleMaximizedWorld, physics, &headroomWatcher);
    EXPECT(throwDecision.action == PetAction::Falling, "Pet immediately enters Falling state upon airborne throw release");
    EXPECT(throwDecision.animState == AnimationState::Fall, "Pet plays Fall animation when thrown airborne");

    // Land on ground
    physics.SetPosition(500.0f, static_cast<float>(singleMaximizedWorld.GetGroundY(500, 100, 0)));
    physics.SetVelocity(0.0f, 0.0f);
    physics.Update(0.016f, singleMaximizedWorld);
    BrainDecision landThrowDecision = brain.Update(0.016f, mouse, system, memory, singleMaximizedWorld, physics, &headroomWatcher);
    EXPECT(landThrowDecision.animState == AnimationState::Land, "Pet plays Land animation upon touchdown after throw");

    // After brief landing recovery (0.55s), pet must naturally resume active behavior without being frozen
    BrainDecision resumeDecision = brain.Update(0.55f, mouse, system, memory, singleMaximizedWorld, physics, &headroomWatcher);
    EXPECT(resumeDecision.action != PetAction::Dragged && resumeDecision.action != PetAction::Falling,
           "Pet resumes autonomous activity without getting stuck in inactive/concussed freeze");

    // 12. Test Right-Click Context Menu Stillness
    brain.SetMenuOpen(true);
    EXPECT(brain.IsMenuOpen() == true, "Brain records menu open state");
    BrainDecision menuDecision = brain.Update(0.016f, mouse, system, memory, singleMaximizedWorld, physics, &headroomWatcher);
    EXPECT(menuDecision.targetHorizontalSpeed == 0.0f, "Pet stops horizontal movement while menu is open");
    EXPECT(menuDecision.animState == AnimationState::Idle, "Pet assumes still Idle animation while menu is open");
    EXPECT(!menuDecision.thought.empty(), "Pet displays attentive thought while menu is open");

    // When menu is closed, brain resumes normal operations
    brain.SetMenuOpen(false);
    EXPECT(brain.IsMenuOpen() == false, "Brain records menu closed state");
    BrainDecision postMenuDecision = brain.Update(0.016f, mouse, system, memory, singleMaximizedWorld, physics, &headroomWatcher);
    EXPECT(postMenuDecision.action != PetAction::Dragged, "Pet operates normally after menu closes");
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
    TestDesktopInteraction();

    std::cout << "\n----------------------------------------\n";
    std::cout << "Passed: " << g_testsPassed << " | Failed: " << g_testsFailed << "\n";
    std::cout << "----------------------------------------\n";

    fs::current_path(previousDirectory);
    fs::remove(scratch);
    return (g_testsFailed == 0) ? 0 : 1;
}
