#include "MouseSensor.h"

#include <cmath>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace VirtualPet {

MouseSensor::MouseSensor(const MouseSensorConfig& config)
    : m_config(config) {
    PollOSCursor();
    m_lastPolledPosition = m_state.screenPosition;
}

void MouseSensor::PollOSCursor() {
#ifdef _WIN32
    POINT pt;
    if (::GetCursorPos(&pt)) {
        m_state.screenPosition = Point(pt.x, pt.y);
    }
    m_state.leftButtonDown = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    m_state.rightButtonDown = (::GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
#endif
}

void MouseSensor::ResetFrameState() {
    m_state.positionDelta = Point(0, 0);
    m_state.leftButtonClicked = false;
    m_state.rightButtonClicked = false;
}

void MouseSensor::Update(float deltaTime, const Rect& petBounds) {
    (void)deltaTime;

    // Reset single-frame triggers
    m_state.leftButtonClicked = false;
    m_state.rightButtonClicked = false;

    // Poll system cursor position and button states
    PollOSCursor();

    // Calculate position delta since last frame
    m_state.positionDelta.x = m_state.screenPosition.x - m_lastPolledPosition.x;
    m_state.positionDelta.y = m_state.screenPosition.y - m_lastPolledPosition.y;
    m_lastPolledPosition = m_state.screenPosition;

    // Calculate position relative to pet window top-left
    m_state.petRelativePosition.x = m_state.screenPosition.x - petBounds.x;
    m_state.petRelativePosition.y = m_state.screenPosition.y - petBounds.y;

    // Distance from cursor to pet center
    const Point center = petBounds.Center();
    const float dx = static_cast<float>(m_state.screenPosition.x - center.x);
    const float dy = static_cast<float>(m_state.screenPosition.y - center.y);
    m_state.distanceToPet = std::hypot(dx, dy);

    // Evaluate proximity & hover
    m_state.isHovering = petBounds.Contains(m_state.screenPosition);
    m_state.isNear = (m_state.distanceToPet <= m_config.proximityDistancePx);

    // Evaluate drag interaction
    if (m_config.detectCursorDrag) {
        if (m_state.leftButtonDown) {
            if (!m_state.isDragging && m_state.isHovering && !m_trackingClick) {
                m_trackingClick = true;
                m_clickStartPos = m_state.screenPosition;
                m_state.dragOffset = m_state.petRelativePosition;
            } else if (m_trackingClick && !m_state.isDragging) {
                const float dragDx = static_cast<float>(m_state.screenPosition.x - m_clickStartPos.x);
                const float dragDy = static_cast<float>(m_state.screenPosition.y - m_clickStartPos.y);
                if (std::hypot(dragDx, dragDy) >= m_config.dragThresholdPx) {
                    m_state.isDragging = true;
                }
            }
        } else {
            if (m_trackingClick && !m_state.isDragging) {
                m_state.leftButtonClicked = true;
            }
            m_state.isDragging = false;
            m_trackingClick = false;
        }
    }

    // Right click detection
    if (!m_state.rightButtonDown && m_prevRightButtonDown && m_state.isHovering) {
        m_state.rightButtonClicked = true;
    }

    m_prevLeftButtonDown = m_state.leftButtonDown;
    m_prevRightButtonDown = m_state.rightButtonDown;
}

void MouseSensor::OnButtonDown(bool leftButton, const Point& screenPos, const Rect& petBounds) {
    m_state.screenPosition = screenPos;
    if (leftButton) {
        m_state.leftButtonDown = true;
        if (petBounds.Contains(screenPos)) {
            m_trackingClick = true;
            m_clickStartPos = screenPos;
            m_state.dragOffset = Point(screenPos.x - petBounds.x, screenPos.y - petBounds.y);
        }
    } else {
        m_state.rightButtonDown = true;
    }
}

void MouseSensor::OnButtonUp(bool leftButton, const Point& screenPos) {
    m_state.screenPosition = screenPos;
    if (leftButton) {
        m_state.leftButtonDown = false;
        if (m_trackingClick && !m_state.isDragging) {
            m_state.leftButtonClicked = true;
        }
        m_state.isDragging = false;
        m_trackingClick = false;
    } else {
        m_state.rightButtonDown = false;
        m_state.rightButtonClicked = true;
    }
}

void MouseSensor::OnMouseMove(const Point& screenPos, const Rect& petBounds) {
    m_state.screenPosition = screenPos;
    m_state.petRelativePosition.x = screenPos.x - petBounds.x;
    m_state.petRelativePosition.y = screenPos.y - petBounds.y;

    if (m_trackingClick && !m_state.isDragging && m_config.detectCursorDrag) {
        const float dx = static_cast<float>(screenPos.x - m_clickStartPos.x);
        const float dy = static_cast<float>(screenPos.y - m_clickStartPos.y);
        if (std::hypot(dx, dy) >= m_config.dragThresholdPx) {
            m_state.isDragging = true;
        }
    }
}

} // namespace VirtualPet
