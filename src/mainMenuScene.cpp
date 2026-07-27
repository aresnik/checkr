/*
 * mainMenuScene.cpp
 *
 *      Author: alex@glassoniongames.com
 */

#include "mainMenuScene.h"
#include "appState.h"
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

MainMenuScene::MainMenuScene(AppState *state)
{
    titleLbl.load(state->renderer, state->assets.font, "Checkr", {255, 255, 255, 255});
    titleLbl.setAlignment(Label::ALIGN_CENTER);
    state->assets.setupButtonSVG(onePlayerBtn, "assets/one_player.svg", "assets/one_player.svg", "assets/one_player_filled.svg");
    state->assets.setupButtonSVG(twoPlayerBtn, "assets/two_players.svg", "assets/two_players.svg", "assets/two_players_filled.svg");
    state->assets.setupButtonSVG(resumeBtn, "assets/resume_game.svg", "assets/resume_game.svg", "assets/resume_game_filled.svg");
    if (state->soundEnabled)
        state->assets.setupButtonSVG(soundBtn, "assets/sound_on.svg", "assets/sound_on.svg", "assets/sound_on_filled.svg");
    else
        state->assets.setupButtonSVG(soundBtn, "assets/sound_off.svg", "assets/sound_off.svg", "assets/sound_off_filled.svg");

    state->assets.setupButtonSVG(homePageBtn, "assets/home_page.svg", "assets/home_page.svg", "assets/home_page_filled.svg");
    state->assets.setupButtonSVG(privacyBtn, "assets/privacy.svg", "assets/privacy.svg", "assets/privacy_filled.svg");

    // Visually disable resume button if no game is in progress
    if (!state->savedGameScene)
    {
        resumeBtn.alpha = 0.45f;
        resumeBtn.enabled = false;
    }

    // Hook up button events via callbacks
    onePlayerBtn.setOnClickCallback([state]()
                                    {
        state->pvpMode = false;
        state->nextScene = SceneID::NewGame; });

    twoPlayerBtn.setOnClickCallback([state]()
                                    {
        state->pvpMode = true;
        state->nextScene = SceneID::NewGame; });

    resumeBtn.setOnClickCallback([state]()
                                 {
        if (state->savedGameScene) {
            state->startNewGame = false;
            state->nextScene = SceneID::Game;
        } });

    soundBtn.setOnClickCallback([this, state]()
                                {
        state->soundEnabled = !state->soundEnabled;
        if (state->soundEnabled)
            state->assets.setupButtonSVG(soundBtn, "assets/sound_on.svg", "assets/sound_on.svg", "assets/sound_on_filled.svg");
        else
            state->assets.setupButtonSVG(soundBtn, "assets/sound_off.svg", "assets/sound_off.svg", "assets/sound_off_filled.svg"); });

    homePageBtn.setOnClickCallback([]()
                                   {
#ifndef __EMSCRIPTEN__
                                       SDL_OpenURL("https://glassoniongames.com");
#endif
                                   });

    privacyBtn.setOnClickCallback([]()
                                  {
#ifndef __EMSCRIPTEN__
                                      SDL_OpenURL("https://glassoniongames.com/privacy-policy/");
#endif
                                  });

    // Build the UI tree
    titleHBox.addChild(&spacers[0], 1.0f);
    titleHBox.addChild(&titleLbl, 4.0f); // Adjust this weight to make the title wider or narrower
    titleHBox.addChild(&spacers[1], 1.0f);

    btnVBox.addChild(&onePlayerBtn, 1.0f);
    btnVBox.addChild(&spacers[2], 0.2f);
    btnVBox.addChild(&twoPlayerBtn, 1.0f);
    btnVBox.addChild(&spacers[3], 0.2f);
    btnVBox.addChild(&resumeBtn, 1.0f);
    btnVBox.addChild(&spacers[4], 0.2f);
    btnVBox.addChild(&soundBtn, 1.0f);
    btnVBox.addChild(&spacers[5], 0.2f);
    btnVBox.addChild(&homePageBtn, 1.0f);
    btnVBox.addChild(&spacers[6], 0.2f);
    btnVBox.addChild(&privacyBtn, 1.0f);

    btnWrapperHBox.addChild(&spacers[7], 1.0f);
    btnWrapperHBox.addChild(&btnVBox, 2.0f);
    btnWrapperHBox.addChild(&spacers[8], 1.0f);

    mainVBox.addChild(&spacers[9], 0.8f);
    mainVBox.addChild(&titleHBox, 1.0f);
    mainVBox.addChild(&spacers[10], 0.3f);
    mainVBox.addChild(&btnWrapperHBox, 5.0f);
    mainVBox.addChild(&spacers[11], 1.0f);

    rootStack.addChild(&mainVBox);
}

void MainMenuScene::enter(AppState *state)
{
    // Re-enable the screensaver when on the main menu
    SDL_EnableScreenSaver();
#ifdef __EMSCRIPTEN__
    emscripten_run_script("window.currentSceneId = 0; window.lastSceneTransitionTime = Date.now();");
#endif
}

void MainMenuScene::handleEvent(AppState *state, SDL_Event *event)
{
    bool dummy = false;
    rootStack.handleEvent(event, dummy);
}

void MainMenuScene::update(AppState *state)
{
    rootStack.updateLayout(0, 0, state->screenW, state->screenH);
#ifdef __EMSCRIPTEN__
    EM_ASM(({
        window.homePageBtnBounds = { x: $0, y: $1, w: $2, h: $3 };
        window.privacyBtnBounds = { x: $4, y: $5, w: $6, h: $7 };
    }), 
    homePageBtn.rect.x, homePageBtn.rect.y, homePageBtn.rect.w, homePageBtn.rect.h,
    privacyBtn.rect.x, privacyBtn.rect.y, privacyBtn.rect.w, privacyBtn.rect.h
    );
#endif
}

void MainMenuScene::render(AppState *state)
{
    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
    SDL_RenderClear(state->renderer);

    rootStack.render(state->renderer);

    SDL_RenderPresent(state->renderer);
}