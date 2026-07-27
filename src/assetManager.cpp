/*
 * assetManager.cpp
 *
 *      Author: alex@glassoniongames.com
 */

#include "assetManager.h"
#include "textureButton.h"

std::string AssetManager::getAssetPath(const std::string &relativePath)
{
#if defined(SDL_PLATFORM_ANDROID)
    // On Android, the 'assets' folder in the project becomes the root of the APK.
    // If the path starts with "assets/", we strip it to match the APK structure.
    std::string path = relativePath;
    std::string prefix = "assets/";
    if (path.compare(0, prefix.length(), prefix) == 0)
    {
        return path.substr(prefix.length());
    }
    return path;
#else
#ifdef PROJECT_ROOT
    if (basePath.empty())
    {
        basePath = PROJECT_ROOT;
        basePath += "/";
    }
#endif
    if (basePath.empty())
    {
        const char *base = SDL_GetBasePath();
        if (base)
        {
            std::string baseStr = base;
            std::string currentCheck = baseStr;
            bool foundRoot = false;

            for (int depth = 0; depth < 3; ++depth)
            {
                std::string markerPath = currentCheck + "assets/board.png";
                SDL_IOStream *io = SDL_IOFromFile(markerPath.c_str(), "rb");
                if (io)
                {
                    basePath = currentCheck;
                    SDL_CloseIO(io);
                    foundRoot = true;
                    break;
                }
                if (currentCheck.length() <= 1)
                    break;
                size_t last = currentCheck.find_last_of("\\/", currentCheck.length() - 2);
                if (last == std::string::npos)
                    break;
                currentCheck = currentCheck.substr(0, last + 1);
            }
            if (!foundRoot)
            {
                basePath = baseStr;
                SDL_Log("Asset marker not found. Falling back to base path: %s", basePath.c_str());
            }
        }
    }
    return basePath + relativePath;
#endif
}

bool AssetManager::loadAssets(SDL_Window *window, SDL_Renderer *renderer, MIX_Mixer *mixer)
{
    m_renderer = renderer;
    m_currentDisplayScale = SDL_GetWindowDisplayScale(window);

    bool success = true;

    // Set window icon
    std::string iconPath = getAssetPath("assets/icon.png");
    SDL_Surface *iconSurface = IMG_Load(iconPath.c_str());
    if (!iconSurface)
    {
        SDL_Log("Warning: icon.png not found. Using procedural fallback icon.");
        iconSurface = createIconSurface(64, 200, 40, 40);
    }
    if (iconSurface)
    {
        SDL_SetWindowIcon(window, iconSurface);
        SDL_DestroySurface(iconSurface);
    }

    // Load board
    std::string boardPath = getAssetPath("assets/board.png");
    boardTexture = IMG_LoadTexture(renderer, boardPath.c_str());
    if (!boardTexture)
    {
        SDL_Log("Warning: Could not load board.png from %s. Error: %s", boardPath.c_str(), SDL_GetError());
        boardTexture = createBoardTexture(renderer, 1024);
    }
    if (boardTexture)
        SDL_SetTextureScaleMode(boardTexture, SDL_SCALEMODE_LINEAR);

    // Load Pieces
    redTexture = IMG_LoadTexture(renderer, getAssetPath("assets/red_piece.png").c_str());
    blackTexture = IMG_LoadTexture(renderer, getAssetPath("assets/black_piece.png").c_str());
    redKingTexture = IMG_LoadTexture(renderer, getAssetPath("assets/red_king.png").c_str());
    blackKingTexture = IMG_LoadTexture(renderer, getAssetPath("assets/black_king.png").c_str());

    if (redTexture)
        SDL_SetTextureScaleMode(redTexture, SDL_SCALEMODE_LINEAR);
    if (blackTexture)
        SDL_SetTextureScaleMode(blackTexture, SDL_SCALEMODE_LINEAR);
    if (redKingTexture)
        SDL_SetTextureScaleMode(redKingTexture, SDL_SCALEMODE_LINEAR);
    if (blackKingTexture)
        SDL_SetTextureScaleMode(blackKingTexture, SDL_SCALEMODE_LINEAR);

    if (!redTexture)
        redTexture = createCircleTexture(renderer, 256, 200, 40, 40, 255);
    if (!redKingTexture)
        redKingTexture = redTexture;
    if (!blackTexture)
        blackTexture = createCircleTexture(renderer, 256, 20, 20, 20, 255);
    if (!blackKingTexture)
        blackKingTexture = blackTexture;

    legalMoveTexture = createCircleTexture(renderer, 256, 0, 0, 255, 180);
    if (legalMoveTexture)
        SDL_SetTextureScaleMode(legalMoveTexture, SDL_SCALEMODE_LINEAR);

    // Load Audio
    if (mixer)
    {
        moveSfx = MIX_LoadAudio(mixer, getAssetPath("assets/move.wav").c_str(), false);
        captureSfx = MIX_LoadAudio(mixer, getAssetPath("assets/capture.wav").c_str(), false);
        winSfx = MIX_LoadAudio(mixer, getAssetPath("assets/win.wav").c_str(), false);
    }

    // Load Fonts
    std::string fontPath = getAssetPath("assets/DayPosterBlackNF.ttf");
    font = TTF_OpenFont(fontPath.c_str(), 160);        // Increased for high-resolution oversampling
    uiFont = TTF_OpenFont(fontPath.c_str(), 128);      // High resolution for crisp UI rendering
    uiFontSmall = TTF_OpenFont(fontPath.c_str(), 96);  // High resolution for small text/sliders

    if (!font || !uiFont || !uiFontSmall)
    {
        SDL_Log("Warning: Could not load font from %s. Error: %s", fontPath.c_str(), SDL_GetError());
        success = false;
    }

    return success;
}

void AssetManager::freeAssets()
{
    clearCache();

    if (boardTexture)
        SDL_DestroyTexture(boardTexture);
        
    if (redTexture)
        SDL_DestroyTexture(redTexture);
    if (blackTexture)
        SDL_DestroyTexture(blackTexture);

    if (redKingTexture && redKingTexture != redTexture)
        SDL_DestroyTexture(redKingTexture);
    if (blackKingTexture && blackKingTexture != blackTexture)
        SDL_DestroyTexture(blackKingTexture);

    if (legalMoveTexture)
        SDL_DestroyTexture(legalMoveTexture);

    if (font)
        TTF_CloseFont(font);
    if (uiFont)
        TTF_CloseFont(uiFont);
    if (uiFontSmall)
        TTF_CloseFont(uiFontSmall);

    if (moveSfx)
        MIX_DestroyAudio(moveSfx);
    if (captureSfx)
        MIX_DestroyAudio(captureSfx);
    if (winSfx)
        MIX_DestroyAudio(winSfx);
}

void AssetManager::updateDisplayScale(float newScale)
{
    if (m_currentDisplayScale != newScale)
    {
        m_currentDisplayScale = newScale;
        clearCache(); // Force re-rasterization on next request
    }
}

SDL_Texture *AssetManager::getButtonTexture(const std::string &filepath, int logicalWidth, int logicalHeight)
{
    // Create a unique cache key that includes the target dimensions
    std::string cacheKey = filepath + "_" + std::to_string(logicalWidth) + "x" + std::to_string(logicalHeight);

    // If it's already cached at this size, return it
    if (m_textureCache.find(cacheKey) != m_textureCache.end())
    {
        return m_textureCache[cacheKey];
    }

    SDL_Texture *newTexture = nullptr;

    // Check if the asset is an SVG file
    size_t dotPos = filepath.find_last_of(".");
    bool isSVG = false;
    if (dotPos != std::string::npos)
    {
        std::string ext = filepath.substr(dotPos + 1);
        for (auto &c : ext)
            c = std::tolower(c);
        if (ext == "svg")
        {
            isSVG = true;
        }
    }

    if (isSVG)
    {
        // Calculate exact physical pixels needed by the VM graphics layer
        int physicalWidth = static_cast<int>(logicalWidth * m_currentDisplayScale);
        int physicalHeight = static_cast<int>(logicalHeight * m_currentDisplayScale);

        // Rasterize natively via SDL3_image
        SDL_IOStream *stream = SDL_IOFromFile(filepath.c_str(), "rb");
        if (stream)
        {
            SDL_Surface *surface = IMG_LoadSizedSVG_IO(stream, physicalWidth, physicalHeight);
            SDL_CloseIO(stream);

            if (surface)
            {
                newTexture = SDL_CreateTextureFromSurface(m_renderer, surface);
                SDL_DestroySurface(surface);
            }
        }
        if (!newTexture)
        {
            SDL_Log("Warning: Could not load SVG from %s. Using procedural fallback.", filepath.c_str());
            newTexture = createRectTexture(m_renderer, logicalWidth, logicalHeight, 100, 100, 100, 255);
        }
    }
    else
    {
        // Fallback for standard PNGs/JPEGs
        newTexture = IMG_LoadTexture(m_renderer, filepath.c_str());
    }

    if (newTexture)
    {
        m_textureCache[cacheKey] = newTexture;
    }

    return newTexture;
}

void AssetManager::clearCache()
{
    for (auto &pair : m_textureCache)
    {
        if (pair.second)
        {
            SDL_DestroyTexture(pair.second);
        }
    }
    m_textureCache.clear();
}

AssetManager::~AssetManager()
{
    clearCache();
}

void AssetManager::setupButtonSVG(TextureButton &button, const std::string &normalPath, const std::string &hoverPath, const std::string &pressedPath)
{
    std::string norm = getAssetPath(normalPath);
    std::string hov = hoverPath.empty() ? norm : getAssetPath(hoverPath);
    std::string press = pressedPath.empty() ? norm : getAssetPath(pressedPath);

    button.setTextureProvider([this, norm, hov, press](int state, int w, int h) -> SDL_Texture * {
        if (state == 0) // TextureButton::STATE_NORMAL
            return this->getButtonTexture(norm, w, h);
        else if (state == 1) // TextureButton::STATE_HOVER
            return this->getButtonTexture(hov, w, h);
        else if (state == 2) // TextureButton::STATE_PRESSED
            return this->getButtonTexture(press, w, h);
        return nullptr;
    });
}


// --- Procedural Generators ---

SDL_Texture *AssetManager::createBoardTexture(SDL_Renderer *renderer, int size)
{
    SDL_Surface *surface = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_RGBA32);
    if (!surface)
        return nullptr;

    const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(surface->format);
    Uint32 darkColor = SDL_MapRGBA(details, NULL, 139, 69, 19, 255);
    Uint32 lightColor = SDL_MapRGBA(details, NULL, 245, 222, 179, 255);

    int tileSize = size / 8;
    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            SDL_Rect rect = {col * tileSize, row * tileSize, tileSize, tileSize};
            bool isDarkSquare = ((row + col) % 2 == 1);
            SDL_FillSurfaceRect(surface, &rect, isDarkSquare ? darkColor : lightColor);
        }
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
    {
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    }
    SDL_DestroySurface(surface);
    return texture;
}

SDL_Texture *AssetManager::createRectTexture(SDL_Renderer *renderer, int w, int h, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_Surface *surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    if (!surface)
        return nullptr;
    const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(surface->format);
    Uint32 color = SDL_MapRGBA(details, NULL, r, g, b, a);
    SDL_FillSurfaceRect(surface, NULL, color);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture)
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_DestroySurface(surface);
    return texture;
}

SDL_Surface *AssetManager::createIconSurface(int size, Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Surface *surface = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_RGBA32);
    if (!surface)
        return nullptr;
    float center = size / 2.0f;
    float radius = (size / 2.0f) - 1.0f;
    for (int y = 0; y < size; y++)
    {
        for (int x = 0; x < size; x++)
        {
            float dx = (float)x + 0.5f - center;
            float dy = (float)y + 0.5f - center;
            float dist = SDL_sqrtf(dx * dx + dy * dy);
            Uint8 *pixel = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
            pixel[0] = r;
            pixel[1] = g;
            pixel[2] = b;
            if (dist <= radius - 0.5f)
                pixel[3] = 255;
            else if (dist <= radius + 0.5f)
                pixel[3] = (Uint8)(255 * (radius + 0.5f - dist));
            else
                pixel[3] = 0;
        }
    }
    return surface;
}

SDL_Texture *AssetManager::createCircleTexture(SDL_Renderer *renderer, int size, Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
    SDL_Surface *surface = createIconSurface(size, r, g, b);
    if (!surface)
        return nullptr;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    SDL_DestroySurface(surface);
    return texture;
}