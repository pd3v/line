//
//  line - tiny command-line MIDI sequencer for live coding
//
//  Created by @pd3v_
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <future>
#include <mutex>
#include <vector>
#include <deque>
#include <stdexcept>
#include <stdlib.h>
#include <algorithm>
#include <random>
#include <tuple>
#include <readline/readline.h>
#include <readline/history.h>
#include "externals/link/examples/linkaudio/AudioPlatform_Dummy.hpp"
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
const float REF_QUANTUM = 0.25; // 1/4
const char *PROMPT = "line>";
const char *PREPEND_CUSTOM_PROMPT = "_";
const std::string VERSION = "0.5.26";
const char REST_SYMBOL = '-';
const uint8_t REST_VAL = 128;
const uint8_t CTRL_RATE = 100; // milliseconds
std::string filenameDefault = "line";

const long iterDur = 5; // milliseconds

double bpm = DEFAULT_BPM;
double quantum;
float amplitude = 127.;
bool muted = false;
std::pair<float,float> range{0,127};
phraseT phrase{};
std::string phraseStr;
std::deque<std::string> prefPhrases{};

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
  size_t tableSize,subtableSize,subsubtableSize;
  std::string musicStructType;

  noteAmpT retreiveNoteAmp() const {
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
  
  void musicStructNumIter() {
    lua_next(L,-2);      
    lua_pushnil(L);
    subtableSize = lua_rawlen(L,-2);

    lua_next(L,-2);
    musicStructType = lua_tostring(L,-1);
    lua_pop(L,1);    

    lua_next(L,-2);      
    lua_pushnil(L);
    subsubtableSize = lua_rawlen(L,-2);
  }

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
  
  void reportErrors(lua_State *L, int status) {
    printf("--- %s\n", lua_tostring(L, -1));
    lua_pop(L, 1); // remove error message from Lua's stack
    exit(EXIT_FAILURE);
  }
  
  std::string rescaling(std::string _phrase, std::pair<float,float> _range) {
    auto parseRange = parserCode + " range_min =\"" + std::to_string(_range.first) +  "\" ;range_max=\"" + std::to_string(_range.second) +
     "\" ;rs = table.concat(lpeg.match(rangeG,\"" + _phrase + "\"),\" \")";
    
    if (int luaError = luaL_dostring(L, parseRange.c_str()) == LUA_OK) {
      lua_getglobal(L,"rs");
      
      if (lua_isstring(L,-1))
        _phrase = lua_tostring(L,-1);
    } else
        reportErrors(L, luaError);
    
    return _phrase;
  }

  phraseT parsing(std::string _phrase) {
    phraseT v{};
    std::vector<std::vector<noteAmpT>> subv{};
    std::vector<noteAmpT> subsubv{};

    auto p = parserCode + " t = lpeg.match(phraseG, \"" + _phrase + "\")";

    if (int luaError = luaL_dostring(L, p.c_str()) == LUA_OK) {
      lua_getglobal(L, "t");
      
      // 3D Lua table to c++ vector
      if (lua_istable(L,-1)) {
        lua_pushnil(L);
        lua_gettable(L,-2);
        tableSize = lua_rawlen(L,-2);
        int8_t note, amp;
      
        for (int i=0; i<tableSize; ++i) {
          musicStructNumIter();

          if (musicStructType == "n") {
            for (int i=0; i<subsubtableSize; ++i) {
              std::tie(note,amp) = retreiveNoteAmp();

              subsubv.push_back({note,amp});
              subv.push_back(subsubv);
              v.push_back(subv);
              subsubv.clear();
              subv.clear();
              lua_pop(L,2);
            }
          } else if (musicStructType == "s") {
              size_t _stabsize = lua_rawlen(L,-2);
              
              for (int i=0; i<_stabsize; ++i) {
                musicStructNumIter();

                if (musicStructType == "n") {
                  for (int i=0; i<subsubtableSize; ++i) {
                    std::tie(note,amp) = retreiveNoteAmp();
                    
                    subsubv.push_back({note,amp});
                    subv.push_back(subsubv);  
                    subsubv.clear();
                    lua_pop(L,2);
                  }
                }

                if (musicStructType == "c") {
                  for (int i=0; i<subsubtableSize; ++i) {                
                    std::tie(note,amp) = retreiveNoteAmp(); 

                    subsubv.push_back({note,amp});
                    lua_pop(L,2);
                  }

                  subv.push_back(subsubv);  
                  subsubv.clear();
                }
              
                lua_pop(L,2);
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
    } else
        reportErrors(L, luaError);
    
    return v;
  }
} parser;

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
    cout << "..ma<[n]>   range max" << endl;
    cout << "..*<[n]>    concat phrs" << endl;
    cout << "..sp        save phr @ 0" << endl;
    cout << "..sp<[n]>   save phr @ n" << endl;
    cout << "..:<[n]>    load phr @ n" << endl;
    cout << "..l         list sp phrs" << endl;
    cout << "..sf<name>  save .line file" << endl;
    cout << "..lf<name>  load .line file" << endl;
  }
  cout << "----------------------" << endl;
  
  if (int r = rand()%5 == 1) cout << "          author:pd3v" << endl;
}

void amp(float& amplitude) {
  amplitude = 127*amplitude;
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
  std::reverse(_phrase.begin(),_phrase.end());

  for_each(_phrase.begin(),_phrase.end(),[&](auto& _subPhrase) {
    std::reverse(_subPhrase.begin(),_subPhrase.end());
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      std::reverse(_subsubPhrase.begin(),_subsubPhrase.end());
    });
  });
  
  return _phrase;
}

phraseT scramble(phraseT _phrase) {
  uint16_t seed = std::chrono::system_clock::now().time_since_epoch().count();
  
  for_each(_phrase.begin(),_phrase.end(),[&](auto& _subPhrase) {
    std::shuffle(_subPhrase.begin(),_subPhrase.end(),std::default_random_engine(seed));
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      std::shuffle(_subsubPhrase.begin(),_subsubPhrase.end(),std::default_random_engine(seed));
    });
  });

  std::shuffle(_phrase.begin(),_phrase.end(),std::default_random_engine(seed));

  return _phrase;
}

phraseT xscramble(phraseT _phrase) {
  uint16_t seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::vector<noteAmpT> pattValues {};
  

  for (auto& _subPhrase : _phrase)
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      for_each(_subsubPhrase.begin(),_subsubPhrase.end(),[&](auto& _v) {
        pattValues.emplace_back(_v);
      });
    });

  std::shuffle(pattValues.begin(),pattValues.end(),std::default_random_engine(seed));
  
  for (auto& _subPhrase : _phrase)
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      for_each(_subsubPhrase.begin(),_subsubPhrase.end(),[&](auto& _v) {
        _v = pattValues.back();
        pattValues.pop_back();
      });
    });
    
  std::shuffle(_phrase.begin(),_phrase.end(),std::default_random_engine(seed));

  return _phrase;
}

phraseT scrambleAmp(phraseT _phrase) {
  uint16_t seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::vector<float> amps{};
  auto idx = 0;

  for_each(_phrase.begin(),_phrase.end(),[&](auto& _subPhrase) {
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      for_each(_subsubPhrase.begin(),_subsubPhrase.end(),[&](auto& _noteAmp) {
        amps.emplace_back(_noteAmp.second);
      });
    });

    std::shuffle(amps.begin(),amps.end(),std::default_random_engine(seed));

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
  uint16_t seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::vector<float> amps{};
  auto cnt = 0;

  auto _phrase = map([&](auto& _noteAmp){
     amps.emplace_back(_noteAmp.second);
  });
     
  std::shuffle(amps.begin(),amps.end(),std::default_random_engine(seed));

  _phrase = map([&](auto& _noteAmp){
    _noteAmp.second = amps.at(cnt);
    ++cnt;
  });

  return _phrase;
}

phraseT replicate(phraseT _phrase,uint8_t times) {
  phraseT nTimesPhrase = _phrase;
  
  for(int n = 0;n < times-1;++n) {
    for_each(_phrase.begin(),_phrase.end(),[&](auto& _subPhrase) {
      nTimesPhrase.push_back(_subPhrase);
    });  
  }

  return nTimesPhrase;
}

const uint16_t barToMs(const double _bpm, const uint16_t barDur) {
  return DEFAULT_BPM/bpm*barDur;
}

long barDur = barToMs(DEFAULT_BPM,REF_BAR_DUR);

void bpmLink(double _bpm) {
  bpm = _bpm;
  barDur = barToMs(_bpm,quantum*REF_QUANTUM*REF_BAR_DUR);
}
 
std::string prompt = PROMPT;

std::tuple<bool,uint8_t,const char*,float,float> lineParamsOnStart(int argc, char **argv) {
  // line args order: notes/cc ch label range_min range_max
  std::tuple<bool,uint8_t,const char*,float,float> lineParams{true,0,PROMPT,0,127};

  if (argc == 2) { // Maybe a loadable .line file 
    try {
      std::string filename = argv[1];
      std::ifstream file(filename + ".line");

      if (file.is_open()) {
        std::vector<std::string>params{};
        std::string param;
        std::string _phrase;
        uint8_t countParams = 0;
        
        // [ load line instance params
        while (std::getline(file,param) && countParams < 5) {
          ++countParams;
          params.push_back(param);
        }

        lineParams = {(params.at(0) == "0" ? false : true),std::stoi(params.at(1)),
          (PREPEND_CUSTOM_PROMPT+params.at(2)+">").c_str(),
          std::stof(params.at(3)),
          std::stof(params.at(4))
        };
        // --- ]

        while (std::getline(file,_phrase))
          prefPhrases.push_back(_phrase.c_str());
        
        file.close();

        std::cout << "File loaded.\n";
      } else throw std::runtime_error("");
    } catch(...) {
      std::cout << "Cound't load file.\n" << std::flush;
    }
  } else if (argc > 3) {
    auto notesOrCC = (strcmp(argv[1],"n") == 0 ? true:false);
    std::string _prompt(argv[3]);
    _prompt = PREPEND_CUSTOM_PROMPT+_prompt+">";
    
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
  } else if (argc == 3) {
      auto notesOrCC = (strcmp(argv[1],"n") == 0 ? true:false);
      std::get<0>(lineParams) = notesOrCC;
      std::get<1>(lineParams) = std::stoi(argv[2],nullptr);
  }

  return lineParams;
}

void parsePhrase(std::string& _phrase) {
  phraseT tempPhrase{};
  phraseStr = _phrase;

  if (range.first != 0 || range.second != 127)
    tempPhrase = parser.parsing(parser.rescaling(_phrase,range));
  else       
    tempPhrase = parser.parsing(_phrase);

  if (!tempPhrase.empty()) {
    phrase = tempPhrase;
    add_history(_phrase.c_str());
  }
}

int main(int argc, char **argv) {
  auto midiOut = RtMidiOut();
  midiOut.openPort(0);
  
  std::vector<uint8_t> noteMessage;
  bool rNotes;
  uint8_t ch=0,ccCh=0,tempCh;
  std::tie(rNotes,tempCh,prompt,range.first,range.second) = lineParamsOnStart(argc,argv);

  if (rNotes) ch = tempCh; else ccCh = tempCh; // ouch!
  bool sync = true;
  std::string opt;
  float latency = 0;
  
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

  barDur = barToMs(DEFAULT_BPM,REF_BAR_DUR);
    
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  auto sequencer = async(std::launch::async, [&](){
    double toNextBar = 0;
    unsigned long partial = 0;
    phraseT _phrase{};
    uint8_t _ch = ch;
    uint8_t _ccCh = ccCh;
    bool _rNotes = rNotes;
    long _barDur = barDur;
    auto _quantum = quantum;
      
    const std::chrono::microseconds time = state.link.clock().micros();
    // const ableton::Link::SessionState sessionState = state.link.captureAppSessionState();
    const bool linkEnabled = state.link.isEnabled();
    const std::size_t numPeers = state.link.numPeers();
    quantum = state.audioPlatform.mEngine.quantum();
    const bool startStopSyncOn = state.audioPlatform.mEngine.isStartStopSyncEnabled();
    // const double late = state.audioPlatform.mEngine.outputLatency; // just a reminder of latency info available in engine

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
      toNextBar = ceil(quantum)-(pow(bpm,0.2)*0.01)-(latency*0.001); // :TODO A better aproach. Works on low lantencies
      
      if (!phrase.empty()) {
        _phrase = phrase;
        _ch = ch;
        _ccCh = ccCh;
        _rNotes = rNotes;
        _barDur = barDur;
        
        if (_rNotes) {
          if (phase >= toNextBar || _quantum < quantum) {
            _quantum = quantum;
            for (auto& subPhrase : _phrase) {
              for (auto& subsubPhrase : subPhrase) {
                for (auto& notes : subsubPhrase) {
                  noteMessage[0] = 144+_ch;
                  noteMessage[1] = notes.first;
                  noteMessage[2] = ((notes.first == REST_VAL) || muted) ? 0 : notes.second;
                  midiOut.sendMessage(&noteMessage);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<unsigned long>(_barDur/_phrase.size()/subPhrase.size()-iterDur)));
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
          if (phase >= toNextBar || _quantum < quantum) {
            _quantum = quantum;
            for (auto& subPhrase : _phrase) {
              for (auto& subsubPhrase : subPhrase) {
                for (auto& ccValues : subsubPhrase) {
                  noteMessage[0] = 176+_ch;
                  noteMessage[1] = _ccCh;
                  noteMessage[2] = ccValues.first;
                  midiOut.sendMessage(&noteMessage);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<unsigned long>(_barDur/_phrase.size()/subPhrase.size()-iterDur)));
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
          if (opt.length() > strlen("ch"))
            try {
              ch = std::abs(std::stoi(opt.substr(2,opt.size()-1)));
            }
            catch (...) {
              std::cerr << "Invalid channel.\n";
            }
          else
            std::cout << (int)ch << '\n';  
      } else if (opt == "n") {
          rNotes = true;
          phrase.clear();
          phrase.push_back({{{REST_VAL,0}}});
      } else if (opt.substr(0,2) == "cc") {
          if (opt.length() > strlen("cc"))
            try {
              ccCh = std::abs(std::stoi(opt.substr(2,opt.size()-1)));
              rNotes = false;
            }
            catch (...) {
              std::cerr << "Invalid cc channel." << std::endl; 
            }
          else
            std::cout << (int)ccCh << '\n';  
      } else if (opt.substr(0,3) == "bpm") {
          if (opt.length() > strlen("bpm"))
            try {
              bpm = static_cast<double>(std::abs(std::stoi(opt.substr(3,opt.size()-1))));
              engine.setTempo(bpm);
            } catch (...) {
              std::cerr << "Invalid bpm." << std::endl;
            }
          else
            std::cout << bpm << '\n';    
      } else if (opt.substr(0,1) == "/") {
          if (opt.length() > strlen("/"))
            try {
              quantum = static_cast<double>(std::stof(opt.substr(1,opt.size()-1)));
              barDur = barToMs(bpm,quantum*REF_QUANTUM*REF_BAR_DUR);
            } catch (...) {
              std::cerr << "Invalid phrase duration." << std::endl; 
            }
          else
            std::cout << quantum << '\n';        
      } else if (opt == "ex") {
          phrase.clear();
          soundingThread = true;
          cv.notify_one();
          std::cout << sequencer.get();
          exit = true;
      } else if (opt.substr(0,2) == "am") {
          try {
            auto newAmp = std::stof(opt.substr(2,opt.size()-1));
            phrase = map([&](auto& _n){_n.second != 0 ? _n.second *= (newAmp*0.01) : _n.second = newAmp;});
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
       } else if (opt == "sp") {
          prefPhrases.push_front(phraseStr);
          if (prefPhrases.size() > 20) prefPhrases.pop_back();
      } else if (opt.substr(0,2) == "sp") {
          try {
            prefPhrases.at(std::stof(opt.substr(2,opt.size()-1))) = phraseStr;
          } catch (...) {
            std::cerr << "Invalid phrase slot." << std::endl; 
          }   
      } else if (opt.substr(0,2) == "lp" || opt.substr(0,1) == ":") {
          try {
            auto cmdLen = opt.substr(0,1) == ":" ? 1 : 2;
            parsePhrase(prefPhrases.at(std::stof(opt.substr(cmdLen,opt.size()-1))));

            soundingThread = true;
            cv.notify_one();
          } catch (...) {
            std::cerr << "Invalid phrase slot." << std::endl; 
          }
      } else if (opt == "l") {
        uint8_t i = 0;
        for_each(prefPhrases.begin(),prefPhrases.end(),[&](std::string _p) {
          std::cout << "[" << (int)i++ << "] " << _p << std::endl;
        });  
      } else if (opt == "i") {    
          sync= true;
      } else if (opt == "o") {    
          sync= false;
      } else if (opt.substr(0,2) == "mi") {
          if (opt.length() > strlen("mi"))    
            try {
              range.first = std::stof(opt.substr(2,opt.size()-1));
            } catch (...) {
              std::cerr << "Invalid range min." << std::endl; 
            }
          else
            std::cout << range.first << '\n';  
      } else if (opt.substr(0,2) == "ma") {    
          if (opt.length() > strlen("ma"))
            try {
              range.second = std::stof(opt.substr(2,opt.size()-1));
            } catch (...) {
              std::cerr << "Invalid range max." << std::endl; 
            }
          else
            std::cout << range.second << '\n';
      } else if (opt.substr(0,1) == "*") {    
          try {
            auto times = static_cast<int>(std::stof(opt.substr(1,opt.size()-1)));

            if (times == 0)
              throw std::runtime_error(""); 
          
            phrase = replicate(phrase,times);
          } catch (...) {
            std::cerr << "Invalid phrase replication." << std::endl; 
          }        
      } else if (opt.substr(0,2) == "lb") {    
          // prompt = _prompt.substr(0,_prompt.length()-1)+"~"+opt.substr(2,opt.length()-1)+_prompt.substr(_prompt.length()-1,_prompt.length()); formats -> line~<newlable>
          prompt = PROMPT;

          if (opt.length() > 2) {
            std::string _prompt =  PROMPT;
            prompt = PREPEND_CUSTOM_PROMPT+opt.substr(2,opt.length()-1)+_prompt.substr(_prompt.length()-1,_prompt.length());
            filenameDefault = opt.substr(2,opt.length()-1);
          }
      } else if (opt.substr(0,2) == "sf") {
          try {
            auto filename = opt.substr(2,opt.size()-1);

            if (filename.empty()) filename = (prompt != PROMPT ? prompt.substr(1,prompt.length()-2) : filenameDefault);
            std::ofstream outfile (filename + ".line");

            // line instance params
            outfile << std::to_string(rNotes) << '\n' << (rNotes ? std::to_string((int)ch) : std::to_string((int)ccCh)) << '\n'
             << filename << '\n' << std::to_string(range.first) << '\n' << std::to_string(range.second) << "\n\n";

            for_each(prefPhrases.begin(),prefPhrases.end(),[&](std::string _phraseStr){outfile << _phraseStr << "\n";});
            outfile.close();

            std::cout << "File " + filename + " saved.\n";
          } catch (...) {
            std::cerr << "Invalid filename." << std::endl; 
          }
      } else if (opt.substr(0,2) == "lf") {
          try {
            auto filename = opt.substr(2,opt.size()-1);
            if (filename.empty()) filename = filenameDefault;
            std::ifstream file(filename + ".line");

            if (file.is_open()) {
              std::vector<std::string>params{};
              std::string param;
              std::string _phrase;
              uint8_t countParams = 0;

              prefPhrases.clear();
              
              // [ load line instance params
              while (std::getline(file,param) && countParams < 5) {
                ++countParams;
                params.push_back(param);
              }

              std::istringstream(params.at(0)) >> rNotes;
              (rNotes) ? ch = std::stoi(params.at(1)) : ccCh = std::stoi(params.at(1));
              prompt = PREPEND_CUSTOM_PROMPT+params.at(2)+">";
              range.first = std::stof(params.at(3));
              range.second = std::stof(params.at(4));
              // --- ]

              while (std::getline(file,_phrase))
                prefPhrases.push_back(_phrase.c_str());
              
              file.close();

              std::cout << "File loaded.\n";
            } else throw std::runtime_error("");
          } catch (...) {
            std::cerr << "Couldn't load file." << std::endl; 
          }
      } else if (opt.substr(0,2) == "lt") {    
          try {
            latency = std::stof(opt.substr(2,opt.size()-1));
          } catch (...) {
            std::cerr << "Invalid latency." << std::endl; 
          }
      } else {
        // it's a phrase, if it's not a command
        parsePhrase(opt);

        soundingThread = true;
        cv.notify_one();
      }
    }
  }
  
  return 0;
}
