#ifndef _WIN32

#include "Animation.h"

#include <gdk/gdk.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <cmath>
#include <cctype>

namespace VirtualPet {

size_t Animation::LoadFromDirectory(const std::string& assetsDirectory) {
    namespace fs = std::filesystem;
    size_t loadedCount = 0;

    std::error_code ec;
    if (!fs::exists(assetsDirectory, ec) || !fs::is_directory(assetsDirectory, ec)) {
        return 0;
    }

    const std::vector<std::pair<AnimationState, std::string>> categories = {
        {AnimationState::Idle, "idle"},
        {AnimationState::Walk, "walk"},
        {AnimationState::Run, "run"},
        {AnimationState::Reaction, "reactions"}
    };

    for (const auto& [state, folderName] : categories) {
        fs::path folderPath = fs::path(assetsDirectory) / folderName;
        if (!fs::exists(folderPath, ec) || !fs::is_directory(folderPath, ec)) {
            continue;
        }

        bool isLooping = (state != AnimationState::Reaction);
        const float frameDuration = state == AnimationState::Idle ? 0.25f
                                  : state == AnimationState::Reaction ? 0.18f
                                  : state == AnimationState::Run ? 0.09f : 0.12f;
        AnimationClip clip(folderName, isLooping, frameDuration);

        std::vector<fs::path> imageFiles;
        for (const auto& entry : fs::directory_iterator(folderPath, ec)) {
            if (entry.is_regular_file(ec)) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (ext == ".png" || ext == ".bmp" || ext == ".jpg" || ext == ".jpeg") {
                    imageFiles.push_back(entry.path());
                }
            }
        }

        std::sort(imageFiles.begin(), imageFiles.end());

        for (const auto& filePath : imageFiles) {
            GError* err = nullptr;
            GdkPixbuf* pb = gdk_pixbuf_new_from_file(filePath.string().c_str(), &err);
            if (!pb) {
                if (err) g_error_free(err);
                continue;
            }

            AnimationFrame frame;
            frame.filePath = filePath.string();
            frame.durationSeconds = frameDuration;
            frame.width = gdk_pixbuf_get_width(pb);
            frame.height = gdk_pixbuf_get_height(pb);
            frame.pixbuf = std::shared_ptr<GdkPixbuf>(pb, g_object_unref);
            clip.AddFrame(frame);
        }

        if (!clip.IsEmpty()) {
            RegisterClip(state, std::move(clip));
            loadedCount++;
        }
    }

    struct ClipConfig { float frameDuration; bool looping; };
    static const std::unordered_map<std::string, ClipConfig> kClipTimings = {
        { "reading",              { 0.20f, true  } },
        { "studying",             { 0.20f, true  } },
        { "digital-drawing",      { 0.18f, true  } },
        { "listening",            { 0.18f, false } },
        { "talking",              { 0.14f, false } },
        { "curious-tilt",         { 0.20f, false } },
        { "thoughtful-nod",       { 0.20f, false } },
        { "heart-flutter",        { 0.15f, false } },
        { "shy-blush",            { 0.20f, false } },
        { "happy-spin",           { 0.12f, false } },
        { "proud-pose",           { 0.20f, false } },
        { "warm-smile",           { 0.22f, false } },
        { "shy-affection",        { 0.18f, false } },
        { "asking-for-space",     { 0.22f, false } },
        { "setting-boundary",     { 0.22f, false } },
        { "coffee-craving",       { 0.20f, true  } },
        { "sipping-coffee",       { 0.18f, false } },
        { "comfort-purr",         { 0.25f, true  } },
        { "longing-look",         { 0.30f, false } },
        { "sitting-idle",         { 0.25f, true  } },
        { "resting",              { 0.30f, true  } },
        { "stretch",              { 0.20f, false } },
        { "special",              { 0.18f, false } },
        { "treat",                { 0.18f, false } },
        { "play",                 { 0.15f, false } },
    };

    for (const auto& [name, cfg] : kClipTimings) {
        fs::path folderPath = fs::path(assetsDirectory) / name;
        if (!fs::exists(folderPath, ec) || !fs::is_directory(folderPath, ec)) continue;

        AnimationClip clip(name, cfg.looping, cfg.frameDuration);
        std::vector<fs::path> imageFiles;
        for (const auto& entry : fs::directory_iterator(folderPath, ec)) {
            if (entry.is_regular_file(ec)) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                if (ext == ".png" || ext == ".bmp" || ext == ".jpg" || ext == ".jpeg")
                    imageFiles.push_back(entry.path());
            }
        }
        std::sort(imageFiles.begin(), imageFiles.end());

        for (const auto& filePath : imageFiles) {
            GError* err = nullptr;
            GdkPixbuf* pb = gdk_pixbuf_new_from_file(filePath.string().c_str(), &err);
            if (!pb) {
                if (err) g_error_free(err);
                continue;
            }

            AnimationFrame frame;
            frame.filePath = filePath.string();
            frame.durationSeconds = cfg.frameDuration;
            frame.width = gdk_pixbuf_get_width(pb);
            frame.height = gdk_pixbuf_get_height(pb);
            frame.pixbuf = std::shared_ptr<GdkPixbuf>(pb, g_object_unref);
            clip.AddFrame(frame);
        }

        if (!clip.IsEmpty()) {
            RegisterClip(name, std::move(clip));
            loadedCount++;
        }
    }

    return loadedCount;
}

size_t Animation::LoadFromSpriteSheet(const std::string& sheetPath,
                                      int32_t cols, int32_t rows,
                                      const std::vector<SpriteSheetClipDef>& clips) {
    if (cols <= 0 || rows <= 0 || clips.empty()) return 0;

    GError* err = nullptr;
    GdkPixbuf* sheet = gdk_pixbuf_new_from_file(sheetPath.c_str(), &err);
    if (!sheet) {
        if (err) {
            std::cerr << "[Animation] Failed to load sprite sheet: " << sheetPath
                      << " (" << err->message << ")\n";
            g_error_free(err);
        }
        return 0;
    }

    const int32_t sheetW = gdk_pixbuf_get_width(sheet);
    const int32_t sheetH = gdk_pixbuf_get_height(sheet);
    const int32_t cellW  = sheetW / cols;
    const int32_t cellH  = sheetH / rows;
    if (cellW <= 0 || cellH <= 0) {
        g_object_unref(sheet);
        return 0;
    }

    size_t loadedCount = 0;

    for (const auto& def : clips) {
        if (def.row < 0 || def.row >= rows || def.frameCount <= 0) continue;

        AnimationClip clip(def.clipName, def.looping, def.frameDuration);

        for (int32_t col = 0; col < def.frameCount && col < cols; ++col) {
            GdkPixbuf* sub = gdk_pixbuf_new_subpixbuf(sheet, col * cellW, def.row * cellH, cellW, cellH);
            if (!sub) continue;
            GdkPixbuf* cellCopy = gdk_pixbuf_copy(sub);
            g_object_unref(sub);
            if (!cellCopy) continue;

            AnimationFrame frame;
            frame.durationSeconds = def.frameDuration;
            frame.pixbuf          = std::shared_ptr<GdkPixbuf>(cellCopy, g_object_unref);
            frame.width           = cellW;
            frame.height          = cellH;
            clip.AddFrame(frame);
        }

        if (!clip.IsEmpty()) {
            RegisterClip(def.clipName, std::move(clip));
            loadedCount++;
        }
    }

    g_object_unref(sheet);
    return loadedCount;
}

bool Animation::RenderCairo(cairo_t* cr, int32_t destX, int32_t destY, int32_t destWidth, int32_t destHeight) const {
    if (!cr) return false;

    const AnimationFrame* frame = GetCurrentFrame();
    if (frame && frame->pixbuf) {
        cairo_save(cr);

        int32_t drawW = (m_renderWidth > 0) ? m_renderWidth : destWidth;
        int32_t drawH = (m_renderHeight > 0) ? m_renderHeight : destHeight;

        if (m_facingLeft) {
            cairo_translate(cr, destX + drawW, destY);
            cairo_scale(cr, -1.0, 1.0);
            destX = 0;
            destY = 0;
        }

        double scaleX = static_cast<double>(drawW) / static_cast<double>(frame->width);
        double scaleY = static_cast<double>(drawH) / static_cast<double>(frame->height);

        cairo_save(cr);
        cairo_translate(cr, destX, destY);
        cairo_scale(cr, scaleX, scaleY);
        gdk_cairo_set_source_pixbuf(cr, frame->pixbuf.get(), 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);

        cairo_restore(cr);
        return true;
    }

    // Procedural rendering fallback: vibrant warm cute companion with expressive details!
    cairo_save(cr);

    int32_t drawW = (m_renderWidth > 0) ? m_renderWidth : destWidth;
    int32_t drawH = (m_renderHeight > 0) ? m_renderHeight : destHeight;

    // Tail (swishing gently behind)
    cairo_save(cr);
    cairo_set_source_rgb(cr, 0.95, 0.58, 0.28);
    cairo_set_line_width(cr, 10.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    double tailStartX = destX + (m_facingLeft ? drawW * 0.72 : drawW * 0.28);
    double tailEndX = destX + (m_facingLeft ? drawW * 0.90 : drawW * 0.10);
    cairo_move_to(cr, tailStartX, destY + drawH * 0.78);
    cairo_curve_to(cr, tailEndX, destY + drawH * 0.75, tailEndX, destY + drawH * 0.50, tailStartX + (m_facingLeft ? 20 : -20), destY + drawH * 0.45);
    cairo_stroke(cr);
    cairo_restore(cr);

    // Ears (Outer)
    cairo_set_source_rgb(cr, 0.98, 0.62, 0.30); // Warm soft orange
    // Left ear outer
    cairo_save(cr);
    cairo_translate(cr, destX + drawW * 0.25, destY + drawH * 0.22);
    cairo_scale(cr, drawW * 0.13, drawH * 0.20);
    cairo_arc(cr, 0, 0, 1.0, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_restore(cr);

    // Right ear outer
    cairo_save(cr);
    cairo_translate(cr, destX + drawW * 0.75, destY + drawH * 0.22);
    cairo_scale(cr, drawW * 0.13, drawH * 0.20);
    cairo_arc(cr, 0, 0, 1.0, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_restore(cr);

    // Ears (Inner pink fluff)
    cairo_set_source_rgb(cr, 1.0, 0.75, 0.80); // Pastel pink
    cairo_save(cr);
    cairo_translate(cr, destX + drawW * 0.25, destY + drawH * 0.22);
    cairo_scale(cr, drawW * 0.07, drawH * 0.12);
    cairo_arc(cr, 0, 0, 1.0, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_restore(cr);

    cairo_save(cr);
    cairo_translate(cr, destX + drawW * 0.75, destY + drawH * 0.22);
    cairo_scale(cr, drawW * 0.07, drawH * 0.12);
    cairo_arc(cr, 0, 0, 1.0, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_restore(cr);

    // Body / Head (Cute rounded chibi form)
    cairo_save(cr);
    cairo_translate(cr, destX + drawW * 0.5, destY + drawH * 0.60);
    cairo_scale(cr, drawW * 0.40, drawH * 0.39);
    cairo_arc(cr, 0, 0, 1.0, 0, 2 * M_PI);
    cairo_set_source_rgb(cr, 1.0, 0.68, 0.36);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0.78, 0.45, 0.18);
    cairo_set_line_width(cr, 0.035);
    cairo_stroke(cr);
    cairo_restore(cr);

    // Belly patch (Creamy warm white)
    cairo_save(cr);
    cairo_translate(cr, destX + drawW * 0.5, destY + drawH * 0.69);
    cairo_scale(cr, drawW * 0.25, drawH * 0.26);
    cairo_arc(cr, 0, 0, 1.0, 0, 2 * M_PI);
    cairo_set_source_rgb(cr, 1.0, 0.96, 0.90);
    cairo_fill(cr);
    cairo_restore(cr);

    // Eyes
    double eyeOffset = m_facingLeft ? -4.0 : 4.0;
    cairo_set_source_rgb(cr, 0.14, 0.10, 0.14); // Rich deep obsidian

    // Left eye
    cairo_save(cr);
    cairo_translate(cr, destX + drawW * 0.37 + eyeOffset, destY + drawH * 0.50);
    cairo_scale(cr, 7.0, 9.0);
    cairo_arc(cr, 0, 0, 1.0, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_restore(cr);

    // Right eye
    cairo_save(cr);
    cairo_translate(cr, destX + drawW * 0.63 + eyeOffset, destY + drawH * 0.50);
    cairo_scale(cr, 7.0, 9.0);
    cairo_arc(cr, 0, 0, 1.0, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_restore(cr);

    // Eye primary sparkle (Bright white)
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_arc(cr, destX + drawW * 0.37 + eyeOffset - 2.0, destY + drawH * 0.50 - 3.0, 2.8, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_arc(cr, destX + drawW * 0.63 + eyeOffset - 2.0, destY + drawH * 0.50 - 3.0, 2.8, 0, 2 * M_PI);
    cairo_fill(cr);

    // Eye secondary sparkle
    cairo_arc(cr, destX + drawW * 0.37 + eyeOffset + 2.5, destY + drawH * 0.50 + 3.0, 1.4, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_arc(cr, destX + drawW * 0.63 + eyeOffset + 2.5, destY + drawH * 0.50 + 3.0, 1.4, 0, 2 * M_PI);
    cairo_fill(cr);

    // Cute blush
    cairo_set_source_rgba(cr, 1.0, 0.42, 0.55, 0.45);
    cairo_arc(cr, destX + drawW * 0.27 + eyeOffset, destY + drawH * 0.58, 8.0, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_arc(cr, destX + drawW * 0.73 + eyeOffset, destY + drawH * 0.58, 8.0, 0, 2 * M_PI);
    cairo_fill(cr);

    // Nose & Sweet Smile (:3)
    cairo_set_source_rgb(cr, 0.45, 0.25, 0.20);
    cairo_set_line_width(cr, 2.2);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    // Tiny pink nose
    cairo_save(cr);
    cairo_set_source_rgb(cr, 0.95, 0.55, 0.65);
    cairo_arc(cr, destX + drawW * 0.50 + eyeOffset, destY + drawH * 0.55, 2.5, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_restore(cr);

    // Left lip curve
    cairo_new_sub_path(cr);
    cairo_arc_negative(cr, destX + drawW * 0.50 + eyeOffset - 5.0, destY + drawH * 0.58, 5.0, 0, -M_PI * 0.75);
    cairo_stroke(cr);
    // Right lip curve
    cairo_new_sub_path(cr);
    cairo_arc_negative(cr, destX + drawW * 0.50 + eyeOffset + 5.0, destY + drawH * 0.58, 5.0, -M_PI * 0.25, -M_PI);
    cairo_stroke(cr);

    // Front little paws
    cairo_set_source_rgb(cr, 1.0, 0.96, 0.92);
    // Left paw
    cairo_save(cr);
    cairo_translate(cr, destX + drawW * 0.38, destY + drawH * 0.93);
    cairo_scale(cr, drawW * 0.08, drawH * 0.07);
    cairo_arc(cr, 0, 0, 1.0, 0, 2 * M_PI);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0.78, 0.45, 0.18);
    cairo_set_line_width(cr, 0.08);
    cairo_stroke(cr);
    cairo_restore(cr);

    // Right paw
    cairo_save(cr);
    cairo_translate(cr, destX + drawW * 0.62, destY + drawH * 0.93);
    cairo_scale(cr, drawW * 0.08, drawH * 0.07);
    cairo_arc(cr, 0, 0, 1.0, 0, 2 * M_PI);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0.78, 0.45, 0.18);
    cairo_set_line_width(cr, 0.08);
    cairo_stroke(cr);
    cairo_restore(cr);

    cairo_restore(cr);
    return true;
}

} // namespace VirtualPet

#endif // !_WIN32
