# User Inputs

The original version of IVy has been modified to include new user-input functionalities.

## In `ivy_check` with model checking

When model checking, it will ask if the user desires the shortest path. If the character "y" or "Y" is in the response, the resulting path will be a shortest counterexample.

It will also ask if the user desires abstraction. If the character "y" or "Y" is in the response, the resulting path will be found using abstraction. This abstraction often speeds up counterexample discovery. Otherwise, the standard PDR method will be called.

Both of these changes can be found in `ivy_mc.py`

