/* 
 *  Multicyclone - a DIY Arduino Multiplayer Cyclone game 
 *  by F. Overkamp 2020
 *  
 *  Basic game implementation based on https://www.instructables.com/id/Cyclone-LED-Arcade-Game/
 *  
 *  Also potential todo's:
 *  Cleaner printing with LCD and buffer strings https://www.baldengineer.com/arduino-lcd-display-tips.html
 *  
 *  Improvements: 
 *  - Player timeout when missing
 *  - Toon ronde teller en/of snelheid
 *  - 5 rondes niet geraakt = einde spel
 *  - Possible extension idea to change directions as per https://www.youtube.com/watch?v=LWt2IgzJpRQ
 *  - Plek van de stip randomisen
 *  - Aan uit fysiek?
 *  - Locatie randomisation na iedere win wenselijk of niet?
 *  
 */
#include "MultiCyclone.h"
#include "buzzertones.h"
#include <FastLED.h>
#include <LiquidCrystal.h>

/* Define connection pins */
#define PIXEL_PIN       2
#define BUTTON_RED      3
#define BUTTON_GREEN    4
#define BUTTON_BLUE     5
#define BUTTON_YELLOW   6
#define LCD_CONTRAST    9
#define LCD_BACKLIGHT   10
#define BUZZER          12
#define LCD_D4          A0
#define LCD_D5          A1
#define LCD_D6          A2
#define LCD_D7          A3
#define LCD_EN          A4
#define LCD_RS          A5


/* Serial debug (comment out to disable) */
#define SERIALDEBUG
bool serialinitialised = false;

void DEBUG(const String& line) {
#ifdef SERIALDEBUG
  if(!serialinitialised){
    Serial.begin(115200);
    serialinitialised = true;
  }
  Serial.print(line);
#endif
}

void DEBUGLN(const String& line) {
#ifdef SERIALDEBUG
  if(!serialinitialised){
    Serial.begin(115200);
    serialinitialised = true;
  }
  Serial.println(line);
#endif
}

/* Button definitions */
button_t red_button;
button_t green_button;
button_t blue_button;
button_t yellow_button;
#define DEBOUNCE        20     // milliseconds to wait for debouncing
int active_players = 0;

/*#define BUTTON_INACTIVE 0
#define BUTTON_HOT      1
#define BUTTON_BLOCKED  2

/* LED definitions */
#define NUMPIXELS       60
#define BRIGHTNESS      20     // 0-64
#define BLUE_PIXEL       5              // This offset is needed depending on how you assemble the ring and the associated buttons
#define RED_PIXEL       20              // This offset is needed depending on how you assemble the ring and the associated buttons
#define GREEN_PIXEL     35              // This offset is needed depending on how you assemble the ring and the associated buttons
#define YELLOW_PIXEL    50              // This offset is needed depending on how you assemble the ring and the associated buttons
CRGB cyclone[NUMPIXELS];

/* Buzzer definitions */
#define RED_TONE 220
#define GREEN_TONE 262
#define BLUE_TONE 330
#define YELLOW_TONE 392
#define TONE_DURATION 250

/* LCD settings */
LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

/* Game state definitions */
#define SNOOZING        0
#define RUNNING         1
#define EXPLODE         2
int GAME_STATUS = SNOOZING;
#define DEFAULT_GAME_SPEED 250  // Any range between 200 and 10 seems fair enough
int GAME_SPEED = DEFAULT_GAME_SPEED;
unsigned long game_delay;
int BALL_PIXEL;
int leapcount = 0;
int wonleap = 0;

/*
 * Initial boot sequence
 */
void setup() {

  // Show banner
  lcd.begin(20, 4);
  lcd.print("    MultiCyclone    ");
  DEBUGLN("MultiCyclone - Booting");   // send an initial string

  pinMode(BUTTON_RED, INPUT_PULLUP);
  pinMode(BUTTON_GREEN, INPUT_PULLUP);
  pinMode(BUTTON_BLUE, INPUT_PULLUP);
  pinMode(BUTTON_YELLOW, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);
  pinMode(LCD_CONTRAST, OUTPUT);
  pinMode(LCD_BACKLIGHT, OUTPUT);
  analogWrite(LCD_CONTRAST, 50); // 0-255; lower value most likely, nearer to 0 means high visibility of the background matrix (via an RC filter)
  analogWrite(LCD_BACKLIGHT, 25); // 0-255; higher means stronger backlight
  
  red_button.name = "Red";
  red_button.pixel_pos = RED_PIXEL;
  red_button.pos_name_x = 1;
  red_button.pos_name_y = 2;
  red_button.pos_score_x = 6;
  red_button.pos_score_y = 2;

  green_button.name = "Green";
  green_button.pixel_pos = GREEN_PIXEL;
  green_button.pos_name_x = 10;
  green_button.pos_name_y = 2;
  green_button.pos_score_x = 17;
  green_button.pos_score_y = 2;

  blue_button.name = "Blue";
  blue_button.pixel_pos = BLUE_PIXEL;
  blue_button.pos_name_x = 0;
  blue_button.pos_name_y = 3;
  blue_button.pos_score_x = 6;
  blue_button.pos_score_y = 3;
  
  yellow_button.name = "Yellow";
  yellow_button.pixel_pos = YELLOW_PIXEL;
  yellow_button.pos_name_x = 9;
  yellow_button.pos_name_y = 3;
  yellow_button.pos_score_x = 17;
  yellow_button.pos_score_y = 3;
  
  // initialize WS2812 LED pixels
  FastLED.addLeds<NEOPIXEL, PIXEL_PIN>(cyclone, NUMPIXELS);
  FastLED.setBrightness(1);

  // Start the ball at random
  BALL_PIXEL = int(random(0, NUMPIXELS-1));
}

/* 
 *  Button pressed events
 */
void button_read(uint8_t state, button_t *button) {

  // Debounce procedure
  if (state == LOW && button->state == 0) {
    // If button is pressed and we did not register it before
    button->state = 1;
    button->debounce = millis();
    // And do the work
  } else {
    if ((millis() - button->debounce) > DEBOUNCE) {
      // check if enough time has passed to consider it a state change
      if (button->state == 1 && state == HIGH){
        // Button was released, register that so we're ready for a new press
        button->state = 0;
      }
    }
    return;
  }

  if(button->playing == false) {
    DEBUG(button->name);
    DEBUGLN(" joined the game"); 
    joined();
    button->playing = true;
    active_players++;       // make sure to reset this once the game routine resets
    // Update overall banner
    lcd.setCursor(0, 1);
    lcd.print("     ");
    lcd.print(active_players);
    lcd.print(" player");
    if(active_players > 1) lcd.print("s");
    // Add player to scoreboard
    lcd.setCursor(button->pos_name_x, button->pos_name_y);
    lcd.print(button->name);
    lcd.print(":");
    lcd.setCursor(button->pos_score_x, button->pos_score_y);
    lcd.print("-");
    if(GAME_STATUS == RUNNING) return;
  }

  switch(GAME_STATUS){
    case SNOOZING:
                  new_game();
                  break;
    case RUNNING: 
                  if(BALL_PIXEL == button->pixel_pos) {
                    // Player wins the round!
                    DEBUG(button->name);
                    DEBUG(" wins the round: score = ");
                    button->score++;
                    DEBUGLN(String(button->score));
                    lcd.setCursor(button->pos_score_x, button->pos_score_y);
                    lcd.print(String(button->score));
                    hit();
                    // SPEED UP!!!
                    if(GAME_SPEED > 100) {
                      GAME_SPEED = GAME_SPEED - 25;
                    } else if(GAME_SPEED > 20) {
                      GAME_SPEED = GAME_SPEED - 10;
                    } else if(GAME_SPEED > 5) {
                      GAME_SPEED = GAME_SPEED - 5;
                    }
                    DEBUG("New speed is "); 
                    DEBUGLN(String(GAME_SPEED)); 
                    wonleap = leapcount;
                    // Restart the ball at random
//                    BALL_PIXEL = int(random(0, NUMPIXELS-1)); // User feedback: sounds nice in theory, sucks in practice
                  } else {
                    DEBUG(button->name);
                    DEBUGLN(" missed!"); 
                    missed();      
                  }
                  break;
  }
  
}

/*
 * Sound and visuals
 */

void joined() {
  tone(BUZZER, 220, 250);
  delay(100);
}

void missed() {
  tone(BUZZER,G4);
  delay(250);
  tone(BUZZER,C4);
  delay(500);
  noTone(BUZZER);

/*       unsigned long time = millis();
      int randomSound=1000;
      while(millis() - time <= 250)  {  
        randomSound--;
        tone(BUZZER, random(randomSound, 1000));   // change the parameters of random() for different sound
      }
      noTone(BUZZER); */
}

void hit() {
  tone(BUZZER,A5);
  delay(100);
  tone(BUZZER,B5);
  delay(100);
  tone(BUZZER,C5);
  delay(100);
  tone(BUZZER,B5);
  delay(100);
  tone(BUZZER,C5);
  delay(100);
  tone(BUZZER,D5);
  delay(100);
  tone(BUZZER,C5);
  delay(100);
  tone(BUZZER,D5);
  delay(100);
  tone(BUZZER,E5);
  delay(100);
  tone(BUZZER,D5);
  delay(100);
  tone(BUZZER,E5);
  delay(100);
  tone(BUZZER,E5);
  delay(100);
  noTone(BUZZER);
}
/* 
 * Start a new game
 */
void new_game() {
  DEBUGLN("New game starting"); 

  analogWrite(LCD_BACKLIGHT, 250); // 0-255; higher means stronger backlight

  // Reset new game defaults
  GAME_STATUS = RUNNING;
  GAME_SPEED = DEFAULT_GAME_SPEED;
  leapcount = 1;
  wonleap = 0;
  active_players = 0;
  red_button.score = 0;
  green_button.score = 0;
  blue_button.score = 0;
  yellow_button.score = 0;
  
  // Start the ball at random
  BALL_PIXEL = int(random(0, NUMPIXELS-1));
  
  // Maybe play some sounds and visuals to acknowledge the new game

  // And set the board
  set_board();
}

void set_board() {
  // Wipe the board
  FastLED.clear();
  // Indicate players
  if(GAME_STATUS == RUNNING) {
    if(red_button.playing == true) cyclone[RED_PIXEL] = CRGB::Red;
    if(green_button.playing == true) cyclone[GREEN_PIXEL] = CRGB::Green;
    if(blue_button.playing == true) cyclone[BLUE_PIXEL] = CRGB::DarkBlue;
    if(yellow_button.playing == true) cyclone[YELLOW_PIXEL] = CRGB::Yellow;
  }
  // Where's the ball
  cyclone[BALL_PIXEL] = CRGB::White;
  FastLED.show();
}

void have_winner() {
  wonleap = leapcount;
  // Wipe the board
  FastLED.clear();
/*  for (int i = 0; i < NUMPIXELS; i++) {
    cyclone[i] = CRGB::White;
  }*/
  // Make some sounds @@@TODO@@@
  delay(250);
}

int proximity(int ball, int button) {
  return 0;
}

void game_end() {
  // highest score
  int highscore = 0;
  if (red_button.score > highscore) highscore = red_button.score;
  if (blue_button.score > highscore) highscore = blue_button.score;
  if (green_button.score > highscore) highscore = green_button.score;
  if (yellow_button.score > highscore) highscore = yellow_button.score;

  unsigned long time = millis();
  int randomSound=1000;

  lcd.setCursor(0, 1);
  lcd.print("   Round finished   ");
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");

  for (int j = 0; j < 5; j++) {
    
    if (red_button.score == highscore) {
      while(millis() - time <= 250)  {  
        randomSound--;
        tone(BUZZER, random(randomSound, 1000));   // change the parameters of random() for different sound
      }
      noTone(BUZZER);

      // Add player to scoreboard
      lcd.setCursor(red_button.pos_name_x, red_button.pos_name_y);
      lcd.print(red_button.name);
      lcd.print(":");
      lcd.setCursor(red_button.pos_score_x, red_button.pos_score_y);
      lcd.print(String(red_button.score));

      for (int i = 0; i < NUMPIXELS; i++) {
        cyclone[i] = CRGB::Red;
        FastLED.show();
        delay(5);
      }
    }

    if (blue_button.score == highscore) {
      while(millis() - time <= 250)  {  
        randomSound--;
        tone(BUZZER, random(randomSound, 1000));   // change the parameters of random() for different sound
      }
      noTone(BUZZER);
      
      // Add player to scoreboard
      lcd.setCursor(blue_button.pos_name_x, blue_button.pos_name_y);
      lcd.print(blue_button.name);
      lcd.print(":");
      lcd.setCursor(blue_button.pos_score_x, blue_button.pos_score_y);
      lcd.print(String(blue_button.score));
 
      for (int i = 0; i < NUMPIXELS; i++) {
        cyclone[i] = CRGB::DarkBlue;
        FastLED.show();
        delay(5);
      }
    }
  
    if (green_button.score == highscore) {
      while(millis() - time <= 250)  {  
        randomSound--;
        tone(BUZZER, random(randomSound, 1000));   // change the parameters of random() for different sound
      }
      noTone(BUZZER);
      
      // Add player to scoreboard
      lcd.setCursor(green_button.pos_name_x, green_button.pos_name_y);
      lcd.print(green_button.name);
      lcd.print(":");
      lcd.setCursor(green_button.pos_score_x, green_button.pos_score_y);
      lcd.print(String(green_button.score));
  
      for (int i = 0; i < NUMPIXELS; i++) {
        cyclone[i] = CRGB::Green;
        FastLED.show();
        delay(5);
      }
    }
  
    if (yellow_button.score == highscore) {
      while(millis() - time <= 250)  {  
        randomSound--;
        tone(BUZZER, random(randomSound, 1000));   // change the parameters of random() for different sound
      }
      noTone(BUZZER);
     
      // Add player to scoreboard
      lcd.setCursor(yellow_button.pos_name_x, yellow_button.pos_name_y);
      lcd.print(yellow_button.name);
      lcd.print(":");
      lcd.setCursor(yellow_button.pos_score_x, yellow_button.pos_score_y);
      lcd.print(String(yellow_button.score));
 
      for (int i = 0; i < NUMPIXELS; i++) {
        cyclone[i] = CRGB::Yellow;
        FastLED.show();
        delay(5);
      }
    }
  }
  
  lcd.setCursor(0, 1);
  lcd.print("                    ");
  lcd.setCursor(0, 2);
  lcd.print("                    ");
  lcd.setCursor(0, 3);
  lcd.print("                    ");

  FastLED.setBrightness(1);
  GAME_SPEED = DEFAULT_GAME_SPEED;
  GAME_STATUS = SNOOZING;
  red_button.playing = false;
  blue_button.playing = false;
  green_button.playing = false;
  yellow_button.playing = false;
  
}

/* 
 *  Run the Cyclone!
 */
void loop() {
  // Check button states
  button_read(digitalRead(BUTTON_RED), &red_button);
  button_read(digitalRead(BUTTON_GREEN), &green_button);
  button_read(digitalRead(BUTTON_BLUE), &blue_button);
  button_read(digitalRead(BUTTON_YELLOW), &yellow_button);

  // Update board to new state
  switch(GAME_STATUS){
    case SNOOZING:
      // Game is inactive (initial state) so just play a running light sequence for kicks
      if(millis() - game_delay > GAME_SPEED) {
        game_delay = millis();
        BALL_PIXEL++;
        if(BALL_PIXEL >= NUMPIXELS) BALL_PIXEL = 0;
        set_board();
      }
      break;
      
    case RUNNING:
      // Play game routine
      FastLED.setBrightness(BRIGHTNESS);
      if(millis() - game_delay > GAME_SPEED) {
        game_delay = millis();
        BALL_PIXEL++;
        if(BALL_PIXEL >= NUMPIXELS) {
          leapcount++;
          BALL_PIXEL = 0;
        }
        if((leapcount - wonleap) > 5) game_end();
        set_board();
      }
      break;

    case EXPLODE:     // defunct
      // Sound and visuals
      // Upon finishing the game, we need to reset the playing var to false;
        GAME_STATUS = RUNNING;


      // explosion sound
      unsigned long time = millis();
      int randomSound=1000;
      while(millis() - time <= 250)  {  
        randomSound--;
        tone(BUZZER, random(randomSound, 1000));   // change the parameters of random() for different sound
      }
      noTone(BUZZER);
      break;
      
  }

}
