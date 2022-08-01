# line
#### A tiny command-line midi sequencer for live coding.

+ Sends MIDI messages to a chosen MIDI channel. Default is 1.

+ 1 instrument or 1 CC

<img src="https://github.com/pd3v/line/blob/develop/line0.1.png" style="width:45%;height:45%"/>

<!-- ### Get *line* binaries -->

<!-- [macos binaries](https://github.com/pd3v/line/actions/runs/1802392423) -->

### To build and run *line* 

#### Install lua language and lpeg (used for parsing) first

##### MacOS, Linux or Windows 

After installing *[luarocks](https://luarocks.org/#quick-start)* package manager

In command-line, type and Enter:

`luarocks install lpeg` 

<!-- ###### On MacOS with *macports*
 
 `port search --name --glob 'lua' ` 

`sudo port install lua @5.3.5` // 5.3.5 or other version

###### On Ubuntu/Debian*

`sudo apt install lua5.3` // 5.3 or other version

###### On Windows* with *Chocolatey*

`choco install lua`

(an alternative for installation would be *Luarocks* package manager)

\* Didn't tried it out -->


#### Build and run *line*

(CMake is necessary)

+ Go to *line*'s directory

In command-line, type and Enter:

`./build.sh`

`./line`

(alternatives for installation would be *macports* or *homebrew* package managers)

**Now run your favorite synth, sampler, DAW, or other MIDI receiver!**

## Manual

With *line* running, type:


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

from default **line>** to **~my_synth>**, type:

`lbmy_synth`

**To exit:**

`ex`  

**To display commands menu/extended menu:**

`ms` or `me`


 
