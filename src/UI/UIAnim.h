#pragma once

#include "raylib.h"
#include <string>

// Lightweight immediate-mode animation helpers for the raylib UI.
// Every interactive element is identified by its bounds, so callers only need to
// replace DrawRectangleRounded(...) with Anim::Button(...) to get hover, press,
// click-ripple and "deny" (shake) feedback with smooth colour transitions.
namespace Anim {

enum class ToastType { Success, Error, Info };
enum class ArrowDir { Up, Down, Left, Right };

// Call once per frame before drawing.
void BeginFrame();

float Dt();
float EaseOutCubic(float t);
float EaseOutBack(float t);
float Smooth(float current, float target, float speed);
Color LerpColor(Color a, Color b, float t);
Color WithAlpha(Color c, float alpha01);

// Animated button background. Returns the visual rectangle (after lift / press
// scale / shake) so the caller can draw its label inside it.
Rectangle Button(Rectangle bounds, Color base, Color hover, Vector2 mouse, bool enabled = true,
	float roundness = 0.25f, bool drawShadow = true);
Rectangle Button(Rectangle bounds, Color base, Vector2 mouse, bool enabled = true, float roundness = 0.25f);

// Hover progress (0..1) of the element with these bounds (valid after Button()).
float HoverAmount(Rectangle bounds);

// Vector arrow glyph (fonts do not contain triangle glyphs).
void DrawArrow(Rectangle bounds, ArrowDir dir, Color color, float size = 5.0f);
Rectangle ArrowButton(Rectangle bounds, ArrowDir dir, Vector2 mouse, bool enabled, Color base);

// Focus glow for text inputs.
void InputFocusGlow(Rectangle bounds, bool focused);

// Animated scrollbar thumb position (smoothly follows the scroll offset).
float SmoothScroll(Rectangle track, float target);

// Toast notifications (slide in from the right, auto dismiss).
ToastType Classify(const std::string& message);
void PushToast(const std::string& message, ToastType type);
void DrawToasts();
Color ToastColor(ToastType type);
void DrawStatusIcon(Vector2 center, float radius, ToastType type, float progress = 1.0f);

}

