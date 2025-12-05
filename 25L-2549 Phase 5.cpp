#include <SFML/Graphics.hpp>
#include <iostream>
using namespace std;

const int TILE_SIZE = 100;
const int BOARD_SIZE = 8;

sf::Texture wPawn, wRook, wKnight, wBishop, wQueen, wKing;
sf::Texture bPawn, bRook, bKnight, bBishop, bQueen, bKing;

int boardState[8][8];
bool whiteKingMoved = false, blackKingMoved = false;
bool whiteRookAMoved = false, whiteRookHMoved = false, blackRookAMoved = false, blackRookHMoved = false;
int ep_col = -1, ep_row = -1;
bool whiteTurn = true;

int selF = -1, selR = -1;
bool pieceSelected = false;

// Legal moves arrays
int legalF[64], legalR[64], legalCount = 0;

// Game ending
bool gameEnded = false;
string endMessage;

// Captured pieces
int whiteCaptured = 0, blackCaptured = 0;

sf::Vector2f tilePos(int f, int r) 
{
    return sf::Vector2f(f * TILE_SIZE, r * TILE_SIZE);
}

bool loadTextures()
{
    return wPawn.loadFromFile("images/w_pawn.png")&&
        wRook.loadFromFile("images/w_rook.png") &&
        wKnight.loadFromFile("images/w_knight.png") &&
        wBishop.loadFromFile("images/w_bishop.png") &&
        wQueen.loadFromFile("images/w_queen.png") &&
        wKing.loadFromFile("images/w_king.png") &&
        bPawn.loadFromFile("images/b_pawn.png") &&
        bRook.loadFromFile("images/b_rook.png") &&
        bKnight.loadFromFile("images/b_knight.png") &&
        bBishop.loadFromFile("images/b_bishop.png") &&
        bQueen.loadFromFile("images/b_queen.png") &&
        bKing.loadFromFile("images/b_king.png");
}

bool isWhiteId(int id) 
{
    return id >= 1 && id <= 16; 
}
bool isBlackId(int id)
{
    return id >= 17 && id <= 32; 
}
bool inside(int f, int r)
{
    return f >= 0 && f < 8 && r >= 0 && r < 8;
}

void copyBoard(int src[8][8], int dst[8][8]) 
{
    for (int r = 0; r < 8; r++)
        for (int f = 0; f < 8; f++)
            dst[r][f] = src[r][f];
}

bool findKingPos(bool white, int& kf, int& kr)
{
    int target = white ? 16 : 32;
    for (int r = 0; r < 8; r++)
        for (int f = 0; f < 8; f++)
            if (boardState[r][f] == target)
            {
                kf = f;
                kr = r; 
                return true; 
            }
    return false;
}

// Generate pseudo moves for a piece
void genPossibleMovesArray(int f, int r, int outF[], int outR[], int& outCount) 
{
    outCount = 0;
    if (!inside(f, r))
        return;
    int id = boardState[r][f]; 
    if (id == 0)
        return;

    // Pawn
    if (id >= 1 && id <= 8)
    {
        int dir = -1;
        if (inside(f, r + dir) && boardState[r + dir][f] == 0) 
        {
            outF[outCount] = f;
            outR[outCount++] = r + dir; 
        }
        if (r == 6 && boardState[5][f] == 0 && boardState[4][f] == 0) 
        {
            outF[outCount] = f; 
            outR[outCount++] = 4; 
        }
        for (int df = -1; df <= 1; df += 2)
        {
            int tf = f + df, tr = r + dir; 
            if (inside(tf, tr) && isBlackId(boardState[tr][tf])) 
            {
                outF[outCount] = tf; outR[outCount++] = tr;
            }
        }
        if (ep_col != -1 && ep_row == r && abs(ep_col - f) == 1) 
        {
            outF[outCount] = ep_col; 
            outR[outCount++] = r + dir;
        }
        return;
    }
    if (id >= 17 && id <= 24)
    {
        int dir = 1;
        if (inside(f, r + dir) && boardState[r + dir][f] == 0) 
        {
            outF[outCount] = f; 
            outR[outCount++] = r + dir; 
        }
        if (r == 1 && boardState[2][f] == 0 && boardState[3][f] == 0) 
        {
            outF[outCount] = f; 
            outR[outCount++] = 3; 
        }
        for (int df = -1; df <= 1; df += 2) 
        {
            int tf = f + df, tr = r + dir;
            if (inside(tf, tr) && isWhiteId(boardState[tr][tf]))
            {
                outF[outCount] = tf; outR[outCount++] = tr; 
            }
        }
        if (ep_col != -1 && ep_row == r && abs(ep_col - f) == 1) 
        {
            outF[outCount] = ep_col;
            outR[outCount++] = r + dir; 
        }
        return;
    }

    // Knights
    if (id == 11 || id == 12 || id == 27 || id == 28)
    {
        int jumps[8][2] = { {1,2},{2,1},{2,-1},{1,-2},{-1,-2},{-2,-1},{-2,1},{-1,2} };
        for (int i = 0; i < 8; i++) 
        {
            int tf = f + jumps[i][0], tr = r + jumps[i][1];
            if (!inside(tf, tr)) 
                continue;
            int t = boardState[tr][tf];
            if (t == 0 || (isWhiteId(id) && isBlackId(t)) || (isBlackId(id) && isWhiteId(t))) 
            {
                outF[outCount] = tf; 
                outR[outCount++] = tr;
            }
        }
        return;
    }

    // King
    if (id == 16 || id == 32)
    {
        for (int df = -1; df <= 1; df++)
            for (int dr = -1; dr <= 1; dr++) 
        {
            if (df == 0 && dr == 0) 
                continue;
            int tf = f + df, tr = r + dr;
            if (!inside(tf, tr)) 
                continue;
            int t = boardState[tr][tf];
            if (t == 0 || (isWhiteId(id) && isBlackId(t)) || (isBlackId(id) && isWhiteId(t)))
            {
                outF[outCount] = tf; outR[outCount++] = tr;
            }
        }
        // Castling
        if (id == 16 && !whiteKingMoved && r == 7)
        {
            if (!whiteRookHMoved && boardState[7][7] == 10 && boardState[7][5] == 0 && boardState[7][6] == 0) 
            {
                outF[outCount] = 6;
                outR[outCount++] = 7; 
            }
            if (!whiteRookAMoved && boardState[7][0] == 9 && boardState[7][1] == 0 && boardState[7][2] == 0 && boardState[7][3] == 0) 
            {
                outF[outCount] = 2; 
              outR[outCount++] = 7;
            }
        }
        if (id == 32 && !blackKingMoved && r == 0)
        {
            if (!blackRookHMoved && boardState[0][7] == 26 && boardState[0][5] == 0 && boardState[0][6] == 0) 
            { outF[outCount] = 6; 
            outR[outCount++] = 0;
            }
            if (!blackRookAMoved && boardState[0][0] == 25 && boardState[0][1] == 0 && boardState[0][2] == 0 && boardState[0][3] == 0) 
            {
                outF[outCount] = 2; outR[outCount++] = 0;
            }
        }
        return;
    }

    // Sliding pieces
    int dirs[8][2] = { {1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1} };
    for (int d = 0; d < 8; d++)
    {
        if ((id == 9 || id == 10 || id == 25 || id == 26) && d >= 4) 
            continue; // Rook only 0-3
        if ((id == 13 || id == 14 || id == 29 || id == 30) && d < 4)
            continue;  // Bishop only 4-7
        if ((id == 15 || id == 31) && d < 8); // Queen all directions
        int df = dirs[d][0], dr = dirs[d][1];
        int tf = f + df, tr = r + dr;
        while (inside(tf, tr))
        {
            int t = boardState[tr][tf];
            if (t == 0)
            { 
                outF[outCount] = tf;
            outR[outCount++] = tr; 
            }
            else
            {
                if ((isWhiteId(id) && isBlackId(t)) || (isBlackId(id) && isWhiteId(t))) 
                {
                    outF[outCount] = tf; 
                    outR[outCount++] = tr;
                }
                break;
            }
            tf += df; tr += dr;
        }
    }
}

// Check if king is in check
bool isKingInCheck(bool white) 
{
    int kf, kr; if (!findKingPos(white, kf, kr))
        return false;
    for (int r = 0; r < 8; r++)
        for (int f = 0; f < 8; f++)
        {
            int id = boardState[r][f];
            if (id == 0) 
                continue;
            if (white && isWhiteId(id)) 
                continue;
            if (!white && isBlackId(id))
                continue;
            int tmpF[64], tmpR[64], cnt = 0;
            genPossibleMovesArray(f, r, tmpF, tmpR, cnt);
            for (int i = 0; i < cnt; i++) 
                if (tmpF[i] == kf && tmpR[i] == kr)
                    return true;
        }
    return false;
}

// Generate legal moves
void genLegalMovesArray(int f, int r, int outF[], int outR[], int& outCount) 
{
    int pseudoF[64], pseudoR[64], pc = 0;
    genPossibleMovesArray(f, r, pseudoF, pseudoR, pc);
    int id = boardState[r][f]; 
    if (id == 0) 
        return;
    outCount = 0;

    for (int i = 0; i < pc; i++)
    {
        int tf = pseudoF[i], tr = pseudoR[i];
        int copyB[8][8];
        copyBoard(boardState, copyB);
        int captured = copyB[tr][tf];
        copyB[tr][tf] = copyB[r][f];
        copyB[r][f] = 0;

        int backup[8][8];
        copyBoard(boardState, backup);
        copyBoard(copyB, boardState);
        if (!isKingInCheck(isWhiteId(id)))
        {
            outF[outCount] = tf; outR[outCount++] = tr; 
        }
        copyBoard(backup, boardState);
    }
}

// Promotion
void promoteIfNeeded(int f, int r) 
{
    int id = boardState[r][f];
    if (id >= 1 && id <= 8 && r == 0) 
        boardState[r][f] = 15;
    if (id >= 17 && id <= 24 && r == 7) 
        boardState[r][f] = 31;
}

// Check for available moves
bool hasLegalMoves(bool white) 
{
    int tmpF[64], tmpR[64], cnt;
    for (int r = 0; r < 8; r++)
        for (int f = 0; f < 8; f++)
            if ((white && isWhiteId(boardState[r][f])) || (!white && isBlackId(boardState[r][f])))
            {
                genLegalMovesArray(f, r, tmpF, tmpR, cnt);
                if (cnt > 0) return true;
            }
    return false;
}

// Perform move
void performMoveApply(int fromF, int fromR, int toF, int toR) 
{
    int id = boardState[fromR][fromF];
    if (id == 0) 
        return;
    int captured = boardState[toR][toF];
    if (captured != 0) 
    {
        if (isWhiteId(captured))
            whiteCaptured++;
        else 
            blackCaptured++;
    }

    ep_col = -1;
    ep_row = -1;
    // Castling and en-passant simplified for brevity
    boardState[toR][toF] = boardState[fromR][fromF];
    boardState[fromR][fromF] = 0;
    promoteIfNeeded(toF, toR);

    whiteTurn = !whiteTurn;

    // Check endgame
    if (!hasLegalMoves(whiteTurn)) 
    {
        if (isKingInCheck(whiteTurn)) 
            endMessage = whiteTurn ? "White is checkmated!" : "Black is checkmated!";
        else
            endMessage = "Stalemate!";
        gameEnded = true;
    }
}

// Setup board
void setupBoard()
{
    for (int r = 0; r < 8; r++)
        for (int f = 0; f < 8; f++)
            boardState[r][f] = 0;
    for (int f = 0; f < 8; f++)
        boardState[6][f] = 1 + f; 
    boardState[7][0] = 9; 
    boardState[7][7] = 10; 
    boardState[7][1] = 11;
    boardState[7][6] = 12;
    boardState[7][2] = 13; 
    boardState[7][5] = 14;
    boardState[7][3] = 15;
    boardState[7][4] = 16;
    for (int f = 0; f < 8; f++)
    boardState[1][f] = 17 + f; 
    boardState[0][0] = 25;
    boardState[0][7] = 26;
    boardState[0][1] = 27;
    boardState[0][6] = 28;
    boardState[0][2] = 29;
    boardState[0][5] = 30;
    boardState[0][3] = 31; 
    boardState[0][4] = 32;
}

// Draw functions
void drawBoard(sf::RenderWindow& window)
{
    for (int r = 0; r < 8; r++)
        for (int f = 0; f < 8; f++) 
        {
            sf::RectangleShape tile(sf::Vector2f(TILE_SIZE, TILE_SIZE));
            tile.setPosition(tilePos(f, r));
            tile.setFillColor((f + r) % 2 == 0 ? sf::Color(245, 222, 179) : sf::Color(139, 69, 19));
            window.draw(tile);
        }
}

void drawLegalMoves(sf::RenderWindow& window)
{
    for (int i = 0; i < legalCount; i++)
    {
        sf::RectangleShape t(sf::Vector2f(TILE_SIZE, TILE_SIZE));
        t.setPosition(tilePos(legalF[i], legalR[i]));
        t.setFillColor(sf::Color(0, 255, 0, 100));
        window.draw(t);
    }
}

void drawPieces(sf::RenderWindow& window)
{
    sf::Texture* textures[33] = { nullptr,
        &wPawn,&wPawn,&wPawn,&wPawn,&wPawn,&wPawn,&wPawn,&wPawn,
        &wRook,&wRook,&wKnight,&wKnight,&wBishop,&wBishop,&wQueen,&wKing,
        &bPawn,&bPawn,&bPawn,&bPawn,&bPawn,&bPawn,&bPawn,&bPawn,
        &bRook,&bRook,&bKnight,&bKnight,&bBishop,&bBishop,&bQueen,&bKing
    };
    for (int r = 0; r < 8; r++)
        for (int f = 0; f < 8; f++)
        {
            int id = boardState[r][f]; if (id == 0) 
                continue;
            sf::Sprite s; 
            s.setTexture(*textures[id]);
            float scX = static_cast<float>(TILE_SIZE) / static_cast<float>(s.getTexture()->getSize().x);
            float scY = static_cast<float>(TILE_SIZE) / static_cast<float>(s.getTexture()->getSize().y);
            s.setScale(scX, scY);
            s.setPosition(tilePos(f, r));
            window.draw(s);
        }
}

int main() 
{
    if (!loadTextures())
    {
        cout << "Failed to load textures!\n";
        return 1; 
    }
    setupBoard();

    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) 
    {
        cout << "Failed to load font!\n";
        return 1;
    }

    sf::RenderWindow window(sf::VideoMode(TILE_SIZE * 8, TILE_SIZE * 8 + 50), "Chess SFML");

    while (window.isOpen())
    {
        sf::Event ev;
        while (window.pollEvent(ev))
        {
            if (ev.type == sf::Event::Closed) window.close();
            if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left && !gameEnded)
            {
                int f = ev.mouseButton.x / TILE_SIZE;
                int r = ev.mouseButton.y / TILE_SIZE;
                int id = boardState[r][f];

                if (pieceSelected)
                {
                    bool ok = false;
                    for (int i = 0; i < legalCount; i++)
                        if (legalF[i] == f && legalR[i] == r)
                        {
                            ok = true; break; 
                        }
                    if (ok) performMoveApply(selF, selR, f, r);
                    pieceSelected = false;
                }
                else
                {
                    if (id == 0)
                        continue;
                    if (whiteTurn && !isWhiteId(id))
                        continue;
                    if (!whiteTurn && !isBlackId(id)) 
                        continue;
                    selF = f; 
                    selR = r;
                    pieceSelected = true;
                    genLegalMovesArray(f, r, legalF, legalR, legalCount);
                }
            }
        }

        window.clear(sf::Color::White);
        drawBoard(window);
        if (pieceSelected) drawLegalMoves(window);
        drawPieces(window);
        int i = 1;
        if (gameEnded) 
        {
            sf::Text text;
            text.setFont(font);
            text.setString(endMessage);
            text.setCharacterSize(70);
            text.setFillColor(sf::Color::Black);
            sf::FloatRect bounds = text.getLocalBounds();
            text.setOrigin(bounds.width / 2, bounds.height / 2);
            text.setPosition(TILE_SIZE * 4, TILE_SIZE * 2.5f);
            window.draw(text);
          
        }


        sf::Text score;
        score.setFont(font);
        score.setString("White captured: " + to_string(whiteCaptured) + " | Black captured: " + to_string(blackCaptured));
        score.setCharacterSize(24);
        score.setFillColor(sf::Color::Black);
        score.setPosition(10, TILE_SIZE * 8 + 5);
        window.draw(score);

        window.display();
        
    }

    return 0;
}
