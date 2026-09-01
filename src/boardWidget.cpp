/*
 * boardWidget.cpp
 *
 *      Author: alex@glassoniongames.com
 */

#include "boardWidget.h"
#include "appState.h"
#include <algorithm>

BoardWidget::BoardWidget(board &bRef, GameController &ctrl, std::vector<MoveRecord> &hist, int &hIndex)
    : b(bRef), controller(ctrl), history(hist), historyIndex(hIndex)
{
}

void BoardWidget::updateLayout(float x, float y, float w, float h)
{
    float boardSize = std::min(w, h);
    tileSize = boardSize / 8.0f;
    float offsetX = x + (w - boardSize) / 2.0f;
    float offsetY = y + (h - boardSize) / 2.0f;
    rect = {offsetX, offsetY, boardSize, boardSize};
}

bool BoardWidget::handleEvent(const SDL_Event *event, bool &isOver)
{
    isOver = false;
    if (!visible || !enabled)
        return false;

    float mouseX = -1.0f, mouseY = -1.0f;

    if (event->type == SDL_EVENT_MOUSE_MOTION)
    {
        mouseX = event->motion.x;
        mouseY = event->motion.y;
    }
    else if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN || event->type == SDL_EVENT_MOUSE_BUTTON_UP)
    {
        mouseX = event->button.x;
        mouseY = event->button.y;
    }

    if (mouseX >= rect.x && mouseX <= rect.x + rect.w &&
        mouseY >= rect.y && mouseY <= rect.y + rect.h)
    {
        isOver = true;
    }

    if (isOver && event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT)
    {
        float adjustedX = mouseX - rect.x;
        float adjustedY = mouseY - rect.y;
        int col = static_cast<int>(adjustedX / tileSize);
        int row = static_cast<int>(adjustedY / tileSize);

        if (col >= 0 && col < 8 && row >= 0 && row < 8)
        {
            return controller.handleClick(b, row, col, history, historyIndex);
        }
    }
    return false;
}

void BoardWidget::render(SDL_Renderer *renderer)
{
    if (!visible || !state)
        return;

    drawCheckerboard(renderer);
    drawSelectedSquare(renderer);
    drawLegalMoves(renderer);
    drawPieces(renderer);
    drawMoveAnimation(renderer);
}

void BoardWidget::drawCheckerboard(SDL_Renderer *renderer)
{
    SDL_RenderTexture(renderer, state->assets.boardTexture, NULL, &rect);
}

void BoardWidget::drawPieceAtPixel(SDL_Renderer *renderer, float centerX, float centerY, char piece)
{
    float radius = tileSize * 0.40f;
    SDL_Texture *tex = nullptr;
    if (piece == 'b')
        tex = state->assets.blackTexture;
    else if (piece == 'r')
        tex = state->assets.redTexture;
    else if (piece == 'R')
        tex = state->assets.redKingTexture;
    else if (piece == 'B')
        tex = state->assets.blackKingTexture;

    if (tex)
    {
        SDL_FRect dst = {centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f};
        SDL_RenderTexture(renderer, tex, NULL, &dst);
    }
}

void BoardWidget::drawPiece(SDL_Renderer *renderer, int row, int col, char piece)
{
    float centerX = col * tileSize + tileSize / 2.0f + rect.x;
    float centerY = row * tileSize + tileSize / 2.0f + rect.y;
    drawPieceAtPixel(renderer, centerX, centerY, piece);
}

void BoardWidget::drawPieces(SDL_Renderer *renderer)
{
    for (int row = 0; row < 8; ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            char piece = b.getPieceAt8x8(row, col);
            if (piece == 'e')
                continue;

            if (controller.isAnimating())
            {
                bool isPending = false;
                for (const auto &cp : controller.pendingCaptures)
                {
                    if (cp.row == row && cp.col == col)
                    {
                        isPending = true;
                        break;
                    }
                }
                if (isPending)
                    continue;

                int hideRow = controller.animation.toRow;
                int hideCol = controller.animation.toCol;
                if (!controller.animationPath.empty())
                {
                    hideRow = controller.animationPath.back().row;
                    hideCol = controller.animationPath.back().col;
                }
                if (row == hideRow && col == hideCol)
                    continue;
            }
            drawPiece(renderer, row, col, piece);
        }
    }
    for (const auto &cap : controller.pendingCaptures)
    {
        bool shouldDraw = false;
        Uint64 elapsed = SDL_GetTicks() - controller.animation.startTime;
        float t = static_cast<float>(elapsed) / static_cast<float>(controller.animation.durationMs);
        if (controller.animation.isUndo)
        {
            if (controller.animationPathIndex > cap.captureStep)
                shouldDraw = true;
            else if (controller.animationPathIndex == cap.captureStep && t >= 0.5f)
                shouldDraw = true;
        }
        else
        {
            if (controller.animationPathIndex < cap.captureStep)
                shouldDraw = true;
            else if (controller.animationPathIndex == cap.captureStep && t < 0.5f)
                shouldDraw = true;
        }
        if (shouldDraw)
            drawPiece(renderer, cap.row, cap.col, cap.piece);
    }
}

void BoardWidget::drawSelectedSquare(SDL_Renderer *renderer)
{
    if (controller.selectedRow < 0 || controller.selectedCol < 0)
        return;
    SDL_FRect highlight = {
        controller.selectedCol * tileSize + rect.x,
        controller.selectedRow * tileSize + rect.y,
        tileSize, tileSize};
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderRect(renderer, &highlight);
}

void BoardWidget::drawLegalMoves(SDL_Renderer *renderer)
{
    float radius = tileSize * 0.15f;
    for (const auto &move : controller.legalMoves)
    {
        float centerX = move.col * tileSize + tileSize / 2.0f + rect.x;
        float centerY = move.row * tileSize + tileSize / 2.0f + rect.y;
        SDL_FRect dst = {centerX - radius, centerY - radius, radius * 2.0f, radius * 2.0f};
        SDL_RenderTexture(renderer, state->assets.legalMoveTexture, NULL, &dst);
    }
}

void BoardWidget::drawMoveAnimation(SDL_Renderer *renderer)
{
    MoveAnimation &animation = controller.animation;
    if (!animation.active)
        return;
    Uint64 elapsed = SDL_GetTicks() - animation.startTime;
    float t = static_cast<float>(elapsed) / static_cast<float>(animation.durationMs);
    if (t > 1.0f)
        t = 1.0f;

    float fromX = animation.fromCol * tileSize + tileSize / 2.0f + rect.x;
    float fromY = animation.fromRow * tileSize + tileSize / 2.0f + rect.y;
    float toX = animation.toCol * tileSize + tileSize / 2.0f + rect.x;
    float toY = animation.toRow * tileSize + tileSize / 2.0f + rect.y;
    float currentX = fromX + (toX - fromX) * t;
    float currentY = fromY + (toY - fromY) * t;
    drawPieceAtPixel(renderer, currentX, currentY, animation.piece);
}