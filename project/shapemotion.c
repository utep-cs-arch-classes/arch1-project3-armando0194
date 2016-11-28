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

AbRect rect10 = {abRectGetBounds, abRectCheck, {5,5}}; /**< 10x10 rectangle */
AbRect carBody = {abRectGetBounds, abRectCheck, {8 ,15}}; 
AbRect carWindow ={abRectGetBounds, abRectCheck, {6 ,4}};
AbRect grass = {abRectGetBounds, abRectCheck, {13, screenHeight/2}};
AbRect roadLine = {abRectGetBounds, abRectCheck, {1, screenHeight/2}};
AbRect enemy = {abRectGetBounds, abRectCheck, {10,10}};
int carHorOffset = 0;
int carVerOffset = 0;

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2, screenHeight/2 + 20}
};


//Layer layer4 = {
// (AbShape *)&rightArrow,
// {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
// {0,0}, {0,0},				    /* last & next pos */
// COLOR_PINK,
// 0
//};

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2 - 20},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  0
};

Layer grassRightSide = {
  (AbShape *)&grass,
  {0, (screenHeight/2)}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GREEN,
  &fieldLayer
};

Layer grassLeftSide = {
  (AbShape *)&grass,
  {(screenWidth), (screenHeight/2)}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_GREEN,
  &grassRightSide
};
/*
Layer roadLineRight = {
  (AbShape *)&roadLine,
  {(screenWidth)-49, (screenHeight/2)},
  {0,0}, {0,0},				   
  COLOR_WHITE,
  &grassLeftSide
};
*/
/*
Layer roadLineLeft = {
  (AbShape *)&roadLine,
  {(screenWidth)-79, (screenHeight/2)}, 
  {0,0}, {0,0},				    
  COLOR_WHITE,
  &roadLineRight,
};
*/
/*
Layer wheelBottomRight = {
  (AbShape *)&circle2,
  {screenWidth/2 - 10, screenHeight/2 + 10}, 
  {0,0}, {0,0},				    
  COLOR_BLACK,
  &roadLineLeft,
};*/

/*
Layer wheelBottomLeft = {
  (AbShape *)&circle2,
  {screenWidth/2 - 10, screenHeight/2 + 10}, 
  {0,0}, {0,0},				    
  COLOR_BLACK,
  &wheelBottomRight,
};*/

/*
Layer wheelTopRight = {
  (AbShape *)&circle2,
  {screenWidth/2 - 10, screenHeight/2 - 10}
  {0,0}, {0,0},			
  COLOR_BLACK,
  &wheelBottomLeft,
};*/

/*
Layer wheelTopLeft = {
  (AbShape *)&circle2,
  {screenWidth/2 + 10, screenHeight/2 - 10},
  {0,0}, {0,0},				  
  COLOR_BLACK,
  &wheelTopRight,
};*/

Layer car = {		/**< Layer with a red square */
  (AbShape *)&carBody,
  {screenWidth/2, screenHeight/2}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  &grassLeftSide,
};
/*
Layer window = {
  (AbShape *)&carWindow,
  {screenWidth/2 , screenHeight/2 - 4}, 
  {0,0}, {0,0},				   
  COLOR_BLACK,
  &car,
  };*/

Layer enemyLeftSide = {
  (AbShape *)&circle4,
  {screenWidth/2 - 30, -13},
  {0,0}, {0,0},
  COLOR_ORANGE,
  &car,
};

Layer enemyRightSide = {
  (AbShape *)&circle4,
  {screenWidth/2 + 30, -13},
  {0,0}, {0,0},
  COLOR_YELLOW,
  &enemyLeftSide,
};

Layer enemyCenter = {
  (AbShape *)&circle4, {screenWidth/2, -13}, {0,0}, {0,0}, COLOR_BLUE, &enemyRightSide };
/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

/* initial value of {0,0} will be overwritten */
//MovLayer ml5 = { &wheelTopRight, {0,0}, 0 }; /**< not all layers move */
//MovLayer ml4 = { &wheelTopLeft, {0,0}, &ml5 }; /**< not all layers move */
//MovLayer ml3 = { &wheelTopLeft, {0,0}, 0 }; 
//MovLayer ml2 = { &wheelTopRight, {0,0}, &ml3 }; /**< not all layers move */
//MovLayer ml1 = { &car, {0,0}, 0 }; 
MovLayer ml0 = { &car, {0,0}, 0 };

MovLayer enemyMl2 = { &enemyLeftSide, {0,3}, 0};
MovLayer enemyMl1 = { &enemyRightSide, {0,2}, &enemyMl2};
MovLayer enemyMl0 = { &enemyCenter, {0,4}, &enemyMl1};

char score = 0;
char scoreDecimal = 0;
char difficulty = 1;
char scoreStr[9] = "score: 00";
char indexScore = 8;

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

//Region fence = {{10,30}, {SHORT_EDGE_PIXELS-10, LONG_EDGE_PIXELS-10}}; /**< Create a fence region */

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);

    //newPos.axes[1] += difficulty;
    if(shapeBoundary.topLeft.axes[1] > screenHeight-20){
      newPos.axes[1] = -10;
      score++;
    }
    
    ml->layer->posNext = newPos;
  } /**< for ml */
}


void moveCar(MovLayer *ml, Region *fence)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;

  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    if(newPos.axes[0] + carHorOffset > 20 &&
       newPos.axes[0] + carHorOffset < screenWidth -20){
      newPos.axes[0] = newPos.axes[0] + carHorOffset;
    }

    if(newPos.axes[1] + carVerOffset > 0 &&
       newPos.axes[1] + carVerOffset < screenHeight){
      newPos.axes[1] = newPos.axes[1] + carVerOffset;
    }
    ml->layer->posNext = newPos;
  } /**< for ml */
}

u_int bgColor = 0xcdff;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */
char showInstruction = 1;

Region fieldFence;		/**< fence around playing field  */

char checkForCollision(MovLayer *enemies, MovLayer *car){

  
  return 0;
}

void readSwitches(){
  char switches =  p2sw_read();
  char isS1Pressed = (switches & SW1) ? 0 : 1;
  char isS2Pressed = (switches & SW2) ? 0 : 1;
  char isS3Pressed = (switches & SW3) ? 0 : 1;
  char isS4Pressed = (switches & SW4) ? 0 : 1;
    
  if(isS1Pressed){
    carHorOffset = -30;
  }
  if(isS2Pressed){
    carVerOffset = 30;
  }
  if(isS3Pressed){
    carVerOffset = -30;
  }
  if(isS4Pressed){
    carHorOffset = 30;
  }

  if(isS1Pressed || isS2Pressed || isS3Pressed || isS4Pressed){
    moveCar(&ml0, &fieldFence);
    redrawScreen = 1;
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

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    carHorOffset = 0;
    carVerOffset = 0;
    movLayerDraw(&ml0, &car);
    movLayerDraw(&enemyMl0, &enemyCenter);

    if(scoreStr[8] == '9'){
      scoreDecimal++;
      scoreStr[7] = '0' + scoreDecimal;
      score = 0;
    }
    scoreStr[indexScore] = '0' + score;

    // if(showInstruction){
    // drawString5x7(screenWidth/2, screenHeight/2, "Evade", COLOR_WHITE, COLOR_BLACK );
    //}
    //else{
      drawString5x7(screenWidth - 45, 2, scoreStr, COLOR_WHITE, COLOR_BLACK );
      //}
  }
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  static short difficultyCounter = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count++;
  
  if (count == 30) {
    //if(!showInstruction){
      readSwitches();
      mlAdvance(&enemyMl0, &fieldFence);
      checkForCollision(&enemyMl0, &ml0);
      redrawScreen = 1;
      //}
    count = 0;
    //buzzer_play_game_song();
  }

  if(difficultyCounter ==  250){
    if(showInstruction){
      showInstruction = 0;
      redrawScreen = 1;
      layerDraw(&enemyCenter);
    }
    difficulty++;
    difficultyCounter = 0;
  }
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
