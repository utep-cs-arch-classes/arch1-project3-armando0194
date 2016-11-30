/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6
#define SW1 BIT0
#define SW2 BIT1
#define SW3 BIT2
#define SW4 BIT3

int carHorOffset = 0;
int carVerOffset = 0;

/** 
   Initialize the shape
*/
AbRect carBody = {abRectGetBounds, abRectCheck, {8 ,10}}; // car body - rectangle 
AbRect grass = {abRectGetBounds, abRectCheck, {13, screenHeight/2}}; // grass in both sides of the road - rectangle
AbRect enemy = {abRectGetBounds, abRectCheck, {10,10}}; // enemies - circle
AbRectOutline fieldOutline = { abRectOutlineGetBounds, abRectOutlineCheck, {screenWidth/2, screenHeight/2 + 20} }; // playing field

/** 
   Initialize the layers
*/
Layer fieldLayer = { (AbShape *) &fieldOutline, {screenWidth/2, screenHeight/2 - 20}, {0,0}, {0,0}, COLOR_BLACK, 0 }; 
Layer grassRightSide = { (AbShape *)&grass, {0, (screenHeight/2)}, {0,0}, {0,0}, COLOR_GREEN, &fieldLayer };
Layer grassLeftSide = { (AbShape *)&grass, {(screenWidth), (screenHeight/2)}, {0,0}, {0,0}, COLOR_GREEN, &grassRightSide };
Layer car = { (AbShape *)&carBody, {screenWidth/2, screenHeight/2}, {0,0}, {0,0}, COLOR_RED, &grassLeftSide };
Layer enemyLeftSide = { (AbShape *)&circle4, {screenWidth/2 - 30, -13}, {0,0}, {0,0}, COLOR_ORANGE, &car };
Layer enemyRightSide = { (AbShape *)&circle4, {screenWidth/2 + 30, -13}, {0,0}, {0,0}, COLOR_YELLOW, &enemyLeftSide };
Layer enemyCenter = { (AbShape *)&circle4, {screenWidth/2, -13}, {0,0}, {0,0}, COLOR_BLUE, &enemyRightSide };

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/** Car Moving Layer Linked List
 */
MovLayer ml0 = { &car, {0,0}, 0 };

/** Enemy Moving Layer Linked List
 */
MovLayer enemyMl2 = { &enemyLeftSide, {0,5}, 0};
MovLayer enemyMl1 = { &enemyRightSide, {0,4}, &enemyMl2};
MovLayer enemyMl0 = { &enemyCenter, {0,7}, &enemyMl1};
 
char score = 0;                  /** Score ones place */
char scoreDecimal = 0;           /** Score decimal place */
char difficulty = 1;             /** game difficulty */
char scoreStr[11] = "score: 00"; /** Score string */
char indexScore = 8;             /** Score index */
short transitionSpeed = 30;      /** number of interrupts */
char isGameOver = 0;             /** Boolean that determins if the game is over */
u_int bgColor = 0xcdff - 1;      /** Background color */
int redrawScreen = 1;            /** Boolean for whether screen needs to be redrawn */
char showInstruction = 1;        /** Boolean for shwoing the instruction */ 
Region fieldFence;		 /** Fence around playing field  */

/** Readraws moving layers in their next location 
 *  
 *  \param movLayers Linked list containing the shapes that will move
 *  \param layers Linked list containing all the layers in the game
 */
movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  

/** Determines the enemies next position
 *  
 *  \param ml Linked list containing the shapes that will move
 *  \param fence The region which will serve as a boundary for ml
 */
void enemyAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    newPos.axes[1] = newPos.axes[1] + difficulty;

    // if the enemy reaches the bottom, move it to the top again
    if(shapeBoundary.topLeft.axes[1] > screenHeight-20){
      newPos.axes[1] = -10;
      score++;
    }
    
    ml->layer->posNext = newPos;
  } /**< for ml */
}


/** Determines the car next position
 *  
 *  \param ml Linked list containing the shapes that will move
 *  \param fence The region which will serve as a boundary for ml
 */
void moveCar(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;

  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    //moves the car horizontally if possible
    if(newPos.axes[0] + carHorOffset > 20 &&
       newPos.axes[0] + carHorOffset < screenWidth -20){
      newPos.axes[0] = newPos.axes[0] + carHorOffset;
    }
    //moves the car vertically if possible
    if(newPos.axes[1] + carVerOffset > 0 &&
       newPos.axes[1] + carVerOffset < screenHeight){
      newPos.axes[1] = newPos.axes[1] + carVerOffset;
    }
    ml->layer->posNext = newPos;
  } /**< for ml */
}

/** Determines if the car has a collision with any of the enemies
 *  
 *  \param enemy Linked list containing the car shapes
 *  \param car Linked list containing the car shapes
 */
char checkForCollision(MovLayer *enemy, MovLayer *car){
  
  Region carBounday;
  Region enemyBoundary;
  Vec2 coordinates;
  
  abShapeGetBounds(car->layer->abShape, &car->layer->posNext, &carBounday);
  for (; enemy; enemy = enemy->next) {
    vec2Add(&coordinates, &enemy->layer->pos, &enemy->velocity);
    abShapeGetBounds(enemy->layer->abShape, &coordinates, &enemyBoundary);

    // check if the enemy corner pixels are inside the boundaries of the car
    if( abShapeCheck(car->layer->abShape, &car->layer->pos, &enemyBoundary.topLeft) ||
        abShapeCheck(car->layer->abShape, &car->layer->pos, &enemyBoundary.botRight) ){
      isGameOver = 1;
    }
  }  
  return 0;
}

/** Reads switches and determines in which direction the car has to move
 */
void readSwitches(){
  char switches =  p2sw_read();
  char isS1Pressed = (switches & SW1) ? 0 : 1;
  char isS2Pressed = (switches & SW2) ? 0 : 1;
  char isS3Pressed = (switches & SW3) ? 0 : 1;
  char isS4Pressed = (switches & SW4) ? 0 : 1;

  if(isS1Pressed){  // if s1 is pressed, move left
    carHorOffset = -30;
  }
  if(isS2Pressed){  // if s2 is pressed, move down
    carVerOffset = 30;
  }
  if(isS3Pressed){  // if s3 is pressed, move up
    carVerOffset = -30;
  }
  if(isS4Pressed){  // if s4 is pressed, move right
    carHorOffset = 30;
  }
  //redraw screen if any button was pressed
  if(isS1Pressed || isS2Pressed || isS3Pressed || isS4Pressed){
    moveCar(&ml0, &fieldFence);
    redrawScreen = 1;
    buzzer_play_car_move();
  }
}

/** Draws the score in the top right corner
 */
void drawTheScore(){
  // draw the score as long as the game oves is not over
  if(!isGameOver){
    if(scoreStr[8] == '9'){
      scoreDecimal++;
      scoreStr[7] = '0' + scoreDecimal;
      score = 0;
    }
    scoreStr[indexScore] = '0' + score;
    drawString5x7(screenWidth - 45, 2, scoreStr, COLOR_WHITE, COLOR_BLACK );
  }
}

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */	       

  configureClocks();
  lcd_init();
  buzzer_init();
  p2sw_init(15);
  
  layerInit(&enemyCenter);
  layerDraw(&enemyCenter);
  layerGetBounds(&fieldLayer, &fieldFence);

  drawString5x7(screenWidth/2 -45, screenHeight/2 - 50, "Evade the obstacles", COLOR_WHITE, COLOR_BLACK );
  drawString5x7(screenWidth/2 -45, screenHeight/2 - 40, "Press S1 to Start", COLOR_WHITE, COLOR_BLACK );

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);/**< CPU OFF */
      readSwitches();
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    carHorOffset = 0;
    carVerOffset = 0;
    movLayerDraw(&ml0, &car);
    movLayerDraw(&enemyMl0, &enemyCenter);
    drawTheScore();
  }
}

/** State machine that determines what will be rendered in the screen
 */
void state_advance(){
  static enum {instructions = 0, play = 1, game_over = 2} currentState =instructions;
  static char switches;
  static char isS1Pressed;
      
  switch(currentState){
  case instructions:  // This state plays a song and ask the user for input
    switches =  p2sw_read();
    isS1Pressed = (switches & SW1) ? 0 : 1;
    //buzzer_play_game_song();
    if(isS1Pressed){         // if s1 was presed move to the next state
      buzzer_set_period(0);
      currentState = play;
      transitionSpeed = 80;
      layerDraw(&enemyCenter);
    }
    break;
  case play:  // In this state, the user is playing the game
    buzzer_set_period(0); // stop music if something is playing
    if(!isGameOver){     // if game is not over move enemies and check for collisions
      enemyAdvance(&enemyMl0, &fieldFence);
      checkForCollision(&enemyMl0, &ml0);
      redrawScreen = 1;
    }
    else{              // if game is over print game over on the screen and move to next state
      layerDraw(&enemyCenter);
      drawString5x7(screenWidth/2 -20, screenHeight/2, "Game Over", COLOR_WHITE, COLOR_BLACK );
      drawString5x7(screenWidth/2 -55, screenHeight/2 + 10, "Press S1 to play again", COLOR_WHITE, COLOR_BLACK );
      currentState = game_over;
      transitionSpeed = 30;
    }
    break;
  case game_over: // plays music and asks the user if he wants to keep playing
    switches =  p2sw_read();
    isS1Pressed = (switches & SW1) ? 0 : 1;
    if(!isS1Pressed){ 
      //buzzer_play_game_song();
    }
    else{        // if the user press s1, reset all the variables and changes state to play
      car.posNext.axes[1] = screenHeight/2;
      car.posNext.axes[0] = screenWidth/2;
      enemyLeftSide.posNext.axes[1] = -10;
      enemyRightSide.posNext.axes[1] = -10;
      enemyCenter.posNext.axes[1] = -10;
      enemyMl0.velocity.axes[1] = 4;
      enemyMl1.velocity.axes[1] = 2;
      enemyMl2.velocity.axes[1] = 3;
      layerDraw(&enemyCenter);
      currentState = play;
      difficulty = 1;
      score = 0;
      scoreDecimal = 0;
      scoreStr[7] = '0';
      redrawScreen = 1;
      isGameOver = 0;
      transitionSpeed = 80;
    }
    break;
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  static short difficultyCounter = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count++;
  
  if (count == transitionSpeed) {
    state_advance();
    count = 0;
  }

  if(difficultyCounter == 100){
    if(difficulty < 10){
      difficulty += 1;
    }
    difficultyCounter = 0;
  }
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}

