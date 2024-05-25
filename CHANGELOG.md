## 0.6.2

. `<` to combine multiple commands in one line command

____


## 0.6.1

. `ga`command generates random amplitudes for each and every note in the playing phrase

, `rl`, `rr` commands rotate, one/various positions, left or right, respectively, each note(s)/cc

. Upgrade notes/cc sync process

____

## 0.5.37

. New meaning to `/` command. Eg. `/8`, phrase duration will be 8 times longer.

. Improve sync

. Fix some parser pattern matching

____

## 0.5.29

. `/` command typed alone echos current set quantum value

. `ch`, `cc`, `bpm`, `mi` and `ma` commands when typd alone, no value typed after, will echos current set value

. Automated loading of a line gig - automated lauching of as much line instances as .line files exist in a directory

. Polyrhythms

. Load *.line file on line's launch, as a parameter

____

## 0.5.20

. Save/load line's instance parameters into/from '.line' file; on loading a '.line' file to a line instance, all parameters will also be applied, say, prompt, notes/cc, ch, values range

. `*` command for length multiplier of playing phrase; e.g. `*2` command on playing phrase "c3 cs3" will become "c3 cs3 c3 cs3". 1/2 to 1/4 note duration

. Save and load a text file with the preferred phrases' list

. Save to preferred phrases queue; load back selected to playing; list saved phrases

. Chords in '.' bar subdivisions; e.g.: c5 .(c4 e4 g4) (c4 d4 g4). Plays *1/2 C 1/4 CMaj 1/4 Csus2*

. Set global amp by %

____

## 0.4.3

. Launch line using arguments, to set n/cc, channel, relabel prompt and input values range

. Set new range of values other than MIDI 0..127. Useful for CC.

. `mi` and `ma` to set min and max of a new range of values

. Set amplitude for each note and chord

. `am`, master amplitude, is a % on notes and chords amplitudes, keeping relative amplitudes

. `sa` and `xa`functions to scramble phrase amplitude

____

## 0.3.4

. Left and right arrow keys enable in-line phrase editing 

. Up and down arrow keys for back and forth phrase history

____


## 0.2.4

. Play chords - polyphony

. Notes cipher

____

## 0.1

. Sends a phrase (music bar) written in a command-line fashion of MIDI values to a MIDI receiver channel or CC.
