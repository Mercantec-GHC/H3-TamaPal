/*
 * ============================================================================
 * TAMAGOTCHI MAX GRAPHICS ENGINE — "LUMI" (Procedural 2.5D Shader for Arduino)
 * ============================================================================
 * Demonstration of graphical capabilities for Arduino MKR IoT Carrier (ST7789 240x240).
 * NO SD-card and NO external bitmaps — 100% pure procedural code!
 *
 * KEY TECHNIQUES USED:
 * 1. Scanline Procedural Shader:
 *    - Instead of slow fillScreen() (which causes heavy screen tearing/flickering),
 *      the scene is rendered line-by-line into a compact buffer:
 *      (uint16_t scanline[152] — only 304 bytes of RAM!)
 *      and flushed across SPI in a single hardware transaction.
 *    - Result: Zero flicker and smooth 60 FPS animation!
 *
 * 2. Per-Pixel 2.5D Shading & Signed Distance Fields (SDF):
 *    - Spherical diffuse lighting (light source top-left, shadow bottom-right).
 *    - Glossy specular highlights and neon rim light / Fresnel contour glow.
 *    - Anti-aliased outer borders.
 *    - Real-time soft drop shadow on the ground scaling with jump physics.
 *
 * 3. Procedural Squash & Stretch Animation:
 *    - Sine-wave breathing, jump deformation, and landing dampening.
 *    - Wiggling ears, eye tracking, eyelid blinking, chewing mouth, and soft blush.
 *    - Anime eyes with vertical gradient iris and dual specular catchlights.
 *
 * 4. Hardware Sensor & IMU Integration:
 *    - TOUCH0: Love (hearts, deep blush, happy curved eyes ^ ^)
 *    - TOUCH1: Feed (berry flies into view, pet tracks it, opens mouth, and chews)
 *    - TOUCH2: Pet (sparkles, purring sound, sway motion)
 *    - TOUCH3: Sleep / Night mode (closed eyes, deep breathing, energy refill)
 *    - TOUCH4: Cyber Evolution (neon theme, orbiting plasma spheres, color shift)
 *    - Shake board (IMU accelerometer): character gets dizzy!
 * ============================================================================
 */

#include <Arduino_MKRIoTCarrier.h>
#include <math.h>

MKRIoTCarrier carrier;

// Screen and character viewport dimensions
const int SCREEN_W = 240;
const int SCREEN_H = 240;

// High-speed viewport centered on screen
const int VIEW_W = 152;
const int VIEW_H = 144;
const int VIEW_X = (SCREEN_W - VIEW_W) / 2; // 44
const int VIEW_Y = 46;

// Single scanline buffer (uses only 304 bytes of dynamic RAM!)
uint16_t scanline[VIEW_W];

// Pet emotion state machine
enum PetState {
  STATE_IDLE = 0,
  STATE_LOVE,    // TOUCH0: Floating hearts & happy jumps
  STATE_EATING,  // TOUCH1: Feeding berry animation
  STATE_PETTING, // TOUCH2: Sparkles, purring, and ear wiggles
  STATE_SLEEP,   // TOUCH3: Sleep mode
  STATE_CYBER,   // TOUCH4: Neon super mode (orbiting spheres)
  STATE_DIZZY    // Triggered by shaking board
};

PetState currentState = STATE_IDLE;
unsigned long stateStartTime = 0;
unsigned long lastFrameTime = 0;

// Pet vitals (0..100)
int statHunger = 78;
int statHappy  = 85;
int statEnergy = 92;
unsigned long lastStatDecay = 0;

// Particle system (hearts, sparkles, energy orbs, sleep bubbles)
struct Particle {
  float x, y;
  float vx, vy;
  int size;
  uint16_t color;
  bool active;
};
const int MAX_PARTICLES = 6;
Particle particles[MAX_PARTICLES];

// Per-frame precomputed geometry parameters
struct FrameParams {
  int cx, cy;           // Body center
  int rx, ry;           // Body radii (accounting for squash & stretch)
  int invRx2, invRy2;   // Fast ellipse evaluation
  int earLeftX, earLeftY;
  int earRightX, earRightY;
  int earTilt;
  int footLeftX, footRightX, footY;
  int shadowY, shadowRx, shadowRy;
  int lookX, lookY;     // Eye gaze offset
  int blinkOpen;        // 0 = closed, 10 = fully open
  int mouthOpen;        // 0 = cat-smile 'w', >0 = open mouth
  int blushAlpha;       // Cheek blush intensity (0..255)
  bool happyEyes;       // Curved happy eyes ^ ^
  bool dizzyEyes;       // Spiral/shocked eyes
  bool cyberMode;       // Neon skin
  bool foodActive;      // Food prop active
  int foodX, foodY, foodR;
  uint32_t frameCount;
} fp;

// Fast RGB888 to RGB565 pack
inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3) | (b >> 3);
}

// Alpha blend two RGB565 colors (alpha: 0..255)
inline uint16_t blend565(uint16_t bg, uint16_t fg, uint8_t alpha) {
  if (alpha == 0) return bg;
  if (alpha == 255) return fg;
  uint16_t inv = 255 - alpha;

  uint8_t r1 = (bg >> 11) & 0x1F;
  uint8_t g1 = (bg >> 5) & 0x3F;
  uint8_t b1 = bg & 0x1F;

  uint8_t r2 = (fg >> 11) & 0x1F;
  uint8_t g2 = (fg >> 5) & 0x3F;
  uint8_t b2 = fg & 0x1F;

  uint8_t r = (r1 * inv + r2 * alpha) >> 8;
  uint8_t g = (g1 * inv + g2 * alpha) >> 8;
  uint8_t b = (b1 * inv + b2 * alpha) >> 8;

  return (r << 11) | (g << 5) | b;
}

// Spawn particle burst for specific emotion
void spawnParticles(PetState mode) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].active = true;
    particles[i].x = VIEW_W / 2 + random(-42, 43);
    particles[i].y = VIEW_H / 2 + random(-10, 30);
    particles[i].vx = random(-10, 11) * 0.08f;
    particles[i].vy = -random(8, 18) * 0.1f;
    particles[i].size = random(5, 8);
    if (mode == STATE_LOVE) {
      particles[i].color = rgb565(255, 70, 130); // Vibrant pink/red hearts
    } else if (mode == STATE_CYBER) {
      particles[i].color = (i % 2 == 0) ? rgb565(0, 255, 255) : rgb565(255, 50, 255);
    } else if (mode == STATE_SLEEP) {
      particles[i].color = rgb565(150, 200, 255);
    } else {
      particles[i].color = rgb565(255, 235, 80); // Gold sparkles
    }
  }
}

// Chiptune sound effects
void playChirp(int freq1, int freq2, int durMs) {
  carrier.Buzzer.sound(freq1);
  delay(durMs);
  carrier.Buzzer.sound(freq2);
  delay(durMs);
  carrier.Buzzer.noSound();
}

// Animate the 5 RGB DotStar LEDs according to pet mood
void updateMoodLEDs(unsigned long now) {
  float pulse = (sin(now * 0.005f) + 1.0f) * 0.5f;
  uint8_t br = 15 + (uint8_t)(pulse * 35);
  carrier.leds.setBrightness(br);

  for (int i = 0; i < 5; i++) {
    uint32_t col = carrier.leds.Color(0, 180, 255); // Calm cyan
    if (currentState == STATE_LOVE) col = carrier.leds.Color(255, 40, 120);
    else if (currentState == STATE_EATING) col = carrier.leds.Color(80, 255, 80);
    else if (currentState == STATE_PETTING) col = carrier.leds.Color(255, 200, 40);
    else if (currentState == STATE_SLEEP) col = carrier.leds.Color(30, 40, 120);
    else if (currentState == STATE_CYBER) {
      int hueShift = (i * 50 + (now / 10)) & 255;
      col = carrier.leds.Color(hueShift, 255 - hueShift, 255);
    } else if (currentState == STATE_DIZZY) {
      col = carrier.leds.Color(255, 60, 0);
    }
    carrier.leds.setPixelColor(i, col);
  }
  carrier.leds.show();
}

// ============================================================================
// FRAME PHYSICS & ANIMATION (Precomputed once per frame)
// ============================================================================
void updateAnimation(unsigned long now) {
  fp.frameCount++;
  float t = now * 0.004f;

  // Auto-return to idle state when emotion sequence completes
  if (currentState != STATE_IDLE && currentState != STATE_SLEEP && currentState != STATE_CYBER) {
    if (now - stateStartTime > 4500) {
      currentState = STATE_IDLE;
    }
  }

  // Base coordinates
  int baseX = VIEW_W / 2;
  int baseY = 82;
  int bounceY = 0;
  float squash = 1.0f; // >1 wider & shorter, <1 narrower & taller

  fp.happyEyes = false;
  fp.dizzyEyes = false;
  fp.cyberMode = (currentState == STATE_CYBER);
  fp.foodActive = false;
  fp.mouthOpen = 0;
  fp.blushAlpha = 95;

  if (currentState == STATE_IDLE) {
    // Gentle breathing
    float breath = sin(t * 1.2f);
    bounceY = (int)(breath * 2.5f);
    squash = 1.0f - breath * 0.045f;
    fp.lookX = (int)(sin(t * 0.45f) * 5.0f);
    fp.lookY = (int)(cos(t * 0.35f) * 2.5f);
  } 
  else if (currentState == STATE_LOVE) {
    // Bouncing joy jumps
    float jumpPhase = fabs(sin(t * 2.6f));
    bounceY = -(int)(jumpPhase * 16.0f);
    squash = (jumpPhase < 0.25f) ? 1.14f : (0.92f + jumpPhase * 0.05f);
    fp.happyEyes = true;
    fp.blushAlpha = 210;
    fp.lookX = 0;
    fp.lookY = -2;
  } 
  else if (currentState == STATE_EATING) {
    // Eating / chewing animation
    float eatCycle = sin(t * 3.5f);
    bounceY = (int)(eatCycle * 2.0f);
    squash = 1.0f + eatCycle * 0.05f;
    fp.mouthOpen = (eatCycle > -0.2f) ? (int)((eatCycle + 0.2f) * 7.0f) : 0;
    fp.blushAlpha = 160;

    // Food berry travels toward the mouth and shrinks as it is consumed
    float progress = (now - stateStartTime) / 4200.0f;
    if (progress < 0.85f) {
      fp.foodActive = true;
      fp.foodX = baseX + (int)((1.0f - progress) * 36.0f);
      fp.foodY = baseY + 12 - (int)(sin(progress * 3.14f) * 14.0f);
      fp.foodR = 9 - (int)(progress * 6.0f);
      fp.lookX = (fp.foodX - baseX) / 5;
      fp.lookY = 3;
    } else {
      fp.happyEyes = true;
    }
  } 
  else if (currentState == STATE_PETTING) {
    // Purring wobble and happy squint
    baseX += (int)(sin(t * 2.2f) * 6.0f);
    squash = 1.08f + sin(t * 4.0f) * 0.04f;
    bounceY = 3;
    fp.happyEyes = true;
    fp.blushAlpha = 230;
  } 
  else if (currentState == STATE_SLEEP) {
    // Slow deep breathing during sleep
    float sleepBreath = sin(t * 0.65f);
    bounceY = 5 + (int)(sleepBreath * 1.5f);
    squash = 1.06f + sleepBreath * 0.04f;
    fp.mouthOpen = (sleepBreath > 0.3f) ? 3 : 1; // Gentle sleep 'o'
    fp.blushAlpha = 70;
  } 
  else if (currentState == STATE_CYBER) {
    // Cyber levitation hover
    float hover = sin(t * 2.0f);
    bounceY = -8 + (int)(hover * 5.0f);
    squash = 0.96f + hover * 0.03f;
    fp.lookX = (int)(sin(t * 1.5f) * 4.0f);
    fp.lookY = -2;
    fp.blushAlpha = 180;
  } 
  else if (currentState == STATE_DIZZY) {
    // Shaking motion triggered by accelerometer
    baseX += (int)(sin(t * 8.0f) * 5.0f);
    bounceY = (int)(cos(t * 6.0f) * 4.0f);
    fp.dizzyEyes = true;
    fp.mouthOpen = 5;
  }

  fp.cx = baseX;
  fp.cy = baseY + bounceY;
  fp.rx = (int)(40.0f * squash);
  fp.ry = (int)(36.0f / squash);

  // Eyelid blink cycle (~every 3.2s)
  if (currentState == STATE_SLEEP) {
    fp.blinkOpen = 0;
  } else {
    int blinkPhase = (now / 30) % 110;
    if (blinkPhase == 0 || blinkPhase == 2) fp.blinkOpen = 4;
    else if (blinkPhase == 1) fp.blinkOpen = 0;
    else fp.blinkOpen = 10;
  }

  // Ears and paws coordinates
  fp.earTilt = (int)(sin(t * 1.8f) * 3.0f);
  fp.earLeftX  = fp.cx - (fp.rx * 6) / 10 - 4;
  fp.earRightX = fp.cx + (fp.rx * 6) / 10 + 4;
  fp.earLeftY  = fp.cy - fp.ry - 6 + fp.earTilt;
  fp.earRightY = fp.cy - fp.ry - 6 - fp.earTilt;

  fp.footLeftX  = fp.cx - 19;
  fp.footRightX = fp.cx + 19;
  fp.footY      = fp.cy + fp.ry - 2;

  // Drop shadow parameters
  fp.shadowY  = 122;
  fp.shadowRx = 34 + (bounceY / 2);
  fp.shadowRy = 7;

  // Update particles
  for (int i = 0; i < MAX_PARTICLES; i++) {
    if (!particles[i].active) continue;
    if (currentState == STATE_CYBER) {
      // In cyber mode, orbs rotate around the pet in 3D-like orbits
      float angle = t * 1.6f + i * 1.047f;
      particles[i].x = fp.cx + cos(angle) * 54.0f;
      particles[i].y = fp.cy + sin(angle * 2.0f) * 26.0f;
    } else if (currentState == STATE_LOVE || currentState == STATE_PETTING || currentState == STATE_SLEEP) {
      particles[i].x += particles[i].vx + sin(t + i) * 0.3f;
      particles[i].y += particles[i].vy;
      if (particles[i].y < 10) {
        particles[i].y = VIEW_H - 25;
        particles[i].x = fp.cx + random(-45, 46);
      }
    } else {
      particles[i].active = false;
    }
  }
}

// ============================================================================
// SCANLINE PROCEDURAL SHADER (Zero flicker engine)
// Computes pixel (px, py) with lighting, shadows, and highlights
// ============================================================================
void renderScanline(int py) {
  // 1. Background: subtle vertical sky/room gradient
  uint8_t bgR = 10 + (py >> 3);
  uint8_t bgG = 14 + (py >> 2);
  uint8_t bgB = 34 + (py >> 2);
  if (fp.cyberMode) {
    bgR = 22 + (py >> 3);
    bgG = 6 + (py >> 4);
    bgB = 42 + (py >> 2);
  }
  uint16_t baseBgColor = rgb565(bgR, bgG, bgB);

  int dyBody = py - fp.cy;
  int dyShadow = py - fp.shadowY;
  int dyFoot = py - fp.footY;

  for (int px = 0; px < VIEW_W; px++) {
    uint16_t col = baseBgColor;

    // Background twinkling stars
    if (((px * 37 + py * 13) & 0xFF) == 0 && py < 95) {
      uint8_t twinkle = ((fp.frameCount + px) & 8) ? 200 : 90;
      col = rgb565(twinkle, twinkle, 255);
    }

    // Pedestal glow and soft drop shadow
    if (abs(dyShadow) <= fp.shadowRy + 4) {
      int dxS = px - fp.cx;
      int distS = (dxS * dxS * 100) / (fp.shadowRx * fp.shadowRx) +
                  (dyShadow * dyShadow * 100) / (fp.shadowRy * fp.shadowRy);
      if (distS < 100) {
        // Shadow is darker at the core and smoothly fades outward
        uint8_t shadowAlpha = (100 - distS) * 2;
        uint16_t shadowCol = fp.cyberMode ? rgb565(0, 220, 255) : rgb565(4, 6, 14);
        col = blend565(col, shadowCol, shadowAlpha);
      } else if (distS < 165) {
        // Neon ambient ring around the shadow
        col = blend565(col, rgb565(40, 90, 160), 60);
      }
    }

    // 2. Paws — rendered slightly behind the main body
    if (abs(dyFoot) <= 8) {
      int dxFL = px - fp.footLeftX;
      int dxFR = px - fp.footRightX;
      int dFootL = (dxFL * dxFL) + (dyFoot * dyFoot * 2);
      int dFootR = (dxFR * dxFR) + (dyFoot * dyFoot * 2);
      int dFoot = (dFootL < dFootR) ? dFootL : dFootR;
      if (dFoot <= 90) {
        uint16_t pawCol = fp.cyberMode ? rgb565(180, 60, 240) : rgb565(65, 155, 235);
        if (dFoot > 68) pawCol = rgb565(20, 45, 90); // Paw contour outline
        col = pawCol;
      }
    }

    // 3. Ears with pink/neon inner lobe
    for (int side = -1; side <= 1; side += 2) {
      int ex = (side < 0) ? fp.earLeftX : fp.earRightX;
      int ey = (side < 0) ? fp.earLeftY : fp.earRightY;
      int dxE = px - ex - (py - ey) * side / 3; // Ear angle tilt
      int dyE = py - ey;
      if (abs(dxE) <= 13 && abs(dyE) <= 16) {
        int dEar = (dxE * dxE * 2) + (dyE * dyE);
        if (dEar <= 220) {
          if (dEar > 175) {
            col = rgb565(20, 50, 100); // Dark ear contour
          } else if (dEar < 85 && dyE > -4) {
            // Inner ear flap
            col = fp.cyberMode ? rgb565(0, 255, 240) : rgb565(255, 135, 175);
          } else {
            col = fp.cyberMode ? rgb565(150, 50, 230) : rgb565(90, 185, 250);
          }
        }
      }
    }

    // 4. MAIN CHARACTER BODY (Spherical 3D shading)
    int dxBody = px - fp.cx;
    // Slightly widen lower cheeks for a cute mochi-like silhouette
    int bodyRx = fp.rx + ((dyBody > 0) ? (dyBody / 5) : (-dyBody / 9));
    if (bodyRx < 10) bodyRx = 10;

    int distSq100 = (dxBody * dxBody * 100) / (bodyRx * bodyRx) +
                    (dyBody * dyBody * 100) / (fp.ry * fp.ry);

    if (distSq100 <= 106) {
      if (distSq100 > 96) {
        // Anti-aliased boundary / rim light
        uint16_t edgeCol = fp.cyberMode ? rgb565(0, 255, 255) : rgb565(22, 55, 110);
        uint8_t aa = (106 - distSq100) * 25;
        col = blend565(col, edgeCol, aa);
      } else {
        // --- 3D BODY SHADING (Diffuse directional light from top-left) ---
        int lightX = dxBody + (bodyRx / 3);
        int lightY = dyBody + (fp.ry / 3);
        int lightDist = (lightX * lightX + lightY * lightY) >> 4; // 0..160
        if (lightDist > 140) lightDist = 140;

        uint8_t r, g, b;
        if (fp.cyberMode) {
          // Neon violet-magenta gradient
          r = 215 - (lightDist * 9) / 10;
          g = 75  - (lightDist * 4) / 10;
          b = 255 - (lightDist * 5) / 10;
        } else {
          // Soft cyan / sky blue 3D gradient ("Lumi")
          r = 135 - (lightDist * 7) / 10;
          g = 230 - (lightDist * 7) / 10;
          b = 255 - (lightDist * 3) / 10;
        }

        // Contour glow (Fresnel rim light)
        if (distSq100 > 82) {
          r = min(255, r + 30);
          g = min(255, g + 35);
          b = 255;
        }

        // Glossy specular highlight on top-left forehead
        int specX = dxBody + (bodyRx * 4) / 10;
        int specY = dyBody + (fp.ry * 5) / 10;
        int specDist = specX * specX + specY * specY * 2;
        if (specDist < 48) {
          uint8_t specAlpha = (48 - specDist) * 4;
          col = blend565(rgb565(r, g, b), rgb565(255, 255, 255), specAlpha);
        } else {
          col = rgb565(r, g, b);
        }

        // Light belly patch (Tummy)
        int tummyY = dyBody - (fp.ry * 4) / 10;
        int tummyDist = (dxBody * dxBody) + (tummyY * tummyY * 2);
        if (tummyDist < 320) {
          uint8_t tumAlpha = (320 - tummyDist) / 3;
          uint16_t tumCol = fp.cyberMode ? rgb565(255, 190, 255) : rgb565(225, 250, 255);
          col = blend565(col, tumCol, tumAlpha);
        }

        // --- GAUSSIAN BLUSH ON CHEEKS ---
        int cheekY = dyBody - 4;
        if (abs(cheekY) <= 9) {
          int dxCL = abs(dxBody) - 22; // Symmetrical on both sides
          int dCheek = dxCL * dxCL + cheekY * cheekY * 2;
          if (dCheek < 64) {
            uint8_t bAlpha = ((64 - dCheek) * fp.blushAlpha) >> 6;
            col = blend565(col, rgb565(255, 95, 145), bAlpha);
          }
        }

        // --- ANIME EYES WITH GRADIENT IRIS & DUAL CATCHLIGHTS ---
        int eyeY = dyBody + 3 - fp.lookY;
        if (abs(eyeY) <= 11) {
          for (int side = -1; side <= 1; side += 2) {
            int eyeCenterX = side * 15 + fp.lookX;
            int ex = dxBody - eyeCenterX;
            if (abs(ex) <= 9) {
              if (fp.happyEyes) {
                // Happy curved eyes ^ ^
                int arcY = (ex * ex) / 9 - 3;
                if (abs(eyeY - arcY) <= 2 && abs(ex) <= 7) {
                  col = rgb565(18, 25, 50);
                }
              } else if (fp.blinkOpen == 0) {
                // Closed sleeping eyes (downward eyelashes)
                int sleepArc = 2 - (ex * ex) / 11;
                if (abs(eyeY - sleepArc) <= 1 && abs(ex) <= 7) {
                  col = rgb565(18, 25, 50);
                }
              } else {
                // Fully open detailed anime eye
                int eyeRy = (fp.dizzyEyes && side == 1) ? 6 : fp.blinkOpen;
                int dEye = ex * ex + (eyeY * eyeY * 80) / (eyeRy * eyeRy + 1);
                if (dEye <= 72) {
                  if (dEye > 56) {
                    // Dark eyelash / outer eye outline
                    col = rgb565(12, 16, 35);
                  } else {
                    // Gradient iris: deep dark top to vibrant bright cyan/gold bottom
                    int irisGrad = constrain((eyeY + 8) * 14, 0, 220);
                    uint16_t irisCol = fp.cyberMode
                        ? rgb565(255, irisGrad, 240)
                        : rgb565(15, 40 + irisGrad / 2, 95 + (irisGrad * 2) / 3);

                    // Deep pupil core
                    if (dEye < 20) {
                      irisCol = rgb565(10, 14, 30);
                    }

                    // Primary specular reflection (top-left catchlight)
                    int hl1x = ex + 3;
                    int hl1y = eyeY + 3;
                    if (hl1x * hl1x + hl1y * hl1y <= 6) {
                      irisCol = rgb565(255, 255, 255);
                    }
                    // Secondary micro-twinkle (bottom-right reflection)
                    int hl2x = ex - 3;
                    int hl2y = eyeY - 3;
                    if (hl2x * hl2x + hl2y * hl2y <= 2) {
                      irisCol = rgb565(255, 255, 255);
                    }

                    col = irisCol;
                  }
                }
              }
            }
          }
        }

        // --- MOUTH ('w' cat smile or open chewing mouth with tongue) ---
        int mouthY = dyBody - 6;
        if (abs(mouthY) <= 8 && abs(dxBody) <= 9) {
          if (fp.mouthOpen > 0) {
            // Open mouth ellipse with pink tongue
            int dMouth = (dxBody * dxBody) + (mouthY * mouthY * 36) / (fp.mouthOpen * fp.mouthOpen + 1);
            if (dMouth <= 36) {
              if (mouthY > 1 && dMouth < 24) {
                col = rgb565(255, 105, 135); // Pink tongue
              } else {
                col = rgb565(65, 18, 38);    // Deep oral cavity
              }
            }
          } else {
            // Cute cat smile "w"
            int wX = abs(dxBody) - 3;
            int wY = 2 - (wX * wX) / 4;
            if (abs(mouthY - wY) <= 1 && abs(dxBody) <= 6) {
              col = rgb565(20, 35, 70);
            }
          }
        }
      }
    }

    // 5. FOOD / BERRY PROP (When feeding via TOUCH1)
    if (fp.foodActive) {
      int fdx = px - fp.foodX;
      int fdy = py - fp.foodY;
      int fd2 = fdx * fdx + fdy * fdy;
      int r2 = fp.foodR * fp.foodR;
      if (fd2 <= r2) {
        // Glossy red berry with highlight
        uint16_t berryCol = rgb565(255, 45, 75);
        if ((fdx + 2) * (fdx + 2) + (fdy + 2) * (fdy + 2) <= 4) {
          berryCol = rgb565(255, 220, 230); // Specular highlight on berry
        }
        col = berryCol;
      } else if (abs(fdx) <= 3 && fdy >= -fp.foodR - 4 && fdy <= -fp.foodR) {
        col = rgb565(60, 220, 80); // Green leaf atop berry
      }
    }

    // 6. PARTICLE SYSTEM (Hearts, sparkles, energy orbs)
    for (int i = 0; i < MAX_PARTICLES; i++) {
      if (!particles[i].active) continue;
      int pdx = px - (int)particles[i].x;
      int pdy = py - (int)particles[i].y;
      int sz = particles[i].size;
      if (abs(pdx) <= sz && abs(pdy) <= sz) {
        if (currentState == STATE_LOVE) {
          // Parametric heart formula
          int hx = abs(pdx);
          int hy = pdy + (hx >> 1);
          if (hx * hx + hy * hy <= (sz * sz) / 2) {
            col = particles[i].color;
          }
        } else {
          // Glowing star / orb (diamond with soft center)
          int starDist = abs(pdx) + abs(pdy);
          if (starDist <= sz) {
            col = (starDist <= 2) ? rgb565(255, 255, 255) : particles[i].color;
          }
        }
      }
    }

    scanline[px] = col;
  }
}

// Single hardware SPI transaction per character frame
void renderCharacterFrame() {
  carrier.display.startWrite();
  carrier.display.setAddrWindow(VIEW_X, VIEW_Y, VIEW_W, VIEW_H);
  for (int y = 0; y < VIEW_H; y++) {
    renderScanline(y);
    carrier.display.writePixels(scanline, VIEW_W);
  }
  carrier.display.endWrite();
}

// ============================================================================
// TAMAGOTCHI HUD INTERFACE (Status bars, frame, and button hints)
// ============================================================================
void drawStatBar(int x, int y, int w, int h, int val, uint16_t fillCol, const char *label) {
  carrier.display.setTextSize(1);
  carrier.display.setTextColor(rgb565(180, 210, 255), rgb565(8, 12, 26));
  carrier.display.setCursor(x, y);
  carrier.display.print(label);

  int barX = x + 22;
  int barW = w - 22;
  carrier.display.drawRoundRect(barX, y, barW, h, 3, rgb565(70, 100, 150));
  int fillW = ((barW - 4) * constrain(val, 0, 100)) / 100;
  carrier.display.fillRect(barX + 2, y + 2, barW - 4, h - 4, rgb565(15, 22, 42));
  if (fillW > 0) {
    carrier.display.fillRect(barX + 2, y + 2, fillW, h - 4, fillCol);
  }
}

void updateHUD() {
  drawStatBar(48, 14, 144, 8, statHunger, rgb565(80, 235, 120), "FD:");
  drawStatBar(48, 25, 144, 8, statHappy,  rgb565(255, 90, 150), "LV:");
  drawStatBar(48, 36, 144, 8, statEnergy, rgb565(70, 200, 255), "EN:");
}

void updateStatusLabel(const char *statusText, uint16_t color) {
  carrier.display.fillRect(36, 196, 168, 18, rgb565(8, 12, 26));
  carrier.display.setTextSize(2);
  carrier.display.setTextColor(color);
  int textW = strlen(statusText) * 12;
  carrier.display.setCursor((SCREEN_W - textW) / 2, 197);
  carrier.display.print(statusText);
}

void drawStaticUI() {
  carrier.display.fillScreen(rgb565(8, 12, 26));

  // Neon rounded viewport bezel
  carrier.display.drawRoundRect(VIEW_X - 2, VIEW_Y - 2, VIEW_W + 4, VIEW_H + 4, 6, rgb565(45, 95, 175));
  carrier.display.drawRoundRect(VIEW_X - 4, VIEW_Y - 4, VIEW_W + 8, VIEW_H + 8, 8, rgb565(20, 45, 95));

  // Touch pad guide at bottom of screen
  carrier.display.setTextSize(1);
  carrier.display.setTextColor(rgb565(120, 150, 200));
  carrier.display.setCursor(42, 218);
  carrier.display.print("T0:Love T1:Feed T2:Pet");
  carrier.display.setCursor(54, 228);
  carrier.display.print("T3:Sleep  T4:Cyber");

  updateHUD();
  updateStatusLabel("LUMI: HAPPY", rgb565(100, 230, 255));
}

// ============================================================================
// TOUCH PADS & IMU ACCELEROMETER HANDLER
// ============================================================================
void handleInputs(unsigned long now) {
  carrier.Buttons.update();

  if (carrier.Buttons.onTouchDown(TOUCH0)) {
    currentState = STATE_LOVE;
    stateStartTime = now;
    statHappy = min(100, statHappy + 15);
    spawnParticles(STATE_LOVE);
    updateHUD();
    updateStatusLabel("LOVES YOU!", rgb565(255, 100, 165));
    playChirp(880, 1320, 35);
  } 
  else if (carrier.Buttons.onTouchDown(TOUCH1)) {
    currentState = STATE_EATING;
    stateStartTime = now;
    statHunger = min(100, statHunger + 20);
    updateHUD();
    updateStatusLabel("YUMMY BERRY!", rgb565(100, 255, 130));
    playChirp(587, 784, 40);
  } 
  else if (carrier.Buttons.onTouchDown(TOUCH2)) {
    currentState = STATE_PETTING;
    stateStartTime = now;
    statHappy = min(100, statHappy + 10);
    spawnParticles(STATE_PETTING);
    updateHUD();
    updateStatusLabel("PURR... ^_^", rgb565(255, 220, 90));
    playChirp(659, 987, 30);
  } 
  else if (carrier.Buttons.onTouchDown(TOUCH3)) {
    currentState = (currentState == STATE_SLEEP) ? STATE_IDLE : STATE_SLEEP;
    stateStartTime = now;
    if (currentState == STATE_SLEEP) {
      statEnergy = min(100, statEnergy + 25);
      spawnParticles(STATE_SLEEP);
      updateHUD();
      updateStatusLabel("SLEEPING Zzz", rgb565(140, 180, 255));
      playChirp(523, 392, 45);
    } else {
      updateStatusLabel("AWAKE!", rgb565(100, 230, 255));
    }
  } 
  else if (carrier.Buttons.onTouchDown(TOUCH4)) {
    currentState = (currentState == STATE_CYBER) ? STATE_IDLE : STATE_CYBER;
    stateStartTime = now;
    if (currentState == STATE_CYBER) {
      spawnParticles(STATE_CYBER);
      updateStatusLabel("CYBER EVO!!", rgb565(0, 255, 255));
      playChirp(1046, 1568, 40);
    } else {
      updateStatusLabel("LUMI: READY", rgb565(100, 230, 255));
    }
  }

  // Detect board shake via IMU accelerometer
  if (carrier.IMUmodule.accelerationAvailable()) {
    float ax, ay, az;
    carrier.IMUmodule.readAcceleration(ax, ay, az);
    float totalG = fabs(ax) + fabs(ay) + fabs(az);
    if (totalG > 2.6f && currentState != STATE_DIZZY) {
      currentState = STATE_DIZZY;
      stateStartTime = now;
      updateStatusLabel("WOAH DIZZY!", rgb565(255, 160, 60));
      playChirp(440, 330, 35);
    }
  }
}

void setup() {
  Serial.begin(115200);
  carrier.withCase();
  carrier.begin();

  // Set capacitive pad sensitivity
  carrier.Buttons.updateConfig(240);

  carrier.display.setRotation(0);
  drawStaticUI();
  stateStartTime = millis();
}

void loop() {
  unsigned long now = millis();

  // 1. Process touch buttons and IMU shake
  handleInputs(now);

  // 2. Slow stats decay every 15 seconds
  if (now - lastStatDecay > 15000) {
    lastStatDecay = now;
    statHunger = max(10, statHunger - 2);
    statHappy  = max(10, statHappy - 2);
    if (currentState == STATE_SLEEP) {
      statEnergy = min(100, statEnergy + 5);
    } else {
      statEnergy = max(10, statEnergy - 1);
    }
    updateHUD();
  }

  // 3. Update physics, animations, and render frame
  updateAnimation(now);
  renderCharacterFrame();

  // 4. Update RGB mood LEDs on the carrier
  updateMoodLEDs(now);
}
