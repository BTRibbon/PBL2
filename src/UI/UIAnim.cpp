#include "UIAnim.h"
#include "UIFont.h"
#include "../datastructures/Vector.h"

#include <cmath>
#include <cstdint>

namespace Anim {
namespace {

struct ButtonState {
	std::uint64_t key = 0;
	float hover = 0.0f;
	float press = 0.0f;
	double clickTime = -10.0;
	double denyTime = -10.0;
	Color color{};
	bool hasColor = false;
	unsigned lastFrame = 0;
};

struct ScalarState {
	std::uint64_t key = 0;
	float value = 0.0f;
	bool init = false;
	unsigned lastFrame = 0;
};

struct Toast {
	String text;
	ToastType type = ToastType::Info;
	double start = 0.0;
	float y = -1.0f;
};

unsigned g_frame = 0;
float g_dt = 1.0f / 60.0f;
double g_now = 0.0;
Vector<ButtonState> g_buttons;
Vector<ScalarState> g_scalars;
Vector<Toast> g_toasts;

constexpr double kToastLife = 3.2;
constexpr double kToastIn = 0.35;
constexpr double kToastOut = 0.35;

std::uint64_t keyOf(Rectangle r, std::uint64_t salt = 0) {
	auto p = [](float v) { return static_cast<std::uint64_t>(static_cast<std::uint16_t>(static_cast<int>(std::lround(v)))); };
	return (p(r.x) | (p(r.y) << 16) | (p(r.width) << 32) | (p(r.height) << 48)) ^ (salt * 0x9E3779B97F4A7C15ull);
}

ButtonState& buttonState(Rectangle r) {
	std::uint64_t k = keyOf(r);
	for (int i = 0; i < g_buttons.size(); ++i) {
		if (g_buttons[i].key == k) {
			g_buttons[i].lastFrame = g_frame;
			return g_buttons[i];
		}
	}
	ButtonState fresh{};
	fresh.key = k;
	fresh.lastFrame = g_frame;
	g_buttons.push_back(fresh);
	return g_buttons.back();
}

ScalarState& scalarState(Rectangle r, std::uint64_t salt) {
	std::uint64_t k = keyOf(r, salt);
	for (int i = 0; i < g_scalars.size(); ++i) {
		if (g_scalars[i].key == k) {
			g_scalars[i].lastFrame = g_frame;
			return g_scalars[i];
		}
	}
	ScalarState fresh{};
	fresh.key = k;
	fresh.lastFrame = g_frame;
	g_scalars.push_back(fresh);
	return g_scalars.back();
}

float luminance(Color c) {
	return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
}

Rectangle inflate(Rectangle r, float d) {
	return {r.x - d, r.y - d, r.width + 2.0f * d, r.height + 2.0f * d};
}

}

void BeginFrame() {
	++g_frame;
	g_dt = clampVal(GetFrameTime(), 0.0f, 0.05f);
	g_now = GetTime();
	if (g_frame % 240 == 0) {
		for (int i = g_buttons.size() - 1; i >= 0; --i) {
			if (g_frame - g_buttons[i].lastFrame > 240) {
				g_buttons.eraseAt(i);
			}
		}
		for (int i = g_scalars.size() - 1; i >= 0; --i) {
			if (g_frame - g_scalars[i].lastFrame > 240) {
				g_scalars.eraseAt(i);
			}
		}
	}
}

float Dt() { return g_dt; }

float EaseOutCubic(float t) {
	t = clampVal(t, 0.0f, 1.0f);
	float f = 1.0f - t;
	return 1.0f - f * f * f;
}

float EaseOutBack(float t) {
	t = clampVal(t, 0.0f, 1.0f);
	const float c1 = 1.70158f;
	const float c3 = c1 + 1.0f;
	float f = t - 1.0f;
	return 1.0f + c3 * f * f * f + c1 * f * f;
}

float Smooth(float current, float target, float speed) {
	float k = 1.0f - std::exp(-speed * g_dt);
	return current + (target - current) * k;
}

Color LerpColor(Color a, Color b, float t) {
	t = clampVal(t, 0.0f, 1.0f);
	auto l = [t](unsigned char x, unsigned char y) {
		return static_cast<unsigned char>(std::lround(x + (static_cast<float>(y) - x) * t));
	};
	return {l(a.r, b.r), l(a.g, b.g), l(a.b, b.b), l(a.a, b.a)};
}

Color WithAlpha(Color c, float alpha01) {
	c.a = static_cast<unsigned char>(clampVal(alpha01, 0.0f, 1.0f) * c.a);
	return c;
}

Rectangle Button(Rectangle bounds, Color base, Color hover, Vector2 mouse, bool enabled,
	float roundness, bool drawShadow) {
	ButtonState& s = buttonState(bounds);
	bool over = CheckCollisionPointRec(mouse, bounds);
	bool hovered = enabled && over;

	s.hover = Smooth(s.hover, hovered ? 1.0f : 0.0f, 14.0f);
	s.press = Smooth(s.press, (hovered && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) ? 1.0f : 0.0f, 32.0f);
	if (over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		if (enabled) s.clickTime = g_now;
		else s.denyTime = g_now;
	}

	Color target = enabled ? LerpColor(base, hover, s.hover) : Color{190, 199, 191, 255};
	if (!s.hasColor) {
		s.color = target;
		s.hasColor = true;
	} else {
		s.color = LerpColor(s.color, target, 1.0f - std::exp(-16.0f * g_dt));
	}

	float lift = enabled ? -1.5f * s.hover : 0.0f;
	float scale = 1.0f - 0.04f * s.press;
	float shake = 0.0f;
	double denyAge = g_now - s.denyTime;
	if (denyAge < 0.4) {
		shake = static_cast<float>(std::sin(denyAge * 55.0) * 6.0 * (1.0 - denyAge / 0.4));
	}

	Rectangle v = {
		bounds.x + bounds.width * (1.0f - scale) * 0.5f + shake,
		bounds.y + bounds.height * (1.0f - scale) * 0.5f + lift + s.press * 1.0f,
		bounds.width * scale,
		bounds.height * scale
	};

	if (drawShadow && enabled && s.hover > 0.02f) {
		DrawRectangleRounded({v.x + 0.5f, v.y + 3.0f, v.width, v.height}, roundness, 6,
			Color{20, 40, 30, static_cast<unsigned char>(34.0f * s.hover * (1.0f - 0.6f * s.press))});
	}
	DrawRectangleRounded(v, roundness, 6, s.color);

	double clickAge = g_now - s.clickTime;
	if (clickAge < 0.45) {
		float e = EaseOutCubic(static_cast<float>(clickAge / 0.45));
		Color ringCol = luminance(base) > 170.0f ? Color{35, 105, 78, 255} : base;
		DrawRectangleRoundedLinesEx(inflate(v, 2.0f + 9.0f * e), roundness, 6, 2.0f, WithAlpha(ringCol, 0.6f * (1.0f - e)));
		DrawRectangleRounded(v, roundness, 6, WithAlpha(WHITE, 0.32f * (1.0f - e)));
	}
	if (denyAge < 0.4) {
		float a = static_cast<float>(1.0 - denyAge / 0.4);
		DrawRectangleRoundedLinesEx(inflate(v, 1.0f), roundness, 6, 2.0f, WithAlpha(Color{215, 60, 50, 255}, a));
	}
	return v;
}

Rectangle Button(Rectangle bounds, Color base, Vector2 mouse, bool enabled, float roundness) {
	return Button(bounds, base, BrightenColor(base, 22), mouse, enabled, roundness, true);
}

float HoverAmount(Rectangle bounds) {
	std::uint64_t k = keyOf(bounds);
	for (int i = 0; i < g_buttons.size(); ++i) {
		if (g_buttons[i].key == k) return g_buttons[i].hover;
	}
	return 0.0f;
}

void DrawArrow(Rectangle b, ArrowDir dir, Color color, float size) {
	float cx = b.x + b.width * 0.5f;
	float cy = b.y + b.height * 0.5f;
	float s = size;
	float h = size * 0.62f;
	switch (dir) {
	case ArrowDir::Up:
		DrawTriangle({cx, cy - h}, {cx - s, cy + h}, {cx + s, cy + h}, color);
		break;
	case ArrowDir::Down:
		DrawTriangle({cx, cy + h}, {cx + s, cy - h}, {cx - s, cy - h}, color);
		break;
	case ArrowDir::Left:
		DrawTriangle({cx - h, cy}, {cx + h, cy + s}, {cx + h, cy - s}, color);
		break;
	case ArrowDir::Right:
		DrawTriangle({cx + h, cy}, {cx - h, cy - s}, {cx - h, cy + s}, color);
		break;
	}
}

Rectangle ArrowButton(Rectangle bounds, ArrowDir dir, Vector2 mouse, bool enabled, Color base) {
	Rectangle v = Button(bounds, base, mouse, enabled, 0.3f);
	float size = clampVal(minVal(v.width, v.height) * 0.22f, 3.5f, 6.5f);
	DrawArrow(v, dir, enabled ? RAYWHITE : Color{240, 243, 240, 255}, size);
	return v;
}

void InputFocusGlow(Rectangle bounds, bool focused) {
	ScalarState& s = scalarState(bounds, 77);
	s.value = Smooth(s.value, focused ? 1.0f : 0.0f, 14.0f);
	if (s.value < 0.02f) return;
	DrawRectangleRoundedLinesEx(inflate(bounds, 1.5f + 1.5f * s.value), 0.2f, 6, 3.0f,
		WithAlpha(Color{35, 105, 78, 255}, 0.22f * s.value));
}

float SmoothScroll(Rectangle track, float target) {
	ScalarState& s = scalarState(track, 991);
	if (!s.init) {
		s.value = target;
		s.init = true;
	} else {
		s.value = Smooth(s.value, target, 18.0f);
	}
	return s.value;
}

ToastType Classify(const String& m) {
	static const char* errorWords[] = {
		"Sai ", "Không", "không thể", "không hợp lệ", "Vui lòng", "đã tồn tại", "rỗng", "trống!",
		"phải lớn", "Lỗi", "không tìm thấy", "trước đó", "Chưa"
	};
	for (const char* w : errorWords) {
		if (m.find(w) != String::npos) return ToastType::Error;
	}
	if (m.startsWith("Đã") || m.find("thành công") != String::npos) return ToastType::Success;
	return ToastType::Info;
}

Color ToastColor(ToastType type) {
	switch (type) {
	case ToastType::Success: return {35, 135, 90, 255};
	case ToastType::Error: return {205, 70, 55, 255};
	default: return {60, 110, 150, 255};
	}
}

void PushToast(const String& message, ToastType type) {
	if (message.empty()) return;
	for (int i = 0; i < g_toasts.size(); ++i) {
		Toast& t = g_toasts[i];
		if (t.text == message && g_now - t.start < kToastLife - kToastOut) {
			t.start = g_now - kToastIn;
			return;
		}
	}
	Toast fresh{message, type, g_now, -1.0f};
	g_toasts.push_back(fresh);
	if (g_toasts.size() > 4) g_toasts.eraseAt(0);
}

void DrawStatusIcon(Vector2 c, float r, ToastType type, float progress) {
	Color col = ToastColor(type);
	float pop = EaseOutBack(progress);
	DrawCircleV(c, r * pop, col);
	float p = clampVal(progress * 1.4f, 0.0f, 1.0f);
	if (type == ToastType::Success) {
		Vector2 a{c.x - r * 0.45f, c.y + r * 0.02f};
		Vector2 b{c.x - r * 0.12f, c.y + r * 0.35f};
		Vector2 d{c.x + r * 0.5f, c.y - r * 0.35f};
		float p1 = clampVal(p / 0.4f, 0.0f, 1.0f);
		float p2 = clampVal((p - 0.4f) / 0.6f, 0.0f, 1.0f);
		DrawLineEx(a, {a.x + (b.x - a.x) * p1, a.y + (b.y - a.y) * p1}, 2.4f, RAYWHITE);
		if (p2 > 0.0f) DrawLineEx(b, {b.x + (d.x - b.x) * p2, b.y + (d.y - b.y) * p2}, 2.4f, RAYWHITE);
	} else if (type == ToastType::Error) {
		float k = r * 0.36f * p;
		DrawLineEx({c.x - k, c.y - k}, {c.x + k, c.y + k}, 2.4f, RAYWHITE);
		DrawLineEx({c.x + k, c.y - k}, {c.x - k, c.y + k}, 2.4f, RAYWHITE);
	} else {
		DrawCircleV({c.x, c.y - r * 0.38f}, 1.6f * p, RAYWHITE);
		DrawLineEx({c.x, c.y - r * 0.1f}, {c.x, c.y - r * 0.1f + r * 0.5f * p}, 2.4f, RAYWHITE);
	}
}

void DrawToasts() {
	for (int i = g_toasts.size() - 1; i >= 0; --i) {
		if (g_now - g_toasts[i].start > kToastLife) {
			g_toasts.eraseAt(i);
		}
	}

	const float rightEdge = 1262.0f;
	const float baseY = 100.0f;
	const float height = 46.0f;
	for (int i = 0; i < g_toasts.size(); ++i) {
		Toast& t = g_toasts[g_toasts.size() - 1 - i];
		double age = g_now - t.start;
		float inT = EaseOutBack(static_cast<float>(age / kToastIn));
		float outT = age > kToastLife - kToastOut
			? EaseOutCubic(static_cast<float>((kToastLife - age) / kToastOut))
			: 1.0f;
		float vis = clampVal(minVal(inT, outT), 0.0f, 1.0f);

		float textW = MeasureUiText(t.text.c_str(), 13);
		float width = clampVal(textW + 64.0f, 240.0f, 520.0f);
		float targetY = baseY + static_cast<float>(i) * (height + 10.0f);
		t.y = t.y < 0.0f ? targetY : Smooth(t.y, targetY, 16.0f);
		float slideX = (1.0f - inT) * (width + 30.0f) + (1.0f - outT) * 60.0f;
		Rectangle box = {rightEdge - width + slideX, t.y, width, height};

		Color accent = ToastColor(t.type);
		DrawRectangleRounded({box.x + 2, box.y + 5, box.width, box.height}, 0.22f, 6, WithAlpha(BLACK, 0.18f * vis));
		DrawRectangleRounded(box, 0.22f, 6, WithAlpha(Color{252, 251, 247, 255}, 0.98f * vis));
		DrawRectangleRoundedLinesEx(box, 0.22f, 6, 1.5f, WithAlpha(accent, 0.7f * vis));
		DrawRectangleRounded({box.x + 5, box.y + 7, 5, box.height - 14}, 0.5f, 4, WithAlpha(accent, vis));

		DrawStatusIcon({box.x + 26, box.y + box.height * 0.5f}, 10.0f, t.type, vis);
		Rectangle textRec = {box.x + 44, box.y, box.width - 52, box.height};
		DrawUiTextInRect(t.text.c_str(), textRec, 13, WithAlpha(Color{35, 50, 44, 255}, vis), -1, 4.0f);

		float lifeFrac = clampVal(static_cast<float>(1.0 - age / kToastLife), 0.0f, 1.0f);
		DrawRectangleRounded({box.x + 12, box.y + box.height - 4, (box.width - 24) * lifeFrac, 2.5f},
			0.5f, 4, WithAlpha(accent, 0.55f * vis));
	}
}

}
