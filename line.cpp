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
#include <stdexcept>
#include <stdlib.h>
#include <algorithm>
#include <random>
#include <tuple>
#include <atomic>
#include <regex>
#include <readline/readline.h>
#include <readline/history.h>
#include "externals/link/examples/linkaudio/AudioPlatform_Dummy.hpp"
#include "externals/rtmidi/RtMidi.h"
// #include "RtMidi.h"

#if (__APPLE__)
  #define __MACOSX_CORE__
#endif
#if (__linux__)
  #define __LINUX_ALSA__
#endif

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
const uint64_t REF_BAR_DUR = 4000000; // microseconds
const float REF_QUANTUM = 4; // 1 bar
const char *PROMPT = "line>";
const std::string VERSION = "0.6.2";
const char REST_SYMBOL = '-';
const uint8_t REST_VAL = 128;
const uint64_t CTRL_RATE = 100000; // microseconds
const long ITER_DUR = 4000; // microseconds

std::string filenameDefault = "line";

double bpm = DEFAULT_BPM;
double quantum = REF_QUANTUM;
float amplitude = 127.;
uint8_t ch=0, ccCh=0;
bool muted = false;
std::pair<float,float> range{0,127};
phraseT phrase{};
std::string phraseStr;
std::deque<std::string> prefPhrases{};
double toNextBar = 0;
bool commandHasPhrase = false; // command(s) is added to history only if it has a phrase in it. Arggg!

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

    // std::cout << "parserCode->" << parserCode << std::endl;
  }
  ~Parser() {lua_close(L);};

  void reportLuaErrorsOnExit(lua_State *L, int status) {
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
        reportLuaErrorsOnExit(L, luaError);

    return _phrase;
  }

  phraseT parsing(std::string _phrase) {
    phraseT v{};
    std::vector<std::vector<noteAmpT>> subv{};
    std::vector<noteAmpT> subsubv{};

    // auto p = parserCode + " phrs_matching = lpeg.match(phraseG, \"" + _phrase +
    // "\"); t = phrs_matching and phrs_matching or NO_MATCH";

    auto p = parserCode;

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
          } else if (musicStructType == "e") {
                lua_pop(L,2);
                v.clear();
          }

          lua_pop(L,2);
          lua_pop(L,2);
        }
      }
    } else
        reportLuaErrorsOnExit(L, luaError);

    return v;
  }
} parser;

struct MidiEvent {
  const std::vector<noteAmpT> notes;
  const float startTime;
  const float endTime;
  bool isPlaying = false;
  const float timeOffset = 0.02;

  void notesPlayStop(const float& _currentTime, std::vector<uint8_t>& _message, RtMidiOut& _midiOut) {
    if (_currentTime >= (startTime == 0 ? 0 : (startTime - timeOffset)) && _currentTime <= (startTime + timeOffset) && !isPlaying) {
      for (auto& note : notes) {
        _message[0] = 0x90 + ch;
        _message[1] = note.first;
        _message[2] = ((note.first == REST_VAL) || muted) ? 0 : note.second;
        _midiOut.sendMessage(&_message);
      }  
      isPlaying = true;
    } else if (_currentTime >= (endTime - timeOffset) && _currentTime <= (endTime + timeOffset) && isPlaying) {
        for (auto& note : notes) {
          _message[0] = 0x80 + ch;
          _message[1] = note.first;
          _message[2] = 0;
          _midiOut.sendMessage(&_message);
        }
        isPlaying = false;
    }
  }

  void ccPlayStop(const float& _currentTime, std::vector<uint8_t>& _message, RtMidiOut& _midiOut) {
    if (_currentTime >= (startTime == 0 ? 0 : (startTime - timeOffset)) && _currentTime <= (startTime + timeOffset))
      for (auto& cc : notes) {
        _message[0] = 0xB0 + ch;
        _message[1] = ccCh;
        _message[2] = ((cc.first == REST_VAL) || muted) ? 0 : cc.first;
        _midiOut.sendMessage(&_message);
      }
  }

  void stop(std::vector<uint8_t>& _message, RtMidiOut& _midiOut) {
    for (auto& note : notes) {
      _message[0] = 0x80 + ch;
      _message[1] = note.first;
      _message[2] = 0;
      _midiOut.sendMessage(&_message);
    }
  }
};

std::vector<MidiEvent>* midiEvents = new std::vector<MidiEvent>();

void displayCommandsList(std::string listVers="") {
  using namespace std;
  cout << "----------------------" << endl;
  cout << "  line " << VERSION << " midi seq  " << endl;
  cout << "----------------------" << endl;
  cout << "..<[n] >    phrase    " << endl;
  cout << "..bpm<[n]>  bpm       " << endl;
  cout << "..ch<[n]>   midi ch   " << endl;
  cout << "..ls        this list " << endl;
  cout << "..le        extnd list" << endl;
  cout << "..ex        exit      " << endl;
  cout << "..am<[n]>   amplitude " << endl;
  cout << "..r         reverse   " << endl;
  cout << "..s         scramble  " << endl;
  cout << "..x         xscramble " << endl;

  if (listVers == "le") {
    cout << "..cc<[n]>   cc ch mode" << endl;
    cout << "..n         notes mode" << endl;
    cout << "..m         mute      " << endl;
    cout << "..um        unmute    " << endl;
    // cout << "..i         sync cc   " << endl;
    // cout << "..o         async cc  " << endl;
    cout << "..lb<[a|n]> label     " << endl;
    cout << "..sa        s amp     " << endl;
    cout << "..xa        x amp     " << endl;
    cout << "..ga        gen amps" << endl;
    cout << "..rl<[n]>   rotate left" << endl;
    cout << "..rr<[n]>   rotate right" << endl;
    cout << "..mi<[n]>   range min" << endl;
    cout << "..ma<[n]>   range max" << endl;
    cout << "..*<[n]>    concat phr" << endl;
    cout << "../<[n]>    prolong phr" << endl;
    cout << "..sp        save phr @ 0" << endl;
    cout << "..sp<[n]>   save phr @ n" << endl;
    cout << "..:<[n]>    load phr @ n" << endl;
    cout << "..l         list sp phrs" << endl;
    cout << "..sf<name>  save file" << endl;
    cout << "..lf<name>  load file" << endl;
  }
  cout << "----------------------" << endl;

  if (/*int r = */rand()%5 == 1) cout << "          author:pd3v" << endl;
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

inline phraseT map(phraseT _phrase, std::function<void(noteAmpT&)> f) {
  for_each(_phrase.begin(),_phrase.end(),[&](auto& _subPhrase) {
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      for_each(_subsubPhrase.begin(),_subsubPhrase.end(),[&](auto& _noteAmp) {
        f(_noteAmp);
      });
    });
  });
  return _phrase;
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

phraseT genAmp(phraseT _phrase) {
  _phrase = map([&](auto& noteAmp){
    noteAmp.second = rand() % 92 + 10;
  });

  return _phrase;
}

phraseT rotateLeft(phraseT _phrase, uint8_t jump=1) {
  std::rotate(_phrase.begin(), _phrase.begin() + jump, _phrase.end());
  
  return _phrase;
}

phraseT rotateRight(phraseT _phrase, int jump=1) {
  std::rotate(_phrase.rbegin(), _phrase.rbegin() + jump, _phrase.rend());
  
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

const uint64_t barToMs(const double _bpm, const uint64_t _barDur) {
  return DEFAULT_BPM / _bpm * _barDur;
}

uint64_t barDur = barToMs(DEFAULT_BPM, REF_BAR_DUR);

void bpmLink(double _bpm) {
  bpm = _bpm;
  barDur = barToMs(_bpm, REF_BAR_DUR);
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

        auto labelAsPrompt = (params.at(2) + ">");
        lineParams = std::make_tuple((params.at(0) == "0" ? false : true),std::stoi(params.at(1)),
          std::move(labelAsPrompt.c_str()),
          std::stof(params.at(3)),
          std::stof(params.at(4))
        );
        // --- ]
        
        while (std::getline(file,_phrase))
          prefPhrases.push_back(_phrase.c_str());
        
        std::cout << "File loaded.\n";
      } else throw std::runtime_error("");
    } catch(...) {
      std::cout << "Cound't load file.\n" << std::flush;
    }
  } else if (argc > 3) {
    auto notesOrCC = (strcmp(argv[1],"n") == 0 ? true:false);
    std::string _prompt(argv[3]);
    _prompt = _prompt+">";

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

std::deque<std::string> splitCommands(const std::string& composedCmd) {
  std::stringstream ssCmd(composedCmd);
  std::string eachCmd;
  std::deque<std::string> sequencedCmds;

  while(std::getline(ssCmd, eachCmd, '<'))
   sequencedCmds.emplace_back(std::regex_replace(std::regex_replace(eachCmd, std::regex("^ +"), ""), std::regex(" +$"), ""));

  return sequencedCmds;
}

bool parsePhrase(std::string& _phrase) {
  phraseT tempPhrase{};
  phraseStr = _phrase;

  // splitCommands(_phrase);

  if (range.first != 0 || range.second != 127)
    tempPhrase = parser.parsing(parser.rescaling(_phrase,range));
  else
    tempPhrase = parser.parsing(_phrase);

  if (!tempPhrase.empty())
    phrase = std::move(tempPhrase);
  else
    return false;

  return true;
}

inline float barEndTimeRef() {
  return ceil(quantum) - (pow(bpm,0.2) * 0.005);
}

/* 
void danglingMidiEvents(std::vector<uint8_t>& _message, RtMidiOut& _midiOut) {
  for (int i = 0; i < 128; ++i) {
    _message[0] = 0x80 + ch;
    _message[1] = i;
    _message[2] = 0;
    _midiOut.sendMessage(&_message);
  }
}
*/

void timeStamping(phraseT _phrase) {  
  std::vector<MidiEvent>* _midiEvents = new std::vector<MidiEvent>();
  std::vector<noteAmpT>_notes;
  float incr = 0;
  float _phase = 0;
  
  for (auto& subPhrase : _phrase) {
    for (auto& subsubPhrase : subPhrase) {      
      incr = quantum * (1.0 / _phrase.size() / subPhrase.size());
      
      for (auto& notes : subsubPhrase)
        _notes.emplace_back(notes);
  
      _midiEvents->emplace_back((MidiEvent){_notes, _phase, static_cast<float>(_phase + incr - 0.001)});
      _notes.clear();
      _phase += incr;
    }
  }

  midiEvents = std::move(_midiEvents);
}

int main(int argc, char **argv) {
  auto midiOut = RtMidiOut();
  midiOut.openPort(0);

  std::vector<uint8_t> noteMessage;
  bool rNotes;
  uint8_t tempCh;
  std::tie(rNotes,tempCh,prompt,range.first,range.second) = lineParamsOnStart(argc,argv);

  if (rNotes) ch = tempCh; else ccCh = tempCh; // ouch!
  bool sync = true;
  std::string opt;
  float latency = 0;

  std::mutex mtxWait, mtxPhrase;
  std::condition_variable cv;

  bool isSoundingThread = false;
  bool exit = false;

  State state;
  const auto tempo = state.link.captureAppSessionState().tempo();
  auto& engine = state.audioPlatform.mEngine;
  state.link.enable(!state.link.isEnabled());
  state.link.setTempoCallback(bpmLink);

  noteMessage.push_back(0x80);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  auto sequencer = async(std::launch::async, [&](){
    phraseT _phrase{};
    std::vector<MidiEvent>* _midiEvents = new std::vector<MidiEvent>();

    const bool linkEnabled = state.link.isEnabled();
    const std::size_t numPeers = state.link.numPeers();
    quantum = state.audioPlatform.mEngine.quantum();
    const bool startStopSyncOn = state.audioPlatform.mEngine.isStartStopSyncEnabled();
    // const double late = state.audioPlatform.mEngine.outputLatency; // just a reminder of latency info available in engine

    toNextBar = barEndTimeRef();

    // waiting for live coder's first phrase
    std::unique_lock<std::mutex> lckWait(mtxWait);
    cv.wait(lckWait, [&](){return isSoundingThread;});
    std::lock_guard<std::mutex> lckPhrase(mtxPhrase);

    state.link.enable(true);

    while (isSoundingThread) {
      const std::chrono::microseconds time = state.link.clock().micros();
      const ableton::Link::SessionState sessionState = state.link.captureAppSessionState();
      // const auto beats = sessionState.beatAtTime(time, quantum);

      auto phase = sessionState.phaseAtTime(time, quantum);

      if(!phrase.empty()) {
        if (phase >= toNextBar && midiEvents)
          _midiEvents = std::move(midiEvents);

        if (rNotes)
          for_each(_midiEvents->begin(), _midiEvents->end(), [&](MidiEvent& _midiEvent){_midiEvent.notesPlayStop(phase, noteMessage, midiOut);});
        else
          for_each(_midiEvents->begin(), _midiEvents->end(), [&](MidiEvent& _midiEvent){_midiEvent.ccPlayStop(phase, noteMessage, midiOut);});

        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<long long>(4000)));
      } else break;
    }
    for_each(_midiEvents->begin(), _midiEvents->end(), [&](MidiEvent& _midiEvent){_midiEvent.stop(noteMessage, midiOut);});

    delete _midiEvents;
    _midiEvents = nullptr;

    return "line is off.\n";
  });

  std::cout << "line " << VERSION << " is on." << std::endl << "Type \"ls\" for commands short list; \"le\" for extended." << std::endl;


  std::function<void(std::string&)> parseCommands = [&](auto& _opt){
    if (!_opt.empty()) {
      if (_opt == "ls") {
        displayCommandsList("");
      } else if (_opt == "le") {
        displayCommandsList(_opt);
      } else if (_opt.substr(0,2) == "ch") {
          if (_opt.length() > strlen("ch"))
            try {
              ch = std::abs(std::stoi(_opt.substr(2,_opt.size()-1)));
              timeStamping(phrase);
            }
            catch (...) {
              std::cerr << "Invalid channel.\n";
            }
          else
            std::cout << (int)ch << '\n';
      } else if (_opt == "n") {
          rNotes = true;
          phrase.clear();
          phrase.push_back({{{REST_VAL,0}}});
      } else if (_opt.substr(0,2) == "cc") {
          if (_opt.length() > strlen("cc"))
            try {
              ccCh = std::abs(std::stoi(_opt.substr(2,_opt.size()-1)));
              rNotes = false;
            }
            catch (...) {
              std::cerr << "Invalid cc channel." << std::endl;
            }
          else
            std::cout << (int)ccCh << '\n';
      } else if (_opt.substr(0,3) == "bpm") {
          if (_opt.length() > strlen("bpm"))
            try {
              bpm = static_cast<double>(std::abs(std::stoi(_opt.substr(3,_opt.size()-1))));
              engine.setTempo(bpm);
              quantum = REF_QUANTUM;
              barDur = barToMs(bpm, REF_BAR_DUR);
              toNextBar = barEndTimeRef();
            } catch (...) {
              std::cerr << "Invalid bpm." << std::endl;
            }
          else
            std::cout << bpm << '\n';
      } else if (_opt.substr(0,1) == "/") {
          if (_opt.length() > strlen("/"))
            try {
              quantum = static_cast<double>(std::stof(_opt.substr(1,_opt.size()-1))) * REF_QUANTUM;
              barDur = barToMs(bpm, quantum / REF_QUANTUM * REF_BAR_DUR);
              toNextBar = barEndTimeRef();
              timeStamping(phrase);
            } catch (...) { 
                std::cerr << "Invalid phrase duration." << std::endl;
            }
          else
            std::cout << quantum / REF_QUANTUM << '\n';
      } else if (_opt == "ex") {
          phrase.clear();
          isSoundingThread = true;
          cv.notify_one();
          std::cout << sequencer.get();
          exit = true;
      } else if (_opt.substr(0,2) == "am") {
          try {
            auto newAmp = std::stof(_opt.substr(2,_opt.size()-1));
            phrase = map([&](auto& _n){_n.second != 0 ? _n.second *= (newAmp*0.01) : _n.second = newAmp * 1.27;});
            timeStamping(phrase);
          } catch (...) {
            std::cerr << "Invalid amplitude." << std::endl;
          }
      } else if (_opt == "m") {
          mute();
      } else if (_opt == "um") {
          unmute();
      } else if (_opt == "r") {
          phrase = reverse(phrase);
          timeStamping(phrase);
      } else if (_opt == "s") {
          phrase = scramble(phrase);
          timeStamping(phrase);
      } else if (_opt == "sa") {
          phrase = scrambleAmp(phrase);
          timeStamping(phrase);
      } else if (_opt == "x") {
          phrase = xscramble(phrase);
          timeStamping(phrase);
      } else if (_opt == "xa") {
          phrase = xscrambleAmp();
          timeStamping(phrase);
      } else if (_opt == "ga") {
          phrase = genAmp(phrase);
          timeStamping(phrase);
      } else if (_opt.substr(0,2) == "rl") {
          if (_opt.length() > strlen("rl")) {
            try {
              auto jump = std::abs(std::stoi(_opt.substr(2,_opt.size()-1)));
              phrase = rotateLeft(phrase, jump);
            } catch (...) {
                std::cerr << "Invalid rotate jump." << std::endl;
            }
          } else 
              phrase = rotateLeft(phrase);
          timeStamping(phrase);
      } else if (_opt.substr(0,2) == "rr") {
          if (_opt.length() > strlen("rr")) {
            try {
              auto jump = std::abs(std::stoi(_opt.substr(2,_opt.size()-1)));
              phrase = rotateRight(phrase, jump);
            } catch (...) {
                std::cerr << "Invalid rotate jump." << std::endl;
            }
          } else 
              phrase = rotateLeft(phrase);
          timeStamping(phrase);
      } else if (_opt == "sp") {
          prefPhrases.push_front(phraseStr);
          if (prefPhrases.size() > 20) prefPhrases.pop_back();
      } else if (_opt.substr(0,2) == "sp") {
          try {
            prefPhrases.at(std::stof(_opt.substr(2,_opt.size()-1))) = phraseStr;
          } catch (...) {
            std::cerr << "Invalid phrase slot." << std::endl;
          }
      } else if (_opt.substr(0,2) == "lp" || _opt.substr(0,1) == ":") {
          try {
            auto cmdLen = _opt.substr(0,1) == ":" ? 1 : 2;
            auto sequencedCommands = splitCommands(prefPhrases.at(std::stof(_opt.substr(cmdLen,_opt.size()-1))));

            for(; !sequencedCommands.empty(); sequencedCommands.pop_front())
              parseCommands(sequencedCommands.front());

            add_history(prefPhrases.at(std::stof(_opt.substr(cmdLen,_opt.size()-1))).c_str());
            
            isSoundingThread = true;
            cv.notify_one();
          } catch (...) {
            std::cerr << "Invalid phrase slot." << std::endl;
          }
      } else if (_opt == "l") {
        uint8_t i = 0;
        for_each(prefPhrases.begin(),prefPhrases.end(),[&](std::string _p) {
          std::cout << "[" << (int)i++ << "] " << _p << std::endl;
        });
      } else if (_opt == "i") {
          sync = true;
      } else if (_opt == "o") {
          sync = false;
      } else if (_opt.substr(0,2) == "mi") {
          if (_opt.length() > strlen("mi"))
            try {
              range.first = std::stof(_opt.substr(2,_opt.size()-1));
            } catch (...) {
              std::cerr << "Invalid range min." << std::endl;
            }
          else
            std::cout << range.first << '\n';
      } else if (_opt.substr(0,2) == "ma") {
          if (_opt.length() > strlen("ma"))
            try {
              range.second = std::stof(_opt.substr(2,_opt.size()-1));
            } catch (...) {
              std::cerr << "Invalid range max." << std::endl;
            }
          else
            std::cout << range.second << '\n';
      } else if (_opt.substr(0,1) == "*") {
          try {
            auto times = static_cast<int>(std::stof(_opt.substr(1,_opt.size()-1)));

            if (times == 0)
              throw std::runtime_error("");

            phrase = replicate(phrase,times);
            timeStamping(phrase);
          } catch (...) {
            std::cerr << "Invalid phrase replication." << std::endl;
          }
      } else if (_opt.substr(0,2) == "lb") {
          // prompt = _prompt.substr(0,_prompt.length()-1)+"~"+_opt.substr(2,_opt.length()-1)+_prompt.substr(_prompt.length()-1,_prompt.length()); formats -> line~<newlable>
          prompt = PROMPT;

          if (_opt.length() > 2) {
            std::string _prompt =  PROMPT;
            prompt = _opt.substr(2,_opt.length()-1)+_prompt.substr(_prompt.length()-1,_prompt.length());
            filenameDefault = _opt.substr(2,_opt.length()-1);
          }
      } else if (_opt.substr(0,2) == "sf") {
          try {
            auto filename = _opt.substr(2,_opt.size()-1);

            if (filename.empty()) filename = (prompt != PROMPT ? prompt.substr(0,prompt.length()-1) : filenameDefault);
            std::ofstream outfile (filename + ".line");

            // line instance params
            outfile << std::to_string(rNotes) << '\n' << (rNotes ? std::to_string((int)ch) : std::to_string((int)ccCh)) << '\n'
            << filename << '\n' << std::to_string(range.first) << '\n' << std::to_string(range.second) << "\n\n";

            for_each(prefPhrases.begin(),prefPhrases.end(),[&](std::string _phraseStr){outfile << _phraseStr << "\n";});

            std::cout << "File " + filename + " saved.\n";
          } catch (...) {
            std::cerr << "Invalid filename." << std::endl;
          }
      } else if (_opt.substr(0,2) == "lf") {
          try {
            auto filename = _opt.substr(2,_opt.size()-1);
            std::cout << filename << '\n';
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
              prompt = params.at(2)+">";
              range.first = std::stof(params.at(3));
              range.second = std::stof(params.at(4));
              // --- ]

              while (std::getline(file,_phrase))
                prefPhrases.push_back(_phrase.c_str());

              std::cout << "File loaded.\n";
            } else throw std::runtime_error("");
          } catch (...) {
            std::cerr << "Couldn't load file." << std::endl;
          }
      } else if (_opt.substr(0,2) == "lt") {
          try {
            latency = std::stof(_opt.substr(2,_opt.size()-1));
          } catch (...) {
            std::cerr << "Invalid latency." << std::endl;
          }
      } else {
        // it's a phrase, if it's not a command
        if (!parsePhrase(_opt))
          std::cout << "Unknown symbol.\n";
        else
          timeStamping(phrase);  
        
        commandHasPhrase = commandHasPhrase || true;

        isSoundingThread = true;
        cv.notify_one();
      }
    }
  };

  while (!exit) {
    opt = readline(prompt.c_str());
    auto sequencedCommands = splitCommands(opt);

    for(; !sequencedCommands.empty(); sequencedCommands.pop_front())
      parseCommands(sequencedCommands.front());

    if (commandHasPhrase) 
      add_history(opt.c_str());

    commandHasPhrase = false;
  }

  return 0;
} 
