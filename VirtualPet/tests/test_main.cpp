#include "../include/BehaviorTree.h"
#include "../include/DesktopWorld.h"
#include "../include/Memory.h"
#include "../include/Physics.h"
#include "../include/SaveManager.h"

#include <cassert>
#include <cmath>
#include <iostream>

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
    DesktopWorld world;
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

int main() {
    std::cout << "========================================\n";
    std::cout << "  VirtualPet Autonomous Test Suite      \n";
    std::cout << "========================================\n\n";

    TestSaveManager();
    TestBehaviorTree();
    TestPhysics();
    TestMemory();

    std::cout << "\n----------------------------------------\n";
    std::cout << "Passed: " << g_testsPassed << " | Failed: " << g_testsFailed << "\n";
    std::cout << "----------------------------------------\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
