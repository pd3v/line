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
#include <tuple>
// #include <std::any>
#if defined(LINK_PLATFORM_UNIX)
#include <termios.h>
#endif
extern "C" {
  #include "externals/lua/lua.h"
	#include "externals/lua/lualib.h"
	#include "externals/lua/lauxlib.h"
}

using phraseT = std::vector<std::vector<std::vector<uint8_t>>>;

const float DEFAULT_BPM = 60.0;
const char *PROMPT = "line>";
const std::string VERSION = "0.3.3";
const char REST_SYMBOL = '-';
const uint8_t REST_VAL = 128;
const uint8_t OFF_SYNC_DUR = 100; // milliseconds

long iterDur = 5; // milliseconds

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

  phraseT parsing(std::string _phrase) {
    phraseT v{};
    std::vector<std::vector<uint8_t>> subv{};
    std::vector<uint8_t> subsubv{};

    auto p = parserCode + " t = lpeg.match(phraseG, \"" + _phrase + "\")";

    if (luaL_dostring(L, p.c_str()) == LUA_OK) {
      lua_getglobal(L, "t");
      
      // 3D Lua table to c++ vector
      if (lua_istable(L,-1)) {
        lua_pushnil(L);
        lua_gettable(L,-2);
        size_t table = lua_rawlen(L,-2);
        size_t subtable,subsubtable;

        for (int i=0; i<table; ++i) {
          lua_next(L,-2);      
          lua_pushnil(L);
          subtable = lua_rawlen(L,-2);

          for (int j=0; j<subtable; ++j) {
            lua_next(L,-2);      
            lua_pushnil(L);
            subsubtable = lua_rawlen(L,-2);  

            for (int k=0; k<subsubtable; ++k) {
              lua_next(L,-2);
              subsubv.push_back(static_cast<uint8_t>(lua_tonumber(L,-1)));
              lua_pop(L,1);
            }
            subv.push_back(subsubv);
            subsubv.clear();
            lua_pop(L,2);
          }
          v.push_back(subv);
          subv.clear();

          lua_pop(L,2);
        }
      }
    }
    return v;
  }
};

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
  cout << "..l<[n]>    last patt " << endl;
  
  if (menuVers == "me") {
    cout << "..cc<[n]>   cc ch mode" << endl;
    cout << "..n         notes mode" << endl;
    cout << "..m         mute      " << endl;
    cout << "..um        unmute    " << endl;
    cout << "..i         sync cc   " << endl;
    cout << "..o         async cc  " << endl;
    cout << "..lb<[a|n]> label     " << endl;
  }
  cout << "----------------------" << endl;
  
  if (rand()%10+1 == 1) cout << "          author:pd3v" << endl;
}

float amplitude = 127;
bool muted = false;

void amp(float _amplitude) {
  amplitude = 127*_amplitude;
}

void mute() {
  muted = true;
}

void unmute() {
  muted = false;
}

phraseT reverse(phraseT _phrase) {
  for_each(_phrase.begin(),_phrase.end(),[&](auto& _subPhrase) {
    std::reverse(_subPhrase.begin(),_subPhrase.end());
    for_each(_subPhrase.begin(),_subPhrase.end(),[&](auto& _subsubPhrase) {
      std::reverse(_subsubPhrase.begin(),_subsubPhrase.end());
    });
  });

  std::reverse(_phrase.begin(),_phrase.end());

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
  std::vector<uint8_t> pattValues {};

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

std::string prompt = PROMPT;

std::tuple<bool,int,std::string,float, float> setUserDefaults() {
  return std::make_tuple(true,1,"a",0.,127.);
}

std::tuple<bool,int,std::string,float, float> setUserDefaults(int argc, char **argv) {
  // line args order: notes/cc ch label scale
  auto noteCC{strcmp(argv[1],"n")};
  auto ch{std::stoi(argv[2],nullptr)};

  // auto noArgsDefaults = std::make_tuple(true,1,"",0,127);
  // std::cout << argc << std::endl;
  // if(argc == 1) {
  //   std::cout << argc << std::endl;
  //   return std::make_tuple(true,1,"a",0.,127.);
  // }  
  // else
  //   return std::make_tuple(!noteCC,ch,argv[3],0,127);

  return std::make_tuple(!noteCC,ch,argv[3],0,127);

  /* 
   std::size_t pos{};
        try
        {
            std::cout << "std::stoi('" << *argv->at(1) << "'): ";
            const int i {std::stoi(*argv->at(1) , &pos)};
            std::cout << i << "; pos: " << pos << '\n';
        }
        catch(std::invalid_argument const& ex)
        {
            std::cout << "std::invalid_argument::what(): " << ex.what() << '\n';
        }
   */       
  // for(int i = 1;i < argc;++i) {
  //   std::cout << argv[i] << std::endl; 
  // }
  // return noteCC;
  
}

// bool beatOn;

int main(int argc, char **argv) {
  Parser parser;
  auto midiOut = RtMidiOut();
  midiOut.openPort(0);

  const uint16_t refBarDur = 4000; // milliseconds
  long barDur = bpm(DEFAULT_BPM,refBarDur);
  
  std::vector<uint8_t> noteMessage;
  phraseT phrase{};
  uint8_t ch = 0;
  uint8_t ccCh = 0;
  bool rNotes = true;
  bool sync = false;
  std::deque<phraseT> last3Phrases{};

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
    uint8_t _ch = 0;
    uint8_t _ccCh = 0;
    bool _rNotes = true;
    
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
            for (auto& _subPhrase : _phrase) {
              for (auto& _subsubPhrase : _subPhrase) {
                for (auto& notes : _subsubPhrase) {
                  noteMessage[0] = 144+_ch;
                  noteMessage[1] = notes;
                  noteMessage[2] = ((notes == REST_VAL) || muted) ? 0 : amplitude;
                  midiOut.sendMessage(&noteMessage);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<unsigned long>((partial/_subPhrase.size())-iterDur)));
                for (auto& notes : _subsubPhrase) {  
                  noteMessage[0] = 128+_ch;
                  noteMessage[1] = notes;
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
                noteMessage[2] = ccValues;
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
                noteMessage[2] = ccValues;
                midiOut.sendMessage(&noteMessage);
              }
                std::this_thread::sleep_for(std::chrono::milliseconds(OFF_SYNC_DUR));
              }
            }
          }
      } else break;
    }
    return "line is off.\n";
  });

  // std::tie(rNotes,ch) = setUserDefaults(argc,argv);
  rNotes = std::get<0>(setUserDefaults(argc,argv));
  if(rNotes) 
    ch = std::get<1>(setUserDefaults(argc,argv));
  else
    ccCh = std::get<1>(setUserDefaults(argc,argv));
    // "~"+opt.substr(2,opt.length()-1)+_prompt.substr(_prompt.length()-1,_prompt.length())
  prompt = "~"+std::get<2>(setUserDefaults(argc,argv))+">" ; 

  std::cout << "line " << VERSION << " is on." << std::endl;

  // std::cout << "rNotes:" << (rNotes == true ? "true" : "false") << " ch:" << static_cast<int>(ch) << " ccCh:" << static_cast<int>(ccCh) << std::endl;
  // displayOptionsMenu("");
  
  while (!exit) {
    opt = readline(prompt.c_str());

    if (!opt.empty()) {
      if (opt == "ms") {
        displayOptionsMenu("");
      } else if (opt == "me") {
        displayOptionsMenu(opt);
      } else if (opt.substr(0,2) == "ch") {
          try {
            ch = std::abs(std::stoi(opt.substr(2,opt.size()-1))-1);
          }
          catch (...) {
            std::cerr << "Invalid channel." << std::endl; 
          }
      } else if (opt == "n") {
          rNotes = true;
          phrase.clear();
          phrase.push_back({{REST_VAL}});
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
            amp(std::stof(opt.substr(2,opt.size()-1)));
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
      } else if (opt == "x") {    
          phrase = xscramble(phrase);
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
      } else if (opt.substr(0,2) == "lb") {    
          // prompt = _prompt.substr(0,_prompt.length()-1)+"~"+opt.substr(2,opt.length()-1)+_prompt.substr(_prompt.length()-1,_prompt.length()); formats -> line~<newlable>
          prompt = PROMPT;

          if (opt.length() > 2) {
            std::string _prompt =  PROMPT;
            prompt = "~"+opt.substr(2,opt.length()-1)+_prompt.substr(_prompt.length()-1,_prompt.length());
          }
      } else {
        // it's a phrase, if it's not a command
        phraseT tempPhrase{};
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
