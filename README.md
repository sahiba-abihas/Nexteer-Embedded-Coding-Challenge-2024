# Nexteer-Embedded-Coding-Challenge-2024

Overall structure: used multiple bools to track whether a phase had been started & finished
void loop() gets run multiple times, and I want to read some parts over and over like whether a button has been pressed, or IR remote/serial monitor readings.
Phase 3 does need to keep repeating until the directions have been done
Hence, I use all the following below to provide a structure to my simulation and only do the full simulation ONCE:


Setup:
Begin serial monitor
Start lcd screen
Wifi setup & sending UART commands with nexteer command interface
Initializing all necessary hardware such as IR remote, pushbutton, and Shift Register
Initializing arrays for phase 4 since setup only runs once

Loop (actual simulation):
Debounce logic for push button (check whether state has changed)
Once button pressed, then bool started becomes true → start printing requirements on LCD screen and checking for IR remote/serial monitor inputs
Once proper ID is inputted, continue on to phases
Using power button to reset the id when needed, because there can be a lot of noise with the receiver at times
Phase 1: String parsing and manipulation logic using .indexOf() and .substring()
Phase 2: Similar string logic, modular operator, and servo movement using for loop for sweep motion
Only attaching and detaching the servo when needed to be used and when done being used, because when it was initially tried attaching in setup, it made a lot of random movements 
This allows me to not use the power supply module, and was able to use that for an extra credit component (the fan)
Phase 3: Joystick inputs, using joySteps counter to make sure the steps are done until all directions have been inputted. Resets push button counter to 1.0 at the end for ease after phase 5. 
Phase 4: Number puzzle algorithm using 2D arrays to identify ALL possible indexes for ALL possible digits. 
Rational: Simulate similar to a human trying to rule out all possible letters in wordle (when the letter gets grayed out). Similarly, the possible index/digit will become false.
Phase 5: Using the shift register (bits & bytes), and a switch/case loop, the 8-digit location code is shown on the LEDs
By now, all goToPhase[X] should be true and phase[X]Done should be true, so no phases will run again
Loop still checking for button and remote/serial inputs. Once buttonCounter = 2.0, the inputted remote code is checked with the phase 5 STARMAP server. 
