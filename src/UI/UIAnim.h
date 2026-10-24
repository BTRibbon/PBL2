#pragma once

#include "raylib.h"
#include "../datastructures/String.h"

namespace Anim {

enum class ToastType { Success, Error, Info };
enum class ArrowDir { Up, Down, Left, Right };

void BeginFrame();

float Dt();
float EaseOutCubic(float t);
float EaseOutBack(float t);
float Smooth(float current, float target, float speed);
Color LerpColor(Color a, Color b, float t);
Color WithAlpha(Color c, float alpha01);

Rectangle Button(Rectangle bounds, Color base, Color hover, Vector2 mouse, bool enabled = true,
	float roundness = 0.25f, bool drawShadow = true);
Rectangle Button(Rectangle bounds, Color base, Vector2 mouse, bool enabled = true, float roundness = 0.25f);

float HoverAmount(Rectangle bounds);

void DrawArrow(Rectangle bounds, ArrowDir dir, Color color, float size = 5.0f);
Rectangle ArrowButton(Rectangle bounds, ArrowDir dir, Vector2 mouse, bool enabled, Color base);

void InputFocusGlow(Rectangle bounds, bool focused);
float SmoothScroll(Rectangle track, float target);

ToastType Classify(const String& message);
void PushToast(const String& message, ToastType type);
void DrawToasts();
Color ToastColor(ToastType type);
void DrawStatusIcon(Vector2 center, float radius, ToastType type, float progress = 1.0f);

}
