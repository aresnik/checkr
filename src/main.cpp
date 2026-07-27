//=================================================================================
// Name        : Checkr
// Author      : Copyright 2026 Glass Onion Games LLC
// Email       : alex@glassoniongames.com
// Version     : 1.30
// License     : MIT License
// Description : A checkers game GUI frontend.
//=================================================================================

#define SDL_MAIN_USE_CALLBACKS 1

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <iostream>
#include <string>
#include <vector>
#include "assetManager.h"
#include "appState.h"
#include "gameScene.h"
#include "mainMenuScene.h"
#include "newGameScene.h"

extern "C" SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Checkr", "1.30", "com.glassoniongames.checkr");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << '\n';
        return SDL_APP_FAILURE;
    }

    // Enable V-Sync to prevent the CPU/GPU from overworking,
    // which solves the overheating issue on mobile devices.
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    AppState *state = new AppState();

    state->window = SDL_CreateWindow(
        "Checkr",
        400,
        800,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

    if (!state->window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << '\n';
        delete state;
        return SDL_APP_FAILURE;
    }

    state->renderer = SDL_CreateRenderer(state->window, nullptr);

    if (!state->renderer)
    {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(state->window);
        delete state;
        return SDL_APP_FAILURE;
    }

    // Initialize the font library
    if (!TTF_Init())
    {
        SDL_Log("TTF_Init Error: %s", SDL_GetError());
        SDL_DestroyRenderer(state->renderer);
        SDL_DestroyWindow(state->window);
        delete state;
        return SDL_APP_FAILURE;
    }

    // SDL3_mixer 3.x Initialization
    if (MIX_Init())
    {
        state->mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    }
    else
    {
        SDL_Log("MIX_Init Error: %s", SDL_GetError());
    }

    // Fetch initial window dimensions
    int w, h;
    SDL_GetWindowSizeInPixels(state->window, &w, &h);
    state->screenW = static_cast<float>(w);
    state->screenH = static_cast<float>(h);

    // Load all visual and audio assets into the manager
    state->assets.loadAssets(state->window, state->renderer, state->mixer);

    // Let the Scene handle creating the board, AI controller, and UI
    state->currentScene = std::make_unique<MainMenuScene>(state);
    state->currentScene->enter(state);
    state->currentSceneId = SceneID::MainMenu;

    *appstate = state;

    return SDL_APP_CONTINUE;
}

extern "C" SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState *state = static_cast<AppState *>(appstate);

    if (event->type == SDL_EVENT_QUIT)
    {
        return SDL_APP_SUCCESS;
    }

    if (event->type == SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED)
    {
        float newScale = SDL_GetWindowDisplayScale(state->window);
        state->assets.updateDisplayScale(newScale);
    }

    // Convert all pointer events (motion and clicks) to the logical 400x800 coordinate system.
    // This ensures hit detection works correctly on High-DPI (Retina) mobile screens.
    if (event->type == SDL_EVENT_MOUSE_MOTION ||
        event->type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
        event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        SDL_ConvertEventToRenderCoordinates(state->renderer, event);

        // Pass converted input to whatever scene is currently active
        if (state->currentScene)
            state->currentScene->handleEvent(state, event);

        return SDL_APP_CONTINUE;
    }

    return SDL_APP_CONTINUE;
}

extern "C" SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *state = static_cast<AppState *>(appstate);

    // Keep screen dimensions continuously updated for responsive UI layout
    int w, h;
    SDL_GetWindowSizeInPixels(state->window, &w, &h);
    state->screenW = static_cast<float>(w);
    state->screenH = static_cast<float>(h);

    // Handle scene transitions safely between frames
    if (state->nextScene != SceneID::None)
    {
        // Stash the game scene securely in memory if we are navigating away from it
        if (state->currentSceneId == SceneID::Game && state->nextScene != SceneID::Game)
        {
            state->savedGameScene = std::move(state->currentScene);
        }

        if (state->nextScene == SceneID::MainMenu)
        {
            state->currentScene = std::make_unique<MainMenuScene>(state);
            state->currentSceneId = SceneID::MainMenu;
        }
        else if (state->nextScene == SceneID::Game)
        {
            if (state->startNewGame || !state->savedGameScene)
            {
                state->savedGameScene.reset(); // Destroy any old stashed game to free memory
                state->currentScene = std::make_unique<GameScene>(state);
            }
            else
            {
                state->currentScene = std::move(state->savedGameScene);
            }
            state->currentSceneId = SceneID::Game;
        }
        else if (state->nextScene == SceneID::NewGame)
        {
            state->currentScene = std::make_unique<NewGameScene>(state);
            state->currentSceneId = SceneID::NewGame;
        }

        if (state->currentScene)
            state->currentScene->enter(state);

        // Reset flag
        state->nextScene = SceneID::None;
    }

    if (state->currentScene)
    {
        state->currentScene->update(state);
        state->currentScene->render(state);
    }

    return SDL_APP_CONTINUE;
}

extern "C" void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    AppState *state = static_cast<AppState *>(appstate);

    if (state)
    {
        // Automatically calls ~GameScene(), which safely shuts down the AI thread!
        state->currentScene.reset();

        // Let the dedicated manager safely handle texture/audio/font destruction
        state->assets.freeAssets();

        if (state->mixer)
            MIX_DestroyMixer(state->mixer);

        MIX_Quit();

        TTF_Quit();

        if (state->renderer)
            SDL_DestroyRenderer(state->renderer);

        if (state->window)
            SDL_DestroyWindow(state->window);

        delete state;
    }
}