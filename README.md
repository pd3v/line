# line
#### A tiny command-line midi sequencer for live coding.

+ Sends MIDI messages to a chosen MIDI channel. Default is 1.

+ 1 instrument or 1 CC

<img src="https://github.com/pd3v/line/blob/develop/line 0.4.3 (7 running).png" alt="7 instances of **line** running simultaneously. 1 synth. 6 cc." style="width:60%;height:60%"/>

7 instances of **line** running simultaneously. 1 for notes, 6 cc. (Using tmux to split macOS terminal into 7 terminals)

---

<!-- ### Get *line* binaries -->

<!-- [macos binaries](https://github.com/pd3v/line/actions/runs/1802392423) -->

### To clone this repo with *rtmidi*, *rtaudio* and *link* submodules included

`git clone --recursive https://github.com/pd3v/line.git`

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

Or if you have multiple **.line* file in a directory, type and Enter:

`./linegig.py`

(alternatively try MacOS' *macports* or *homebrew*, or Windows' Chocolatey package managers)

**Now run your favorite synth, sampler, DAW, or other MIDI receiver!**

## Manual

With *line* running, type:


**Send a MIDI message:**

4 1/4 midi notes

`45 46 47 48` or `a3 as3 b3 c4`

1/4 1/8 1/8 1/4 1/4 notes

`45 .34 35. 48 49` or `a3 .as2 b2. c4 cs4`

'-' for a silent note

`36 .37 38. 41 .- 46.` 

C Major chord + C Major arpeggio. All 1/4

`(c3 e3 g3) g5 e6 c2`

d note with 0.5 of amplitude

`d3~.5`

C Minor chord with 0.7 of amplitude

`(c4 eb4 g4)~.7`

*Note: amplitude is 1.0, by default*

**Set a MIDI channel:**  

channel 2

`ch2`

**Set bpm:**  

120 bpm

`bpm120`

**Set phrase duration (polyrhythm):**  

3 beats per phrase

`\3`

*Note: 4 beats (1/4 note), by default*

**Set a range of values (MIDI default):**

minimum is 100

`mi100`

maximum is 1000

`ma1000`

**Set overall relative amplitude:**

50% of previous overall amplitude

`am50`

**Concatenate phrase n times:**

concat phrase 4x 

`*4`

**Mute and unmute:**

`m` and `um`

**Reverse:**  

`r`

**Scramble:**  

scrambles notes within each rhythmic part and rhythmic structure

`s`

**Extra scramble:**  

scrambles notes across the phrases and rhythmic structure

`x`

**Amplitude Scramble and Extra Scramble:**

`sa` and `xa`

**Save phrase to a queue:**

place it on top of the queue; position 0

`sp`

replace phrase in position 3

`sp3`

**Load phrase:**  

load postion 0 phrase; it will play next

`lp0` or `:0`

**List saved phrases**

`l`

**Save queued phrases to *.line* (= txt) file**

Will assume prompt text as file name

`sf`

Save file with *mysynth.line* name

`sfmysynth`

*Note: Will also save line instance parameters (prompt, channel, notes/cc, range of values)*

**Load *.line* file to line**

Load sampler.line

`lfsampler`

*Note: Will apply to line instance saved parameters*

**Switch between notes and cc modes:**  

notes mode

`n` 

cc mode on channel 2

`cc2`

**Set cc sync:**  

In-sync or Out-sync

`i` or `o`

**Relabel prompt:** 

from default **line>** to **_my_synth>**, type:

`lbmy_synth`

**Exit:**

`ex`  

**Display commands menu/extended menu:**

`ms` or `me`

**Set latency:**

subtract 10 ms 

`lt10`

 
