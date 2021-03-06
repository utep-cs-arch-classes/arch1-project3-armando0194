#include <msp430.h>
#include "buzzer.h"

// Define all the notes to its specific frequency 
#define C3 7634
#define D3 6803
#define E3 6061
#define F3 5714
#define G3 5102
#define A3 4545
#define B3 4049
#define C4 3838
#define D4 3400
#define E4 3038
#define F4 2864
#define G4 2550
#define A4 2272
#define A4S 2146
#define B4 2028
#define C5 1912
#define D5 1706
#define E5 1517
#define F5 1433
#define G5 1276
#define A5 1136
#define B5 1012
#define C6 955

//Array that contains the notes of the testris song
int game_song[] = {F4,  0,  0, D4,  0,  0, D4,  0, C4,  0,
		    0, D4,  0,  0, D4,  0, F4,  0,  0, D4,
		    0,  0, D4,  0, C4, C4, D4, D4, F4,  0,
		   G4,  0,  0, F4,  0,  0, A4,  0, G4,  0,
		    0, G4,  0,  0, F4,  0, G4,  0,  0, F4,
		    0,  0, D4,  0, D4};
int game_song_length = 55; //length of the tetris song
                                                                                                                        void buzzer_init()
{
    /* 
       Direct timer A output "TA0.1" to P2.6.  
        According to table 21 from data sheet:
          P2SEL2.6, P2SEL2.7, anmd P2SEL.7 must be zero
          P2SEL.6 must be 1
        Also: P2.6 direction must be output
    */
    timerAUpmode();		/* used to drive speaker */
    P2SEL2 &= ~(BIT6 | BIT7);
    P2SEL &= ~BIT7; 
    P2SEL |= BIT6;
    P2DIR = BIT6;		/* enable output to speaker (P2.6) */
    buzzer_set_period(0);       /*set buzzer to  0 at the begining */
}

/*Plays tetris notes */
void buzzer_play_game_song(){
  static int current_note = 0;
  current_note %= game_song_length;

  // Send current note to set period and moves to the next note
  buzzer_set_period(game_song[current_note]);
  current_note++;
}

/*Plays tetris notes */
void buzzer_play_car_move (){
  buzzer_set_period(G4);
}

/* Sets the buzzer period */
void buzzer_set_period(short cycles)
{
  CCR0 = cycles; 
  CCR1 = cycles >> 1;		/* one half cycle */
}

