// Interactive Bloch Sphere Simulator IBSS ... ... ... Not a professional... idk how my code works...

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <deque>
#include <vector>

namespace {

constexpr int kWindowW = 1200;
constexpr int kWindowH = 760;
constexpr int kPanelX = 835;
constexpr double kPi = 3.1415926535897932384626433832795;
constexpr float kGateSeconds = 1.05f;

using Complex = std::complex<double>;

struct Qubit {
    Complex alpha{1.0, 0.0};
    Complex beta{0.0, 0.0};
};

struct Bloch {
    double x = 0.0;
    double y = 0.0;
    double z = 1.0;
};

enum class Gate { H, X, Y, Z };

const char* gateName(Gate gate) {
    switch (gate) {
        case Gate::H: return "H";
        case Gate::X: return "X";
        case Gate::Y: return "Y";
        case Gate::Z: return "Z";
    }
    return "?";
}

Qubit applyGate(const Qubit& q, Gate gate) {
    const Complex i{0.0, 1.0};
    switch (gate) {
        case Gate::H: {
            const double s = 1.0 / std::sqrt(2.0);
            return {(q.alpha + q.beta) * s, (q.alpha - q.beta) * s};
        }
        case Gate::X: return {q.beta, q.alpha};
        case Gate::Y: return {-i * q.beta, i * q.alpha};
        case Gate::Z: return {q.alpha, -q.beta};
    }
    return q;
}

//wow... people read this stuff...
Qubit gateAtAngle(const Qubit& start, Gate gate, double angle) {
    const Qubit after = applyGate(start, gate);
    const Complex i{0.0, 1.0};
    const Complex phase = std::polar(1.0, angle / 2.0);
    const double c = std::cos(angle / 2.0);
    const double s = std::sin(angle / 2.0);
    return {
        phase * (c * start.alpha - i * s * after.alpha),
        phase * (c * start.beta  - i * s * after.beta)
    };
}

Bloch toBloch(const Qubit& q) {
    const Complex overlap = std::conj(q.alpha) * q.beta;
    return {2.0 * overlap.real(), 2.0 * overlap.imag(),
            std::norm(q.alpha) - std::norm(q.beta)};
}


Vector3 world(Bloch v) {
    return {static_cast<float>(v.x), static_cast<float>(v.z),
            static_cast<float>(v.y)};
}

Bloch polar(double theta, double phi) {
    return {std::sin(theta) * std::cos(phi),
            std::sin(theta) * std::sin(phi), std::cos(theta)};
}

void drawBlochGrid() {
    const Color grid = {151, 167, 186, 115};
    const Color equator = {100, 119, 145, 180};
    constexpr int segments = 96;

    // Latitude circles: 60 S, 30 S, equator, 30 N, 60 N.
    for (int latitude = -60; latitude <= 60; latitude += 30) {
        const double lat = latitude * kPi / 180.0;
        const double radius = std::cos(lat);
        const double z = std::sin(lat);
        const Color lineColor = latitude == 0 ? equator : grid;
        for (int j = 0; j < segments; ++j) {
            const double a = 2.0 * kPi * j / segments;
            const double b = 2.0 * kPi * (j + 1) / segments;
            DrawLine3D(world({radius * std::cos(a), radius * std::sin(a), z}),
                       world({radius * std::cos(b), radius * std::sin(b), z}),
                       lineColor);
        }
    }


    for (int m = 0; m < 12; ++m) {
        const double phi = 2.0 * kPi * m / 12.0;
        for (int j = 0; j < segments / 2; ++j) {
            const double t0 = kPi * j / (segments / 2);
            const double t1 = kPi * (j + 1) / (segments / 2);
            DrawLine3D(world(polar(t0, phi)), world(polar(t1, phi)), grid);
        }
    }
}

void drawAxes() {
    const Color xColor = {215, 80, 76, 255};
    const Color yColor = {47, 165, 111, 255};
    const Color zColor = {52, 117, 211, 255};
    DrawLine3D(world({-1.32, 0, 0}), world({1.32, 0, 0}), xColor);
    DrawLine3D(world({0, -1.32, 0}), world({0, 1.32, 0}), yColor);
    DrawLine3D(world({0, 0, -1.32}), world({0, 0, 1.32}), zColor);

    
    DrawSphere(world({1.32, 0, 0}), 0.020f, xColor);
    DrawSphere(world({0, 1.32, 0}), 0.020f, yColor);
    DrawSphere(world({0, 0, 1.32}), 0.020f, zColor);
}

void drawStateArrow(Bloch vector) {
    const Vector3 tip = world(vector);
    const Vector3 shaftEnd = {tip.x * 0.82f, tip.y * 0.82f, tip.z * 0.82f};
    const Color shaftColor = {191, 101, 16, 255};
    const Color tipColor = {249, 153, 35, 255};
    DrawCylinderEx({0, 0, 0}, shaftEnd, 0.024f, 0.024f, 12, shaftColor);
    DrawCylinderEx(shaftEnd, tip, 0.070f, 0.0f, 16, tipColor);
    DrawSphere(tip, 0.022f, tipColor);
}

void drawTrail(const std::vector<Bloch>& trail) {
    const Color trailColor = {231, 135, 45, 210};
    for (size_t j = 1; j < trail.size(); ++j) {
        DrawLine3D(world(trail[j - 1]), world(trail[j]), trailColor);
    }
}

void drawText(Font font, const char* label, float x, float y,
              float size, Color color) {
    DrawTextEx(font, label, {x, y}, size, 1.0f, color);
}

void centeredText(Font font, const char* label, Rectangle rect,
                  float size, Color color) {
    const Vector2 dim = MeasureTextEx(font, label, size, 1.0f);
    drawText(font, label,
             rect.x + (rect.width - dim.x) / 2.0f,
             rect.y + (rect.height - dim.y) / 2.0f,
             size, color);
}

bool button(Font font, Rectangle rect, const char* label, bool selected,
            bool enabled = true) {
    const bool hovered = enabled && CheckCollisionPointRec(GetMousePosition(), rect);
    const bool clicked = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    const Color fill = !enabled ? Color{197, 204, 214, 255}
                     : selected ? Color{184, 109, 32, 255}
                     : hovered  ? Color{49, 101, 156, 255}
                                : Color{30, 62, 101, 255};
    DrawRectangleRounded(rect, 0.18f, 8, fill);
    centeredText(font, label, rect, 20.0f, WHITE);
    return clicked;
}

double nearZero(double x) {
    return std::abs(x) < 1e-9 ? 0.0 : x;
}

}  

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(kWindowW, kWindowH, "Bloch Sphere | Quantum Gate Simulator");
    SetTargetFPS(60);


    Font font = GetFontDefault();
    bool customFont = false;
    if (FileExists("C:/Windows/Fonts/segoeui.ttf")) {
        Font loaded = LoadFontEx("C:/Windows/Fonts/segoeui.ttf", 40, nullptr, 0);
        if (loaded.texture.id != 0) {
            font = loaded;
            customFont = true;
            SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
        }
    }

    Qubit state;
    Qubit animationStart;
    Qubit animationEnd;
    Qubit visibleState = state;
    Gate activeGate = Gate::H;
    bool animating = false;
    float elapsed = 0.0f;
    std::deque<Gate> pending;
    std::vector<Bloch> trail;
    trail.push_back(toBloch(state));
    int completedGates = 0;

    float yaw = 0.70f;
    float pitch = 0.33f;
    float cameraDistance = 4.35f;

    while (!WindowShouldClose()) {
        const float dt = std::min(GetFrameTime(), 0.08f);
        const Vector2 mouse = GetMousePosition();

        // Camera movement was a pain...😭
        if (mouse.x < kPanelX) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                const Vector2 delta = GetMouseDelta();
                yaw -= delta.x * 0.007f;
                pitch += delta.y * 0.007f;
            }
            cameraDistance = std::clamp(cameraDistance - GetMouseWheelMove() * 0.25f,
                                        2.75f, 8.0f);
        }
        pitch = std::clamp(pitch, -1.35f, 1.35f);

    
        const Vector3 orbit = {
            cameraDistance * std::cos(pitch) * std::sin(yaw),
            cameraDistance * std::sin(pitch),
            cameraDistance * std::cos(pitch) * std::cos(yaw)
        };
        const Vector3 right = {std::cos(yaw), 0.0f, -std::sin(yaw)};
        const Vector3 target = {right.x * 0.97f, 0.0f, right.z * 0.97f};
        Camera3D camera{};
        camera.target = target;
        camera.position = {target.x + orbit.x, target.y + orbit.y, target.z + orbit.z};
        camera.up = {0, 1, 0};
        camera.fovy = 45.0f;
        camera.projection = CAMERA_PERSPECTIVE;

 
        const Gate gateIds[] = {Gate::H, Gate::X, Gate::Y, Gate::Z};
        const int keys[] = {KEY_H, KEY_X, KEY_Y, KEY_Z};
        for (int j = 0; j < 4; ++j) {
            const Rectangle rect = {855.0f + j * 83.0f, 132.0f, 73.0f, 51.0f};
            if ((CheckCollisionPointRec(mouse, rect) &&
                 IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) || IsKeyPressed(keys[j])) {
                if (pending.size() < 16) pending.push_back(gateIds[j]);
            }
        }

        const Rectangle resetRect = {855, 201, 145, 44};
        const Rectangle demoRect  = {1010, 201, 160, 44};
        const bool resetClicked =
            CheckCollisionPointRec(mouse, resetRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        const bool demoClicked =
            CheckCollisionPointRec(mouse, demoRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        if (resetClicked || IsKeyPressed(KEY_R) || demoClicked || IsKeyPressed(KEY_D)) {
            state = Qubit{};
            visibleState = state;
            animating = false;
            pending.clear();
            trail.clear();
            trail.push_back(toBloch(state));
            completedGates = 0;
            if (demoClicked || IsKeyPressed(KEY_D)) {
                pending = {Gate::H, Gate::Z, Gate::H};
            }
        }

        
        if (!animating && !pending.empty()) {
            activeGate = pending.front();
            pending.pop_front();
            animationStart = state;
            animationEnd = applyGate(state, activeGate);
            elapsed = 0.0f;
            animating = true;
        }

    
        if (animating) {
            elapsed += dt;
            const double t = std::clamp(static_cast<double>(elapsed / kGateSeconds),
                                        0.0, 1.0);
            const double ease = t * t * (3.0 - 2.0 * t);
            visibleState = gateAtAngle(animationStart, activeGate, kPi * ease);
            if (t >= 1.0) {
                state = animationEnd;
                visibleState = state;
                animating = false;
                ++completedGates;
            }
            if (trail.size() >= 1800) trail.erase(trail.begin());
            trail.push_back(toBloch(visibleState));
        } else {
            visibleState = state;
        }

        const Bloch position = toBloch(visibleState);
        const double theta = std::acos(std::clamp(position.z, -1.0, 1.0)) * 180.0 / kPi;
        double phi = std::atan2(position.y, position.x) * 180.0 / kPi;
        if (phi < 0.0) phi += 360.0;
        const bool pole = std::hypot(position.x, position.y) < 1e-7;

        const Color ink = {29, 47, 73, 255};
        const Color muted = {93, 110, 131, 255};
        const Color blue = {41, 88, 146, 255};
        const Color background = {248, 250, 253, 255};

        BeginDrawing();
        ClearBackground(background);


        BeginMode3D(camera);
        drawBlochGrid();
        drawAxes();
        drawTrail(trail);
        drawStateArrow(position);
        EndMode3D();


        struct Label { const char* name; Bloch pos; Color color; };
        const Label labels[] = {
            {"|0>", {0, 0, 1.10}, blue},
            {"|1>", {0, 0, -1.11}, blue},
            {"X",   {1.46, 0, 0}, Color{215, 80, 76, 255}},
            {"Y",   {0, 1.46, 0}, Color{47, 165, 111, 255}},
            {"Z",   {0, 0, 1.48}, blue},
        };
        for (const auto& label : labels) {
            const Vector2 p = GetWorldToScreen(world(label.pos), camera);
            if (p.x >= 0 && p.x < kPanelX - 25 && p.y > 95 && p.y < kWindowH - 60) {
                drawText(font, label.name, p.x, p.y, 22, label.color);
            }
        }

        drawText(font, "BLOCH SPHERE", 28, 22, 30, ink);
        drawText(font, "Single-qubit state and gate rotations", 29, 61, 19, muted);
        drawText(font, "Left-drag: orbit   |   Scroll: zoom", 28, 659, 18, muted);
        drawText(font, "Keyboard: H  X  Y  Z   |   R: reset   |   D: demo", 28, 689, 18, muted);
        drawText(font, TextFormat("Completed gates: %d", completedGates), 28, 724, 17, muted);

     
        DrawRectangle(kPanelX, 0, kWindowW - kPanelX, kWindowH,
                      {231, 238, 247, 255});
        DrawLine(kPanelX, 0, kPanelX, kWindowH, {204, 216, 231, 255});
        drawText(font, "QUANTUM GATES", 855, 28, 27, ink);
        drawText(font, "Click or use the keyboard", 855, 75, 18, muted);

        for (int j = 0; j < 4; ++j) {
            const Rectangle rect = {855.0f + j * 83.0f, 132.0f, 73.0f, 51.0f};
            button(font, rect, gateName(gateIds[j]),
                   animating && activeGate == gateIds[j]);
        }
        button(font, resetRect, "Reset (R)", false);
        button(font, demoRect, "H-Z-H demo", false);

        drawText(font, animating ? TextFormat("Animating: %s", gateName(activeGate))
                                 : "Ready for a gate", 855, 260, 20,
                 animating ? Color{173, 95, 22, 255} : Color{36, 120, 82, 255});
        drawText(font, TextFormat("Queued: %d", (int)pending.size()),
                 1050, 263, 17, muted);

        DrawLine(855, 302, 1170, 302, {199, 212, 229, 255});
        drawText(font, "STATE AMPLITUDES", 855, 319, 22, ink);
        drawText(font, TextFormat("alpha = %+.3f %+.3fi",
                              nearZero(visibleState.alpha.real()),
                              nearZero(visibleState.alpha.imag())),
                 855, 359, 18, ink);
        drawText(font, TextFormat("beta  = %+.3f %+.3fi",
                              nearZero(visibleState.beta.real()),
                              nearZero(visibleState.beta.imag())),
                 855, 387, 18, ink);

        DrawLine(855, 425, 1170, 425, {199, 212, 229, 255});
        drawText(font, "BLOCH COORDINATES", 855, 441, 22, ink);
        drawText(font, TextFormat("x = %+.3f", nearZero(position.x)),
                 855, 478, 18, ink);
        drawText(font, TextFormat("y = %+.3f", nearZero(position.y)),
                 855, 504, 18, ink);
        drawText(font, TextFormat("z = %+.3f", nearZero(position.z)),
                 855, 530, 18, ink);
        drawText(font, TextFormat("theta = %.1f deg", theta), 855, 561, 18, ink);
        drawText(font, pole ? "phi = undefined at pole"
                            : TextFormat("phi = %.1f deg", phi),
                 855, 586, 18, ink);

        DrawLine(855, 626, 1170, 626, {199, 212, 229, 255});
        drawText(font, "MEASUREMENT", 855, 640, 21, ink);
        drawText(font, TextFormat("P(0) = %.1f%%", 100.0 * std::norm(visibleState.alpha)),
                 855, 676, 19, ink);
        drawText(font, TextFormat("P(1) = %.1f%%", 100.0 * std::norm(visibleState.beta)),
                 855, 704, 19, ink);

        EndDrawing();
    }

    if (customFont) UnloadFont(font);
    CloseWindow();
    return 0;
}
