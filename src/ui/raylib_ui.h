#ifndef POKER_UI_RAYLIB_UI_H
#define POKER_UI_RAYLIB_UI_H

#include "core/card.h"
#include "core/game_state.h"
#include "core/simulation.h"

#include "raylib.h"

namespace poker::ui {

struct UiCardSlot {
    int kind; // 0 player, 1 board
    int player;
    int index;
    Rectangle rect;
};

struct FlipEvent {
    int slot;
    float timer;
};

struct CardTextures {
    Texture2D faces[15][5];
    Texture2D back;
};

struct OrbitCamera {
    float yaw;
    float pitch;
    float distance;
    Vector3 target;
};

struct SceneCardSlot {
    int slot;
    Rectangle rect;
};

bool pointIn(Rectangle rect, Vector2 point);
int timerIndexForSlot(const UiCardSlot &slot);
float clampFloat(float value, float minValue, float maxValue);
Color suitAccent(Suit suit);
Font loadUiFont();
void drawText(Font font, const char *text, float x, float y, float size, Color color);
CardTextures loadCardTextures();
void unloadCardTextures(CardTextures &textures);
Card getSlotCard(const GameState &state, const UiCardSlot &slot);
void setSlotCard(GameState &state, const UiCardSlot &slot, Card card);
GameState packedForSimulation(const GameState &uiState);
void drawGlassPanel(Rectangle rect, Color tint);
void drawButton(Font font, Rectangle rect, const char *text, bool hot, Color color);
void drawResultsPanel(Font font, Rectangle rect, const GameState &state, const OddsResult &odds, const float displayedWin[], const float displayedTie[], bool running, float spinner);
void drawCard2D(CardTextures &textures, Card card, Rectangle dest, float rotation, Color tint);
void drawCardSlot(Font font, CardTextures &textures, Rectangle rect, Card card, bool selected, const char *label, float flash);
void updateCamera(OrbitCamera &orbit, Rectangle worldView, float wheel);
Camera3D makeCamera(const OrbitCamera &orbit, float time);
int buildSceneCardSlots(const GameState &state, Camera3D camera, int screenWidth, int screenHeight, SceneCardSlot sceneSlots[]);
void draw3DScene(const GameState &state, const OddsResult &odds, CardTextures &textures, Camera3D camera, float flashTimer[]);

} // namespace poker::ui

#endif
