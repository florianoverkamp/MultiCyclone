/* 
 *  Multicyclone - a DIY Arduino Multiplayer Cyclone game 
 *  by F. Overkamp 2020
 *  
 *  
 *  Basic game implementation based on https://www.instructables.com/id/Cyclone-LED-Arcade-Game/
 *  Possible extension idea to change directions as per https://www.youtube.com/watch?v=LWt2IgzJpRQ
 *  
 */
#include "buzzertones.h"
#include <FastLED.h>
#include <LiquidCrystal.h>

/* Define connection pins */
#define PIXEL_PIN       2
#define BUTTON_RED      3
#define BUTTON_GREEN    4
#define BUTTON_BLUE     5
#define BUTTON_YELLOW   6
#define LCD_D4          7
#define LCD_D5          8
#define LCD_D6          9
#define LCD_D7          10
#define LCD_EN          11
#define LCD_RS          12
#define BUZZER          13

/* Serial debug (comment out to disable) */
//#define SERIAL

/* Button definitions */
struct button_t
{
  byte state = 0;
  bool playing = false;
  unsigned long debounce;
  String name;
  int score = 0;
  int pixel_pos;
  int pos_name_x;
  int pos_name_y;
  int pos_score_x;
  int pos_score_y;
};
button_t red_button;
button_t green_button;
button_t blue_button;
button_t yellow_button;
#define DEBOUNCE        80     // milliseconds to wait for debouncing

/*#define BUTTON_INACTIVE 0
#define BUTTON_HOT      1
#define BUTTON_BLOCKED  2

/* LED definitions */
#define NUMPIXELS       60
#define BRIGHTNESS      10     // 0-64
#define RED_PIXEL       0     // @@@TODO@@@ Check offset to match once pixels and buttons are fixed
#define GREEN_PIXEL     RED_PIXEL+15
#define BLUE_PIXEL      RED_PIXEL+30
#define YELLOW_PIXEL    RED_PIXEL+45
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
#define DEFAULT_GAME_SPEED 350  // Any range between 200 and 10 seems fair enough
int GAME_SPEED = DEFAULT_GAME_SPEED;
unsigned long game_delay;
int BALL_PIXEL;

/*
 * Initial boot sequence
 */
void setup() {

  // Show banner
  lcd.begin(20, 4);
//  lcd.print("01234567890123456789");  
  lcd.print("    MultiCyclone    ");
  
#ifdef SERIAL 
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  // Banner
  Serial.println("MultiCyclone - Booting");   // send an initial string
#endif

  pinMode(BUTTON_RED, INPUT_PULLUP);
  pinMode(BUTTON_GREEN, INPUT_PULLUP);
  pinMode(BUTTON_BLUE, INPUT_PULLUP);
  pinMode(BUTTON_YELLOW, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  red_button.name = "Red";
  red_button.pixel_pos = RED_PIXEL;
  red_button.pos_name_x = 9;
  red_button.pos_name_y = 2;

  green_button.name = "Green";
  green_button.pixel_pos = GREEN_PIXEL;
  green_button.pos_name_x = 14;
  green_button.pos_name_y = 2;

  blue_button.name = "Blue";
  blue_button.pixel_pos = BLUE_PIXEL;
  blue_button.pos_name_x = 0;
  blue_button.pos_name_y = 3;

  yellow_button.name = "Yellow";
  yellow_button.pixel_pos = YELLOW_PIXEL;
  yellow_button.pos_name_x = 5;
  yellow_button.pos_name_y = 3;

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
#ifdef SERIAL
  Serial.println("Joining the game"); 
#endif
    joined();      
    button->playing = true;
    lcd.setCursor(button->pos_name_x, button->pos_name_y);
    lcd.print(button->name);

    // @@@TODO@@@ Play a little sound and return, or the second/third/fourth player get a miss right away
  }

  switch(GAME_STATUS){
    case SNOOZING:
                  new_game();
                  break;
    case RUNNING: 
        if(BALL_PIXEL == button->pixel_pos) {
          // Player wins the round!
#ifdef SERIAL
  Serial.println("Player wins the round!"); 
#endif
          hit();
          // SPEED UP!!!
          if(GAME_SPEED > 50) {
            GAME_SPEED = GAME_SPEED - 25;
#ifdef SERIAL
  Serial.print("Speed timer decreased to "); 
  Serial.println(GAME_SPEED); 
#endif
          }
        } else {
#ifdef SERIAL
  Serial.println("Player missed!"); 
#endif
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
#ifdef SERIAL
  Serial.println("New game starting"); 
#endif

  lcd.setCursor(0, 1);
  lcd.print("Game commencing...");
  
  // Reset new game defaults
  GAME_STATUS = RUNNING;
  GAME_SPEED = DEFAULT_GAME_SPEED;
  
  // Start the ball at random
  BALL_PIXEL = int(random(0, NUMPIXELS-1));

  // Game setup display (individual players are displayed elsewhere)
  lcd.setCursor(0, 2);
  lcd.print("Players: ");
  
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
        if(BALL_PIXEL >= NUMPIXELS) BALL_PIXEL = 0;
        set_board();
      }
      break;

    case EXPLODE:
      // Sound and visuals
#ifdef SERIAL
  Serial.println("Exploded"); 
#endif
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
  
  // Check for button activity
/*  if (digitalRead(BUTTON1) == LOW && button1_state == 0) {
    // check if enough time has passed to consider it a switch press
    if ((millis() - debouncetimer) > DEBOUNCE) {
      button1_state=1;
      if(state == STATE_AVAILABLE) {
        setBUSY();
      } else if(state == STATE_BUSY) {
        setMEETING();
      } else {
        setAVAILABLE();
      }
      debouncetimer = millis();
    }
  } else {
    if (button1_state == 1 && digitalRead(BUTTON1) == HIGH){
      button1_state=0;
    }
  }
*/



}
