# line
#### A tiny command-line midi sequencer for live coding.

+ Sends MIDI messages to a chosen MIDI channel. Default is 1.

+ 1 instrument or 1 CC

<!-- ![line_screenshot](https://github.com/pd3v/line/blob/develop/line0.1.png) -->
<img src="https://github.com/pd3v/line/blob/develop/line0.1.png" style="width:45%;height:45%"/>

### To build and run *line* 

(CMake is necessary)

+ Go to `line`'s directory

+ In command-line, type and Enter:

+ ./build.sh

+ ./line

**Now run your favorite synth, sampler, DAW, or other MIDI receiver!**

## Manual

With `line` running, type:


**To send a MIDI message:**

4 1/4 midi notes

`45 46 47 48` or `a3 as3 b3 c4`

1/4 1/8 1/8 1/4 1/4 notes

`45 .34 35. 48 49` or `a3 .as2 b2. c4 cs4`

'-' for a silent note

`36 .37 38. 41 .- 46.` 

C Major chord + C Major arpeggio. All 1/4

`(c3 e3 g3) g5 e6 c2`

**To set a MIDI channel:**  

channel 2

`ch2`

**To set bpm:**  

120 bpm

`bpm120`

**To set amplitude:**

amplitude 0.5

`am0.5` or `am.5`

**To mute and unmute:**

`m` and `um`

**To reverse:**  

`r`

**To scramble:**  

scrambles notes within each rhythmic part and rhythmic structure

`s`

**To extra scramble:**  

scrambles notes across the pattern and rhythmic structure

`x`

**To load 3 last patterns :**  

`l` or `l1`

`l2`

`l3`

**To switch between notes and cc modes:**  

notes mode

`n` 

cc mode

`cc2`

**To set cc sync lock/unlock:**  

sync or unsync

`i` or `o`

**To rename prompt:** 

from default line> to line~my_synth>, type:

`lbmy_synth`

**To exit:**

`ex`  

**To display commands menu/extended menu:**

`ms` or `me`


 
