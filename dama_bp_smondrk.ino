#include <Adafruit_NeoPixel.h>
#include <SPI.h>

#define PIN 6
#define NUM_PIXELS 64  // 8x8
Adafruit_NeoPixel strip(NUM_PIXELS, PIN, NEO_GRB + NEO_KHZ800);

const uint8_t BOARD_SIZE = 8;

struct Piece {
  bool active;
  bool isKing;
  bool isRed; // inak false = black
};

Piece board[BOARD_SIZE][BOARD_SIZE];

bool redTurn = true; 
int selectedX = -1;  
int selectedY = -1;
int cursorX = 0;
int cursorY = 0;

const int btnUp = 2;
const int btnDown = 3;
const int btnLeft = 4;
const int btnRight = 5;
const int btnSelect = 8;
const int btnMove = 7;

// Farby
uint32_t darkColor       = 0x321900; // tmavšia hnedá
uint32_t lightColor      = 0x964B00; // svetlejšia hnedá
uint32_t redPieceColor   = 0xFF0000; // červená 
uint32_t blackPieceColor = 0x0000FF; // modrá 
uint32_t kingOverlay     = 0xFFFF00; // žltá
uint32_t invalidMoveCol  = 0x00FFFF; // tyrkysová na signalizáciu chybného ťahu

uint8_t xyToIndex(uint8_t x, uint8_t y) {
  return y * BOARD_SIZE + x;
}

void setPixelColorXY(uint8_t x, uint8_t y, uint32_t color) {
  if (x < BOARD_SIZE && y < BOARD_SIZE) {
    strip.setPixelColor(xyToIndex(x, y), color);
  }
}

// Pomocná funkcia na zmiešanie dvoch farieb
uint32_t mixColor(uint32_t c1, uint32_t c2) {
  uint8_t r1 = (uint8_t)(c1 >> 16);
  uint8_t g1 = (uint8_t)(c1 >> 8);
  uint8_t b1 = (uint8_t)c1;

  uint8_t r2 = (uint8_t)(c2 >> 16);
  uint8_t g2 = (uint8_t)(c2 >> 8);
  uint8_t b2 = (uint8_t)c2;

  uint8_t rMix = (r1 + r2) / 2;
  uint8_t gMix = (g1 + g2) / 2;
  uint8_t bMix = (b1 + b2) / 2;
  return strip.Color(rMix, gMix, bMix);
}

// UPRAVENÁ funkcia initBoard() pre 2 rady figúriek na každej strane
void initBoard() {
  for (uint8_t y = 0; y < BOARD_SIZE; y++) {
    for (uint8_t x = 0; x < BOARD_SIZE; x++) {
      board[y][x].active = false;
      board[y][x].isKing = false;
      board[y][x].isRed = false;
    }
  }

  // Červené figúrky 
  for (uint8_t y = 0; y < 2; y++) {
    for (uint8_t x = 0; x < BOARD_SIZE; x++) {
      if ((x + y) % 2 == 1) { 
        board[y][x].active = true;
        board[y][x].isRed = true;
      }
    }
  }

  // Čierne figúrky 
  for (uint8_t y = BOARD_SIZE - 2; y < BOARD_SIZE; y++) {
    for (uint8_t x = 0; x < BOARD_SIZE; x++) {
      if ((x + y) % 2 == 1) {
        board[y][x].active = true;
        board[y][x].isRed = false;
      }
    }
  }
}

// Zistí, či daný hráč má nejaké aktívne figúrky
bool hasPieces(bool red) {
  for (uint8_t y = 0; y < BOARD_SIZE; y++) {
    for (uint8_t x = 0; x < BOARD_SIZE; x++) {
      if (board[y][x].active && board[y][x].isRed == red) return true;
    }
  }
  return false;
}

// Zistí, či hráč môže vôbec urobiť nejaký ťah alebo skok
bool canPlayerMove(bool red);

// Kontrola hry - či už neskončila
bool checkGameOver() {
  bool redHas = hasPieces(true);
  bool blackHas = hasPieces(false);
  if (!redHas) {
    Serial.println("Hra skoncila! Vyhral cierny.");
    return true;
  }
  if (!blackHas) {
    Serial.println("Hra skoncila! Vyhral cerveny.");
    return true;
  }
  if (!canPlayerMove(redTurn)) {
    if (redTurn) {
      Serial.println("Cerveny nema tah, vyhral cierny.");
    } else {
      Serial.println("Cierny nema tah, vyhral cerveny.");
    }
    return true;
  }
  return false;
}

// Funkcia na vykreslenie celej dosky
void drawBoardAndPieces() {
  for (uint8_t y = 0; y < BOARD_SIZE; y++) {
    for (uint8_t x = 0; x < BOARD_SIZE; x++) {
      bool isDark = ((x + y) % 2 == 0);
      uint32_t cellColor = isDark ? lightColor : darkColor;
      if (board[y][x].active) {
        cellColor = board[y][x].isRed ? redPieceColor : blackPieceColor;
        if (board[y][x].isKing) {
          cellColor = mixColor(cellColor, kingOverlay);
        }
      }
      if (x == (uint8_t)selectedX && y == (uint8_t)selectedY) {
        cellColor = mixColor(cellColor, 0x00FF00);
      }
      if (x == (uint8_t)cursorX && y == (uint8_t)cursorY) {
        cellColor = mixColor(cellColor, 0x00FF00);
      }
      setPixelColorXY(x, y, cellColor);
    }
  }
  strip.show();
}

bool inBounds(int x, int y) {
  return x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE;
}

bool canMove(int fromX, int fromY, int toX, int toY) {
  if (!inBounds(fromX, fromY) || !inBounds(toX, toY)) return false;
  if (!board[fromY][fromX].active) return false;
  if (board[toY][toX].active) return false;
  Piece p = board[fromY][fromX];
  int dy = toY - fromY;
  int dx = toX - fromX;
  if (!p.isKing) {
    if (p.isRed && dy != 1) return false;
    if (!p.isRed && dy != -1) return false;
  } else {
    if (dy != 1 && dy != -1) return false;
  }
  if (dx != 1 && dx != -1) return false;
  return true;
}

bool canJumpDirection(int x, int y, int dx, int dy) {
  if (!inBounds(x, y)) return false;
  if (!board[y][x].active) return false;
  Piece p = board[y][x];
  int midX = x + dx; 
  int midY = y + dy;
  int landX = x + 2*dx;
  int landY = y + 2*dy;
  if (!inBounds(midX, midY) || !inBounds(landX, landY)) return false;
  if (board[landY][landX].active) return false;
  if (!board[midY][midX].active) return false;
  if (board[midY][midX].isRed == p.isRed) return false;
  if (!p.isKing) {
    if (p.isRed && dy != 1) return false;
    if (!p.isRed && dy != -1) return false;
  }
  return true;
}

bool canJump(int x, int y) {
  if (!inBounds(x, y)) return false;
  if (!board[y][x].active) return false;
  Piece p = board[y][x];
  int directions[4][2] = { {1,1},{-1,1},{1,-1},{-1,-1} };
  for (int i=0; i<4; i++) {
    int dx = directions[i][0];
    int dy = directions[i][1];
    if (canJumpDirection(x, y, dx, dy)) return true;
  }
  return false;
}

bool playerHasJump(bool red) {
  for (uint8_t y = 0; y < BOARD_SIZE; y++) {
    for (uint8_t x = 0; x < BOARD_SIZE; x++) {
      if (board[y][x].active && board[y][x].isRed == red) {
        if (canJump(x, y)) return true;
      }
    }
  }
  return false;
}

bool canPlayerMove(bool red) {
  if (playerHasJump(red)) return true;
  for (uint8_t y = 0; y < BOARD_SIZE; y++) {
    for (uint8_t x = 0; x < BOARD_SIZE; x++) {
      if (board[y][x].active && board[y][x].isRed == red) {
        Piece p = board[y][x];
        int dyOptions[2];
        int dyCount = 1;
        if (p.isKing) {
          dyOptions[0] = 1;
          dyOptions[1] = -1;
          dyCount = 2;
        } else {
          dyOptions[0] = p.isRed ? 1 : -1;
        }
        int dxOptions[2] = {1,-1};
        for (int i=0; i<dyCount; i++) {
          for (int j=0; j<2; j++) {
            int toX = x + dxOptions[j];
            int toY = y + dyOptions[i];
            if (canMove(x,y,toX,toY)) return true;
          }
        }
      }
    }
  }
  return false;
}

bool doJump(int fromX, int fromY, int toX, int toY) {
  int dx = (toX - fromX) / 2;
  int dy = (toY - fromY) / 2;
  int midX = fromX + dx;
  int midY = fromY + dy;
  board[midY][midX].active = false;
  board[toY][toX] = board[fromY][fromX];
  board[fromY][fromX].active = false;
  if ((board[toY][toX].isRed && toY == BOARD_SIZE - 1) ||
      (!board[toY][toX].isRed && toY == 0)) {
    // Ak ešte nie je dáma, povýšime ju a vypíšeme správu.
    if (!board[toY][toX].isKing) {
      board[toY][toX].isKing = true;
      Serial.println("Hráč urobil dámu!");
    }
  }
  return true;
}

bool doMove(int fromX, int fromY, int toX, int toY) {
  board[toY][toX] = board[fromY][fromX];
  board[fromY][fromX].active = false;
  if ((board[toY][toX].isRed && toY == BOARD_SIZE - 1) ||
      (!board[toY][toX].isRed && toY == 0)) {
    // Ak ešte nie je dáma, povýšime ju a vypíšeme správu.
    if (!board[toY][toX].isKing) {
      board[toY][toX].isKing = true;
      Serial.println("Hráč urobil dámu!");
    }
  }
  return true;
}

bool attemptMove(int fromX, int fromY, int toX, int toY) {
  if (!inBounds(fromX,fromY) || !inBounds(toX,toY)) return false;
  if (!board[fromY][fromX].active) return false;
  Piece p = board[fromY][fromX];
  if (p.isRed != redTurn) return false;
  bool jumpAvailable = playerHasJump(redTurn);
  int dx = toX - fromX;
  int dy = toY - fromY;
  if (jumpAvailable) {
    if (abs(dx) == 2 && abs(dy) == 2) {
      int midX = fromX + dx/2;
      int midY = fromY + dy/2;
      if (inBounds(midX, midY) && board[midY][midX].active && board[midY][midX].isRed != p.isRed && !board[toY][toX].active) {
        if (!p.isKing) {
          if (p.isRed && dy != 2) return false;
          if (!p.isRed && dy != -2) return false;
        }
        doJump(fromX, fromY, toX, toY);
        return true;
      }
    }
    return false;
  } else {
    if (canMove(fromX, fromY, toX, toY)) {
      doMove(fromX, fromY, toX, toY);
      return true;
    }
    return false;
  }
}

bool canContinueJump(int x, int y) {
  return canJump(x, y);
}

void nextTurn() {
  redTurn = !redTurn;
}

void signalInvalidMove() {
  for (uint8_t y=0; y<BOARD_SIZE; y++) {
    for (uint8_t x=0; x<BOARD_SIZE; x++) {
      setPixelColorXY(x,y,invalidMoveCol);
    }
  }
  strip.show();
  delay(200);
  drawBoardAndPieces();
}

void setup() {
  Serial.begin(9600);
  pinMode(btnUp, INPUT_PULLUP);
  pinMode(btnDown, INPUT_PULLUP);
  pinMode(btnLeft, INPUT_PULLUP);
  pinMode(btnRight, INPUT_PULLUP);
  pinMode(btnSelect, INPUT_PULLUP);
  pinMode(btnMove, INPUT_PULLUP);
  
  strip.begin();
  // Nižší jas
  strip.setBrightness(50);
  strip.show(); // vypnutie všetkych LED

  initBoard();
  drawBoardAndPieces();
  Serial.println("Hra dama spustena. Zacina cerveny.");
}

void loop() {
  if (digitalRead(btnUp) == LOW) {
    if (cursorY > 0) cursorY--;
    delay(150);
  }
  if (digitalRead(btnDown) == LOW) {
    if (cursorY < BOARD_SIZE - 1) cursorY++;
    delay(150);
  }
  if (digitalRead(btnLeft) == LOW) {
    if (cursorX > 0) cursorX--;
    delay(150);
  }
  if (digitalRead(btnRight) == LOW) {
    if (cursorX < BOARD_SIZE - 1) cursorX++;
    delay(150);
  }
  if (digitalRead(btnSelect) == LOW) {
    if (selectedX == cursorX && selectedY == cursorY) {
      selectedX = -1;
      selectedY = -1;
    } else {
      if (board[cursorY][cursorX].active && board[cursorY][cursorX].isRed == redTurn) {
        selectedX = cursorX;
        selectedY = cursorY;
      } else {
        Serial.println("Nie je to tvoja figurka!");
      }
    }
    delay(200);
  }
  if (digitalRead(btnMove) == LOW) {
    if (selectedX != -1 && selectedY != -1) {
      int fromX = selectedX;
      int fromY = selectedY;
      int toX = cursorX;
      int toY = cursorY;
      bool wasJumpAvailable = playerHasJump(redTurn);
      if (attemptMove(fromX, fromY, toX, toY)) {
        selectedX = toX;
        selectedY = toY;
        if (wasJumpAvailable) {
          if (canContinueJump(selectedX, selectedY)) {
            Serial.println("Pokračuj v skákaní tou istou figurkou.");
          } else {
            selectedX = -1;
            selectedY = -1;
            nextTurn();
          }
        } else {
          selectedX = -1;
          selectedY = -1;
          nextTurn();
        }
      } else {
        Serial.println("Neplatny tah!");
        signalInvalidMove();
      }
    }
    delay(200);
  }
  if (checkGameOver()) {
    while(true) {}
  }
  drawBoardAndPieces();
}
