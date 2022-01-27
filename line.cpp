#include <iostream>
#include <string>
#include <chrono>
#include <future>
#include <mutex>
#include <vector>
#include <regex>
#include <stdexcept>
#include <sstream>
#include "externals/rtmidi/RtMidi.h"

using namespace std;

const float DEFAULT_BPM = 60.0;

const int bpm(const int bpm, const unsigned int barDur) {
  return DEFAULT_BPM/bpm*barDur;
}

void displayOptionsMenu() {
  cout << "---------------------" << endl;
  cout << "  line 0.1 midi seq  " << endl;
  cout << "---------------------" << endl;
  cout << "..<[n] >   pattern   " << endl;
  cout << "..b<[n]>   bpm       " << endl;
  cout << "..m        this menu " << endl;
  cout << "..e        exit      " << endl;
  cout << "---------------------" << endl;
}

int main() {
  auto midiOut = RtMidiOut();
  midiOut.openPort(0);

  const unsigned int refBarDur = 4000; // milliseconds
  long barDur = bpm(DEFAULT_BPM,refBarDur);
  
  vector<unsigned char> noteMessage;
  vector<vector<int>> pattern{};
  
  // const string prompt = "\nh:>";s
  string opt;
  
  mutex mtxWait, mtxPattern;
  condition_variable cv;
  
  smatch matchExp;
  regex regExp(R"([a-zA-Z!\"#$%&\\/()=?±§*+´`|\\^~<>;,:-_]|[.]{2,})");
  
  bool soundingThread = false;
  bool exit = false;
    
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  noteMessage.push_back(0);
  
  auto fut = async(launch::async, [&](){
    unsigned long partial = 0;
    vector<vector<int>> _patt{};
    
    // waiting for live coder's first pattern 
    unique_lock<mutex> lckWait(mtxWait);
    cv.wait(lckWait, [&](){return soundingThread == true;});
    lock_guard<mutex> lckPattern(mtxPattern);

    while (soundingThread) {
      if (!pattern.empty())
        partial = barDur/pattern.size();
      
      _patt = pattern;

      for (auto& subPattern : _patt) {
        for (auto& n : subPattern) {
          noteMessage[0] = 144;
          noteMessage[1] = n;
          noteMessage[2] = (n == 0) ? 0 : 127;
          midiOut.sendMessage(&noteMessage);
          
          std::this_thread::sleep_for(chrono::milliseconds(partial/subPattern.size()));

          noteMessage[0] = 128;
          noteMessage[1] = n;
          noteMessage[2] = 0;
          midiOut.sendMessage(&noteMessage);
        }
      }
    }
    return "line is off.\n";
  });
  
  displayOptionsMenu();
  
  while (!exit) {
    getline(cin, opt);
    
    if (!opt.empty()) {
      if (opt.at(0) == 'm') {
        displayOptionsMenu();
      } else if (opt.at(0) == 'b') {
        try {
          barDur = bpm(std::stoi(opt.substr(1,opt.size()-1)),refBarDur);
        } catch (...) {
          cerr << "Invalid bpm value." << endl;
        }
      } else if (opt.at(0) == 'e') {
          pattern.clear();
          soundingThread = false;
          exit = true;
          std::cout << fut.get();
      } else {
        // parser
        regex_search(opt, matchExp, regExp);
        sregex_iterator pos(opt.cbegin(), opt.cend(), regExp);
        sregex_iterator end;
        
        if (pos == end) {
          vector<vector<int>> tempPattern{};
          istringstream iss(opt);
          bool subBarFlag = 0;
          vector<int> subPatt{};
          
          vector<string> results((istream_iterator<string>(iss)), istream_iterator<string>());
          
          for_each(results.begin(),results.end(),[&](string i) {
              string s;
              
              if (i.substr(0,1) == ".") {
                s = i.substr(1,i.back());
                subPatt.push_back(stoi(s));
                subBarFlag = true;
              } else if (i.substr(i.length()-1,i.length()) == ".") {
                s = i.substr(0,i.length()-1);
                subPatt.push_back(stoi(s));
                tempPattern.push_back(subPatt);
                subPatt.clear();
                subBarFlag = false;
              } else {
                if (subBarFlag == true)
                  subPatt.push_back(stoi(i));
                else if (subBarFlag == false)
                  tempPattern.push_back({stoi(i)});
              }
            }
          );
          
          if (!tempPattern.empty()) {
            pattern = tempPattern;
            
            soundingThread = true;
            cv.notify_one();
          }
        }
      }
    }
  }

  return 0;
}
