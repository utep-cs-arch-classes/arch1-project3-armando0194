# Lab 3: LCD Game
Lab 3 consisted in turning the msp430 into a simple game. The msp430 is equipped with several features such as buzzer, LED's,
LCD screen and buttons. During my childhood, I used to play a game that consisted in controlling a little car, and the objective
was to evade the obstacles. I decided to recreate the game by using the LCD screen. The game has a start screen that shows the
objective of the game and a simple menu. The user has to press S1 to start. Once the game starts the user is able to move the
car up, down, left or right. If an obstacle reaches the end of the screen, a new one will appear and it will move faster and be
darker. If the user loses, s/he can restart the game by pressing S1.

# Collaborations:
I collaborated with Abner Palomino in order to come up with a collision algorithm.
At the end, I decided to take advantage of the checkAbShape to determine if the car
had a collision with the obstacles

# Project
This project contains the following files:

1. buzzer.h:  header file of buzzer
2. buzzer.c:  implementation of the msp430 buzzer
3. carGame.c  Handles the logic of the game and the LCD                                                       

During the game every button in the msp430 has a different functionality:

1. S1 Moves car left and also works as a start button on menues
2. S2 Moves car down
3. S3 Moves car up
4. S4 Moves car right

To compile and install libraries (root directory):
~~~
$ make install
~~~

To test it, try:
~~~
$ make load
~~~

To delete binaries and lib:
~~~
$ make clean
~~~
