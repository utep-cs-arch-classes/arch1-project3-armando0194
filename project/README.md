# Lab 2: Blinky Toy
Lab 2 consisted in turning the msp430 into a simple game. The msp430 is equipped with several features such as buzzer, LED's, LCD screen and buttons. During my childhood, I used to play a game that consisted in controlling a little car, and the objective was to evade the obstacles. I decided to recreate the game by using the LCD screen. The game has a start screen that shows the objective of the game and s/he has to press s1 to start. Then, while the user is playing the game, the difficulty increases. when the difficulty increases, the obstacles move faster, and their color turns darker. if the user loses, s/he can restart the game by pressing S1.

# Collaborations:
I collaborated with Abner Palomino in order to come up with a collision algorithm.

# Project
This project contains the following files:

1. buzzer.h: header file of buzzer
2. buzzer.c: implementation of the msp430 buzzer
3. carGame.c initialize the switches, led's, buzzer, and interrupts 

During the game every button in the msp430 has a different functionality:

1. S1 Moves car left and also works as a start button on menues
2. S2 Moves car down
3. S3 Moves car up
4. S4 Moves car right

To compile and install libraries (root directory):
~~~
$ make install
~~~

To test it, go to project directory and try:
~~~
$ make load
~~~

To delete binaries and lib:
~~~
$ make clean
~~~
