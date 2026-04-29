#include "core/card.h"
#include "core/game_state.h"
#include "core/simulation.h"
#include "ui/raylib_ui.h"

#include "raylib.h"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <future>
#include <queue>
#include <string>

using namespace poker;
using namespace poker::ui;

struct SimulationJob {
    std::future<OddsResult> future;
    bool running;
    float spinner;
};

static bool cardAlreadySelected(const GameState &state, Card card) {
    if (!card.known) return false;

    for (int i = 0; i < state.playerCount; ++i) {
        if (sameCard(state.players[i].first, card) || sameCard(state.players[i].second, card)) {
            return true;
        }
    }

    for (int i = 0; i < BoardCards; ++i) {
        if (sameCard(state.board[i], card)) {
            return true;
        }
    }

    return false;
}

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1500, 920, "Texas Hold'em Odds Calculator");
    SetTargetFPS(60);

    Font font = loadUiFont();
    CardTextures textures = loadCardTextures();

    GameState state = makeEmptyGameState(2);
    OddsResult odds;
    odds.valid = true;
    odds.simulationsRun = 0;
    odds.error[0] = '\0';
    for (int i = 0; i < MaxPlayers; ++i) {
        odds.winPct[i] = 0.0;
        odds.tiePct[i] = 0.0;
    }

    OrbitCamera orbit;
    orbit.yaw = 0.0f;
    orbit.pitch = 0.96f;
    orbit.distance = 10.8f;
    orbit.target = Vector3{0, 0.18f, -0.20f};

    int selectedSlot = 0;
    int simulations = 10000;
    char status[160] = "Select a slot, then choose a card.";
    UiCardSlot slots[MaxPlayers * 2 + BoardCards];
    std::queue<FlipEvent> flipEvents;
    float flashTimer[MaxPlayers * 2 + BoardCards] = {0};
    float displayedWin[MaxPlayers] = {0};
    float displayedTie[MaxPlayers] = {0};
    SimulationJob simulationJob;
    simulationJob.running = false;
    simulationJob.spinner = 0.0f;
    float pickerScroll = 0.0f;

    while (!WindowShouldClose()) {
        float time = static_cast<float>(GetTime());
        float dt = GetFrameTime();
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        simulationJob.spinner = std::fmod(simulationJob.spinner + dt * 1.45f, 1.0f);

        float leftW = clampFloat(screenWidth * 0.24f, 320.0f, 410.0f);
        float rightW = clampFloat(screenWidth * 0.22f, 300.0f, 370.0f);
        Rectangle worldView = Rectangle{leftW, 0, screenWidth - leftW - rightW, static_cast<float>(screenHeight)};
        Rectangle rightPanel = Rectangle{screenWidth - rightW + 16, 16, rightW - 32, screenHeight - 32.0f};
        float wheel = GetMouseWheelMove();

        updateCamera(orbit, worldView, wheel);
        Camera3D camera = makeCamera(orbit, time);
        SceneCardSlot sceneSlots[MaxPlayers * 2 + BoardCards];
        int sceneSlotCount = buildSceneCardSlots(state, camera, screenWidth, screenHeight, sceneSlots);

        if (simulationJob.running && simulationJob.future.valid()
            && simulationJob.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            odds = simulationJob.future.get();
            simulationJob.running = false;
            if (odds.valid) std::snprintf(status, sizeof(status), "Ran %d simulations.", odds.simulationsRun);
            else std::snprintf(status, sizeof(status), "Error: %s", odds.error);
        }

        for (int i = 0; i < MaxPlayers; ++i) {
            float targetWin = odds.valid && odds.simulationsRun > 0 ? static_cast<float>(odds.winPct[i]) : 0.0f;
            float targetTie = odds.valid && odds.simulationsRun > 0 ? static_cast<float>(odds.tiePct[i]) : 0.0f;
            if (simulationJob.running) {
                targetWin = displayedWin[i] * 0.86f + (8.0f + 7.0f * std::sin(time * 4.0f + i)) * 0.14f;
                targetTie = displayedTie[i] * 0.92f;
            }
            displayedWin[i] += (targetWin - displayedWin[i]) * clampFloat(dt * 8.5f, 0.0f, 1.0f);
            displayedTie[i] += (targetTie - displayedTie[i]) * clampFloat(dt * 8.5f, 0.0f, 1.0f);
        }

        for (int i = 0; i < MaxPlayers * 2 + BoardCards; ++i) {
            if (flashTimer[i] > 0.0f) flashTimer[i] -= dt * 1.9f;
            if (flashTimer[i] < 0.0f) flashTimer[i] = 0.0f;
        }
        if (!flipEvents.empty()) {
            FlipEvent event = flipEvents.front();
            flipEvents.pop();
            if (event.slot >= 0 && event.slot < MaxPlayers * 2 + BoardCards) flashTimer[event.slot] = event.timer;
        }

        Vector2 mouse = GetMousePosition();
        int slotCount = 0;
        float slotW = 42.0f;
        float slotH = 59.0f;
        float playerTop = 126.0f;
        float rowGap = 88.0f;
        int playerColumns = 2;
        int playerRows = (state.playerCount + playerColumns - 1) / playerColumns;
        for (int i = 0; i < state.playerCount; ++i) {
            int row = i / playerColumns;
            int col = i % playerColumns;
            float y = playerTop + row * rowGap;
            float x = 36.0f + col * 126.0f;
            slots[slotCount++] = UiCardSlot{0, i, 0, Rectangle{x, y, slotW, slotH}};
            slots[slotCount++] = UiCardSlot{0, i, 1, Rectangle{x + slotW + 8.0f, y, slotW, slotH}};
        }
        float boardY = playerTop + playerRows * rowGap + 28.0f;
        float boardSlotW = clampFloat((leftW - 92.0f) / 5.0f, 36.0f, 48.0f);
        float boardSlotH = boardSlotW * 1.39f;
        boardY = fminf(boardY, screenHeight - 330.0f);
        for (int i = 0; i < BoardCards; ++i) {
            slots[slotCount++] = UiCardSlot{1, -1, i, Rectangle{36.0f + i * (boardSlotW + 7.0f), boardY, boardSlotW, boardSlotH}};
        }

        Rectangle runButton = Rectangle{34, screenHeight - 328.0f, leftW * 0.47f, 44};
        Rectangle clearButton = Rectangle{runButton.x + runButton.width + 12.0f, screenHeight - 328.0f, leftW - runButton.width - 80.0f, 44};
        Rectangle minusPlayers = Rectangle{34, screenHeight - 268.0f, 42, 36};
        Rectangle plusPlayers = Rectangle{84, screenHeight - 268.0f, 42, 36};
        Rectangle minusSims = Rectangle{34, screenHeight - 218.0f, 42, 36};
        Rectangle plusSims = Rectangle{84, screenHeight - 218.0f, 42, 36};
        Rectangle resultsPanel = Rectangle{34, screenHeight - 156.0f, leftW - 68.0f, 122.0f};
        Rectangle unknownRect = Rectangle{screenWidth - rightW + 30, screenHeight - 86.0f, rightW - 60.0f, 42};
        float rightX = screenWidth - rightW + 30.0f;
        Rectangle pickerViewport = Rectangle{rightX, 126.0f, rightW - 60.0f, unknownRect.y - 146.0f};
        float thumbW = clampFloat((rightW - 84.0f) / 4.0f, 50.0f, 66.0f);
        float thumbH = thumbW * 1.39f;
        float pickerGap = 8.0f;
        float rowStep = thumbH + 10.0f;
        float pickerContentH = 13.0f * rowStep - 10.0f;
        float pickerMaxScroll = pickerContentH > pickerViewport.height ? pickerContentH - pickerViewport.height : 0.0f;
        if (pointIn(rightPanel, mouse) && wheel != 0.0f) {
            pickerScroll = clampFloat(pickerScroll - wheel * 58.0f, 0.0f, pickerMaxScroll);
        } else {
            pickerScroll = clampFloat(pickerScroll, 0.0f, pickerMaxScroll);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            for (int i = 0; i < slotCount; ++i) {
                if (pointIn(slots[i].rect, mouse)) selectedSlot = i;
            }
            if (pointIn(worldView, mouse)) {
                for (int i = sceneSlotCount - 1; i >= 0; --i) {
                    if (pointIn(sceneSlots[i].rect, mouse)) {
                        for (int j = 0; j < slotCount; ++j) {
                            if (timerIndexForSlot(slots[j]) == sceneSlots[i].slot) {
                                selectedSlot = j;
                                std::snprintf(status, sizeof(status), "Selected %s.", slots[j].kind == 1 ? TextFormat("Board %d", slots[j].index + 1) : TextFormat("P%d %s", slots[j].player + 1, slots[j].index == 0 ? "A" : "B"));
                            }
                        }
                    }
                }
            }

            if (pointIn(minusPlayers, mouse) && state.playerCount > 2 && !simulationJob.running) {
                state.playerCount--;
                state = makeEmptyGameState(state.playerCount);
                odds.simulationsRun = 0;
                selectedSlot = 0;
            } else if (pointIn(plusPlayers, mouse) && state.playerCount < MaxPlayers && !simulationJob.running) {
                state.playerCount++;
                state = makeEmptyGameState(state.playerCount);
                odds.simulationsRun = 0;
                selectedSlot = 0;
            } else if (pointIn(minusSims, mouse) && simulations > 1000 && !simulationJob.running) {
                simulations /= 2;
            } else if (pointIn(plusSims, mouse) && simulations < 200000 && !simulationJob.running) {
                simulations *= 2;
            } else if (pointIn(clearButton, mouse) && !simulationJob.running) {
                state = makeEmptyGameState(state.playerCount);
                odds.simulationsRun = 0;
                for (int i = 0; i < MaxPlayers; ++i) {
                    displayedWin[i] = 0.0f;
                    displayedTie[i] = 0.0f;
                }
                std::snprintf(status, sizeof(status), "Cleared cards.");
            } else if (pointIn(runButton, mouse) && !simulationJob.running) {
                GameState simState = packedForSimulation(state);
                int simCount = simulations;
                simulationJob.running = true;
                simulationJob.spinner = 0.0f;
                std::snprintf(status, sizeof(status), "Running %d simulations...", simulations);
                simulationJob.future = std::async(std::launch::async, [simState, simCount]() {
                    return calculateOdds(simState, simCount);
                });
            }

            for (int rank = Ace; rank >= Two; --rank) {
                for (int suit = Clubs; suit <= Spades; ++suit) {
                    int row = Ace - rank;
                    int col = suit - Clubs;
                    Rectangle rect = Rectangle{rightX + col * (thumbW + pickerGap), pickerViewport.y + row * rowStep - pickerScroll, thumbW, thumbH};
                    Card card = makeCard(static_cast<Rank>(rank), static_cast<Suit>(suit));
                    bool disabled = cardAlreadySelected(state, card);
                    if (pointIn(pickerViewport, mouse) && pointIn(rect, mouse) && selectedSlot >= 0 && selectedSlot < slotCount && !simulationJob.running && !disabled) {
                        setSlotCard(state, slots[selectedSlot], card);
                        FlipEvent event;
                        event.slot = slots[selectedSlot].kind == 1 ? MaxPlayers * 2 + slots[selectedSlot].index : slots[selectedSlot].player * 2 + slots[selectedSlot].index;
                        event.timer = 1.0f;
                        flipEvents.push(event);
                        odds.simulationsRun = 0;
                        std::snprintf(status, sizeof(status), "Set slot to %s.", cardToString(getSlotCard(state, slots[selectedSlot])).c_str());
                    }
                }
            }

            if (pointIn(unknownRect, mouse) && selectedSlot >= 0 && selectedSlot < slotCount && !simulationJob.running) {
                setSlotCard(state, slots[selectedSlot], makeUnknownCard());
                odds.simulationsRun = 0;
                std::snprintf(status, sizeof(status), "Set slot to unknown.");
            }
        }

        BeginDrawing();
        ClearBackground(Color{18, 12, 8, 255});
        DrawRectangleGradientV(0, 0, screenWidth, screenHeight, Color{38, 25, 16, 255}, Color{10, 7, 5, 255});
        draw3DScene(state, odds, textures, camera, flashTimer);
        DrawRectangleGradientV(0, 0, screenWidth, screenHeight, Color{255, 255, 255, 0}, Color{8, 5, 3, 70});
        if (selectedSlot >= 0 && selectedSlot < slotCount) {
            int selectedTimer = timerIndexForSlot(slots[selectedSlot]);
            for (int i = 0; i < sceneSlotCount; ++i) {
                if (sceneSlots[i].slot == selectedTimer && pointIn(worldView, Vector2{sceneSlots[i].rect.x + sceneSlots[i].rect.width * 0.5f, sceneSlots[i].rect.y + sceneSlots[i].rect.height * 0.5f})) {
                    DrawRectangleRoundedLines(sceneSlots[i].rect, 0.10f, 8, Color{245, 158, 11, 220});
                }
            }
        }
        if (pointIn(worldView, mouse)) {
            for (int i = 0; i < sceneSlotCount; ++i) {
                if (pointIn(sceneSlots[i].rect, mouse)) {
                    DrawRectangleRoundedLines(sceneSlots[i].rect, 0.10f, 8, Color{255, 244, 214, 160});
                }
            }
        }

        drawGlassPanel(Rectangle{16, 16, leftW - 32, screenHeight - 32.0f}, Color{31, 25, 21, 232});
        drawGlassPanel(rightPanel, Color{31, 25, 21, 232});

        Rectangle cameraBadge = Rectangle{leftW + 24, 24, clampFloat(worldView.width - 48.0f, 320.0f, 620.0f), 38};
        DrawRectangleRounded(cameraBadge, 0.16f, 10, Color{24, 18, 14, 170});
        drawText(font, "Right drag orbit  |  WASD/arrows orbit  |  Wheel zoom  |  Shift+drag pan  |  R reset", cameraBadge.x + 16, cameraBadge.y + 10, 15, Color{244, 236, 220, 255});

        drawText(font, "Hold'em Odds", 34, 40, 31, Color{255, 251, 242, 255});
        drawText(font, status, 34, 82, 14, Color{201, 191, 174, 255});

        for (int i = 0; i < slotCount; ++i) {
            const char *label = slots[i].kind == 1
                ? TextFormat("Board %d", slots[i].index + 1)
                : TextFormat("P%d %s", slots[i].player + 1, slots[i].index == 0 ? "A" : "B");
            int timerIndex = slots[i].kind == 1 ? MaxPlayers * 2 + slots[i].index : slots[i].player * 2 + slots[i].index;
            drawCardSlot(font, textures, slots[i].rect, getSlotCard(state, slots[i]), selectedSlot == i, label, flashTimer[timerIndex]);
        }

        drawButton(font, runButton, simulationJob.running ? "Running..." : "Run odds", pointIn(runButton, mouse) && !simulationJob.running, simulationJob.running ? Color{12, 92, 78, 255} : Color{13, 148, 112, 255});
        drawButton(font, clearButton, "Clear", pointIn(clearButton, mouse) && !simulationJob.running, Color{82, 64, 48, 255});
        drawButton(font, minusPlayers, "-", pointIn(minusPlayers, mouse) && !simulationJob.running, Color{146, 94, 35, 255});
        drawButton(font, plusPlayers, "+", pointIn(plusPlayers, mouse) && !simulationJob.running, Color{146, 94, 35, 255});
        drawText(font, TextFormat("Players: %d", state.playerCount), 142, screenHeight - 261.0f, 16, Color{244, 236, 220, 255});
        drawButton(font, minusSims, "-", pointIn(minusSims, mouse) && !simulationJob.running, Color{146, 94, 35, 255});
        drawButton(font, plusSims, "+", pointIn(plusSims, mouse) && !simulationJob.running, Color{146, 94, 35, 255});
        drawText(font, TextFormat("Sims: %d", simulations), 142, screenHeight - 211.0f, 16, Color{244, 236, 220, 255});

        drawResultsPanel(font, resultsPanel, state, odds, displayedWin, displayedTie, simulationJob.running, simulationJob.spinner);

        drawText(font, "Card Picker", rightX, 40, 31, Color{255, 251, 242, 255});
        drawText(font, "Choose a slot, then pick from the deck.", rightX, 82, 14, Color{201, 191, 174, 255});

        BeginScissorMode(static_cast<int>(pickerViewport.x), static_cast<int>(pickerViewport.y), static_cast<int>(pickerViewport.width), static_cast<int>(pickerViewport.height));
        for (int rank = Ace; rank >= Two; --rank) {
            for (int suit = Clubs; suit <= Spades; ++suit) {
                int row = Ace - rank;
                int col = suit - Clubs;
                Rectangle rect = Rectangle{rightX + col * (thumbW + pickerGap), pickerViewport.y + row * rowStep - pickerScroll, thumbW, thumbH};
                Card card = makeCard(static_cast<Rank>(rank), static_cast<Suit>(suit));
                bool disabled = cardAlreadySelected(state, card);
                bool hot = pointIn(rect, mouse) && !disabled && !simulationJob.running;
                DrawRectangleRounded(Rectangle{rect.x - 3, rect.y - 3, rect.width + 6, rect.height + 6}, 0.08f, 10, hot ? Color{245, 158, 11, 42} : Color{255, 244, 214, 14});
                drawCard2D(textures, card, rect, 0.0f, disabled ? Color{150, 150, 150, 255} : WHITE);
                if (disabled) {
                    DrawRectangleRounded(rect, 0.06f, 12, Color{20, 20, 20, 118});
                    DrawLineEx(Vector2{rect.x + 8.0f, rect.y + rect.height - 8.0f}, Vector2{rect.x + rect.width - 8.0f, rect.y + 8.0f}, 3.0f, Color{235, 235, 235, 150});
                }
                if (hot) DrawRectangleRoundedLines(Rectangle{rect.x - 4, rect.y - 4, rect.width + 8, rect.height + 8}, 0.07f, 10, suitAccent(static_cast<Suit>(suit)));
            }
        }
        EndScissorMode();
        if (pickerMaxScroll > 1.0f) {
            float trackH = pickerViewport.height;
            float thumbScrollH = clampFloat(trackH * (pickerViewport.height / pickerContentH), 42.0f, trackH);
            float thumbY = pickerViewport.y + (pickerScroll / pickerMaxScroll) * (trackH - thumbScrollH);
            DrawRectangleRounded(Rectangle{pickerViewport.x + pickerViewport.width + 8.0f, pickerViewport.y, 4.0f, trackH}, 0.5f, 6, Color{255, 244, 214, 35});
            DrawRectangleRounded(Rectangle{pickerViewport.x + pickerViewport.width + 8.0f, thumbY, 4.0f, thumbScrollH}, 0.5f, 6, Color{245, 158, 11, 145});
        }

        drawButton(font, unknownRect, "Unknown", pointIn(unknownRect, mouse), Color{82, 64, 48, 255});

        if (odds.valid && odds.simulationsRun > 0) {
            Rectangle pill = Rectangle{leftW + worldView.width * 0.5f - 110.0f, screenHeight - 90.0f, 220, 42};
            DrawRectangleRounded(pill, 0.22f, 16, Color{24, 18, 14, 175});
            drawText(font, TextFormat("%d simulations", odds.simulationsRun), pill.x + 18, pill.y + 11, 17, Color{255, 251, 242, 255});
        }

        EndDrawing();
    }

    unloadCardTextures(textures);
    if (font.texture.id != GetFontDefault().texture.id) UnloadFont(font);
    CloseWindow();
    return 0;
}
