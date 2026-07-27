/*
 * assetManager.h
 *
 *      Author: alex@glassoniongames.com
 */

#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <string>
#include <unordered_map>

class TextureButton;

class AssetManager
{
public:
    std::string basePath;

    // Core Assets
    TTF_Font *font = nullptr;
    TTF_Font *uiFont = nullptr;
    TTF_Font *uiFontSmall = nullptr;

    SDL_Texture *boardTexture = nullptr;
    SDL_Texture *redTexture = nullptr;
    SDL_Texture *blackTexture = nullptr;
    SDL_Texture *redKingTexture = nullptr;
    SDL_Texture *blackKingTexture = nullptr;
    SDL_Texture *legalMoveTexture = nullptr;

    // Audio
    MIX_Audio *moveSfx = nullptr;
    MIX_Audio *captureSfx = nullptr;
    MIX_Audio *winSfx = nullptr;

    std::string getAssetPath(const std::string &relativePath);
    bool loadAssets(SDL_Window *window, SDL_Renderer *renderer, MIX_Mixer *mixer);
    void freeAssets();

    // High-DPI and SVG Support
    void updateDisplayScale(float newScale);
    SDL_Texture *getButtonTexture(const std::string &filepath, int logicalWidth, int logicalHeight);
    void clearCache();
    
    // Bind SVG textures to a button using a generic callback (maintaining project separation)
    void setupButtonSVG(TextureButton &button, const std::string &normalPath, const std::string &hoverPath = "", const std::string &pressedPath = "");

    float getDisplayScale() const { return m_currentDisplayScale; }

    ~AssetManager();

private:
    SDL_Renderer *m_renderer = nullptr;
    float m_currentDisplayScale = 1.0f;
    std::unordered_map<std::string, SDL_Texture *> m_textureCache;

    // Procedural Fallback Generators
    SDL_Texture *createBoardTexture(SDL_Renderer *renderer, int size);
    SDL_Texture *createRectTexture(SDL_Renderer *renderer, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
    SDL_Surface *createIconSurface(int size, Uint8 r, Uint8 g, Uint8 b);
    SDL_Texture *createCircleTexture(SDL_Renderer *renderer, int size, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
};

#endif