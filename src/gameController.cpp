/*
 * gameController.cpp
 *
 *      Author: alex@glassoniongames.com
 */

#include "gameController.h"
#include <iostream>
#include <thread>

// Constructor initializes AI state flags and invalid default AI move positions.
GameController::GameController()

    : aiThinking(false), aiMoveReady(false), currentSearchDepth(0), aiTimeLimit(3), pvpMode(false)
{
    aiFrom = {-1, -1};
    aiTo = {-1, -1};
}

// Starts a visual animation for one piece moving from one square to another.
void GameController::startMoveAnimation(char piece, int fromRow, int fromCol, int toRow, int toCol, bool isUndo)
{
    // Calculate the distance of this segment.
    // Move = 1, Jump = 2.
    int distance = std::abs(fromRow - toRow);

    animation.active = true;
    animation.piece = piece;
    animation.fromRow = fromRow;
    animation.fromCol = fromCol;
    animation.toRow = toRow;
    animation.toCol = toCol;
    animation.isUndo = isUndo;
    animation.startTime = SDL_GetTicks();

    // Use a constant velocity: 250ms per square traveled.
    // A move will take 250ms, while a jump will take 500ms.
    animation.durationMs = distance * 250;

    // Set the sound trigger: moves of distance > 1 are jumps (captures)
    // We don't play capture sounds on Undo to avoid confusion
    soundTrigger = (distance > 1 && !isUndo) ? 2 : 1;
}

void GameController::setupPathAnimation(board &b, char piece, int fR, int fC, int tR, int tC, bool isUndo)
{
    // Always query the forward path from the engine.
    // If isUndo is true, we will reverse this path visually for the animation later.
    std::vector<std::pair<int, int>> pathPairs = b.getMovePath8x8(fR, fC, tR, tC);
    animationPath.clear();
    pendingCaptures.clear();
    animationPathIndex = 1;

    if (!pathPairs.empty())
    {
        for (size_t i = 0; i < pathPairs.size(); ++i)
        {
            animationPath.push_back({pathPairs[i].first, pathPairs[i].second});

            // Identify captured pieces for both Redo and Undo.
            if (i < pathPairs.size() - 1)
            {
                int r1 = pathPairs[i].first, c1 = pathPairs[i].second;
                int r2 = pathPairs[i + 1].first, c2 = pathPairs[i + 1].second;

                if (std::abs(r1 - r2) > 1)
                {
                    int capR = (r1 + r2) / 2;
                    int capC = (c1 + c2) / 2;
                    int step = isUndo ? (int)(pathPairs.size() - 1 - i) : (int)i + 1;
                    pendingCaptures.push_back({capR, capC, b.getPieceAt8x8(capR, capC), step});
                }
            }
        }

        if (isUndo)
        {
            std::reverse(animationPath.begin(), animationPath.end());
        }

        startMoveAnimation(piece,
                           animationPath[0].row, animationPath[0].col,
                           animationPath[1].row, animationPath[1].col, isUndo);
    }
    else
    {
        // Fallback if path finding fails
        startMoveAnimation(piece, fR, fC, tR, tC, isUndo);
    }
}

// Converts the board's legal destination list into Square objects for the UI.
std::vector<Square> GameController::getLegalMovesForSelection(board &b, int row, int col)
{
    std::vector<Square> legalMoves;

    // Ask the board engine for all legal destination squares for this selected piece.
    std::vector<std::pair<int, int>> destinations =
        b.getLegalDestinationsForSquare(row, col);

    // Convert each pair into a Square struct used by the controller/UI.
    for (size_t i = 0; i < destinations.size(); ++i)
    {
        Square sq;
        sq.row = destinations[i].first;
        sq.col = destinations[i].second;
        legalMoves.push_back(sq);
    }

    return legalMoves;
}

// Checks if a checker movement animation is currently active or has remaining path segments
bool GameController::isAnimating() const
{
    return animation.active || !animationPath.empty();
}

// Handles a mouse click on the board.
bool GameController::handleClick(board &b, int row, int col, std::vector<MoveRecord> &history, int &historyIndex)
{
    // Ignore input while an animation is running, while AI is thinking,
    // or when it is the black AI player's turn.
    if (isAnimating() || aiThinking || (!pvpMode && b.getTurnPublic() == 'b'))
        return false;

    // Ignore clicks outside the board.
    if (row < 0 || row >= 8 || col < 0 || col >= 8)
        return false;

    // Ignore light squares because checkers pieces only move on dark squares.
    if ((row + col) % 2 == 0)
        return false;

    // No piece is currently selected.
    if (selectedRow == -1 && selectedCol == -1)
    {
        // If the clicked square contains the current player's piece, select it.
        if (isCurrentPlayersPiece(b, row, col))
        {
            selectedRow = row;
            selectedCol = col;

            // Store legal moves so the UI can highlight possible destinations.
            legalMoves = getLegalMovesForSelection(b, row, col);

            std::cout << "Selected piece: (" << row << ", " << col << ")\n";
            return false; // Selection only, no sound needed
        }
        return false;
    }
    else
    {
        // If another current-player piece is clicked, switch selection to it.
        if (isCurrentPlayersPiece(b, row, col))
        {
            selectedRow = row;
            selectedCol = col;
            legalMoves = getLegalMovesForSelection(b, row, col);

            std::cout << "Reselected piece: (" << row << ", " << col << ")\n";
            return false; // Reselection only, no sound needed
        }
        else
        {
            // Check if the clicked square is a legal move destination before animating.
            bool isLegal = false;
            for (const auto &m : legalMoves)
            {
                if (m.row == row && m.col == col)
                {
                    isLegal = true;
                    break;
                }
            }

            bool moved = false;
            if (isLegal)
            {
                // Store piece type and setup animation before applying the move.
                char movingPiece = b.getPieceAt8x8(selectedRow, selectedCol);
                setupPathAnimation(b, movingPiece, selectedRow, selectedCol, row, col);
                moved = b.tryMove8x8(selectedRow, selectedCol, row, col);

                if (moved)
                {
                    if (historyIndex < (int)history.size())
                        history.erase(history.begin() + historyIndex, history.end());

                    history.push_back({selectedRow, selectedCol, row, col});
                    historyIndex++;

                    std::cout << "Moved from (" << selectedRow << ", " << selectedCol
                              << ") to (" << row << ", " << col << ")\n";
                }
            }

            if (!moved)
            {
                std::cout << "Illegal move from (" << selectedRow << ", "
                          << selectedCol << ") to (" << row << ", " << col << ")\n";
            }

            // Clear human selection state after attempting a move.
            selectedRow = -1;
            selectedCol = -1;
            legalMoves.clear();

            return moved; // Return whether the move was successful to trigger sound
        }
    }
    return false; // Fallback for any other path
}

// Returns true if the piece at the given square belongs to the player whose turn it is.
bool GameController::isCurrentPlayersPiece(const board &b, int row, int col) const
{
    const char piece = b.getPieceAt8x8(row, col);
    const char turn = b.getTurnPublic();

    if (turn == 'b')
        return (piece == 'b' || piece == 'B');
    if (turn == 'r')
        return (piece == 'r' || piece == 'R');

    return false;
}

// Applies the AI's completed move to the live board once the background thread has found it.
bool GameController::updateAI(board &b, std::vector<MoveRecord> &history, int &historyIndex)
{
    // If animations are still running or it's PVP mode, do not process AI.
    if (isAnimating() || pvpMode)
        return false;

    // 1. If it is AI's turn and AI is not thinking and no AI move is ready, start AI search!
    if (b.getTurnPublic() == 'b' && !aiThinking && !aiMoveReady)
    {
        aiThinking = true;

        // Copy the board so the AI thread does not directly mutate the live board.
        board boardCopy = b;

        // Join the previous thread if it finished but wasn't cleaned up
        if (aiThread.joinable())
            aiThread.join();

        int limit = aiTimeLimit;
        aiThread = std::thread([boardCopy, this, limit]() mutable
                               {
            currentSearchDepth = 0; // Reset depth at start of search

            int fr, fc, tr, tc;

            // Search for the AI's best move at specified depth
            bool found = boardCopy.findBestMove8x8(limit, fr, fc, tr, tc, &currentSearchDepth, &aiThinking);

            if (found && aiThinking)
            {
                // Store the chosen AI move safely because this runs on another thread.
                std::lock_guard<std::mutex> lock(aiMutex);
                aiFrom = {fr, fc};
                aiTo = {tr, tc};
                aiMoveReady = true;
            }

            // Mark the AI as done thinking.
            aiThinking = false; });

        return false;
    }

    // 2. Only apply the AI move once it is ready and no animation is currently active.
    if (aiMoveReady && !isAnimating())
    {
        Square from;
        Square to;

        // Copy the AI move out of shared state safely.
        {
            std::lock_guard<std::mutex> lock(aiMutex);
            from = aiFrom;
            to = aiTo;
            aiMoveReady = false;
        }

        // Store the moving piece before modifying the board.
        char movingPiece = b.getPieceAt8x8(from.row, from.col);

        // Setup animation (handles path finding and captures)
        setupPathAnimation(b, movingPiece, from.row, from.col, to.row, to.col);

        // Apply the AI move to the actual live board.
        bool aiApplied = b.tryMove8x8(from.row, from.col, to.row, to.col);

        if (aiApplied)
        {
            // Record AI move in history
            if (historyIndex < (int)history.size())
                history.erase(history.begin() + historyIndex, history.end());

            history.push_back({from.row, from.col, to.row, to.col});
            historyIndex++;

            std::cout << "AI moved from (" << from.row << ", " << from.col
                      << ") to (" << to.row << ", " << to.col << ")\n";
        }

        return aiApplied;
    }
    return false;
}

// Continues a multi-segment animation path, such as a multi-jump move.
void GameController::updateAnimation()
{
    // If a segment is still animating, check if it reached its duration.
    if (animation.active)
    {
        Uint64 elapsed = SDL_GetTicks() - animation.startTime;
        if (elapsed >= (Uint64)animation.durationMs)
        {
            animation.active = false;
        }
        else
        {
            return;
        }
    }

    // No path or only one point means nothing to continue.
    if (animationPath.size() < 2)
    {
        animationPath.clear();
        pendingCaptures.clear();
        animationPathIndex = 0;
        return;
    }

    // If we already reached the last segment, clear the path state.
    if (animationPathIndex >= static_cast<int>(animationPath.size()) - 1)
    {
        animationPath.clear();
        pendingCaptures.clear();
        animationPathIndex = 0;
        return;
    }

    // Continue animating the same piece along the next segment of the path.
    char piece = animation.piece;

    int fromIndex = animationPathIndex;
    int toIndex = animationPathIndex + 1;

    startMoveAnimation(piece,
                       animationPath[fromIndex].row,
                       animationPath[fromIndex].col,
                       animationPath[toIndex].row,
                       animationPath[toIndex].col,
                       animation.isUndo);

    // Advance to the next segment for the following update.
    animationPathIndex++;
}

void GameController::stopAI()
{
    aiThinking = false;
    if (aiThread.joinable())
        aiThread.join();
}