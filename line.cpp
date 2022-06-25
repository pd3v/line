//
//  line - tiny command-line MIDI sequencer for live coding
//
//  Created by @pd3v_
//

#include <iostream>
#include <string>
#include <chrono>
#include <future>
#include <mutex>
#include <vector>
#include <deque>
#include <regex>
#include <stdexcept>
#include <sstream>
#include <stdlib.h>
#include <algorithm> 
#include "externals/rtmidi/RtMidi.h"

using namespace std;

const float DEFAULT_BPM = 60.0;
const string PROMPT = "line> ";
const string VERSION = "0.1.1";
const char REST_SYMBOL = '-';
const uint8_t REST_VAL = 128;
const uint8_t OFF_SYNC_DUR = 100; // milliseconds

void replaceRests(string& s) {
  auto rests = true;
  std::size_t p;

  while(rests) {
    p = s.find(REST_SYMBOL);
    if (p != std::string::npos) {
      s.erase(p,1);
      s.insert(p,to_string(REST_VAL));
    } else 
        rests = false;
  }
}

const uint16_t bpm(const int16_t bpm, const uint16_t barDur) {
  return DEFAULT_BPM/bpm*barDur;
}

void displayOptionsMenu(string menuVers="") {
  cout << "----------------------" << endl;
  cout << "  line " << VERSION << " midi seq  " << endl;
  cout << "----------------------" << endl;
  cout << "..<[n] >    pattern   " << endl;
  cout << "..b<[n]>    bpm       " << endl;
  cout << "..ch<[n]>   midi ch   " << endl;
  cout << "..m         this menu " << endl;
  cout << "..d         extnd menu" << endl;
  cout << "..e         exit      " << endl;
  cout << "..a<[n]>    amplitude " << endl;
  cout << "..r         reverse   " << endl;
  cout << "..s         scramble  " << endl;
  cout << "..x         xscramble " << endl;
  cout << "..l<[n]>    last patt " << endl;
  
  if (menuVers == "d") {
    cout << "..cc<[n]>   cc ch mode" << endl;
    cout << "..n         notes mode" << endl;
    cout << "..t         mute      " << endl;
    cout << "..u         unmute    " << endl;
    cout << "..i         sync cc   " << endl;
    cout << "..o         async cc  " << endl;
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

vector<vector<uint16_t>> reverse(vector<vector<uint16_t>> _pattern) {
  for_each(_pattern.begin(),_pattern.end(),[&](auto& _subPattern) {std::reverse(_subPattern.begin(),_subPattern.end());});
  std::reverse(_pattern.begin(),_pattern.end());

  return _pattern;
}

vector<vector<uint16_t>> scramble(vector<vector<uint16_t>> _pattern) {
  for_each(_pattern.begin(),_pattern.end(),[&](auto& _subPattern) {std::random_shuffle(_subPattern.begin(),_subPattern.end());});
  std::random_shuffle(_pattern.begin(),_pattern.end());

  return _pattern;
}

vector<vector<uint16_t>> xscramble(vector<vector<uint16_t>> _pattern) {
  vector<uint16_t> pattValues {};

  for_each(_pattern.begin(),_pattern.end(),[&](auto& _subPattern) {
    for_each(_subPattern.begin(),_subPattern.end(),[&](auto& _v) {
      pattValues.emplace_back(_v);
    });
  });

  std::random_shuffle(pattValues.begin(),pattValues.end());
  
  for_each(_pattern.begin(),_pattern.end(),[&](auto& _subPattern) {
    for_each(_subPattern.begin(),_subPattern.end(),[&](auto& _v) {
      _v = pattValues.back();
      pattValues.pop_back();
    });
  });
  
  std::random_shuffle(_pattern.begin(),_pattern.end());

  return _pattern;
}

int main() {
  auto midiOut = RtMidiOut();
  midiOut.openPort(0);

  const uint16_t refBarDur = 4000; // milliseconds
  long barDur = bpm(DEFAULT_BPM,refBarDur);
  
  vector<uint8_t> noteMessage;
  vector<vector<uint16_t>> pattern{};
  uint8_t ch = 0;
  uint8_t ccCh = 0;
  bool rNotes = true;
  bool sync = false;
  deque<vector<vector<uint16_t>>> last3Patterns{};
  string opt;
  
  mutex mtxWait, mtxPattern;
  condition_variable cv;
  
  smatch matchExp;
  regex regExp(R"([a-zA-Z!\"#$%&\\/()=?±§*+´`|\\^~<>;,:-_]|[.]{2,})");
  
  bool soundingThread = false;
  bool exit = false;
  bool syntaxError = false;
    
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  auto fut = async(launch::async, [&](){
    unsigned long partial = 0;
    vector<vector<uint16_t>> _patt{};
    uint8_t _ch = 0;
    uint8_t _ccCh = 0;
    bool _rNotes = true;
    // bool sync = false;
    
    // waiting for live coder's first pattern 
    unique_lock<mutex> lckWait(mtxWait);
    cv.wait(lckWait, [&](){return soundingThread == true;});
    lock_guard<mutex> lckPattern(mtxPattern);
    
    while (soundingThread) {
      if (!pattern.empty()) {
        partial = barDur/pattern.size();
        _patt = pattern;
        _ch = ch;
        _ccCh = ccCh;
        _rNotes = rNotes;
        // _sync = sync;

        if (_rNotes)
          for (auto& subPattern : _patt) {
            for (auto& n : subPattern) {
              noteMessage[0] = 144+_ch;
              noteMessage[1] = n;
              noteMessage[2] = ((n == REST_VAL) || muted) ? 0 : amplitude;
              midiOut.sendMessage(&noteMessage);
              
              std::this_thread::sleep_for(chrono::milliseconds(partial/subPattern.size()));

              noteMessage[0] = 128+_ch;
              noteMessage[1] = n;
              noteMessage[2] = 0;
              midiOut.sendMessage(&noteMessage);
            }
          }
        else
          for (auto& subPattern : _patt) {
            for (auto& n : subPattern) {
              noteMessage[0] = 176+_ch;
              noteMessage[1] = _ccCh;
              noteMessage[2] = n;
              midiOut.sendMessage(&noteMessage);
              
              std::this_thread::sleep_for(chrono::milliseconds(sync ? partial/subPattern.size() : OFF_SYNC_DUR));
            }
          }
      } else break;
    }
    return "line is off.\n";
  });
  
  displayOptionsMenu("");
  
  while (!exit) {
    std::cout << PROMPT;
    getline(cin, opt);
    
    if (!opt.empty()) {
      if (opt == "m") {
        displayOptionsMenu("");
      } else if (opt == "d") {
        displayOptionsMenu(opt);
      } else if (opt.substr(0,2) == "ch") {
          try {
            ch = std::abs(std::stoi(opt.substr(2,opt.size()-1))-1);
          }
          catch (...) {
            cerr << "Invalid channel value." << endl; 
          }
      } else if (opt == "n") {
          rNotes = true;
          pattern.clear();
          pattern.push_back({REST_VAL});
      } else if (opt.substr(0,2) == "cc") {
          try {
            ccCh = std::abs(std::stoi(opt.substr(2,opt.size()-1)));
            rNotes = false;
          }
          catch (...) {
            cerr << "Invalid cc channel value." << endl; 
          }
      } else if (opt.at(0) == 'b') {
          try {
            barDur = bpm(std::abs(std::stoi(opt.substr(1,opt.size()-1))),refBarDur);
          } catch (...) {
            cerr << "Invalid bpm value." << endl;
          }
      } else if (opt == "e") {
          pattern.clear();
          soundingThread = true;
          cv.notify_one();
          std::cout << fut.get();
          exit = true;
      } else if (opt.at(0) == 'a') {
          try {
            amp(std::stof(opt.substr(1,opt.size()-1)));
          } catch (...) {
            cerr << "Invalid amplitude." << endl; 
          }
      } else if (opt == "t") {
          mute();
      } else if (opt == "u") {
          unmute();
      } else if (opt == "r") {    
          pattern = reverse(pattern);
      } else if (opt == "s") {    
          pattern = scramble(pattern);
      } else if (opt == "x") {    
          pattern = xscramble(pattern);
      } else if (opt == "l" || opt == "l1") {    
          pattern = last3Patterns.front();
      } else if (opt == "l2") {    
          pattern = last3Patterns.at(1);
      } else if (opt == "l3") {    
          pattern = last3Patterns.at(2);
      } else if (opt == "i") {    
          sync= true;
      } else if (opt == "o") {    
          sync= false;
      } else {
        // parser
        regex_search(opt, matchExp, regExp);
        sregex_iterator pos(opt.cbegin(), opt.cend(), regExp);
        sregex_iterator end;
        
        if (pos == end) {
          vector<vector<uint16_t>> tempPattern{};
          replaceRests(opt);
          istringstream iss(opt);
          bool subBarFlag = false;
          vector<uint16_t> subPatt{};
  
          vector<string> results((istream_iterator<string>(iss)), istream_iterator<string>());
        
          for_each(results.begin(),results.end(),[&](string i) {
              string s;

              try {
                if (i.substr(0,1) == ".") {
                  s = i.substr(1,i.back());
                  subPatt.push_back(static_cast<uint16_t>(stoi(s)));
                  subBarFlag = true;
                } else if (i.substr(i.length()-1,i.length()) == ".") {
                  s = i.substr(0,i.length()-1);
                  subPatt.push_back(static_cast<uint16_t>(stoi(s)));
                  tempPattern.push_back(subPatt);
                  subPatt.clear();
                  subBarFlag = false;
                } else {
                  if (subBarFlag == true)
                    subPatt.push_back(stoi(i));
                  else if (subBarFlag == false)
                    tempPattern.push_back({static_cast<uint16_t>(stoi(i))});
                }
              } catch(...) { 
                syntaxError = true;
              }
            }
          );
          
          if (syntaxError) {
            tempPattern.clear();
            syntaxError = false;
          } 
          
          if (!tempPattern.empty()) {
            pattern = tempPattern;
            
            soundingThread = true;
            cv.notify_one();

            last3Patterns.push_front(pattern);
            if (last3Patterns.size() > 3) last3Patterns.pop_back();
          }
        }
      }
    }
  }

  return 0;
}