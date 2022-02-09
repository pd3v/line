# line
#### A tiny command-line midi sequencer for live coding.

+ Sends MIDI messages to a chosen MIDI channel. Default is 1.

+ 1 instrument

![line_screenshot](https://github.com/pd3v/line/blob/develop/line0.1.png)

### Get *line* binaries

[macos binaries](https://github.com/pd3v/line/actions/runs/1802392423)

### To build and run *line*

+ Go to `line`'s directory

+ In command-line, type and Enter:

+ ./build.sh

+ ./line

**Now run your favorite synth, sampler, DAW, or other MIDI receiver!**

## Manual

With `line` running, type:


**To send a MIDI message:**

// 4 1/4 midi notes

`45 46 47 48`

// 1/4 1/8 1/8 1/4 1/4 notes

`45 .34 35. 48 49` 

// 0 for a silent note

`36 .37 38. 41 .0 46.` 

**To set a MIDI channel:**  

channel 2

`c2`

**To set bpm:**  

120 bpm

`b120`

**To set overall playing pattern amplitude:**

amplitude 0.5

`a0.5`

**To mute and umute:**

`m` and `um`

**To reverse:**  

`r`

**To scramble:**  

// scrambles notes within each rhythmic part and rhytmic struct

`s`

**To extra scramble :**  

// scrambles notes across the pattern and rhythmic struct

`x`

**To exit:**

`e`  

**To display commands menu:**

`m`
 
