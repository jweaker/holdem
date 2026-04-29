#include "ui/raylib_ui.h"

#include "raymath.h"
#include "rlgl.h"

#include <cmath>
#include <cstdio>

namespace poker::ui {

bool pointIn(Rectangle rect, Vector2 point) {
    return point.x >= rect.x && point.x <= rect.x + rect.width
        && point.y >= rect.y && point.y <= rect.y + rect.height;
}

static Rectangle rectFromCorners(Vector2 a, Vector2 b, Vector2 c, Vector2 d, float padding) {
    float minX = fminf(fminf(a.x, b.x), fminf(c.x, d.x)) - padding;
    float maxX = fmaxf(fmaxf(a.x, b.x), fmaxf(c.x, d.x)) + padding;
    float minY = fminf(fminf(a.y, b.y), fminf(c.y, d.y)) - padding;
    float maxY = fmaxf(fmaxf(a.y, b.y), fmaxf(c.y, d.y)) + padding;
    return Rectangle{minX, minY, maxX - minX, maxY - minY};
}

int timerIndexForSlot(const UiCardSlot &slot) {
    return slot.kind == 1 ? MaxPlayers * 2 + slot.index : slot.player * 2 + slot.index;
}

float clampFloat(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

static float easeOutCubic(float t) {
    t = clampFloat(t, 0.0f, 1.0f);
    float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

static const char *rankFileName(Rank rank) {
    switch (rank) {
        case Two: return "2";
        case Three: return "3";
        case Four: return "4";
        case Five: return "5";
        case Six: return "6";
        case Seven: return "7";
        case Eight: return "8";
        case Nine: return "9";
        case Ten: return "10";
        case Jack: return "jack";
        case Queen: return "queen";
        case King: return "king";
        case Ace: return "ace";
        default: return "unknown";
    }
}

static const char *suitFileName(Suit suit) {
    switch (suit) {
        case Clubs: return "clubs";
        case Diamonds: return "diamonds";
        case Hearts: return "hearts";
        case Spades: return "spades";
        default: return "unknown";
    }
}

Color suitAccent(Suit suit) {
    if (suit == Hearts) return Color{244, 63, 94, 255};
    if (suit == Diamonds) return Color{239, 68, 68, 255};
    if (suit == Clubs) return Color{16, 185, 129, 255};
    if (suit == Spades) return Color{226, 232, 240, 255};
    return Color{148, 163, 184, 255};
}

Font loadUiFont() {
    const char *paths[] = {
        "assets/fonts/Inter.ttf",
        "assets/fonts/Inter-Regular.ttf",
        "/System/Library/Fonts/SFNS.ttf",
        "/System/Library/Fonts/HelveticaNeue.ttc",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };

    for (int i = 0; i < static_cast<int>(sizeof(paths) / sizeof(paths[0])); ++i) {
        if (FileExists(paths[i])) {
            Font font = LoadFontEx(paths[i], 72, 0, 0);
            SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
            return font;
        }
    }

    return GetFontDefault();
}

void drawText(Font font, const char *text, float x, float y, float size, Color color) {
    DrawTextEx(font, text, Vector2{x, y}, size, 0.0f, color);
}

static void compositeImageOverColor(Image &image, Color background) {
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *pixels = static_cast<Color *>(image.data);
    int count = image.width * image.height;
    for (int i = 0; i < count; ++i) {
        unsigned int alpha = pixels[i].a;
        pixels[i].r = static_cast<unsigned char>((pixels[i].r * alpha + background.r * (255 - alpha) + 127) / 255);
        pixels[i].g = static_cast<unsigned char>((pixels[i].g * alpha + background.g * (255 - alpha) + 127) / 255);
        pixels[i].b = static_cast<unsigned char>((pixels[i].b * alpha + background.b * (255 - alpha) + 127) / 255);
        pixels[i].a = 255;
    }
}

static Texture2D loadTextureWithFallback(const char *path, bool opaqueCardFace = false) {
    if (FileExists(path)) {
        Image image = LoadImage(path);
        if (opaqueCardFace) {
            compositeImageOverColor(image, Color{255, 252, 244, 255});
        }
        Texture2D texture = LoadTextureFromImage(image);
        UnloadImage(image);
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        return texture;
    }

    Image image = GenImageColor(320, 448, Color{236, 241, 248, 255});
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    GenTextureMipmaps(&texture);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

CardTextures loadCardTextures() {
    CardTextures textures;
    for (int rank = Two; rank <= Ace; ++rank) {
        for (int suit = Clubs; suit <= Spades; ++suit) {
            char path[160];
            std::snprintf(path, sizeof(path), "assets/cards/%s_of_%s.png",
                          rankFileName(static_cast<Rank>(rank)),
                          suitFileName(static_cast<Suit>(suit)));
            textures.faces[rank][suit] = loadTextureWithFallback(path, true);
        }
    }
    textures.back = loadTextureWithFallback("assets/cards/back.png");
    return textures;
}

void unloadCardTextures(CardTextures &textures) {
    for (int rank = Two; rank <= Ace; ++rank) {
        for (int suit = Clubs; suit <= Spades; ++suit) {
            UnloadTexture(textures.faces[rank][suit]);
        }
    }
    UnloadTexture(textures.back);
}

static Texture2D textureForCard(CardTextures &textures, Card card) {
    if (!card.known) return textures.back;
    return textures.faces[card.rank][card.suit];
}

Card getSlotCard(const GameState &state, const UiCardSlot &slot) {
    if (slot.kind == 1) return state.board[slot.index];
    return slot.index == 0 ? state.players[slot.player].first : state.players[slot.player].second;
}

void setSlotCard(GameState &state, const UiCardSlot &slot, Card card) {
    if (slot.kind == 1) {
        state.board[slot.index] = card;
        return;
    }
    if (slot.index == 0) state.players[slot.player].first = card;
    else state.players[slot.player].second = card;
}

GameState packedForSimulation(const GameState &uiState) {
    GameState state = makeEmptyGameState(uiState.playerCount);
    for (int i = 0; i < uiState.playerCount; ++i) state.players[i] = uiState.players[i];

    state.boardCount = 0;
    for (int i = 0; i < BoardCards; ++i) {
        if (uiState.board[i].known) state.board[state.boardCount++] = uiState.board[i];
    }
    return state;
}

void drawGlassPanel(Rectangle rect, Color tint) {
    DrawRectangleRounded(Rectangle{rect.x + 6, rect.y + 8, rect.width, rect.height}, 0.045f, 16, Color{8, 5, 3, 85});
    DrawRectangleRounded(rect, 0.045f, 16, tint);
    DrawRectangleRoundedLines(rect, 0.045f, 16, Color{255, 236, 194, 58});
}

void drawButton(Font font, Rectangle rect, const char *text, bool hot, Color color) {
    DrawRectangleRounded(Rectangle{rect.x, rect.y + 3, rect.width, rect.height}, 0.16f, 14, Color{8, 5, 3, 88});
    DrawRectangleRounded(rect, 0.16f, 14, hot ? ColorBrightness(color, 0.10f) : color);
    DrawRectangleRoundedLines(rect, 0.16f, 14, Color{255, 244, 214, 62});
    Vector2 size = MeasureTextEx(font, text, 17, 0.0f);
    drawText(font, text, rect.x + rect.width * 0.5f - size.x * 0.5f, rect.y + rect.height * 0.5f - size.y * 0.5f, 17, Color{255, 251, 242, 255});
}

void drawResultsPanel(Font font, Rectangle rect, const GameState &state, const OddsResult &odds, const float displayedWin[], const float displayedTie[], bool running, float spinner) {
    DrawRectangleRounded(Rectangle{rect.x, rect.y + 3, rect.width, rect.height}, 0.06f, 12, Color{8, 5, 3, 72});
    DrawRectangleRounded(rect, 0.06f, 12, Color{20, 17, 14, 220});
    DrawRectangleRoundedLines(rect, 0.06f, 12, Color{255, 236, 194, 48});

    drawText(font, "Results", rect.x + 14.0f, rect.y + 12.0f, 18, Color{255, 251, 242, 255});
    const char *summary = running ? "Simulating..." : (odds.valid && odds.simulationsRun > 0 ? TextFormat("%d sims", odds.simulationsRun) : "Run odds to compare");
    Vector2 summarySize = MeasureTextEx(font, summary, 12, 0.0f);
    drawText(font, summary, rect.x + rect.width - summarySize.x - 14.0f, rect.y + 16.0f, 12, Color{201, 191, 174, 255});

    if (running) {
        Vector2 center = Vector2{rect.x + rect.width - summarySize.x - 30.0f, rect.y + 21.0f};
        DrawRing(center, 5.0f, 7.0f, spinner * 220.0f, spinner * 220.0f + 250.0f, 24, Color{45, 212, 191, 225});
    }

    float listY = rect.y + 42.0f;
    float rowH = (rect.height - 52.0f) / static_cast<float>(MaxPlayers);
    rowH = clampFloat(rowH, 12.0f, 17.0f);
    float barX = rect.x + 46.0f;
    float percentX = rect.x + rect.width - 78.0f;
    float barW = percentX - barX - 12.0f;

    BeginScissorMode(static_cast<int>(rect.x), static_cast<int>(rect.y), static_cast<int>(rect.width), static_cast<int>(rect.height));
    for (int i = 0; i < state.playerCount; ++i) {
        float y = listY + i * rowH;
        float win = clampFloat(displayedWin[i], 0.0f, 100.0f);
        float tie = clampFloat(displayedTie[i], 0.0f, 100.0f);
        Color rowBg = i % 2 == 0 ? Color{255, 244, 214, 12} : Color{255, 244, 214, 6};
        DrawRectangleRounded(Rectangle{rect.x + 10.0f, y - 1.0f, rect.width - 20.0f, rowH - 1.0f}, 0.20f, 8, rowBg);
        drawText(font, TextFormat("P%d", i + 1), rect.x + 16.0f, y + 1.0f, 12, Color{244, 236, 220, 255});
        DrawRectangleRounded(Rectangle{barX, y + 3.0f, barW, 6.0f}, 0.50f, 8, Color{255, 244, 214, 32});
        DrawRectangleRounded(Rectangle{barX, y + 3.0f, barW * (win / 100.0f), 6.0f}, 0.50f, 8, Color{20, 184, 166, 230});
        if (tie > 0.05f) {
            DrawRectangleRounded(Rectangle{barX, y + 10.0f, barW * (tie / 100.0f), 3.0f}, 0.50f, 8, Color{245, 158, 11, 190});
        }
        drawText(font, TextFormat("%.1f%%", win), percentX, y - 1.0f, 12, Color{45, 212, 191, 255});
        drawText(font, TextFormat("T %.1f", tie), percentX + 44.0f, y - 1.0f, 11, Color{201, 191, 174, 255});
    }
    EndScissorMode();
}

void drawCard2D(CardTextures &textures, Card card, Rectangle dest, float rotation, Color tint) {
    Texture2D texture = textureForCard(textures, card);
    Rectangle source = Rectangle{0, 0, static_cast<float>(texture.width), static_cast<float>(texture.height)};
    Vector2 origin = Vector2{dest.width * 0.5f, dest.height * 0.5f};
    Rectangle centered = Rectangle{dest.x + origin.x, dest.y + origin.y, dest.width, dest.height};
    DrawRectangleRounded(Rectangle{dest.x + 4, dest.y + 6, dest.width, dest.height}, 0.06f, 12, Color{2, 6, 23, 52});
    DrawRectangleRounded(dest, 0.06f, 12, Color{255, 252, 244, 255});
    DrawTexturePro(texture, source, centered, origin, rotation, tint);
    DrawRectangleRoundedLines(dest, 0.06f, 12, Color{15, 23, 42, 42});
}

void drawCardSlot(Font font, CardTextures &textures, Rectangle rect, Card card, bool selected, const char *label, float flash) {
    drawText(font, label, rect.x, rect.y - 18, 12, Color{220, 211, 196, 255});
    drawCard2D(textures, card, rect, 0.0f, WHITE);
    if (selected) {
        DrawRectangleRoundedLines(Rectangle{rect.x - 4, rect.y - 4, rect.width + 8, rect.height + 8}, 0.06f, 12, Color{245, 158, 11, 255});
    }
    if (flash > 0.0f) {
        DrawRectangleRounded(rect, 0.06f, 12, Color{245, 158, 11, static_cast<unsigned char>(flash * 120.0f)});
    }
}

void updateCamera(OrbitCamera &orbit, Rectangle worldView, float wheel) {
    Vector2 mouse = GetMousePosition();
    bool inWorld = pointIn(worldView, mouse);
    Vector2 delta = GetMouseDelta();

    if (inWorld && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        orbit.yaw -= delta.x * 0.008f;
        orbit.pitch += delta.y * 0.006f;
    }

    if (inWorld && (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) || (IsKeyDown(KEY_LEFT_SHIFT) && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)))) {
        Vector3 right = Vector3Normalize(Vector3{std::cos(orbit.yaw), 0.0f, -std::sin(orbit.yaw)});
        Vector3 forward = Vector3Normalize(Vector3{std::sin(orbit.yaw), 0.0f, std::cos(orbit.yaw)});
        orbit.target = Vector3Add(orbit.target, Vector3Scale(right, -delta.x * 0.012f));
        orbit.target = Vector3Add(orbit.target, Vector3Scale(forward, delta.y * 0.012f));
    }

    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) orbit.yaw += 0.025f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) orbit.yaw -= 0.025f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) orbit.pitch += 0.018f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) orbit.pitch -= 0.018f;
    if (IsKeyPressed(KEY_R)) {
        orbit.yaw = 0.0f;
        orbit.pitch = 0.96f;
        orbit.distance = 10.8f;
        orbit.target = Vector3{0, 0.18f, -0.20f};
    }

    if (inWorld && wheel != 0.0f) orbit.distance -= wheel * 0.85f;

    orbit.pitch = clampFloat(orbit.pitch, 0.34f, 1.30f);
    orbit.distance = clampFloat(orbit.distance, 7.0f, 20.0f);
    orbit.target.x = clampFloat(orbit.target.x, -3.0f, 3.0f);
    orbit.target.z = clampFloat(orbit.target.z, -2.3f, 2.3f);
}

Camera3D makeCamera(const OrbitCamera &orbit, float time) {
    float cinematicYaw = orbit.yaw + std::sin(time * 0.18f) * 0.018f;
    Camera3D camera;
    camera.position = Vector3{
        orbit.target.x + orbit.distance * std::cos(orbit.pitch) * std::sin(cinematicYaw),
        orbit.target.y + orbit.distance * std::sin(orbit.pitch),
        orbit.target.z + orbit.distance * std::cos(orbit.pitch) * std::cos(cinematicYaw)
    };
    camera.target = orbit.target;
    camera.up = Vector3{0.0f, 1.0f, 0.0f};
    camera.fovy = 37.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    return camera;
}

static void drawFlatQuad(Vector3 center, Vector2 size, float yaw, Color color) {
    Vector3 right = Vector3Normalize(Vector3{std::cos(yaw), 0.0f, -std::sin(yaw)});
    Vector3 forward = Vector3Normalize(Vector3{std::sin(yaw), 0.0f, std::cos(yaw)});
    Vector3 a = Vector3Add(Vector3Add(center, Vector3Scale(right, -size.x * 0.5f)), Vector3Scale(forward, -size.y * 0.5f));
    Vector3 b = Vector3Add(Vector3Add(center, Vector3Scale(right, size.x * 0.5f)), Vector3Scale(forward, -size.y * 0.5f));
    Vector3 c = Vector3Add(Vector3Add(center, Vector3Scale(right, size.x * 0.5f)), Vector3Scale(forward, size.y * 0.5f));
    Vector3 d = Vector3Add(Vector3Add(center, Vector3Scale(right, -size.x * 0.5f)), Vector3Scale(forward, size.y * 0.5f));

    rlSetTexture(0);
    rlBegin(RL_QUADS);
        rlColor4ub(color.r, color.g, color.b, color.a);
        rlVertex3f(a.x, a.y, a.z);
        rlVertex3f(d.x, d.y, d.z);
        rlVertex3f(c.x, c.y, c.z);
        rlVertex3f(b.x, b.y, b.z);
    rlEnd();
}

void drawTexturedQuad(Texture2D texture, Vector3 center, Vector2 size, float yaw, Color tint) {
    Vector3 right = Vector3Normalize(Vector3{std::cos(yaw), 0.0f, -std::sin(yaw)});
    Vector3 forward = Vector3Normalize(Vector3{std::sin(yaw), 0.0f, std::cos(yaw)});
    Vector3 a = Vector3Add(Vector3Add(center, Vector3Scale(right, -size.x * 0.5f)), Vector3Scale(forward, -size.y * 0.5f));
    Vector3 b = Vector3Add(Vector3Add(center, Vector3Scale(right, size.x * 0.5f)), Vector3Scale(forward, -size.y * 0.5f));
    Vector3 c = Vector3Add(Vector3Add(center, Vector3Scale(right, size.x * 0.5f)), Vector3Scale(forward, size.y * 0.5f));
    Vector3 d = Vector3Add(Vector3Add(center, Vector3Scale(right, -size.x * 0.5f)), Vector3Scale(forward, size.y * 0.5f));

    rlSetTexture(texture.id);
    rlBegin(RL_QUADS);
        rlColor4ub(tint.r, tint.g, tint.b, tint.a);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(a.x, a.y, a.z);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(d.x, d.y, d.z);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(c.x, c.y, c.z);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(b.x, b.y, b.z);
    rlEnd();
    rlSetTexture(0);
}

static void drawCard3D(CardTextures &textures, Card card, Vector3 pos, Vector2 size, float yaw, float animation) {
    Texture2D texture = textureForCard(textures, card);
    float deal = easeOutCubic(animation);
    Vector3 from = Vector3{-2.75f, 0.48f, 1.95f};
    Vector3 drawPos = Vector3Lerp(from, pos, deal);
    drawPos.y += (1.0f - deal) * 0.26f;
    Vector2 drawSize = Vector2Scale(size, 0.92f + deal * 0.08f);
    drawFlatQuad(Vector3{drawPos.x, drawPos.y - 0.006f, drawPos.z}, Vector2{drawSize.x + 0.05f, drawSize.y + 0.05f}, yaw, Color{255, 252, 244, 255});
    drawTexturedQuad(texture, drawPos, drawSize, yaw, WHITE);
}

static Rectangle projectCardRect(Camera3D camera, Vector3 pos, Vector2 size, float yaw, int screenWidth, int screenHeight) {
    Vector3 right = Vector3Normalize(Vector3{std::cos(yaw), 0.0f, -std::sin(yaw)});
    Vector3 forward = Vector3Normalize(Vector3{std::sin(yaw), 0.0f, std::cos(yaw)});
    Vector3 a = Vector3Add(Vector3Add(pos, Vector3Scale(right, -size.x * 0.5f)), Vector3Scale(forward, -size.y * 0.5f));
    Vector3 b = Vector3Add(Vector3Add(pos, Vector3Scale(right, size.x * 0.5f)), Vector3Scale(forward, -size.y * 0.5f));
    Vector3 c = Vector3Add(Vector3Add(pos, Vector3Scale(right, size.x * 0.5f)), Vector3Scale(forward, size.y * 0.5f));
    Vector3 d = Vector3Add(Vector3Add(pos, Vector3Scale(right, -size.x * 0.5f)), Vector3Scale(forward, size.y * 0.5f));
    return rectFromCorners(
        GetWorldToScreenEx(a, camera, screenWidth, screenHeight),
        GetWorldToScreenEx(b, camera, screenWidth, screenHeight),
        GetWorldToScreenEx(c, camera, screenWidth, screenHeight),
        GetWorldToScreenEx(d, camera, screenWidth, screenHeight),
        6.0f
    );
}

static Vector3 playerCardPosition(float angle, int cardIndex) {
    float seatX = std::cos(angle) * 3.03f;
    float seatZ = std::sin(angle) * 2.48f;
    Vector3 inward = Vector3{-std::cos(angle), 0.0f, -std::sin(angle)};
    Vector3 lateral = Vector3{-std::sin(angle), 0.0f, std::cos(angle)};
    float spread = cardIndex == 0 ? -0.20f : 0.20f;
    Vector3 base = Vector3Add(Vector3{seatX, 0.255f, seatZ}, Vector3Scale(inward, 0.56f));
    return Vector3Add(base, Vector3Scale(lateral, spread));
}

int buildSceneCardSlots(const GameState &state, Camera3D camera, int screenWidth, int screenHeight, SceneCardSlot sceneSlots[]) {
    int count = 0;
    for (int i = 0; i < state.playerCount; ++i) {
        float angle = (2.0f * PI * i / state.playerCount) - PI / 2.0f;
        for (int h = 0; h < 2; ++h) {
            Vector3 pos = playerCardPosition(angle, h);
            float yaw = -angle + PI * 0.5f;
            sceneSlots[count++] = SceneCardSlot{i * 2 + h, projectCardRect(camera, pos, Vector2{0.36f, 0.53f}, yaw, screenWidth, screenHeight)};
        }
    }

    for (int i = 0; i < BoardCards; ++i) {
        float x = (i - 2) * 0.52f;
        Vector3 pos = Vector3{x, 0.225f, -0.08f};
        sceneSlots[count++] = SceneCardSlot{MaxPlayers * 2 + i, projectCardRect(camera, pos, Vector2{0.41f, 0.60f}, 0.0f, screenWidth, screenHeight)};
    }
    return count;
}

static void drawChipStack(Vector3 base, int chips, Color mainColor) {
    for (int i = 0; i < chips; ++i) {
        float y = base.y + i * 0.035f;
        DrawCylinder(Vector3{base.x, y, base.z}, 0.13f, 0.13f, 0.028f, 32, mainColor);
        DrawCylinderWires(Vector3{base.x, y, base.z}, 0.13f, 0.13f, 0.03f, 32, Color{255, 255, 255, 120});
    }
}

void draw3DScene(const GameState &state, const OddsResult &odds, CardTextures &textures, Camera3D camera, float flashTimer[]) {
    BeginMode3D(camera);

    DrawPlane(Vector3{0, -0.12f, 0}, Vector2{22.0f, 16.0f}, Color{18, 12, 8, 255});
    DrawCylinder(Vector3{0, -0.22f, 0}, 4.05f, 4.95f, 0.36f, 128, Color{47, 24, 13, 255});
    DrawCylinder(Vector3{0, -0.06f, 0}, 3.72f, 4.58f, 0.17f, 128, Color{113, 62, 29, 255});
    DrawCylinder(Vector3{0, 0.025f, 0}, 3.38f, 4.22f, 0.075f, 128, Color{16, 112, 77, 255});
    DrawCylinderWires(Vector3{0, 0.048f, 0}, 3.38f, 4.22f, 0.09f, 128, Color{245, 158, 11, 125});

    DrawCircle3D(Vector3{0, 0.081f, 0}, 1.92f, Vector3{1, 0, 0}, 90.0f, Color{255, 244, 214, 28});
    DrawCircle3D(Vector3{0, 0.084f, 0}, 0.92f, Vector3{1, 0, 0}, 90.0f, Color{3, 44, 34, 70});

    for (int i = 0; i < state.playerCount; ++i) {
        float angle = (2.0f * PI * i / state.playerCount) - PI / 2.0f;
        float x = std::cos(angle) * 3.03f;
        float z = std::sin(angle) * 2.48f;
        DrawCylinder(Vector3{x, 0.13f, z}, 0.36f, 0.42f, 0.10f, 40, Color{42, 28, 20, 255});
        DrawCylinder(Vector3{x, 0.10f, z}, 0.50f, 0.56f, 0.052f, 40, Color{194, 116, 43, 255});

        float oddsScale = odds.valid && odds.simulationsRun > 0 ? static_cast<float>(odds.winPct[i] / 100.0) : 0.15f;
        drawChipStack(Vector3{x + 0.34f, 0.18f, z + 0.25f}, 3 + static_cast<int>(oddsScale * 8.0f), Color{220, 38, 38, 255});
        drawChipStack(Vector3{x + 0.50f, 0.18f, z + 0.16f}, 2 + static_cast<int>(oddsScale * 6.0f), Color{245, 158, 11, 255});

        for (int h = 0; h < 2; ++h) {
            Card card = h == 0 ? state.players[i].first : state.players[i].second;
            int slot = i * 2 + h;
            float anim = card.known ? 1.0f : 0.84f;
            if (flashTimer[slot] > 0.0f) anim = 1.0f - flashTimer[slot] * 0.65f;
            drawCard3D(textures, card, playerCardPosition(angle, h), Vector2{0.36f, 0.53f}, -angle + PI * 0.5f, anim);
        }
    }

    int boardSlotStart = MaxPlayers * 2;
    for (int i = 0; i < BoardCards; ++i) {
        float x = (i - 2) * 0.52f;
        float anim = state.board[i].known ? 1.0f : 0.84f;
        if (flashTimer[boardSlotStart + i] > 0.0f) anim = 1.0f - flashTimer[boardSlotStart + i] * 0.65f;
        drawCard3D(textures, state.board[i], Vector3{x, 0.225f, -0.08f}, Vector2{0.41f, 0.60f}, 0.0f, anim);
    }

    EndMode3D();
}

} // namespace poker::ui
