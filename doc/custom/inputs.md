# User Inputs

The original version of IVy has been modified to include new user-input functionalities.

> **Important Note**
>
> This version of IVy requires real-time user input. With other versions of IVy, it is possible to execute IVy without paying any attention to it. All user inputs for each function happen at the same time, but it would be wise for the user to pay attention to the program's execution until the inputs are completed.

## In `ivy_check` with model checking

When model checking, it will ask if the user desires the shortest path. If the character "y" or "Y" is in the response, the resulting path will be a shortest counterexample.

It will also ask if the user desires abstraction. If the character "y" or "Y" is in the response, the resulting path will be found using abstraction. This abstraction often speeds up counterexample discovery. Otherwise, the standard PDR method will be called.

Both of these changes can be found in `ivy_mc.py`

## In `ivy_to_cpp` for model simulation

When creating and compiling the C++ simulation, IVy will ask the user for the number of simulation steps and runs. If the user inputs 100 steps and 20 runs, the model will initialize from state 0 a total of 20 times. Each of these 20 initializations will run for 100 steps (or until an error occurs).