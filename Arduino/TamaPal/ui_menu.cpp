// WHAT: Implementation of top menu drawing, icon vector art, and smooth sliding frame
// WHY:  Renders directly to the ST7789 display without lag or flickering
// USED BY: TamaPal.ino

#include "ui_menu.h"
#include <string.h>

UiMenuRenderer::UiMenuRenderer()
  : carrier(nullptr),
    lastDrawnCursorX(-1.0f),
    lastDrawnIcon((MenuIcon)99),
    lastDrawnScreen((ScreenType)99),
    isMenuInitialized(false) {
}

void UiMenuRenderer::begin(MKRIoTCarrier* hardwareCarrier) {
  carrier = hardwareCarrier;
}

// Draw static dark background and screen border once at startup
void UiMenuRenderer::drawInitialScreen() {
  if (carrier == nullptr) return;

  // Dark retro navy background
  carrier->display.fillScreen(color565(8, 12, 26));

  // Circular inner bezel highlight (subtle boundary guide)
  carrier->display.drawCircle(120, 120, 118, color565(25, 45, 80));

  // Subtle separator line below the top menu bar (width 152 px, fits safely inside circle)
  carrier->display.drawFastHLine(44, 66, 152, color565(30, 50, 90));

  // Touch button guide at the bottom of the screen (tightly centered so round bezel won't cut it off)
  carrier->display.setTextSize(1);
  carrier->display.setTextColor(color565(110, 140, 190));
  carrier->display.setCursor(72, 202);
  carrier->display.print("T0:<  T1:>  T2:OK");
  carrier->display.setCursor(69, 213);
  carrier->display.print("T3:Tilb  T4:Stats");
}

// Icon 0: Mad (Ramen bowl with noodles lifted by chopsticks, 2.5D shading and steam)
void UiMenuRenderer::drawIconFood(int centerX, int centerY, bool isSelected) {
  uint16_t bowlColor   = isSelected ? color565(255, 65, 80)  : color565(190, 45, 60);  // Ceramic red bowl
  uint16_t bowlShadow  = isSelected ? color565(140, 25, 40)  : color565(110, 20, 30);  // 2.5D depth shadow
  uint16_t bowlFoot    = isSelected ? color565(110, 18, 28)  : color565(80, 15, 22);   // Bottom stand
  uint16_t noodleColor = isSelected ? color565(255, 225, 50) : color565(190, 160, 40); // Golden noodles
  uint16_t stickColor  = isSelected ? color565(240, 185, 110): color565(170, 120, 60); // Wooden chopsticks
  uint16_t rimColor    = isSelected ? color565(255, 255, 255): color565(180, 190, 200); // Glossy ceramic rim
  uint16_t steamColor  = isSelected ? color565(255, 255, 255): color565(140, 170, 200); // Cute steam swirl

  // 1. Cute steam swirl rising at top
  carrier->display.drawPixel(centerX - 2, centerY - 8, steamColor);
  carrier->display.drawPixel(centerX - 1, centerY - 9, steamColor);
  carrier->display.drawPixel(centerX,     centerY - 8, steamColor);

  // 2. Pair of wooden chopsticks held across top
  carrier->display.drawLine(centerX - 7, centerY - 6, centerX + 7, centerY - 4, stickColor);
  carrier->display.drawLine(centerX - 7, centerY - 4, centerX + 7, centerY - 2, stickColor);

  // 3. Golden noodles lifted up and hanging into bowl
  carrier->display.drawLine(centerX - 3, centerY - 4, centerX - 4, centerY + 1, noodleColor);
  carrier->display.drawLine(centerX - 1, centerY - 4, centerX - 1, centerY + 1, noodleColor);
  carrier->display.drawLine(centerX + 1, centerY - 3, centerX + 1, centerY + 1, noodleColor);
  carrier->display.drawLine(centerX + 3, centerY - 3, centerX + 4, centerY + 1, noodleColor);

  // 4. Ceramic ramen bowl with glossy rim and 2.5D bottom curve
  carrier->display.drawFastHLine(centerX - 7, centerY + 1, 15, rimColor);
  carrier->display.fillRoundRect(centerX - 6, centerY + 2, 13, 5, 2, bowlColor);
  carrier->display.drawFastHLine(centerX - 4, centerY + 2, 9, noodleColor); // Noodles visible inside
  carrier->display.drawFastHLine(centerX - 5, centerY + 6, 11, bowlShadow); // 2.5D depth shadow
  carrier->display.drawFastHLine(centerX - 3, centerY + 7, 7, bowlFoot);    // Bowl foot stand
}

// Icon 1: Leg (Classic 3D soccer ball with pentagons, light highlight and shadow)
void UiMenuRenderer::drawIconPlay(int centerX, int centerY, bool isSelected) {
  uint16_t ballWhite  = isSelected ? color565(255, 255, 255) : color565(215, 225, 240);
  uint16_t ballShadow = isSelected ? color565(160, 180, 205) : color565(120, 135, 155);
  uint16_t darkPatch  = isSelected ? color565(25, 30, 48)    : color565(40, 45, 65);
  uint16_t outline    = isSelected ? color565(0, 230, 255)   : color565(50, 75, 110);

  // 1. White circular ball base with 2.5D bottom shadow arc
  carrier->display.fillCircle(centerX, centerY, 7, ballWhite);
  carrier->display.drawCircle(centerX + 1, centerY + 1, 6, ballShadow);
  carrier->display.drawCircle(centerX, centerY, 7, outline);

  // 2. Center dark pentagon
  carrier->display.drawFastHLine(centerX - 1, centerY - 1, 3, darkPatch);
  carrier->display.fillRect(centerX - 2, centerY, 5, 2, darkPatch);
  carrier->display.drawFastHLine(centerX - 1, centerY + 2, 3, darkPatch);

  // 3. Seam lines connecting outward
  carrier->display.drawLine(centerX, centerY - 1, centerX, centerY - 7, darkPatch);
  carrier->display.drawLine(centerX - 2, centerY, centerX - 6, centerY - 3, darkPatch);
  carrier->display.drawLine(centerX + 2, centerY, centerX + 6, centerY - 3, darkPatch);
  carrier->display.drawLine(centerX - 1, centerY + 2, centerX - 5, centerY + 5, darkPatch);
  carrier->display.drawLine(centerX + 1, centerY + 2, centerX + 5, centerY + 5, darkPatch);

  // 4. Edge black patches
  carrier->display.drawFastHLine(centerX - 1, centerY - 7, 3, darkPatch);
  carrier->display.drawPixel(centerX - 6, centerY - 2, darkPatch);
  carrier->display.drawPixel(centerX + 6, centerY - 2, darkPatch);
  carrier->display.drawPixel(centerX - 4, centerY + 5, darkPatch);
  carrier->display.drawPixel(centerX + 4, centerY + 5, darkPatch);

  // 5. 2.5D specular light highlight on top-left
  carrier->display.drawPixel(centerX - 3, centerY - 4, color565(255, 255, 255));
}

// Icon 2: Hjem (Lumis Hus: Cozy house with terracotta roof, chimney & door)
void UiMenuRenderer::drawIconHouse(int centerX, int centerY, bool isSelected) {
  if (carrier == nullptr) return;

  uint16_t roofColor  = isSelected ? color565(255, 120, 60) : color565(200, 80, 40);   // Terracotta roof
  uint16_t wallColor  = isSelected ? color565(255, 230, 180) : color565(200, 180, 140); // Cozy wooden wall
  uint16_t doorColor  = color565(110, 60, 30);                                          // Dark brown door
  uint16_t winColor   = isSelected ? color565(0, 240, 255) : color565(255, 220, 80);    // Glowing cyan/yellow window

  // Triangular roof
  carrier->display.drawPixel(centerX, centerY - 6, roofColor);
  carrier->display.drawFastHLine(centerX - 1, centerY - 5, 3, roofColor);
  carrier->display.drawFastHLine(centerX - 2, centerY - 4, 5, roofColor);
  carrier->display.drawFastHLine(centerX - 3, centerY - 3, 7, roofColor);
  carrier->display.drawFastHLine(centerX - 4, centerY - 2, 9, roofColor);
  carrier->display.drawFastHLine(centerX - 5, centerY - 1, 11, roofColor);

  // Chimney on right
  carrier->display.drawFastVLine(centerX + 3, centerY - 6, 3, color565(150, 60, 30));

  // House body
  carrier->display.fillRect(centerX - 4, centerY, 9, 6, wallColor);

  // Door in center
  carrier->display.fillRect(centerX - 1, centerY + 2, 3, 4, doorColor);

  // Window on left
  carrier->display.drawPixel(centerX - 3, centerY + 2, winColor);
}

// Icon 3: Medicin (Apothecary medicine bottle with healing red cross +)
void UiMenuRenderer::drawIconMedicin(int centerX, int centerY, bool isSelected) {
  uint16_t capColor    = isSelected ? color565(200, 150, 90)  : color565(140, 100, 60); // Wood cork / cap
  uint16_t liquidColor = isSelected ? color565(220, 245, 255) : color565(160, 190, 210); // Clear medical liquid
  uint16_t outline     = isSelected ? color565(0, 210, 230)   : color565(50, 80, 100);
  uint16_t crossRed    = isSelected ? color565(255, 40, 60)   : color565(200, 30, 45);   // Medical cross
  uint16_t gleamWhite  = color565(255, 255, 255);

  // 1. Cap / Cork (top)
  carrier->display.fillRect(centerX - 2, centerY - 8, 5, 2, capColor);
  carrier->display.drawPixel(centerX - 2, centerY - 8, color565(240, 190, 130)); // Cap highlight

  // 2. Bottle neck
  carrier->display.fillRect(centerX - 1, centerY - 6, 3, 2, liquidColor);

  // 3. Bottle body (rounded rect)
  carrier->display.fillRoundRect(centerX - 5, centerY - 4, 11, 12, 2, liquidColor);
  carrier->display.drawRoundRect(centerX - 5, centerY - 4, 11, 12, 2, outline);

  // 4. Specular glass highlight (left rim)
  carrier->display.drawFastVLine(centerX - 4, centerY - 2, 7, gleamWhite);

  // 5. Red medical cross (+)
  // Horizontal bar
  carrier->display.fillRect(centerX - 3, centerY + 1, 7, 3, crossRed);
  // Vertical bar
  carrier->display.fillRect(centerX - 1, centerY - 1, 3, 7, crossRed);
}

// Icon 3: Skæld ud (Cute angry mochi face with > < squint eyes, blush and 💢 anger mark)
void UiMenuRenderer::drawIconScold(int centerX, int centerY, bool isSelected) {
  uint16_t faceColor   = isSelected ? color565(255, 80, 90)   : color565(190, 60, 70);
  uint16_t glossColor  = isSelected ? color565(255, 140, 150) : color565(210, 95, 105);
  uint16_t outlineFace = isSelected ? color565(140, 25, 35)   : color565(100, 20, 28);
  uint16_t darkFeature = color565(20, 20, 35);
  uint16_t blushColor  = isSelected ? color565(255, 160, 185) : color565(200, 110, 130);
  uint16_t angerColor  = isSelected ? color565(255, 220, 50)  : color565(190, 150, 40);

  // 1. Round puffed-up mochi face with 2.5D top-left light volume
  carrier->display.fillCircle(centerX, centerY, 7, faceColor);
  carrier->display.fillCircle(centerX - 2, centerY - 2, 3, glossColor);
  carrier->display.drawCircle(centerX, centerY, 7, outlineFace);

  // 2. Angry squinting > < eyes
  // Left eye (>)
  carrier->display.drawLine(centerX - 5, centerY - 2, centerX - 3, centerY - 1, darkFeature);
  carrier->display.drawLine(centerX - 5, centerY, centerX - 3, centerY - 1, darkFeature);
  // Right eye (<)
  carrier->display.drawLine(centerX + 5, centerY - 2, centerX + 3, centerY - 1, darkFeature);
  carrier->display.drawLine(centerX + 5, centerY, centerX + 3, centerY - 1, darkFeature);

  // 3. Furrowed angry eyebrows (\ /)
  carrier->display.drawLine(centerX - 5, centerY - 4, centerX - 2, centerY - 3, darkFeature);
  carrier->display.drawLine(centerX + 5, centerY - 4, centerX + 2, centerY - 3, darkFeature);

  // 4. Cute puffy blushed cheeks
  carrier->display.drawPixel(centerX - 4, centerY + 2, blushColor);
  carrier->display.drawPixel(centerX + 4, centerY + 2, blushColor);

  // 5. Open grumpy pout
  carrier->display.drawFastHLine(centerX - 2, centerY + 3, 5, darkFeature);
  carrier->display.drawPixel(centerX, centerY + 4, color565(140, 15, 25));

  // 6. Cute manga anger mark 💢 on top-right
  carrier->display.drawPixel(centerX + 5, centerY - 6, angerColor);
  carrier->display.drawPixel(centerX + 6, centerY - 5, angerColor);
  carrier->display.drawPixel(centerX + 5, centerY - 4, angerColor);
  carrier->display.drawPixel(centerX + 4, centerY - 5, angerColor);
}

// Icon 3: Statistik (Modern 2.5D glossy capsule bars with neon tips and high-contrast baseline)
void UiMenuRenderer::drawIconStats(int centerX, int centerY, bool isSelected) {
  uint16_t capColor   = isSelected ? color565(160, 250, 255) : color565(100, 180, 210);
  uint16_t barColor   = isSelected ? color565(0, 200, 240)   : color565(0, 130, 175);
  uint16_t trackColor = isSelected ? color565(35, 70, 120)   : color565(25, 45, 80);
  uint16_t starColor  = isSelected ? color565(255, 230, 70)  : color565(170, 150, 40);

  // 1. Subtle baseline guide
  carrier->display.drawFastHLine(centerX - 8, centerY + 7, 17, trackColor);

  // 2. Bar 1 (left, small capsule)
  carrier->display.fillRoundRect(centerX - 6, centerY + 2, 3, 5, 1, barColor);
  carrier->display.drawPixel(centerX - 5, centerY + 2, capColor);

  // 3. Bar 2 (center, medium capsule)
  carrier->display.fillRoundRect(centerX - 1, centerY - 2, 3, 9, 1, barColor);
  carrier->display.drawPixel(centerX, centerY - 2, capColor);

  // 4. Bar 3 (right, tall capsule)
  carrier->display.fillRoundRect(centerX + 4, centerY - 6, 3, 13, 1, barColor);
  carrier->display.drawPixel(centerX + 5, centerY - 6, capColor);

  // 5. Star / sparkle over the peak bar
  carrier->display.drawPixel(centerX + 5, centerY - 8, starColor);
}

// Icon 4: Forbind (Multiplayer duo gamepads: P1 front + P2 back, wired to online cloud)
void UiMenuRenderer::drawIconConnect(int centerX, int centerY, bool isSelected) {
  // Colors for Player 1 (Foreground modern console controller)
  uint16_t p1Body     = isSelected ? color565(245, 250, 255) : color565(170, 185, 205);
  uint16_t p1Shadow   = isSelected ? color565(120, 140, 170) : color565(80, 95, 120);
  uint16_t p1Dpad     = color565(30, 35, 55);
  uint16_t buttonRed  = isSelected ? color565(255, 60, 75)   : color565(180, 40, 50);
  uint16_t buttonLime = isSelected ? color565(60, 240, 110)  : color565(40, 160, 75);
  uint16_t buttonGold = isSelected ? color565(255, 220, 60)  : color565(180, 150, 40);
  uint16_t p1Led      = isSelected ? color565(0, 230, 255)   : color565(0, 140, 180);

  // Colors for Player 2 (Background multiplayer controller - warm coral/orange)
  uint16_t p2Body     = isSelected ? color565(255, 120, 70)  : color565(175, 80, 45);
  uint16_t p2Shadow   = isSelected ? color565(160, 55, 25)   : color565(110, 35, 15);
  uint16_t p2Dpad     = color565(30, 25, 40);
  uint16_t p2Button   = isSelected ? color565(255, 225, 70)  : color565(190, 150, 40);
  uint16_t p2Led      = isSelected ? color565(255, 190, 90)  : color565(150, 100, 45);

  // Online network wires and cloud node
  uint16_t cableColor = isSelected ? color565(0, 230, 255)   : color565(0, 150, 190);
  uint16_t cloudColor = isSelected ? color565(140, 230, 255) : color565(80, 150, 190);
  uint16_t socketColor= isSelected ? color565(75, 95, 125)   : color565(50, 65, 90);

  // 1. Internet cloud node at top-left
  carrier->display.drawFastHLine(centerX - 7, centerY - 8, 5, cloudColor);
  carrier->display.drawFastHLine(centerX - 6, centerY - 9, 3, isSelected ? color565(230, 255, 255) : color565(150, 200, 240));

  // 2. Cables from both controllers snaking into the cloud
  // P1 cable:
  carrier->display.drawPixel(centerX - 2, centerY - 1, cableColor);
  carrier->display.drawPixel(centerX - 3, centerY - 2, cableColor);
  carrier->display.drawPixel(centerX - 4, centerY - 3, cableColor);
  carrier->display.drawPixel(centerX - 5, centerY - 5, cableColor);
  carrier->display.drawPixel(centerX - 4, centerY - 7, cableColor);
  // P2 cable:
  carrier->display.drawFastHLine(centerX - 1, centerY - 8, 3, cableColor);
  carrier->display.drawPixel(centerX + 2, centerY - 7, cableColor);

  // 3. Player 2 Gamepad (in background, top-right)
  carrier->display.fillRoundRect(centerX - 1, centerY - 6, 10, 5, 2, p2Body);
  carrier->display.drawPixel(centerX - 1, centerY - 1, p2Shadow); // Left grip tip
  carrier->display.drawPixel(centerX + 8, centerY - 1, p2Shadow); // Right grip tip
  carrier->display.drawFastHLine(centerX + 1, centerY - 2, 6, p2Shadow); // Notch shadow
  carrier->display.drawFastHLine(centerX + 2, centerY - 7, 3, socketColor); // Plug socket
  carrier->display.drawPixel(centerX + 1, centerY - 4, p2Dpad);   // D-pad
  carrier->display.drawPixel(centerX + 3, centerY - 5, p2Led);    // P2 LED
  carrier->display.drawPixel(centerX + 6, centerY - 4, p2Button); // Action button

  // 4. Player 1 Gamepad (in foreground, larger with ergonomic grips)
  // 4a. Main upper chassis and grip extensions
  carrier->display.fillRoundRect(centerX - 7, centerY + 1, 14, 6, 2, p1Body);
  carrier->display.fillRect(centerX - 7, centerY + 5, 4, 3, p1Body); // Left handle
  carrier->display.fillRect(centerX + 3, centerY + 5, 4, 3, p1Body); // Right handle
  // Center notch between handles stays open (transparent background)

  // 4b. 2.5D bottom grip depth shadows
  carrier->display.drawFastHLine(centerX - 6, centerY + 8, 3, p1Shadow);
  carrier->display.drawFastHLine(centerX + 4, centerY + 8, 3, p1Shadow);
  carrier->display.drawFastHLine(centerX - 3, centerY + 5, 6, p1Shadow); // Center notch shadow
  carrier->display.drawFastHLine(centerX - 3, centerY, 3, socketColor);  // Top socket plug

  // 4c. D-pad cross on left grip (+)
  carrier->display.drawPixel(centerX - 5, centerY + 3, p1Dpad);
  carrier->display.drawPixel(centerX - 6, centerY + 4, p1Dpad);
  carrier->display.drawPixel(centerX - 5, centerY + 4, p1Dpad);
  carrier->display.drawPixel(centerX - 4, centerY + 4, p1Dpad);
  carrier->display.drawPixel(centerX - 5, centerY + 5, p1Dpad);

  // 4d. Center Player 1 Cyan status/sync LED
  carrier->display.drawPixel(centerX - 1, centerY + 3, p1Led);

  // 4e. Colorful candy action buttons on right grip
  carrier->display.drawPixel(centerX + 5, centerY + 3, buttonLime);   // Top: lime
  carrier->display.drawPixel(centerX + 4, centerY + 5, buttonRed);    // Bottom: red
  carrier->display.drawPixel(centerX + 6, centerY + 5, buttonGold);   // Right: gold
}

// Draw or update top menu bar without screen flicker (no global fillRect during sliding)
void UiMenuRenderer::drawMenuBar(const MenuLogic& menu, bool forceRedrawAll) {
  if (carrier == nullptr) return;

  float currentX = menu.getCursorPositionX();
  MenuIcon currentIcon = menu.getSelectedIcon();

  bool cursorMoved = (currentX != lastDrawnCursorX);
  bool iconChanged = (currentIcon != lastDrawnIcon);

  if (!forceRedrawAll && !cursorMoved && !iconChanged) {
    return;
  }

  uint16_t bgColor = color565(8, 12, 26);

  // 1. Initial draw or forced full redraw
  if (forceRedrawAll || !isMenuInitialized) {
    carrier->display.fillRect(38, 14, 164, 50, bgColor);
    isMenuInitialized = true;
  } else if (cursorMoved) {
    // Erase ONLY the previous frame outline with background color (no black flash!)
    int oldFrameX = (int)lastDrawnCursorX - 12;
    int oldFrameY = MENU_BAR_CENTER_Y - 12;
    carrier->display.drawRoundRect(oldFrameX, oldFrameY, 24, 24, 4, bgColor);
    carrier->display.drawRoundRect(oldFrameX - 1, oldFrameY - 1, 26, 26, 5, bgColor);
  }

  // 2. Draw/refresh the 5 icons cleanly without wiping the background
  for (int i = 0; i < ICON_TOTAL_COUNT; i++) {
    MenuIcon icon = (MenuIcon)i;
    int iconCenterX = (int)MenuLogic::calculateIconCenterX(icon);
    int iconCenterY = MENU_BAR_CENTER_Y;
    bool isSelected = (icon == currentIcon);

    if (isSelected) {
      iconCenterY -= 2; // Selected icon hops up slightly
    }

    // Clear icon's internal bounding box to prevent ghost pixels without screen-wide flicker
    carrier->display.fillRect(iconCenterX - 8, MENU_BAR_CENTER_Y - 11, 17, 20, bgColor);

    switch (icon) {
      case ICON_FOOD:    drawIconFood(iconCenterX, iconCenterY, isSelected); break;
      case ICON_PLAY:    drawIconPlay(iconCenterX, iconCenterY, isSelected); break;
      case ICON_HOUSE:   drawIconHouse(iconCenterX, iconCenterY, isSelected); break;
      case ICON_MEDICIN: drawIconMedicin(iconCenterX, iconCenterY, isSelected); break;
      case ICON_SCOLD:   drawIconScold(iconCenterX, iconCenterY, isSelected); break;
      case ICON_STATS:   drawIconStats(iconCenterX, iconCenterY, isSelected); break;
      case ICON_CONNECT: drawIconConnect(iconCenterX, iconCenterY, isSelected); break;
      default: break;
    }
  }

  // 3. Draw new sliding neon selection frame
  int frameX = (int)currentX - 12;
  int frameY = MENU_BAR_CENTER_Y - 12;
  carrier->display.drawRoundRect(frameX, frameY, 24, 24, 4, color565(0, 230, 255));
  carrier->display.drawRoundRect(frameX - 1, frameY - 1, 26, 26, 5, color565(15, 80, 130));

  // 4. Update Danish label ONLY when the selected icon has actually changed
  if (iconChanged || forceRedrawAll) {
    carrier->display.fillRect(40, 52, 160, 12, bgColor);

    const char* label = MenuLogic::getIconLabelDanish(currentIcon);
    int textWidth = strlen(label) * 6;
    int textStartX = (SCREEN_WIDTH - textWidth) / 2;

    carrier->display.setTextSize(1);
    carrier->display.setTextColor(color565(0, 240, 255), bgColor);
    carrier->display.setCursor(textStartX, 54);
    carrier->display.print(label);
  }

  lastDrawnCursorX = currentX;
  lastDrawnIcon = currentIcon;
}

// Display temporary placeholder screen for other views (Stats, Controller, Action)
void UiMenuRenderer::drawPlaceholderScreen(const char* title, const char* hint) {
  if (carrier == nullptr || title == nullptr || hint == nullptr) return;

  uint16_t boxBg = color565(12, 18, 38);

  // Clear center area of screen (matches the 152x120 viewport area of Lumi)
  carrier->display.fillRect(38, 72, 164, 124, boxBg);
  carrier->display.drawRoundRect(42, 76, 156, 116, 6, color565(40, 80, 150));

  carrier->display.setTextSize(2);
  carrier->display.setTextColor(color565(255, 230, 80), boxBg);
  int titleWidth = strlen(title) * 12;
  carrier->display.setCursor((SCREEN_WIDTH - titleWidth) / 2, 108);
  carrier->display.print(title);

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(color565(150, 190, 240), boxBg);
  int hintWidth = strlen(hint) * 6;
  carrier->display.setCursor((SCREEN_WIDTH - hintWidth) / 2, 142);
  carrier->display.print(hint);
}

// Display games selection menu (Solo Pong vs Labyrinth)
void UiMenuRenderer::drawGamesMenu(uint8_t selectedGame, uint16_t pongHighScore, uint16_t mazeHighScore, bool fullRedraw) {
  if (carrier == nullptr) return;

  uint16_t bgDark    = color565(8, 12, 26);
  uint16_t cardBg    = color565(14, 22, 44);
  uint16_t selCardBg = color565(22, 36, 72);
  uint16_t selBorder = color565(0, 240, 255);
  uint16_t unselBrd  = color565(40, 60, 100);
  uint16_t goldText  = color565(255, 215, 60);
  uint16_t whiteText = 0xFFFF;
  uint16_t hintText  = color565(110, 150, 195);

  if (fullRedraw) {
    carrier->display.fillScreen(bgDark);

    // 1. Header
    carrier->display.setTextSize(2);
    carrier->display.setTextColor(goldText, bgDark);
    const char* title = "MINISPIL";
    int tw = strlen(title) * 12;
    carrier->display.setCursor((SCREEN_WIDTH - tw) / 2, 24);
    carrier->display.print(title);

    // 4. Bottom controls hint
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(hintText, bgDark);
    const char* bottomHint = "T0/T1:Skift | OK | T3:Exit";
    int bw = strlen(bottomHint) * 6;
    carrier->display.setCursor((SCREEN_WIDTH - bw) / 2, 198);
    carrier->display.print(bottomHint);
  }

  // 2. Card 0: SOLO PONG (Y = 56, H = 54, W = 196)
  bool isPong = (selectedGame == 0);
  uint16_t c0Bg = isPong ? selCardBg : cardBg;
  uint16_t c0Br = isPong ? selBorder : unselBrd;
  carrier->display.fillRoundRect(22, 56, 196, 54, 6, c0Bg);
  carrier->display.drawRoundRect(22, 56, 196, 54, 6, c0Br);
  if (isPong) {
    carrier->display.drawRoundRect(21, 55, 198, 56, 7, color565(0, 140, 200));
  }

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(isPong ? color565(0, 240, 255) : whiteText, c0Bg);
  carrier->display.setCursor(34, 66);
  carrier->display.print(isPong ? "> 1. SOLO PONG" : "  1. SOLO PONG");

  carrier->display.setTextColor(hintText, c0Bg);
  carrier->display.setCursor(46, 80);
  carrier->display.print("Mur-Pong bat & bold");

  carrier->display.setTextColor(goldText, c0Bg);
  carrier->display.setCursor(46, 94);
  char buf[32];
  snprintf(buf, sizeof(buf), "Rekord: %u", pongHighScore);
  carrier->display.print(buf);

  // 3. Card 1: LABYRINT (Y = 118, H = 54, W = 196)
  bool isMaze = (selectedGame == 1);
  uint16_t c1Bg = isMaze ? selCardBg : cardBg;
  uint16_t c1Br = isMaze ? selBorder : unselBrd;
  carrier->display.fillRoundRect(22, 118, 196, 54, 6, c1Bg);
  carrier->display.drawRoundRect(22, 118, 196, 54, 6, c1Br);
  if (isMaze) {
    carrier->display.drawRoundRect(21, 117, 198, 56, 7, color565(0, 140, 200));
  }

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(isMaze ? color565(0, 240, 255) : whiteText, c1Bg);
  carrier->display.setCursor(34, 128);
  carrier->display.print(isMaze ? "> 2. LABYRINT" : "  2. LABYRINT");

  carrier->display.setTextColor(hintText, c1Bg);
  carrier->display.setCursor(46, 142);
  carrier->display.print("IMU marmor-kugle");

  carrier->display.setTextColor(goldText, c1Bg);
  carrier->display.setCursor(46, 156);
  if (mazeHighScore > 0) {
    snprintf(buf, sizeof(buf), "Rekord: %u sek", mazeHighScore);
  } else {
    snprintf(buf, sizeof(buf), "Rekord: --");
  }
  carrier->display.print(buf);
}

// Draw cozy House sub-menu (1. Garderobe, 2. Sengetid/Soevn, 3. Mindelund/Grav med MicroSD statistik)
void UiMenuRenderer::drawHouseMenu(uint8_t selectedOption, PetOutfit currentOutfit, bool isSleeping, bool fullRedraw,
                                   uint16_t graveCount, uint16_t generation, uint16_t pongHigh, uint16_t mazeHigh) {
  if (carrier == nullptr) return;

  uint16_t bgDark    = color565(8, 12, 26);
  uint16_t cardBg    = color565(14, 22, 44);
  uint16_t selCardBg = color565(22, 36, 72);
  uint16_t selBorder = color565(0, 240, 255);
  uint16_t unselBrd  = color565(40, 60, 100);
  uint16_t goldText  = color565(255, 215, 60);
  uint16_t whiteText = 0xFFFF;
  uint16_t hintText  = color565(110, 150, 195);
  uint16_t cyanText  = color565(0, 220, 255);
  uint16_t greenText = color565(80, 255, 120);

  if (fullRedraw) {
    carrier->display.fillScreen(bgDark);
    carrier->display.drawCircle(120, 120, 114, color565(20, 42, 78));

    // 1. Header
    carrier->display.setTextSize(2);
    carrier->display.setTextColor(goldText, bgDark);
    const char* title = "LUMIS HUS";
    int tw = strlen(title) * 12;
    carrier->display.setCursor((SCREEN_WIDTH - tw) / 2, 18);
    carrier->display.print(title);

    // Bottom controls hint
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(hintText, bgDark);
    const char* bottomHint = "T0/T1:Skift | OK | T3:Ud";
    int bw = strlen(bottomHint) * 6;
    carrier->display.setCursor((SCREEN_WIDTH - bw) / 2, 196);
    carrier->display.print(bottomHint);
  }

  // Card 0: 1. Garderobe (X=34, Y=40, W=172, H=46)
  bool selGarderobe = (selectedOption == 0);
  uint16_t bgCard0  = selGarderobe ? selCardBg : cardBg;
  uint16_t brdCard0 = selGarderobe ? selBorder : unselBrd;

  carrier->display.fillRoundRect(34, 40, 172, 46, 6, bgCard0);
  carrier->display.drawRoundRect(34, 40, 172, 46, 6, brdCard0);

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(selGarderobe ? color565(0, 240, 255) : whiteText, bgCard0);
  carrier->display.setCursor(42, 47);
  carrier->display.print(selGarderobe ? "> 1. GARDEROBE (SKAB)" : "  1. GARDEROBE (SKAB)");

  carrier->display.setTextColor(currentOutfit == OUTFIT_WINTER ? cyanText : greenText, bgCard0);
  carrier->display.setCursor(52, 60);
  carrier->display.print(currentOutfit == OUTFIT_WINTER ? "Aktiv: Vinterhue PAA" : "Aktiv: Standard toej");

  carrier->display.setTextColor(hintText, bgCard0);
  carrier->display.setCursor(52, 72);
  carrier->display.print("[Tryk OK for at klæde]");

  // Card 1: 2. Sengetid / Soevn (X=34, Y=92, W=172, H=46)
  bool selSleep     = (selectedOption == 1);
  uint16_t bgCard1  = selSleep ? selCardBg : cardBg;
  uint16_t brdCard1 = selSleep ? selBorder : unselBrd;

  carrier->display.fillRoundRect(34, 92, 172, 46, 6, bgCard1);
  carrier->display.drawRoundRect(34, 92, 172, 46, 6, brdCard1);

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(selSleep ? color565(0, 240, 255) : whiteText, bgCard1);
  carrier->display.setCursor(42, 99);
  carrier->display.print(selSleep ? "> 2. SENGETID (SOEVN)" : "  2. SENGETID (SOEVN)");

  carrier->display.setTextColor(isSleeping ? cyanText : greenText, bgCard1);
  carrier->display.setCursor(52, 112);
  carrier->display.print(isSleeping ? "Status: SOVER Zzz" : "Status: VAAGEN");

  carrier->display.setTextColor(hintText, bgCard1);
  carrier->display.setCursor(52, 124);
  carrier->display.print(isSleeping ? "[OK: Vaek Lumi nu]" : "[OK: Sov til 100% ener]");

  // Card 2: 3. Mindelund / Kirkegaard (X=34, Y=144, W=172, H=46)
  bool selGrave     = (selectedOption == 2);
  uint16_t bgCard2  = selGrave ? selCardBg : cardBg;
  uint16_t brdCard2 = selGrave ? selBorder : unselBrd;

  carrier->display.fillRoundRect(34, 144, 172, 46, 6, bgCard2);
  carrier->display.drawRoundRect(34, 144, 172, 46, 6, brdCard2);

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(selGrave ? color565(0, 240, 255) : whiteText, bgCard2);
  carrier->display.setCursor(42, 151);
  carrier->display.print(selGrave ? "> 3. MINDELUND (GRAV)" : "  3. MINDELUND (GRAV)");

  char sdBuf[32];
  snprintf(sdBuf, sizeof(sdBuf), "SD: %u grave | Gen %u", (unsigned int)graveCount, (unsigned int)generation);
  carrier->display.setTextColor(goldText, bgCard2);
  carrier->display.setCursor(52, 164);
  carrier->display.print(sdBuf);

  if (mazeHigh > 0) {
    snprintf(sdBuf, sizeof(sdBuf), "Pong:%u | Lab:%us [OK]", (unsigned int)pongHigh, (unsigned int)mazeHigh);
  } else {
    snprintf(sdBuf, sizeof(sdBuf), "Pong:%u | Lab:--s [OK]", (unsigned int)pongHigh);
  }
  carrier->display.setTextColor(hintText, bgCard2);
  carrier->display.setCursor(52, 176);
  carrier->display.print(sdBuf);
}

// Draw Wardrobe dressing room screen
void UiMenuRenderer::drawWardrobeMenu(uint8_t selectedOption, PetOutfit currentOutfit, bool fullRedraw) {
  if (carrier == nullptr) return;

  uint16_t bgDark    = color565(8, 12, 26);
  uint16_t cardBg    = color565(14, 22, 44);
  uint16_t selCardBg = color565(22, 36, 72);
  uint16_t selBorder = color565(0, 240, 255);
  uint16_t unselBrd  = color565(40, 60, 100);
  uint16_t goldText  = color565(255, 215, 60);
  uint16_t whiteText = 0xFFFF;
  uint16_t hintText  = color565(110, 150, 195);
  uint16_t greenText = color565(80, 255, 120);

  if (fullRedraw) {
    carrier->display.fillScreen(bgDark);

    // 1. Header
    carrier->display.setTextSize(2);
    carrier->display.setTextColor(goldText, bgDark);
    const char* title = "GARDEROBE";
    int tw = strlen(title) * 12;
    carrier->display.setCursor((SCREEN_WIDTH - tw) / 2, 22);
    carrier->display.print(title);

    // Bottom controls hint
    carrier->display.setTextSize(1);
    carrier->display.setTextColor(hintText, bgDark);
    const char* bottomHint = "T0/T1:Skift | OK:Tag paa | T3:Exit";
    int bw = strlen(bottomHint) * 6;
    carrier->display.setCursor((SCREEN_WIDTH - bw) / 2, 198);
    carrier->display.print(bottomHint);
  }

  // Card 0: Standard toej (Y = 56, H = 54, W = 196)
  bool selStd   = (selectedOption == 0);
  uint16_t bg0  = selStd ? selCardBg : cardBg;
  uint16_t brd0 = selStd ? selBorder : unselBrd;

  carrier->display.fillRoundRect(22, 56, 196, 54, 6, bg0);
  carrier->display.drawRoundRect(22, 56, 196, 54, 6, brd0);
  if (selStd) {
    carrier->display.drawRoundRect(21, 55, 198, 56, 7, color565(0, 140, 200));
  }

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(selStd ? color565(0, 240, 255) : whiteText, bg0);
  carrier->display.setCursor(34, 66);
  carrier->display.print(selStd ? "> 1. STANDARDTOEJ" : "  1. STANDARDTOEJ");

  carrier->display.setTextColor(hintText, bg0);
  carrier->display.setCursor(46, 80);
  carrier->display.print("Hue & toerklaede AF");

  if (currentOutfit == OUTFIT_DEFAULT) {
    carrier->display.setTextColor(greenText, bg0);
    carrier->display.setCursor(46, 94);
    carrier->display.print("[AKTIV PAA LUMI]");
  } else {
    carrier->display.setTextColor(hintText, bg0);
    carrier->display.setCursor(46, 94);
    carrier->display.print("[Tryk OK for at vaelge]");
  }

  // Card 1: Vintertoej (Y = 118, H = 54, W = 196)
  bool selWin   = (selectedOption == 1);
  uint16_t bg1  = selWin ? selCardBg : cardBg;
  uint16_t brd1 = selWin ? selBorder : unselBrd;

  carrier->display.fillRoundRect(22, 118, 196, 54, 6, bg1);
  carrier->display.drawRoundRect(22, 118, 196, 54, 6, brd1);
  if (selWin) {
    carrier->display.drawRoundRect(21, 117, 198, 56, 7, color565(0, 140, 200));
  }

  carrier->display.setTextSize(1);
  carrier->display.setTextColor(selWin ? color565(0, 240, 255) : whiteText, bg1);
  carrier->display.setCursor(34, 128);
  carrier->display.print(selWin ? "> 2. VINTERTOEJ" : "  2. VINTERTOEJ");

  carrier->display.setTextColor(hintText, bg1);
  carrier->display.setCursor(46, 142);
  carrier->display.print("Strikket hue & toerklaede");

  if (currentOutfit == OUTFIT_WINTER) {
    carrier->display.setTextColor(greenText, bg1);
    carrier->display.setCursor(46, 156);
    carrier->display.print("[AKTIV PAA LUMI]");
  } else {
    carrier->display.setTextColor(hintText, bg1);
    carrier->display.setCursor(46, 156);
    carrier->display.print("[Tryk OK for at vaelge]");
  }
}


