//----------------------------------------------------------------------------//
//               █      █                                                     //
//               ████████                                                     //
//             ██        ██                                                   //
//            ███  █  █  ███        GameCore.cpp                              //
//            █ █        █ █        CoreColorGrid                             //
//             ████████████                                                   //
//           █              █       Copyright (c) 2015, 2016                  //
//          █     █    █     █      AmazingCow - www.AmazingCow.com           //
//          █     █    █     █                                                //
//           █              █       N2OMatt - n2omatt@amazingcow.com          //
//             ████████████         www.amazingcow.com/n2omatt                //
//                                                                            //
//                  This software is licensed as GPLv3                        //
//                 CHECK THE COPYING FILE TO MORE DETAILS                     //
//                                                                            //
//    Permission is granted to anyone to use this software for any purpose,   //
//   including commercial applications, and to alter it and redistribute it   //
//               freely, subject to the following restrictions:               //
//                                                                            //
//     0. You **CANNOT** change the type of the license.                      //
//     1. The origin of this software must not be misrepresented;             //
//        you must not claim that you wrote the original software.            //
//     2. If you use this software in a product, an acknowledgment in the     //
//        product IS HIGHLY APPRECIATED, both in source and binary forms.     //
//        (See opensource.AmazingCow.com/acknowledgment.html for details).    //
//        If you will not acknowledge, just send us a email. We'll be         //
//        *VERY* happy to see our work being used by other people. :)         //
//        The email is: acknowledgment_opensource@AmazingCow.com              //
//     3. Altered source versions must be plainly marked as such,             //
//        and must not be misrepresented as being the original software.      //
//     4. This notice may not be removed or altered from any source           //
//        distribution.                                                       //
//     5. Most important, you must have fun. ;)                               //
//                                                                            //
//      Visit opensource.amazingcow.com for more open-source projects.        //
//                                                                            //
//                                  Enjoy :)                                  //
//----------------------------------------------------------------------------//

//Header
#include "../include/GameCore.h"
//std
#include <sstream>
#include <algorithm>
//ColorGridCore
#include "../include/AIPlayer.h"

#include <iostream>

//Usings.
USING_NS_CORECOLORGRID;
using namespace std;

// Constants //
const int GameCore::kUnlimitedMoves = -1;
const int GameCore::kRandomSeed     = -1;

// CTOR/DTOR //
GameCore::GameCore(const Options &options):
    m_options(options),
    m_currentPlayerIndex(options.startPlayerIndex),
    //m_board   - Initialized in GameCore::initBoard.
    //m_players - Initialized in GameCore::initPlayers.
    m_pWinnerPlayer(nullptr)
{
    initRandomGenerator();
    initBoard();
    initPlayers();
}

// Public Methods //
//Action
CoreCoord::Coord::Vec GameCore::changeColor(int colorIndex)
{
    CoreCoord::Coord::Vec changedCoords;

    auto &player = getCurrentPlayer();

    //Already in this color, don't do anything else.
    if(player.getCurrentColorIndex() == colorIndex)
        return changedCoords;

    //Add the Player's current coords.
    changedCoords = getAffectedCoords(player.getOwnedCoords(),
                                      colorIndex);

    //Add the changed coords to player.
    for(auto &coord : changedCoords)
        player.addOwnedCoord(coord);

    //Make sure that all player's colors is the same.
    for(auto &coord : player.getOwnedCoords())
    {
        auto &color = getColorAt(coord);
        color.changeColorIndex(colorIndex, player.getIndex());
    }

    //Update Player's state
    player.setCurrentColorIndex(colorIndex);
    player.incrementMovesCount();

    //Update the game state.
    checkStatus();
    turnCurrentPlayer();

    return changedCoords;
}

//Board
const Color::Board& GameCore::getBoard() const
{
    return m_board;
}

const Color& GameCore::getColorAt(const CoreCoord::Coord & coord) const
{
    return m_board[coord.y][coord.x];
}

//Player
Player* GameCore::getWinnerPlayer() const
{
    return m_pWinnerPlayer;
}

Player& GameCore::getCurrentPlayer() const
{
    return *m_players[m_currentPlayerIndex];
}

Player& GameCore::getPlayer(int index) const
{
    return *m_players[index];
}

int GameCore::getPlayersCount() const
{
    return m_options.playersCount;
}
int GameCore::getHumanPlayersCount() const
{
    return m_options.humanPlayersCount;
}

//AI
int GameCore::getAIStrength() const
{
    return m_options.aiStrength;
}


//Helpers
bool GameCore::gameIsOver() const
{
    return (m_pWinnerPlayer != nullptr);
}

int GameCore::getMaxMoves() const
{
    return m_options.maxMoves;
}

int GameCore::getSeed() const
{
    return m_options.seed;
}


bool GameCore::isValidCoord(const CoreCoord::Coord &coord) const
{
    return (coord.y >= 0 && coord.y < static_cast<int>(m_board.size()))
        && (coord.x >= 0 && coord.x < static_cast<int>(m_board[coord.y].size()));
}

CoreCoord::Coord::Vec GameCore::getAffectedCoords(const CoreCoord::Coord::Vec &coords,
                                                  int colorIndex) const
{
    //Add the Player's current coords.
    CoreCoord::Coord::Vec changedCoords = coords;

    for(int i = 0; i < changedCoords.size(); ++i)
    {
        auto ownedCoord = changedCoords[i];

        for(auto &orthogonalCoord : ownedCoord.getOrthogonal())
        {
            //Coord if out of Board bounds.
            if(!isValidCoord(orthogonalCoord))
                continue;

            //Coord is already processed.
            auto it = std::find(begin(changedCoords),
                                end  (changedCoords),
                                orthogonalCoord);

            if(it != end(changedCoords))
                continue;

            const auto &color = getColorAt(orthogonalCoord);

            //Coord's color isn't the same of target color.
            if(color.getColorIndex() != colorIndex)
                continue;

            //Color is already owned by other player.
            if(color.getOwnerIndex() != Color::kInvalidOwner)
                continue;

            //Update the changed coords.
            changedCoords.push_back(orthogonalCoord);
        }
    }

    return changedCoords;
}

std::string GameCore::ascii() const
{
    std::stringstream ss;
    for(const auto &line : m_board)
    {
        for(const auto &color : line)
        {
            auto index = color.getColorIndex();
            ss << index;
        }
        ss << std::endl;
    }

    return ss.str();
}


// Private Methods //
//Init.
void GameCore::initRandomGenerator()
{
    if(m_options.seed == GameCore::kRandomSeed)
        m_options.seed = static_cast<int>(time(nullptr));

    srand(m_options.seed);
}

void GameCore::initBoard()
{
    m_board.reserve(m_options.boardHeight);

    //Initialize all Board colors.
    for(int i = 0; i < m_options.boardHeight; ++i)
    {
        m_board.push_back(std::vector<Color>());
        m_board[i].reserve(m_options.boardWidth);

        for(int j = 0; j < m_options.boardWidth; ++j)
        {
            //Randomize a color.
            int colorIndex = rand() % m_options.colorsCount;
            m_board[i].push_back(Color(colorIndex));
        }
    }
}

void GameCore::initPlayers()
{
    //First player is **always** a human player.
    createPlayerHelper(0, false);

    //Create the other players.
    for(int i = 1; i < m_options.playersCount; ++i)
        createPlayerHelper(i, m_options.humanPlayersCount >= i);

    m_currentPlayerIndex = m_options.startPlayerIndex;
}


//Colors
Color& GameCore::getColorAt(const CoreCoord::Coord &coord)
{
    return m_board[coord.y][coord.x];
}


//Status
void GameCore::checkStatus()
{
    //COWTODO: Today we're not checking for a draw.
    int possibleWinnerIndex = -1;
    int maxCoordsCount      = 0;
    int totalCoords         = 0;

    for(int i = 0; i < m_players.size(); ++i)
    {
        int playerCoordsCount = m_players[i]->getOwnedCoords().size();

        totalCoords += playerCoordsCount;

        if(playerCoordsCount > maxCoordsCount)
            possibleWinnerIndex = i;
    }

    //All coords are already owned - Game is over and we have a winner.
    if(totalCoords == (m_board.size() * m_board[0].size()))
    {
        m_pWinnerPlayer = m_players[possibleWinnerIndex].get();
        return;
    }

    //There are some coords left, so check if player
    //already did the max moves allowed.
    if(m_options.maxMoves != GameCore::kUnlimitedMoves &&
       m_players[0]->getMovesCount() == m_options.maxMoves)
    {
        m_pWinnerPlayer = m_players[possibleWinnerIndex].get();
        return;
    }

    //We don't have a winner yet and max moves count wasn't
    //reached - So continue the game.
    m_pWinnerPlayer = nullptr;
}


//Players
void GameCore::turnCurrentPlayer()
{
    m_currentPlayerIndex = (m_currentPlayerIndex + 1) % m_players.size();
}

void GameCore::createPlayerHelper(int playerIndex,
                                  bool isAIPlayer)

{
    Player *resetPlayerPtr = nullptr;
    resetPlayerPtr = (isAIPlayer)
                     ? new AIPlayer(playerIndex, m_options.aiStrength)
                     : new Player(playerIndex);

    std::shared_ptr<Player> player(resetPlayerPtr);
    m_players.push_back(player);

    m_currentPlayerIndex = playerIndex;

    //Get the start coord for the player.
    auto startCoord = CoreCoord::Coord::Vec {
            //Player1 - Left Top Side
            CoreCoord::Coord(0, 0),
            //Player2 - Right Bottom Side.
            CoreCoord::Coord(m_options.boardHeight - 1, m_options.boardWidth - 1),
            //Player3 - Right Top Side.
            CoreCoord::Coord(0, m_options.boardWidth -1),
            //Player4 - Left Bottm Side.
            CoreCoord::Coord(m_options.boardHeight -1, 0)
    }[playerIndex];

    //Add the the first Coord for Player and let the game
    //change all colors that are possible - This will
    //increase the Player's moves count, but we gonna reset
    //it later.
    player->addOwnedCoord(startCoord);
    changeColor(getColorAt(startCoord).getColorIndex());

    //Reset the moves for this Player.
    player->m_movesCount = 0;
}
