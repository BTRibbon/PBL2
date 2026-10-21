#pragma once

#include "raylib.h"
#include <string>
#include <vector>

void InitUiFont();
void UnloadUiFont();
Font GetUiFont();

void DrawUiText(const char* text, float x, float y, float size, Color color);
void DrawUiTextRight(const char* text, float rightX, float y, float size, Color color);
void DrawUiTextCenter(const char* text, float centerX, float y, float size, Color color);
float MeasureUiText(const char* text, float size);

// Bounded text helpers to prevent UI text overflow
std::string FitTextToWidth(const char* text, float maxWidth, float size);
void DrawUiTextFitted(const char* text, float x, float y, float maxWidth, float size, Color color);
void DrawUiTextCenterFitted(const char* text, float centerX, float y, float maxWidth, float size, Color color);
void DrawUiTextInRect(const char* text, Rectangle bounds, float size, Color color, int align = -1, float paddingX = 10.0f);
void DrawUiTextWrapped(const char* text, Rectangle bounds, float size, Color color, float lineSpacing = 4.0f);

// UTF-8 string manipulation helpers
void PopUtf8Char(std::string& text);
void AppendUtf8Codepoint(std::string& text, int codepoint, std::size_t maxChars = 64);
