#pragma once

#include "raylib.h"
#include "../datastructures/String.h"
#include "../datastructures/Vector.h"

void InitUiFont();
void UnloadUiFont();
Font GetUiFont();

void DrawUiText(const char* text, float x, float y, float size, Color color);
void DrawUiTextRight(const char* text, float rightX, float y, float size, Color color);
void DrawUiTextCenter(const char* text, float centerX, float y, float size, Color color);
float MeasureUiText(const char* text, float size);

// Các hàm cắt chuỗi & canh lề vừa khung để không tràn chữ
String FitTextToWidth(const char* text, float maxWidth, float size);
void DrawUiTextFitted(const char* text, float x, float y, float maxWidth, float size, Color color);
void DrawUiTextCenterFitted(const char* text, float centerX, float y, float maxWidth, float size, Color color);
void DrawUiTextInRect(const char* text, Rectangle bounds, float size, Color color, int align = -1, float paddingX = 10.0f);
void DrawUiTextWrapped(const char* text, Rectangle bounds, float size, Color color, float lineSpacing = 4.0f);

// Xử lý ký tự tiếng Việt UTF-8
void PopUtf8Char(String& text);
void AppendUtf8Codepoint(String& text, int codepoint, std::size_t maxChars = 64);

// Các thành phần giao diện dùng chung
void DrawVerticalScrollbar(Rectangle trackBounds, int totalItems, int visibleItems, int scrollOffset);
void DrawUiInputBox(Rectangle bounds, const String& value, const char* placeholder, bool isFocused, Vector2 mouse, bool isPassword = false);
Color BrightenColor(Color c, int delta = 18);
