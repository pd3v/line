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
const char *PROMPT = "line>";
const char *PREPEND_CUSTOM_PROMPT = "_";
const std::string VERSION = "0.4.16";
const char REST_SYMBOL = '-';
const uint8_t REST_VAL = 128;
const uint8_t CTRL_RATE = 100; // milliseconds

const long iterDur = 5; // milliseconds
float amplitude = 127;
bool muted = false;
std::pair<float,float> range{0,127};
phraseT phrase{};
std::string phraseStr;
std::string filenameDefault = "line";

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
    }
    return v;
  }
} parser;

const uint16_t bpm(const int16_t bpm, const uint16_t barDur) {
  return DEFAULT_BPM/bpm*barDur;
}

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
    cout << "..sp        save phr @ 0" << endl;
    cout << "..sp<[n]>   save phr @ n" << endl;
    cout << "..lp<[n]>   load phr @ n" << endl;
    cout << "..l         list sp phrs" << endl;
    cout << "..sf<name>  save .line file" << endl;
    cout << "..lf<name>  load .line file" << endl;
  }
  cout << "----------------------" << endl;
  
  if (rand()%10+1 == 1) cout << "          author:pd3v" << endl;
}

void amp(float &amplitude) {
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
 
std::string prompt = PROMPT;

std::tuple<bool,uint8_t,const char*,float,float> lineParamsOnStart(int argc, char **argv) {
  // line args order: notes/cc ch label range_min range_max
  std::tuple<bool,uint8_t,const char*,float,float> lineParams{true,0,PROMPT,0,127};

  if (argc > 3) {
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

  const uint16_t refBarDur = 4000; // milliseconds
  long barDur = bpm(DEFAULT_BPM,refBarDur);
  
  std::vector<uint8_t> noteMessage;

  bool rNotes;
  uint8_t ch=0,ccCh=0,tempCh;
  std::tie(rNotes,tempCh,prompt,range.first,range.second) = lineParamsOnStart(argc,argv);
  if (rNotes) ch = tempCh; else ccCh = tempCh; // ouch!
  
  bool sync = true;
  std::deque<std::string> prefPhrases{};

  std::string opt;
  
  std::mutex mtxWait, mtxPhrase;
  std::condition_variable cv;
    
  bool soundingThread = false;
  bool exit = false;
    
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  auto fut = async(std::launch::async, [&](){
    unsigned long partial = 0;
    phraseT _phrase{};
    uint8_t _ch = ch;
    uint8_t _ccCh = ccCh;
    bool _rNotes = rNotes;
    
    // waiting for live coder's first phrase 
    std::unique_lock<std::mutex> lckWait(mtxWait);
    cv.wait(lckWait, [&](){return soundingThread == true;});
    std::lock_guard<std::mutex> lckPhrase(mtxPhrase);
    
    while (soundingThread) {
      if (!phrase.empty()) {
        partial = barDur/phrase.size();
        _phrase = phrase;
        _ch = ch;
        _ccCh = ccCh;
        _rNotes = rNotes;
                
        if (_rNotes) {
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
        } else
          if (sync)  {
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
          } else {
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
            barDur = bpm(std::abs(std::stoi(opt.substr(3,opt.size()-1))),refBarDur);
          } catch (...) {
            std::cerr << "Invalid bpm." << std::endl;
          }
      } else if (opt == "ex") {
          phrase.clear();
          soundingThread = true;
          cv.notify_one();
          std::cout << fut.get();
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
            auto cmdLen = opt.substr(0,1) == ":" ? 1 : 2
            parsePhrase(prefPhrases.at(std::stof(opt.substr(cmdLen,opt.size()-1))));
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
          try {
            range.first = std::stof(opt.substr(2,opt.size()-1));
          } catch (...) {
            std::cerr << "Invalid range min value." << std::endl; 
          }
      } else if (opt.substr(0,2) == "ma") {    
          try {
            range.second = std::stof(opt.substr(2,opt.size()-1));
          } catch (...) {
            std::cerr << "Invalid range max value." << std::endl; 
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
            if (filename.empty()) filename = filenameDefault;
            std::ofstream outfile (filename + ".line");
            for_each(prefPhrases.begin(),prefPhrases.end(),[&](std::string _phraseStr){outfile << _phraseStr << "\n";});
            outfile.close();
            std::cout << "File " + filename + " saved.\n";
          } catch (...) {
            std::cerr << "Invalid filename." << std::endl; 
          }
      } else if (opt.substr(0,2) == "lf") {
          try {
            auto filename = opt.substr(2,opt.size()-1);
            if (filename.empty()) throw std::runtime_error("");
            std::ifstream file(filename + ".line");
            if (file.is_open()) {
              std::string _phrase;
              while (std::getline(file, _phrase))
                prefPhrases.push_back(_phrase.c_str());
              file.close();
              std::cout << "File loaded.\n";
            } else throw std::runtime_error("");
          } catch (...) {
            std::cerr << "Couldn't load file." << std::endl; 
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
