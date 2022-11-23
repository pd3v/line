//
//  line - tiny command-line MIDI sequencer for live coding
//
//  Created by @pd3v_
//

#include <iostream>
#include <fstream>  
#include <string>
#include <chrono>
#include <future>
#include <mutex>
#include <vector>
#include <deque>
#include <stdexcept>
#include <sstream>
#include <stdlib.h>
#include <algorithm> 
#include <readline/readline.h>
#include <readline/history.h>
#include <thread>
#include "AudioPlatform_rtaudio.hpp"
#include "externals/rtmidi/RtMidi.h"

#if defined(LINK_PLATFORM_UNIX)
#include <termios.h>
#endif

extern "C" {
  #include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
}

using noteAmpT = std::pair<uint8_t,uint8_t>;
using phraseT = std::vector<std::vector<std::vector<noteAmpT>>>;

const float DEFAULT_BPM = 60.0;
const uint16_t REF_BAR_DUR = 4000; // milliseconds
const char *PROMPT = "line>";
const char *PREPEND_CUSTOM_PROMPT = "_";
const std::string VERSION = "0.4.3";
const char REST_SYMBOL = '-';
const uint8_t REST_VAL = 128;
const uint8_t CTRL_RATE = 100; // milliseconds

const long iterDur = 5; // milliseconds

float amplitude = 127.;
bool muted = false;
std::pair<float,float> range{0,127};
phraseT phrase{};

struct State {
  std::atomic<bool> running;
  ableton::Link link;
  ableton::linkaudio::AudioPlatform audioPlatform;
  
  State(): running(true),link(DEFAULT_BPM),audioPlatform(link){}
};

void disableBufferedInput() {
#if defined(LINK_PLATFORM_UNIX)
  termios t;
  tcgetattr(STDIN_FILENO, &t);
  t.c_lflag &= static_cast<unsigned long>(~ICANON);
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
#endif
}

void enableBufferedInput() {
#if defined(LINK_PLATFORM_UNIX)
  termios t;
  tcgetattr(STDIN_FILENO, &t);
  t.c_lflag |= ICANON;
  tcsetattr(STDIN_FILENO, TCSANOW, &t);
#endif
}

class Parser {
  std::string restSymbol = {REST_SYMBOL};
  lua_State *L = luaL_newstate();
  std::string parserCode;

public:
  Parser()  {
    luaL_openlibs(L);
    const std::string parserFile = "../lineparser.lua";
    std::ostringstream textBuffer;
    std::ifstream input (parserFile.c_str());
    textBuffer << input.rdbuf();
    parserCode = textBuffer.str();
  }

  ~Parser() {lua_close(L);};

  noteAmpT retreiveNoteAmp() {
    lua_next(L,-2);      
    lua_pushnil(L);

    lua_next(L,-2);
    int note = lua_tonumber(L,-1);
    lua_pop(L,1);

    lua_next(L,-2);
    float amp = lua_tonumber(L,-1);
    lua_pop(L,1);
    return {note,amp};
  }

  std::string rescaling(std::string _phrase, std::pair<float,float> _range) {
    auto parseRange = parserCode + " range_min =\"" + std::to_string(_range.first) +  "\" ;range_max=\"" + std::to_string(_range.second) +
     "\" ;rs = table.concat(lpeg.match(rangeG,\"" + _phrase + "\"),\" \")";
    
    if (luaL_dostring(L, parseRange.c_str()) == LUA_OK) {
      lua_getglobal(L,"rs");
      
      if (lua_isstring(L,-1))
        _phrase = lua_tostring(L,-1);
    }
    return _phrase;
  }

  phraseT parsing(std::string _phrase) {
    phraseT v{};
    std::vector<std::vector<noteAmpT>> subv{};
    std::vector<noteAmpT> subsubv{};

    auto p = parserCode + " t = lpeg.match(phraseG, \"" + _phrase + "\")";

    if (luaL_dostring(L, p.c_str()) == LUA_OK) {
      lua_getglobal(L, "t");
      
      // 3D Lua table to c++ vector
      if (lua_istable(L,-1)) {
        lua_pushnil(L);
        lua_gettable(L,-2);
        size_t tableSize = lua_rawlen(L,-2);
        size_t subtableSize,subsubtableSize;
        std::string musicStructType;
        int8_t note, amp;
        
        for (int i=0; i<tableSize; ++i) {
          lua_next(L,-2);      
          lua_pushnil(L);
          subtableSize = lua_rawlen(L,-2);

          lua_next(L,-2);
          musicStructType = lua_tostring(L,-1);
          lua_pop(L,1);    

          lua_next(L,-2);      
          lua_pushnil(L);
          subsubtableSize = lua_rawlen(L,-2);

          if (musicStructType == "n") {
            for (int i=0; i<subsubtableSize; ++i) {
              std::tie(note,amp) = retreiveNoteAmp();

              subsubv.push_back({note,amp});
              subv.push_back(subsubv);
              v.push_back(subv);
              subsubv.clear();
              subv.clear();
              lua_pop(L,2);
            };
          } else if (musicStructType == "s") {
              for (int i=0; i<subsubtableSize; ++i) {
                std::tie(note,amp) = retreiveNoteAmp();

                subsubv.push_back({note,amp});
                subv.push_back(subsubv);  
                subsubv.clear();
                lua_pop(L,2);
              }
              v.push_back(subv);
              subv.clear();
              subsubv.clear();
          } else if (musicStructType == "c") {
              for (int i=0; i<subsubtableSize; ++i) {                
                std::tie(note,amp) = retreiveNoteAmp();

                subsubv.push_back({note,amp});
                lua_pop(L,2);
              }
              subv.push_back(subsubv);  
              v.push_back(subv);
              subsubv.clear();
              subv.clear();
          }
      
          lua_pop(L,2);
          lua_pop(L,2);
        }    
      }
    }
    return v;
  }
};

void displayOptionsMenu(std::string menuVers="") {
  using namespace std;
  cout << "----------------------" << endl;
  cout << "  line " << VERSION << " midi seq  " << endl;
  cout << "----------------------" << endl;
  cout << "..<[n] >    phrase    " << endl;
  cout << "..bpm<[n]>  bpm       " << endl;
  cout << "..ch<[n]>   midi ch   " << endl;
  cout << "..ms        this menu " << endl;
  cout << "..me        extnd menu" << endl;
  cout << "..ex        exit      " << endl;
  cout << "..am<[n]>   amplitude " << endl;
  cout << "..r         reverse   " << endl;
  cout << "..s         scramble  " << endl;
  cout << "..x         xscramble " << endl;
  cout << "..l<[n]>    last patt " << endl;
  
  if (menuVers == "me") {
    cout << "..cc<[n]>   cc ch mode" << endl;
    cout << "..n         notes mode" << endl;
    cout << "..m         mute      " << endl;
    cout << "..um        unmute    " << endl;
    cout << "..i         sync cc   " << endl;
    cout << "..o         async cc  " << endl;
    cout << "..lb<[a|n]> label     " << endl;
    cout << "..sa        s amp     " << endl;
    cout << "..xa        x amp     " << endl;
    cout << "..mi<[n]>   range min" << endl;
    cout << "..ma<[n]> range max" << endl;
  }
  cout << "----------------------" << endl;
  
  if (int r = rand()%5 == 1) cout << "          author:pd3v" << endl;
}

void amp(float _amplitude) {
  amplitude = 127*_amplitude;
}

void mute() {
  muted = true;
}

void unmute() {
  muted = false;
}

inline phraseT map(std::function<void(noteAmpT&)> f) {
  for_each(phrase.begin(),phrase.end(),[&](auto& _subPhrase) {
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      for_each(_subsubPhrase.begin(),_subsubPhrase.end(),[&](auto& _noteAmp) {
        f(_noteAmp);
      });
    });
  });
  return phrase;
}

phraseT reverse(phraseT _phrase) {
  for_each(_phrase.begin(),_phrase.end(),[&](auto& _subPhrase) {
    std::reverse(_subPhrase.begin(),_subPhrase.end());
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      std::reverse(_subsubPhrase.begin(),_subsubPhrase.end());
    });
  });

  return _phrase;
}

phraseT scramble(phraseT _phrase) {
  for_each(_phrase.begin(),_phrase.end(),[&](auto& _subPhrase) {
    std::random_shuffle(_subPhrase.begin(),_subPhrase.end());
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      std::random_shuffle(_subsubPhrase.begin(),_subsubPhrase.end());
    });
  });

  std::random_shuffle(_phrase.begin(),_phrase.end());

  return _phrase;
}

phraseT xscramble(phraseT _phrase) {
  std::vector<noteAmpT> pattValues {};

  for (auto& _subPhrase : _phrase)
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      for_each(_subsubPhrase.begin(),_subsubPhrase.end(),[&](auto& _v) {
        pattValues.emplace_back(_v);
      });
    });

  std::random_shuffle(pattValues.begin(),pattValues.end());
  
  for (auto& _subPhrase : _phrase)
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      for_each(_subsubPhrase.begin(),_subsubPhrase.end(),[&](auto& _v) {
        _v = pattValues.back();
        pattValues.pop_back();
      });
    });
    
  std::random_shuffle(_phrase.begin(),_phrase.end());

  return _phrase;
}

phraseT scrambleAmp(phraseT _phrase) {
  std::vector<float> amps{};
  auto idx = 0;

  for_each(_phrase.begin(),_phrase.end(),[&](auto& _subPhrase) {
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      for_each(_subsubPhrase.begin(),_subsubPhrase.end(),[&](auto& _noteAmp) {
        amps.emplace_back(_noteAmp.second);
      });
    });

    std::random_shuffle(amps.begin(),amps.end());

    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {  
      for_each(_subsubPhrase.begin(),_subsubPhrase.end(),[&](auto& _noteAmp) {    
        _noteAmp.second = amps.at(idx);
      });
      ++idx;  
    });
    amps.clear();
    idx = 0;
  });

  return _phrase;
}

phraseT xscrambleAmp() {
  std::vector<float> amps{};
  auto cnt = 0;

  auto _phrase = map([&](auto& _noteAmp){
     amps.emplace_back(_noteAmp.second);
  });
     
  std::random_shuffle(amps.begin(),amps.end());

  _phrase = map([&](auto& _noteAmp){
    _noteAmp.second = amps.at(cnt);
    ++cnt;
  });

  return _phrase;
}

const uint16_t bpm(const int16_t bpm, const uint16_t barDur) {
  return DEFAULT_BPM/bpm*barDur;
}

long barDur = bpm(DEFAULT_BPM,REF_BAR_DUR);

void bpmLink(double _bpm) {
  barDur = bpm(_bpm,REF_BAR_DUR);
}
 
std::string prompt = PROMPT;

std::tuple<bool,uint8_t,const char*,float,float> lineParamsOnStart(int argc, char **argv) {
  // line args order: notes/cc ch label range_min range_max
  std::tuple<bool,uint8_t,const char*,float,float> lineParams{"n",0,PROMPT,0,127};
  if (argc > 1) {
    std::string _prompt(argv[3]);
    _prompt = PREPEND_CUSTOM_PROMPT+_prompt+">";
    auto notesOrCC = (strcmp(argv[1],"n") == 0 ? true:false);
    
    if (argc == 6) {
      try {
        lineParams = {notesOrCC,std::stoi(argv[2],nullptr),strcpy(new char[_prompt.length()+1],_prompt.c_str()),std::stoi(argv[4],nullptr),std::stoi(argv[5],nullptr)};
      } catch(const std::exception& _err) {
          std::cerr << "Invalid n/cc/min/max." << std::endl;
      }
    } else if (argc == 4) {
      try {
        lineParams = {notesOrCC,std::stoi(argv[2],nullptr),strcpy(new char[_prompt.length()+1],_prompt.c_str()),0,127};
      } catch(const std::exception& _err) {
          std::cerr << "Invalid n/cc." << std::endl;
      }
    }
  }
  return lineParams;
}

int main(int argc, char **argv) {
  Parser parser;
  auto midiOut = RtMidiOut();
  midiOut.openPort(0);
  
  std::vector<uint8_t> noteMessage;
  bool rNotes;
  uint8_t ch=0,ccCh=0,tempCh;
  std::tie(rNotes,tempCh,prompt,range.first,range.second) = lineParamsOnStart(argc,argv);
  if (rNotes) ch = tempCh; else ccCh = tempCh; // ouch!
  bool sync = true;
  std::deque<phraseT> last3Phrases{};
  std::string opt;
  
  std::mutex mtxWait, mtxPhrase;
  std::condition_variable cv;
    
  bool soundingThread = false;
  bool exit = false;
  bool syntaxError = false;

  State state;
  const auto tempo = state.link.captureAppSessionState().tempo();
  auto& engine = state.audioPlatform.mEngine;
  state.link.enable(!state.link.isEnabled());
  state.link.setTempoCallback(bpmLink);

  barDur = bpm(DEFAULT_BPM,REF_BAR_DUR);
    
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  auto sequencer = async(std::launch::async, [&](){
    unsigned long partial = 0;
    phraseT _phrase{};
    uint8_t _ch = ch;
    uint8_t _ccCh = ccCh;
    bool _rNotes = rNotes;

    const std::chrono::microseconds time = state.link.clock().micros();
    const ableton::Link::SessionState sessionState = state.link.captureAppSessionState();
    const bool linkEnabled = state.link.isEnabled();
    const std::size_t numPeers = state.link.numPeers();
    const double quantum = state.audioPlatform.mEngine.quantum();
    const bool startStopSyncOn = state.audioPlatform.mEngine.isStartStopSyncEnabled();

    // waiting for live coder's first phrase 
    std::unique_lock<std::mutex> lckWait(mtxWait);
    cv.wait(lckWait, [&](){return soundingThread == true;});
    std::lock_guard<std::mutex> lckPhrase(mtxPhrase);

    state.link.enable(true);
    
    while (soundingThread) {
      const std::chrono::microseconds time = state.link.clock().micros();
      const ableton::Link::SessionState sessionState = state.link.captureAppSessionState();
      const auto beats = sessionState.beatAtTime(time, quantum);
      const auto phase = sessionState.phaseAtTime(time, quantum);
  
      if (!phrase.empty()) {
        partial = barDur/phrase.size();
        _phrase = phrase;
        _ch = ch;
        _ccCh = ccCh;
        _rNotes = rNotes;

        if (_rNotes) {
          if (phase >= 0.0 && phase < 0.15)  { 
            for (auto& subPhrase : _phrase) {
              for (auto& subsubPhrase : subPhrase) {
                for (auto& notes : subsubPhrase) {
                  noteMessage[0] = 144+_ch;
                  noteMessage[1] = notes.first;
                  noteMessage[2] = ((notes.first == REST_VAL) || muted) ? 0 : notes.second;
                  midiOut.sendMessage(&noteMessage);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<unsigned long>((partial/subPhrase.size())-iterDur)));
                for (auto& notes : subsubPhrase) {  
                  noteMessage[0] = 128+_ch;
                  noteMessage[1] = notes.first;
                  noteMessage[2] = 0;
                  midiOut.sendMessage(&noteMessage);
                }
              }
            }
          }
        } else
          if (sync && phase >= 0.0 && phase < 0.15) {
            for (auto& subPhrase : _phrase) {
              for (auto& subsubPhrase : subPhrase) {
                for (auto& ccValues : subsubPhrase) {
                  noteMessage[0] = 176+_ch;
                  noteMessage[1] = _ccCh;
                  noteMessage[2] = ccValues.first;
                  midiOut.sendMessage(&noteMessage);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<unsigned long>((partial/subPhrase.size())-iterDur)));
              }
            }
          } else if (!sync) {
            for (auto& subPhrase : _phrase) {
              for (auto& subsubPhrase : subPhrase) {
                for (auto& ccValues : subsubPhrase) {
                  noteMessage[0] = 176+_ch;
                  noteMessage[1] = _ccCh;
                  noteMessage[2] = ccValues.first;
                  midiOut.sendMessage(&noteMessage);
                } 
                std::this_thread::sleep_for(std::chrono::milliseconds(CTRL_RATE));
              }
            }
          }
      } else break;
    }
    return "line is off.\n";
  });

  std::cout << "line " << VERSION << " is on." << std::endl;

  while (!exit) {
    opt = readline(prompt.c_str());

    if (!opt.empty()) {
      if (opt == "ms") {
        displayOptionsMenu("");
      } else if (opt == "me") {
        displayOptionsMenu(opt);
      } else if (opt.substr(0,2) == "ch") {
          try {
            ch = std::abs(std::stoi(opt.substr(2,opt.size()-1)));
          }
          catch (...) {
            std::cerr << "Invalid channel." << std::endl; 
          }
      } else if (opt == "n") {
          rNotes = true;
          phrase.clear();
          phrase.push_back({{{REST_VAL,0}}});
      } else if (opt.substr(0,2) == "cc") {
          try {
            ccCh = std::abs(std::stoi(opt.substr(2,opt.size()-1)));
            rNotes = false;
          }
          catch (...) {
            std::cerr << "Invalid cc channel." << std::endl; 
          }
      } else if (opt.substr(0,3) == "bpm") {
          try {
            engine.setTempo(std::abs(std::stoi(opt.substr(3,opt.size()-1))));
          } catch (...) {
            std::cerr << "Invalid bpm." << std::endl;
          }
      } else if (opt == "ex") {
          phrase.clear();
          soundingThread = true;
          cv.notify_one();
          std::cout << sequencer.get();
          exit = true;
      } else if (opt.substr(0,2) == "am") {
          try {
            auto newAmp = std::stof(opt.substr(2,opt.size()-1))*127;
            phrase = map([&](auto& _n){_n.second != 0 ? _n.second *= newAmp : _n.second = newAmp;});
          } catch (...) {
            std::cerr << "Invalid amplitude." << std::endl; 
          }
      } else if (opt == "m") {
          mute();
      } else if (opt == "um") {
          unmute();
      } else if (opt == "r") {    
          phrase = reverse(phrase);
      } else if (opt == "s") {    
          phrase = scramble(phrase);
      } else if (opt == "sa") {    
          phrase = scrambleAmp(phrase);
      } else if (opt == "x") {    
          phrase = xscramble(phrase);
      } else if (opt == "xa") {    
          phrase = xscrambleAmp();
      } else if (opt == "l" || opt == "l1") { 
          if (!last3Phrases.empty()) phrase = last3Phrases.at(0); 
          else std::cout << "Invalid phrase reference." << std::endl; 
      } else if (opt == "l2") {    
          if (last3Phrases.size() >= 2) phrase = last3Phrases.at(1);
          else std::cout << "Invalid phrase reference." << std::endl; 
      } else if (opt == "l3") {    
          if (last3Phrases.size() == 3) phrase = last3Phrases.at(2);
          else std::cout << "Invalid phrase reference." << std::endl; 
      } else if (opt == "i") {    
          sync= true;
      } else if (opt == "o") {    
          sync= false;
      } else if (opt.substr(0,2) == "mi") {    
          try {
            range.first = std::stof(opt.substr(2,opt.size()-1));
          } catch (...) {
            std::cerr << "Invalid range min." << std::endl; 
          }
      } else if (opt.substr(0,2) == "ma") {    
          try {
            range.second = std::stof(opt.substr(2,opt.size()-1));
          } catch (...) {
            std::cerr << "Invalid range max." << std::endl; 
          }    
      } else if (opt.substr(0,2) == "lb") {    
          // prompt = _prompt.substr(0,_prompt.length()-1)+"~"+opt.substr(2,opt.length()-1)+_prompt.substr(_prompt.length()-1,_prompt.length()); formats -> line~<newlable>
          prompt = PROMPT;

          if (opt.length() > 2) {
            std::string _prompt =  PROMPT;
            prompt = PREPEND_CUSTOM_PROMPT+opt.substr(2,opt.length()-1)+_prompt.substr(_prompt.length()-1,_prompt.length());
          }
      } else {
        // it's a phrase, if it's not a command
        phraseT tempPhrase{};
        
        if (range.first != 0 || range.second != 127)
          tempPhrase = parser.parsing(parser.rescaling(opt,range));
        else       
          tempPhrase = parser.parsing(opt);
    
        if (!tempPhrase.empty()) {
          phrase = tempPhrase;
          
          add_history(opt.c_str());
          tempPhrase.clear();

          soundingThread = true;
          cv.notify_one();

          last3Phrases.push_front(phrase);
          if (last3Phrases.size() > 3) last3Phrases.pop_back();
        }
      }
    }
  }

  return 0;
}
