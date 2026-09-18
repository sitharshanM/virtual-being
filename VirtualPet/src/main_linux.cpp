#ifndef _WIN32

#include "Pet.h"
#include "SaveManager.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>
#include <pango/pangocairo.h>

#include <chrono>
#include <memory>
#include <string>
#include <iostream>
#include <csignal>
#include <cmath>
#include <dlfcn.h>

namespace LayerShell {

typedef enum {
    LAYER_BACKGROUND,
    LAYER_BOTTOM,
    LAYER_TOP,
    LAYER_OVERLAY
} Layer;

typedef enum {
    EDGE_LEFT,
    EDGE_RIGHT,
    EDGE_TOP,
    EDGE_BOTTOM
} Edge;

typedef enum {
    KEYBOARD_MODE_NONE,
    KEYBOARD_MODE_EXCLUSIVE,
    KEYBOARD_MODE_ON_DEMAND
} KeyboardMode;

typedef gboolean (*fn_is_supported)(void);
typedef void (*fn_init_for_window)(GtkWindow*);
typedef void (*fn_set_layer)(GtkWindow*, int);
typedef void (*fn_set_anchor)(GtkWindow*, int, gboolean);
typedef void (*fn_set_margin)(GtkWindow*, int, int);
typedef void (*fn_set_exclusive_zone)(GtkWindow*, int);
typedef void (*fn_set_keyboard_mode)(GtkWindow*, int);
typedef void (*fn_set_namespace)(GtkWindow*, const char*);

static void* s_handle = nullptr;
static fn_is_supported s_is_supported = nullptr;
static fn_init_for_window s_init = nullptr;
static fn_set_layer s_set_layer = nullptr;
static fn_set_anchor s_set_anchor = nullptr;
static fn_set_margin s_set_margin = nullptr;
static fn_set_exclusive_zone s_set_exclusive_zone = nullptr;
static fn_set_keyboard_mode s_set_keyboard_mode = nullptr;
static fn_set_namespace s_set_namespace = nullptr;
static bool s_active = false;

inline bool Init() {
    s_handle = dlopen("libgtk-layer-shell.so.0", RTLD_LAZY);
    if (!s_handle) return false;

    s_is_supported = (fn_is_supported)dlsym(s_handle, "gtk_layer_is_supported");
    s_init = (fn_init_for_window)dlsym(s_handle, "gtk_layer_init_for_window");
    s_set_layer = (fn_set_layer)dlsym(s_handle, "gtk_layer_set_layer");
    s_set_anchor = (fn_set_anchor)dlsym(s_handle, "gtk_layer_set_anchor");
    s_set_margin = (fn_set_margin)dlsym(s_handle, "gtk_layer_set_margin");
    s_set_exclusive_zone = (fn_set_exclusive_zone)dlsym(s_handle, "gtk_layer_set_exclusive_zone");
    s_set_keyboard_mode = (fn_set_keyboard_mode)dlsym(s_handle, "gtk_layer_set_keyboard_mode");
    s_set_namespace = (fn_set_namespace)dlsym(s_handle, "gtk_layer_set_namespace");

    if (s_is_supported && s_init && s_set_layer && s_set_anchor && s_set_margin && s_is_supported()) {
        s_active = true;
        std::cout << "[VirtualPet] Wayland Layer Shell active — overlaying directly on top of desktop!\n";
        return true;
    }
    return false;
}

inline bool IsActive() { return s_active; }

inline void SetupOverlayWindow(GtkWidget* window, const char* nameSpace = "VirtualPet") {
    if (!s_active) return;
    s_init(GTK_WINDOW(window));
    s_set_layer(GTK_WINDOW(window), LAYER_OVERLAY);
    if (s_set_namespace) s_set_namespace(GTK_WINDOW(window), nameSpace);
    s_set_anchor(GTK_WINDOW(window), EDGE_LEFT, TRUE);
    s_set_anchor(GTK_WINDOW(window), EDGE_TOP, TRUE);
    if (s_set_exclusive_zone) s_set_exclusive_zone(GTK_WINDOW(window), -1);
    if (s_set_keyboard_mode) s_set_keyboard_mode(GTK_WINDOW(window), KEYBOARD_MODE_NONE);
}

inline void Move(GtkWidget* window, int x, int y) {
    if (s_active) {
        s_set_margin(GTK_WINDOW(window), EDGE_LEFT, x);
        s_set_margin(GTK_WINDOW(window), EDGE_TOP, y);
    } else {
        gtk_window_move(GTK_WINDOW(window), x, y);
    }
}

} // namespace LayerShell

namespace {

std::unique_ptr<VirtualPet::Pet> g_pet = nullptr;

GtkWidget* g_petWindow = nullptr;
GtkWidget* g_petArea = nullptr;
GtkWidget* g_toyWindow = nullptr;
GtkWidget* g_toyArea = nullptr;
GtkWidget* g_bubbleWindow = nullptr;
GtkWidget* g_bubbleArea = nullptr;
GtkWidget* g_chatWindow = nullptr;
GtkWidget* g_chatTextView = nullptr;
GtkWidget* g_chatEntry = nullptr;
GtkStatusIcon* g_statusIcon = nullptr;

bool g_isPetVisible = true;
std::string g_bubbleText;
static double g_bubbleTailX = 150.0;

using Clock = std::chrono::steady_clock;
auto g_prevTime = Clock::now();

void ShowChatPanel();
void RefreshChatHistory();
void ShowPetContextMenu(GdkEventButton* event);
void ShowPetStatusDialog();

// ── Cairo helper for drawing rounded rectangles ─────────────────────────────
void DrawRoundedRect(cairo_t* cr, double x, double y, double width, double height, double radius) {
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + width - radius, y + radius, radius, -M_PI / 2, 0);
    cairo_arc(cr, x + width - radius, y + height - radius, radius, 0, M_PI / 2);
    cairo_arc(cr, x + radius, y + height - radius, radius, M_PI / 2, M_PI);
    cairo_arc(cr, x + radius, y + radius, radius, M_PI, 3 * M_PI / 2);
    cairo_close_path(cr);
}

// ── Toy Window Drawing ──────────────────────────────────────────────────────
gboolean OnToyDraw(GtkWidget* /*widget*/, cairo_t* cr, gpointer /*data*/) {
    // Clear transparent
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    if (!g_pet) return FALSE;
    const auto& toy = g_pet->GetPhysics().GetToy();

    if (toy.isTreat) {
        // Draw Boba / Coffee Cup
        // Cup body
        cairo_set_source_rgb(cr, 0.82, 0.63, 0.43);
        DrawRoundedRect(cr, 4, 8, 20, 18, 4);
        cairo_fill(cr);

        // Cup lid
        cairo_set_source_rgb(cr, 0.96, 0.94, 0.90);
        DrawRoundedRect(cr, 2, 4, 24, 6, 2);
        cairo_fill(cr);

        // Straw
        cairo_set_source_rgb(cr, 0.39, 0.78, 0.47);
        cairo_set_line_width(cr, 2.0);
        cairo_move_to(cr, 17, 5);
        cairo_line_to(cr, 20, 1);
        cairo_stroke(cr);
    } else {
        // Draw Pink Plushie Heart
        cairo_set_source_rgb(cr, 1.0, 0.43, 0.65);
        cairo_arc(cr, 9, 10, 6, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_arc(cr, 19, 10, 6, 0, 2 * M_PI);
        cairo_fill(cr);

        cairo_move_to(cr, 4, 12);
        cairo_line_to(cr, 24, 12);
        cairo_line_to(cr, 14, 24);
        cairo_close_path(cr);
        cairo_fill(cr);
    }
    return FALSE;
}

// ── Speech Bubble Drawing ───────────────────────────────────────────────────
gboolean OnBubbleDraw(GtkWidget* widget, cairo_t* cr, gpointer /*data*/) {
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);
    if (width <= 0) width = 300;
    if (height <= 0) height = 92;

    double x = 4.0;
    double y = 4.0;
    double w = width - 8.0;
    double h = height - 18.0;
    double r = 16.0;

    double tailX = std::clamp(g_bubbleTailX, 40.0, width - 40.0);
    double tailW = 18.0;
    double tailH = 12.0;

    // Unified continuous outline for bubble body and centered tail
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + r, y + r, r, M_PI, 1.5 * M_PI);
    cairo_arc(cr, x + w - r, y + r, r, -0.5 * M_PI, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, 0.5 * M_PI);

    double tailLeft = tailX - tailW / 2.0;
    double tailRight = tailX + tailW / 2.0;
    cairo_line_to(cr, tailRight, y + h);
    cairo_line_to(cr, tailX, y + h + tailH);
    cairo_line_to(cr, tailLeft, y + h);

    cairo_arc(cr, x + r, y + h - r, r, 0.5 * M_PI, M_PI);
    cairo_close_path(cr);

    // Soft warm creamy pinkish-white fill
    cairo_set_source_rgb(cr, 1.0, 0.98, 0.99);
    cairo_fill_preserve(cr);

    // Clean aesthetic border
    cairo_set_source_rgb(cr, 0.42, 0.26, 0.32);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);

    // Text rendering via Pango
    if (!g_bubbleText.empty()) {
        PangoLayout* layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(layout, g_bubbleText.c_str(), -1);
        pango_layout_set_width(layout, (width - 32) * PANGO_SCALE);
        pango_layout_set_wrap(layout, PANGO_WRAP_WORD_CHAR);
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

        PangoFontDescription* fontDesc = pango_font_description_from_string("Sans Bold 10");
        pango_layout_set_font_description(layout, fontDesc);
        pango_font_description_free(fontDesc);

        cairo_set_source_rgb(cr, 0.16, 0.11, 0.14);
        cairo_move_to(cr, 16, 12);
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);
    }
    return FALSE;
}

gboolean OnBubbleButtonPress(GtkWidget* /*widget*/, GdkEventButton* event, gpointer /*data*/) {
    if (event->button == 1) {
        ShowChatPanel();
        return TRUE;
    }
    return FALSE;
}

// ── Pet Window Drawing ──────────────────────────────────────────────────────
gboolean OnPetDraw(GtkWidget* /*widget*/, cairo_t* cr, gpointer /*data*/) {
    // 1. Clear background to pure transparent
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);

    // 2. Render Pet with Cairo
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    if (g_pet && g_isPetVisible) {
        g_pet->RenderCairo(cr);
    }
    return FALSE;
}

// ── Mouse & Interaction Events ──────────────────────────────────────────────
gboolean OnPetButtonPress(GtkWidget* /*widget*/, GdkEventButton* event, gpointer /*data*/) {
    if (!g_pet) return FALSE;

    VirtualPet::Point petPos = g_pet->GetPosition();
    int screenX = petPos.x + static_cast<int>(event->x);
    int screenY = petPos.y + static_cast<int>(event->y);

    if (event->button == 1) {
        if (event->type == GDK_2BUTTON_PRESS) {
            ShowChatPanel();
            return TRUE;
        }
        if (g_petWindow) gtk_grab_add(g_petWindow);
        g_pet->OnLButtonDown(VirtualPet::Point(screenX, screenY));
        return TRUE;
    } else if (event->button == 3) {
        g_pet->OnRButtonDown(VirtualPet::Point(screenX, screenY));
        ShowPetContextMenu(event);
        return TRUE;
    }
    return FALSE;
}

gboolean OnPetButtonRelease(GtkWidget* /*widget*/, GdkEventButton* event, gpointer /*data*/) {
    if (!g_pet) return FALSE;

    VirtualPet::Point petPos = g_pet->GetPosition();
    int screenX = petPos.x + static_cast<int>(event->x);
    int screenY = petPos.y + static_cast<int>(event->y);

    if (event->button == 1) {
        if (g_petWindow) gtk_grab_remove(g_petWindow);
        g_pet->OnLButtonUp(VirtualPet::Point(screenX, screenY));
        return TRUE;
    } else if (event->button == 3) {
        g_pet->OnRButtonUp(VirtualPet::Point(screenX, screenY));
        return TRUE;
    }
    return FALSE;
}

gboolean OnPetMotion(GtkWidget* /*widget*/, GdkEventMotion* event, gpointer /*data*/) {
    if (g_pet) {
        VirtualPet::Point petPos = g_pet->GetPosition();
        int screenX = petPos.x + static_cast<int>(event->x);
        int screenY = petPos.y + static_cast<int>(event->y);
        g_pet->OnMouseMove(VirtualPet::Point(screenX, screenY));
        return TRUE;
    }
    return FALSE;
}

// ── Context Menu Callbacks ──────────────────────────────────────────────────
void OnMenuToggleVisible(GtkMenuItem* /*item*/, gpointer /*data*/) {
    g_isPetVisible = !g_isPetVisible;
    if (g_isPetVisible) {
        gtk_widget_show(g_petWindow);
    } else {
        gtk_widget_hide(g_petWindow);
        if (g_toyWindow) gtk_widget_hide(g_toyWindow);
        if (g_bubbleWindow) gtk_widget_hide(g_bubbleWindow);
    }
}

void OnMenuStatus(GtkMenuItem* /*item*/, gpointer /*data*/) {
    ShowPetStatusDialog();
}

void OnMenuHeadpat(GtkMenuItem* /*item*/, gpointer /*data*/) {
    if (g_pet) g_pet->GiveHeadpat();
}

void OnMenuCoffee(GtkMenuItem* /*item*/, gpointer /*data*/) {
    if (g_pet) g_pet->SendCoffee();
}

void OnMenuToy(GtkMenuItem* /*item*/, gpointer /*data*/) {
    if (g_pet) g_pet->TossPlushieHeart();
}

void OnMenuStudy(GtkMenuItem* /*item*/, gpointer /*data*/) {
    if (g_pet) g_pet->StartStudyMode();
}

void OnMenuChat(GtkMenuItem* /*item*/, gpointer /*data*/) {
    ShowChatPanel();
}

void OnMenuExit(GtkMenuItem* /*item*/, gpointer /*data*/) {
    if (g_pet) g_pet->SaveState();
    gtk_main_quit();
}

void ShowPetContextMenu(GdkEventButton* event) {
    if (g_pet) g_pet->SetMenuOpen(true);
    GtkWidget* menu = gtk_menu_new();
    g_signal_connect(menu, "deactivate", G_CALLBACK(+[](GtkMenuShell*, gpointer) {
        if (g_pet) g_pet->SetMenuOpen(false);
    }), nullptr);

    GtkWidget* itemToggle = gtk_menu_item_new_with_label(g_isPetVisible ? "Hide Companion" : "Show Companion");
    g_signal_connect(itemToggle, "activate", G_CALLBACK(OnMenuToggleVisible), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemToggle);

    GtkWidget* itemChat = gtk_menu_item_new_with_label("Open Chat Panel 💬");
    g_signal_connect(itemChat, "activate", G_CALLBACK(OnMenuChat), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemChat);

    GtkWidget* itemStatus = gtk_menu_item_new_with_label("Relationship Diary & Status... 💌");
    g_signal_connect(itemStatus, "activate", G_CALLBACK(OnMenuStatus), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemStatus);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget* itemHeadpat = gtk_menu_item_new_with_label("Give Headpat ❤️");
    g_signal_connect(itemHeadpat, "activate", G_CALLBACK(OnMenuHeadpat), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemHeadpat);

    GtkWidget* itemCoffee = gtk_menu_item_new_with_label("Send Coffee / Boba Break ☕");
    g_signal_connect(itemCoffee, "activate", G_CALLBACK(OnMenuCoffee), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemCoffee);

    GtkWidget* itemToy = gtk_menu_item_new_with_label("Toss Plushie Heart 🧸");
    g_signal_connect(itemToy, "activate", G_CALLBACK(OnMenuToy), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemToy);

    GtkWidget* itemStudy = gtk_menu_item_new_with_label("Study Together (Focus Mode) 📖");
    g_signal_connect(itemStudy, "activate", G_CALLBACK(OnMenuStudy), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemStudy);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget* itemExit = gtk_menu_item_new_with_label("Exit Companion");
    g_signal_connect(itemExit, "activate", G_CALLBACK(OnMenuExit), nullptr);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), itemExit);

    gtk_widget_show_all(menu);

    guint button = event ? event->button : 0;
    guint32 time = event ? event->time : gtk_get_current_event_time();
    gtk_menu_popup_at_pointer(GTK_MENU(menu), reinterpret_cast<GdkEvent*>(event));
}

// ── Pet Status Dialog ───────────────────────────────────────────────────────
void ShowPetStatusDialog() {
    if (!g_pet) return;
    const auto& needs = g_pet->GetMemory().GetNeeds();
    const auto& name = g_pet->GetMemory().GetName();
    const auto& history = g_pet->GetMemory().GetHistory();
    const auto& identity = g_pet->GetMemory().GetIdentity();
    const auto& thought = g_pet->GetBrain().GetThought();
    std::string tierTitle = g_pet->GetMemory().GetRelationshipTitle();
    std::string moodTitle = g_pet->GetMemory().GetMoodTitle();
    std::string llmStatus = g_pet->GetLocalLlmStatus();

    char statusMsg[2048]{};
    snprintf(statusMsg, sizeof(statusMsg),
             "🌸 Companion: %s\n"
             "💕 Relationship: %s (Affection: %d%%)\n\n"
             "✨ Mood: %s (%d%%)\n"
             "🫧 Comfort: %d%%\n"
             "💤 Beauty Rest: %d%%\n"
             "☕ Coffee / Boba Craving: %d%%\n\n"
             "🎧 Her own world:\n"
             "  • Loves: %s\n"
             "  • Usually listening to: %s\n"
             "  • Imperfection: %s\n"
             "  • Goal: %s (%d%%)\n\n"
             "📜 Our Diary:\n"
             "  • Headpats: %d\n"
             "  • Coffee Dates: %d\n"
             "  • Study Sessions: %d\n"
             "  • Honest boundaries: %d\n"
             "  • Days Together: %d\n\n"
             "🧠 Local AI: %s\n\n"
             "💭 Current Thought:\n"
             "\"%s\"",
             name.c_str(),
             tierTitle.c_str(),
             static_cast<int>(needs.GetAffection() * 100.0f),
             moodTitle.c_str(),
             static_cast<int>(needs.mood * 100.0f),
             static_cast<int>(needs.comfort * 100.0f),
             static_cast<int>(needs.energy * 100.0f),
             static_cast<int>(needs.GetCoffeeCraving() * 100.0f),
             identity.favoriteActivity.c_str(),
             identity.favoriteMusic.c_str(),
             identity.imperfection.c_str(),
             identity.personalGoal.c_str(),
             static_cast<int>(identity.goalProgress * 100.0f),
             history.timesPetted,
             history.timesFed,
             history.studySessionsTogether,
             history.boundariesExpressed,
             history.daysTogether,
             llmStatus.c_str(),
             thought.c_str());

    GtkWidget* dialog = gtk_message_dialog_new(nullptr,
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "%s", statusMsg);
    gtk_window_set_title(GTK_WINDOW(dialog), "Relationship Diary & Status");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// ── Chat Panel ──────────────────────────────────────────────────────────────
void RefreshChatHistory() {
    if (!g_pet || !g_chatTextView) return;
    std::string text = g_pet->GetConversationText();
    if (g_pet->IsAiTyping()) {
        text += "Astra is thinking locally...\n";
    }
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(g_chatTextView));
    gtk_text_buffer_set_text(buffer, text.c_str(), -1);

    // Scroll to end
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    GtkTextMark* mark = gtk_text_buffer_create_mark(buffer, nullptr, &end, FALSE);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(g_chatTextView), mark, 0.0, TRUE, 0.0, 1.0);
}

void OnChatSendClicked(GtkButton* /*btn*/, gpointer /*data*/) {
    if (!g_pet || !g_chatEntry) return;
    const gchar* text = gtk_entry_get_text(GTK_ENTRY(g_chatEntry));
    if (!text || text[0] == '\0') return;

    std::string message(text);
    if (g_pet->SendChatMessage(message)) {
        gtk_entry_set_text(GTK_ENTRY(g_chatEntry), "");
        RefreshChatHistory();
    } else if (g_pet->IsAiTyping()) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(g_chatWindow),
                                                   GTK_DIALOG_MODAL,
                                                   GTK_MESSAGE_INFO,
                                                   GTK_BUTTONS_OK,
                                                   "Astra is still thinking. Give her a moment.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

void OnChatDiaryClicked(GtkButton* /*btn*/, gpointer /*data*/) {
    if (!g_pet) return;
    std::string diary = g_pet->GetDiaryText();
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(g_chatWindow),
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_INFO,
                                               GTK_BUTTONS_OK,
                                               "%s", diary.c_str());
    gtk_window_set_title(GTK_WINDOW(dialog), "Astra's Diary");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

gboolean OnChatWindowDelete(GtkWidget* widget, GdkEvent* /*event*/, gpointer /*data*/) {
    gtk_widget_hide(widget);
    return TRUE; // Do not destroy
}

gboolean OnChatTimer(gpointer /*data*/) {
    if (g_chatWindow && gtk_widget_get_visible(g_chatWindow)) {
        RefreshChatHistory();
    }
    return G_SOURCE_CONTINUE;
}

void CreateChatWindow() {
    g_chatWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(g_chatWindow), "Chat with Astra");
    gtk_window_set_default_size(GTK_WINDOW(g_chatWindow), 520, 460);
    g_signal_connect(g_chatWindow, "delete-event", G_CALLBACK(OnChatWindowDelete), nullptr);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
    gtk_container_add(GTK_CONTAINER(g_chatWindow), vbox);

    // Chat History TextView
    GtkWidget* scrolled = gtk_scrolled_window_new(nullptr, nullptr);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

    g_chatTextView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(g_chatTextView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(g_chatTextView), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(g_chatTextView), 8);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(g_chatTextView), 8);
    gtk_container_add(GTK_CONTAINER(scrolled), g_chatTextView);

    // Input Bar
    GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    g_chatEntry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(g_chatEntry), "Say something to Astra...");
    g_signal_connect(g_chatEntry, "activate", G_CALLBACK(OnChatSendClicked), nullptr);
    gtk_box_pack_start(GTK_BOX(hbox), g_chatEntry, TRUE, TRUE, 0);

    GtkWidget* sendBtn = gtk_button_new_with_label("Send");
    g_signal_connect(sendBtn, "clicked", G_CALLBACK(OnChatSendClicked), nullptr);
    gtk_box_pack_start(GTK_BOX(hbox), sendBtn, FALSE, FALSE, 0);

    GtkWidget* diaryBtn = gtk_button_new_with_label("Diary");
    g_signal_connect(diaryBtn, "clicked", G_CALLBACK(OnChatDiaryClicked), nullptr);
    gtk_box_pack_start(GTK_BOX(hbox), diaryBtn, FALSE, FALSE, 0);

    g_timeout_add(500, OnChatTimer, nullptr);
}

void ShowChatPanel() {
    if (!g_chatWindow) CreateChatWindow();
    gtk_widget_show_all(g_chatWindow);
    gtk_window_present(GTK_WINDOW(g_chatWindow));
    gtk_widget_grab_focus(g_chatEntry);
    RefreshChatHistory();
}

// ── System Tray ─────────────────────────────────────────────────────────────
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
void SetupSystemTray() {
    const char* wayland = std::getenv("WAYLAND_DISPLAY");
    if (wayland && *wayland) {
        // Wayland compositors do not implement legacy XEmbed GtkStatusIcon.
        // Full context menu and chat panel are accessible directly via right-click / double-click.
        return;
    }
    GdkPixbuf* pb = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 24, 24);
    if (pb) {
        gdk_pixbuf_fill(pb, 0xFFA060FF);
        g_statusIcon = gtk_status_icon_new_from_pixbuf(pb);
        g_object_unref(pb);
    }
    if (g_statusIcon) {
        gtk_status_icon_set_tooltip_text(g_statusIcon, "Virtual Companion - Astra ❤️");
        g_signal_connect(g_statusIcon, "activate", G_CALLBACK(ShowChatPanel), nullptr);
        g_signal_connect_swapped(g_statusIcon, "popup-menu", G_CALLBACK(ShowPetContextMenu), nullptr);
        gtk_status_icon_set_visible(g_statusIcon, TRUE);
    }
}
#pragma GCC diagnostic pop

// ── Main Simulation Tick (60 FPS) ───────────────────────────────────────────
gboolean SimulationTick(gpointer /*data*/) {
    if (!g_pet) return G_SOURCE_REMOVE;

    auto currentTime = Clock::now();
    float deltaTime = std::chrono::duration<float>(currentTime - g_prevTime).count();
    g_prevTime = currentTime;

    // Advance pet logic and physics
    g_pet->Update(deltaTime);

    int width = g_pet->GetWidth();
    int height = g_pet->GetHeight();
    VirtualPet::Point pos = g_pet->GetPosition();

    // 1. Move Pet Window & Request Redraw
    if (g_isPetVisible) {
        LayerShell::Move(g_petWindow, pos.x, pos.y);
        if (g_petArea) gtk_widget_queue_draw(g_petArea);
    }

    // 2. Update Toy Window
    const auto& toy = g_pet->GetPhysics().GetToy();
    if (g_toyWindow) {
        if (toy.active && g_isPetVisible) {
            int toyX = static_cast<int>(toy.x - toy.radius);
            int toyY = static_cast<int>(toy.y - toy.radius);
            int toySize = static_cast<int>(toy.radius * 2);
            if (g_toyArea) gtk_widget_set_size_request(g_toyArea, toySize, toySize);
            LayerShell::Move(g_toyWindow, toyX, toyY);
            if (!gtk_widget_get_visible(g_toyWindow)) {
                gtk_widget_show_all(g_toyWindow);
            }
            if (g_toyArea) gtk_widget_queue_draw(g_toyArea);
        } else if (gtk_widget_get_visible(g_toyWindow)) {
            gtk_widget_hide(g_toyWindow);
        }
    }

    // 3. Update Speech Bubble Window
    if (g_bubbleWindow) {
        std::string nextText = g_pet->GetSpeechText();
        if (nextText != g_bubbleText) {
            g_bubbleText = nextText;
            if (g_bubbleArea) gtk_widget_queue_draw(g_bubbleArea);
        }
        if (g_isPetVisible && !g_bubbleText.empty()) {
            int bubbleW = 300;
            int bubbleH = 92;
            int bubbleX = pos.x + (width - bubbleW) / 2;
            int bubbleY = (pos.y >= 96) ? pos.y - 88 : pos.y + height + 6;

            g_bubbleTailX = (pos.x + width / 2.0) - bubbleX;

            LayerShell::Move(g_bubbleWindow, bubbleX, bubbleY);
            if (!gtk_widget_get_visible(g_bubbleWindow)) {
                gtk_widget_show_all(g_bubbleWindow);
            }
            if (g_bubbleArea) gtk_widget_queue_draw(g_bubbleArea);
        } else if (gtk_widget_get_visible(g_bubbleWindow)) {
            gtk_widget_hide(g_bubbleWindow);
        }
    }

    return G_SOURCE_CONTINUE;
}

void SignalHandler(int signum) {
    (void)signum;
    if (gtk_main_level() > 0) {
        gtk_main_quit();
    }
}

} // namespace

int main(int argc, char* argv[]) {
    // Register signal handlers for clean dev exit
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    gtk_init(&argc, &argv);

    // 1. Initialize Companion core
    g_pet = std::make_unique<VirtualPet::Pet>();
    if (!g_pet->Init()) {
        std::cerr << "[VirtualPet] Error: Failed to initialize Companion.\n";
        return 1;
    }

    // Try to initialize Wayland Layer Shell (for desktop overlay above all windows)
    LayerShell::Init();

    int width = g_pet->GetWidth();
    int height = g_pet->GetHeight();
    VirtualPet::Point initPos = g_pet->GetPosition();

    // Global CSS provider for alpha transparency across themes
    GtkCssProvider* cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider,
        ".transparent-layer, .transparent-layer > *, .transparent-layer drawingarea {"
        "  background-color: transparent;"
        "  background-image: none;"
        "  border-style: none;"
        "  box-shadow: none;"
        "}", -1, nullptr);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(cssProvider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(cssProvider);

    // 2. Create Transparent Borderless Pet Window
    g_petWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    LayerShell::SetupOverlayWindow(g_petWindow, "VirtualPet");
    gtk_window_set_title(GTK_WINDOW(g_petWindow), g_pet->GetConfig().title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(g_petWindow), width, height);
    gtk_window_set_decorated(GTK_WINDOW(g_petWindow), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(g_petWindow), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(g_petWindow), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(g_petWindow), TRUE);
    gtk_widget_set_app_paintable(g_petWindow, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(g_petWindow), "transparent-layer");

    // Enable per-pixel alpha compositor visual
    GdkScreen* screen = gtk_widget_get_screen(g_petWindow);
    GdkVisual* visual = gdk_screen_get_rgba_visual(screen);
    if (visual) {
        gtk_widget_set_visual(g_petWindow, visual);
    }

    g_petArea = gtk_drawing_area_new();
    gtk_widget_set_size_request(g_petArea, width, height);
    gtk_style_context_add_class(gtk_widget_get_style_context(g_petArea), "transparent-layer");
    gtk_container_add(GTK_CONTAINER(g_petWindow), g_petArea);

    gtk_widget_add_events(g_petWindow,
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK);
    gtk_widget_add_events(g_petArea,
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK);

    g_signal_connect(g_petArea, "draw", G_CALLBACK(OnPetDraw), nullptr);
    g_signal_connect(g_petWindow, "button-press-event", G_CALLBACK(OnPetButtonPress), nullptr);
    g_signal_connect(g_petArea, "button-press-event", G_CALLBACK(OnPetButtonPress), nullptr);
    g_signal_connect(g_petWindow, "button-release-event", G_CALLBACK(OnPetButtonRelease), nullptr);
    g_signal_connect(g_petArea, "button-release-event", G_CALLBACK(OnPetButtonRelease), nullptr);
    g_signal_connect(g_petWindow, "motion-notify-event", G_CALLBACK(OnPetMotion), nullptr);
    g_signal_connect(g_petArea, "motion-notify-event", G_CALLBACK(OnPetMotion), nullptr);

    // 3. Create Toy Window (Transparent, overlay)
    g_toyWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    LayerShell::SetupOverlayWindow(g_toyWindow, "VirtualPetToy");
    gtk_window_set_default_size(GTK_WINDOW(g_toyWindow), 28, 28);
    gtk_window_set_decorated(GTK_WINDOW(g_toyWindow), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(g_toyWindow), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(g_toyWindow), TRUE);
    gtk_widget_set_app_paintable(g_toyWindow, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(g_toyWindow), "transparent-layer");
    if (visual) gtk_widget_set_visual(g_toyWindow, visual);

    g_toyArea = gtk_drawing_area_new();
    gtk_widget_set_size_request(g_toyArea, 28, 28);
    gtk_style_context_add_class(gtk_widget_get_style_context(g_toyArea), "transparent-layer");
    gtk_container_add(GTK_CONTAINER(g_toyWindow), g_toyArea);
    g_signal_connect(g_toyArea, "draw", G_CALLBACK(OnToyDraw), nullptr);

    // 4. Create Speech Bubble Window
    g_bubbleWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    LayerShell::SetupOverlayWindow(g_bubbleWindow, "VirtualPetBubble");
    gtk_window_set_default_size(GTK_WINDOW(g_bubbleWindow), 300, 92);
    gtk_window_set_decorated(GTK_WINDOW(g_bubbleWindow), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(g_bubbleWindow), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(g_bubbleWindow), TRUE);
    gtk_widget_set_app_paintable(g_bubbleWindow, TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(g_bubbleWindow), "transparent-layer");
    if (visual) gtk_widget_set_visual(g_bubbleWindow, visual);

    g_bubbleArea = gtk_drawing_area_new();
    gtk_widget_set_size_request(g_bubbleArea, 300, 92);
    gtk_style_context_add_class(gtk_widget_get_style_context(g_bubbleArea), "transparent-layer");
    gtk_container_add(GTK_CONTAINER(g_bubbleWindow), g_bubbleArea);
    gtk_widget_add_events(g_bubbleWindow, GDK_BUTTON_PRESS_MASK);
    gtk_widget_add_events(g_bubbleArea, GDK_BUTTON_PRESS_MASK);
    g_signal_connect(g_bubbleArea, "draw", G_CALLBACK(OnBubbleDraw), nullptr);
    g_signal_connect(g_bubbleWindow, "button-press-event", G_CALLBACK(OnBubbleButtonPress), nullptr);
    g_signal_connect(g_bubbleArea, "button-press-event", G_CALLBACK(OnBubbleButtonPress), nullptr);

    // 5. Pre-create Chat Window
    CreateChatWindow();

    // 6. Setup System Tray
    SetupSystemTray();

    // 7. Initial positioning and show
    LayerShell::Move(g_petWindow, initPos.x, initPos.y);
    gtk_widget_show_all(g_petWindow);

    std::cout << "[VirtualPet] Linux companion running smoothly!\n";
    std::cout << "[VirtualPet] Left-click to drag or pet. Double-click or click bubble to chat. Right-click for menu.\n";

    // 8. 60 FPS simulation loop timer (~16ms)
    g_prevTime = Clock::now();
    g_timeout_add(16, SimulationTick, nullptr);

    gtk_main();

    if (g_pet) {
        g_pet->SaveState();
        g_pet.reset();
    }
    return 0;
}

#endif // !_WIN32
