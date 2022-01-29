# line
#### A tiny command-line midi sequencer for live coding.

+ Sends MIDI messages to a chosen MIDI channel. Default is 1.

+ 1 instrument

### To build and run *line*

+ Go to `line`'s directory

+ In command-line, type and Enter:

+ ./build.sh

+ ./line

**Now run your favorite synth, sampler, DAW, or other MIDI receiver!**

## Manual

With `live` running, type:


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

**To exit:**

`e`  

**To display commands menu:**

`m`
 
