#include "UIFont.h"
#include "UIAnim.h"

#include <cmath>
#include <cstdio>

static Font g_uiFont{};

static Vector<int> generateVietnameseCodepoints() {
	Vector<int> codepoints;
	codepoints.reserve(1024);
	for (int cp = 32; cp <= 126; ++cp) codepoints.push_back(cp);
	for (int cp = 160; cp <= 255; ++cp) codepoints.push_back(cp);
	for (int cp = 256; cp <= 383; ++cp) codepoints.push_back(cp);
	for (int cp = 384; cp <= 591; ++cp) codepoints.push_back(cp);
	for (int cp = 768; cp <= 879; ++cp) codepoints.push_back(cp);
	for (int cp = 8192; cp <= 8303; ++cp) codepoints.push_back(cp);
	for (int cp = 8592; cp <= 8703; ++cp) codepoints.push_back(cp);
	for (int cp = 8352; cp <= 8399; ++cp) codepoints.push_back(cp);
	for (int cp = 7840; cp <= 7935; ++cp) codepoints.push_back(cp);
	codepoints.push_back(10003); // ✓
	codepoints.push_back(10004); // ✔
	return codepoints;
}

static String findCandidateFont() {
#ifdef _WIN32
	const char* candidates[] = {
		"C:/Windows/Fonts/segoeui.ttf",
		"C:/Windows/Fonts/arial.ttf",
		"C:/Windows/Fonts/segoeuib.ttf",
		"C:/Windows/Fonts/tahoma.ttf"
	};
	const int count = 4;
#else
	const char* candidates[] = {
		"/System/Library/Fonts/Supplemental/Arial.ttf",
		"/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
		"/System/Library/Fonts/SFNS.ttf",
		"/System/Library/Fonts/Helvetica.ttc",
		"/System/Library/Fonts/Supplemental/Tahoma.ttf",
		"/System/Library/Fonts/Supplemental/Times New Roman.ttf",
		"/Library/Fonts/Arial.ttf"
	};
	const int count = 7;
#endif
	for (int i = 0; i < count; ++i) {
		if (FileExists(candidates[i])) return String(candidates[i]);
	}
	return String();
}

void InitUiFont() {
	String fontPath = findCandidateFont();
	const Vector<int> codepoints = generateVietnameseCodepoints();
	if (!fontPath.empty()) {
		g_uiFont = LoadFontEx(fontPath.c_str(), 48, const_cast<int*>(codepoints.data()), codepoints.size());
		if (g_uiFont.texture.id != 0) {
			SetTextureFilter(g_uiFont.texture, TEXTURE_FILTER_BILINEAR);
			return;
		}
	}
	std::fprintf(stderr, "Could not load Vietnamese UI font. Falling back to default font.\n");
	g_uiFont = GetFontDefault();
}

void UnloadUiFont() {
	if (g_uiFont.texture.id != 0 && g_uiFont.texture.id != GetFontDefault().texture.id) {
		UnloadFont(g_uiFont);
	}
}

Font GetUiFont() {
	return g_uiFont.texture.id != 0 ? g_uiFont : GetFontDefault();
}

static float effectiveFontSize(float size) {
	return size + 1.0f;
}

void DrawUiText(const char* text, float x, float y, float size, Color color) {
	if (!text || text[0] == '\0') return;
	Font font = GetUiFont();
	float renderSize = effectiveFontSize(size);
	DrawTextEx(font, text, {std::round(x), std::round(y)}, renderSize, 0.4f, color);
}

float MeasureUiText(const char* text, float size) {
	if (!text || text[0] == '\0') return 0.0f;
	Font font = GetUiFont();
	float renderSize = effectiveFontSize(size);
	return MeasureTextEx(font, text, renderSize, 0.4f).x;
}

void DrawUiTextRight(const char* text, float rightX, float y, float size, Color color) {
	float width = MeasureUiText(text, size);
	DrawUiText(text, rightX - width, y, size, color);
}

void DrawUiTextCenter(const char* text, float centerX, float y, float size, Color color) {
	float width = MeasureUiText(text, size);
	DrawUiText(text, centerX - width / 2.0f, y, size, color);
}

void PopUtf8Char(String& text) {
	while (!text.empty()) {
		unsigned char c = static_cast<unsigned char>(text.back());
		text.pop_back();
		if ((c & 0xC0) != 0x80) break;
	}
}

static std::size_t countUtf8Codepoints(const String& text) {
	std::size_t count = 0;
	for (std::size_t i = 0; i < text.size(); ++i) {
		unsigned char c = static_cast<unsigned char>(text[i]);
		if ((c & 0xC0) != 0x80) ++count;
	}
	return count;
}

void AppendUtf8Codepoint(String& text, int codepoint, std::size_t maxChars) {
	if (codepoint < 32) return;
	if (countUtf8Codepoints(text) >= maxChars) return;
	int byteSize = 0;
	const char* utf8 = CodepointToUTF8(codepoint, &byteSize);
	if (utf8 && byteSize > 0) {
		text.append(utf8, static_cast<std::size_t>(byteSize));
	}
}

String FitTextToWidth(const char* text, float maxWidth, float size) {
	if (!text || text[0] == '\0' || maxWidth <= 4.0f) return "";
	String result(text);
	if (MeasureUiText(result.c_str(), size) <= maxWidth) return result;

	const String ellipsis = "...";
	float ellipsisWidth = MeasureUiText(ellipsis.c_str(), size);
	if (ellipsisWidth >= maxWidth) return "";

	while (!result.empty() && MeasureUiText((result + ellipsis).c_str(), size) > maxWidth) {
		PopUtf8Char(result);
	}
	return result + ellipsis;
}

void DrawUiTextFitted(const char* text, float x, float y, float maxWidth, float size, Color color) {
	if (!text || text[0] == '\0' || maxWidth <= 0.0f) return;
	float currentSize = size;
	while (currentSize > maxVal(10.0f, size - 2.0f) && MeasureUiText(text, currentSize) > maxWidth) {
		currentSize -= 0.5f;
	}
	float yOffset = (size - currentSize) * 0.5f;
	if (MeasureUiText(text, currentSize) <= maxWidth) {
		DrawUiText(text, x, y + yOffset, currentSize, color);
	} else {
		String fitted = FitTextToWidth(text, maxWidth, currentSize);
		DrawUiText(fitted.c_str(), x, y + yOffset, currentSize, color);
	}
}

void DrawUiTextCenterFitted(const char* text, float centerX, float y, float maxWidth, float size, Color color) {
	if (!text || text[0] == '\0' || maxWidth <= 0.0f) return;
	float currentSize = size;
	while (currentSize > maxVal(10.0f, size - 2.5f) && MeasureUiText(text, currentSize) > maxWidth) {
		currentSize -= 0.5f;
	}
	float yOffset = (size - currentSize) * 0.5f;
	String fitted = MeasureUiText(text, currentSize) <= maxWidth
		? String(text)
		: FitTextToWidth(text, maxWidth, currentSize);
	float width = MeasureUiText(fitted.c_str(), currentSize);
	DrawUiText(fitted.c_str(), centerX - width / 2.0f, y + yOffset, currentSize, color);
}

void DrawUiTextInRect(const char* text, Rectangle bounds, float size, Color color, int align, float paddingX) {
	if (!text || text[0] == '\0') return;
	float availWidth = bounds.width - paddingX * 2.0f;
	if (availWidth <= 2.0f) return;

	float currentSize = size;
	while (currentSize > maxVal(10.0f, size - 2.5f) && MeasureUiText(text, currentSize) > availWidth) {
		currentSize -= 0.5f;
	}
	String fitted = MeasureUiText(text, currentSize) <= availWidth
		? String(text)
		: FitTextToWidth(text, availWidth, currentSize);

	float textW = MeasureUiText(fitted.c_str(), currentSize);
	float textH = effectiveFontSize(currentSize);
	float y = bounds.y + (bounds.height - textH) / 2.0f;
	float x = bounds.x + paddingX;
	if (align == 0) {
		x = bounds.x + (bounds.width - textW) / 2.0f;
	} else if (align > 0) {
		x = bounds.x + bounds.width - paddingX - textW;
	}
	DrawUiText(fitted.c_str(), x, y, currentSize, color);
}

void DrawUiTextWrapped(const char* text, Rectangle bounds, float size, Color color, float lineSpacing) {
	if (!text || text[0] == '\0' || bounds.width <= 4.0f) return;

	String currentLine;
	float curY = bounds.y;
	float lineHeight = effectiveFontSize(size) + lineSpacing;

	std::size_t i = 0;
	while (text[i] != '\0') {
		while (text[i] == ' ' || text[i] == '\t' || text[i] == '\n' || text[i] == '\r') {
			++i;
		}
		if (text[i] == '\0') break;

		String word;
		while (text[i] != '\0' && text[i] != ' ' && text[i] != '\t' && text[i] != '\n' && text[i] != '\r') {
			word.push_back(text[i]);
			++i;
		}

		String testLine = currentLine.empty() ? word : (currentLine + " " + word);
		if (MeasureUiText(testLine.c_str(), size) <= bounds.width) {
			currentLine = testLine;
		} else {
			if (!currentLine.empty()) {
				if (curY + lineHeight > bounds.y + bounds.height) {
					DrawUiTextFitted((currentLine + " " + word).c_str(), bounds.x, curY, bounds.width, size, color);
					return;
				}
				DrawUiTextFitted(currentLine.c_str(), bounds.x, curY, bounds.width, size, color);
				curY += lineHeight;
				currentLine = word;
			} else {
				DrawUiTextFitted(word.c_str(), bounds.x, curY, bounds.width, size, color);
				curY += lineHeight;
			}
			if (curY > bounds.y + bounds.height) return;
		}
	}
	if (!currentLine.empty() && curY <= bounds.y + bounds.height) {
		DrawUiTextFitted(currentLine.c_str(), bounds.x, curY, bounds.width, size, color);
	}
}

Color BrightenColor(Color c, int delta) {
	return Color{
		static_cast<unsigned char>(clampVal(static_cast<int>(c.r) + delta, 0, 255)),
		static_cast<unsigned char>(clampVal(static_cast<int>(c.g) + delta, 0, 255)),
		static_cast<unsigned char>(clampVal(static_cast<int>(c.b) + delta, 0, 255)),
		c.a
	};
}

void DrawVerticalScrollbar(Rectangle trackBounds, int totalItems, int visibleItems, int scrollOffset) {
	if (totalItems <= visibleItems || visibleItems <= 0) return;
	DrawRectangleRounded(trackBounds, 0.5f, 4, Color{226, 232, 226, 255});

	int maxScroll = maxVal(1, totalItems - visibleItems);
	float ratio = static_cast<float>(visibleItems) / static_cast<float>(totalItems);
	float thumbH = maxVal(28.0f, trackBounds.height * ratio);
	float availH = trackBounds.height - thumbH;
	float targetFrac = static_cast<float>(clampVal(scrollOffset, 0, maxScroll)) / static_cast<float>(maxScroll);
	float frac = Anim::SmoothScroll(trackBounds, targetFrac);
	float thumbY = trackBounds.y + availH * frac;

	Rectangle thumb = {trackBounds.x, thumbY, trackBounds.width, thumbH};
	bool moving = std::fabs(frac - targetFrac) > 0.002f;
	DrawRectangleRounded(thumb, 0.5f, 4, moving ? Color{35, 125, 88, 235} : Color{35, 105, 78, 200});
}

void DrawUiInputBox(Rectangle bounds, const String& value, const char* placeholder, bool isFocused, Vector2 mouse, bool isPassword) {
	bool hovered = CheckCollisionPointRec(mouse, bounds);
	Anim::InputFocusGlow(bounds, isFocused);
	DrawRectangleRounded(bounds, 0.15f, 4, isFocused ? RAYWHITE : (hovered ? Color{252, 254, 252, 255} : Color{248, 250, 248, 255}));
	Color borderCol = isFocused ? Color{35, 105, 78, 255} : (hovered ? Color{140, 165, 150, 255} : Color{205, 214, 206, 255});
	DrawRectangleRoundedLines(bounds, 0.15f, 4, borderCol);

	String displayStr = isPassword ? String(value.size(), '*') : value;
	if (isFocused) {
		bool blink = std::fmod(GetTime(), 1.0) < 0.55;
		if (blink) displayStr += "|";
	}

	if (value.empty() && !isFocused) {
		DrawUiTextInRect(placeholder, bounds, 13, Color{150, 158, 151, 255}, -1, 12.0f);
	} else {
		DrawUiTextInRect(displayStr.c_str(), bounds, 13, Color{38, 52, 45, 255}, -1, 12.0f);
	}
}
