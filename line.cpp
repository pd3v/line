#include <iostream>
#include <string>
#include "RtMidi.h"
#include <chrono>
#include <future>
#include <mutex>
#include <vector>
#include <regex>
#include <stdexcept>
#include <sstream>

using namespace std;
using namespace chrono;

const float DEFAULT_BPM = 60.0;

const int bpm(const int bpm, const unsigned int barDur) {
  return DEFAULT_BPM/bpm*barDur;
}

void displayOptions() {
  cout << "..line.1.0..seq..." << endl;
  cout << "<[n]+>  pattern" << endl;
  cout << "b<[n]+> bpm" << endl;
  cout << "o       options" << endl;
  cout << "e       exit" << endl;
  cout << ".................." << endl;
}

int main(int argc, char const *argv[]) {
  auto midiOut = RtMidiOut();
  midiOut.openPort(0);

  const unsigned int refBarDur = 4000; // milliseconds
  long barDur = bpm(DEFAULT_BPM,refBarDur);
  
  vector<unsigned char> noteMessage;
  vector<vector<int>> pattern{};
  
  const string prompt = "\nh:>";
  string op;
  
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
    int _n;
    unsigned long partial = 0;
    
    unique_lock<mutex> lckWait(mtxWait);
    cv.wait(lckWait, [&](){return soundingThread == true;});
    
    lock_guard<mutex> lckPattern(mtxPattern);
    while (soundingThread) {
      if (!pattern.empty())
        partial = barDur/pattern.size();
      
      for (auto& subPattern : pattern) {
        for (auto& n : subPattern) {
          _n = n;
          
          noteMessage[0] = 144;
          noteMessage[1] = _n;
          noteMessage[2] = (_n == 0) ? 0 : 127;
          midiOut.sendMessage(&noteMessage);
          
          std::this_thread::sleep_for(milliseconds(partial/subPattern.size()));

          noteMessage[0] = 128;
          noteMessage[1] = _n;
          noteMessage[2] = 0;
          midiOut.sendMessage(&noteMessage);
        }
      }
    }
  });
  
  displayOptions();
  
  while (!exit) {
    getline(cin, op);
    
    if (!op.empty()) {
      if (op.at(0) == 'o') {
        displayOptions();
      } else if (op.at(0) == 'b') {
        try {
          barDur = bpm(std::stoi(op.substr(1,op.size()-1)),refBarDur);
        } catch (...) {
          cout << "Invalid bpm value." << endl;
        }
      } else if (op.at(0) == 'e') {
          soundingThread = false;
          exit = true;
          midiOut.closePort();
      } else {
        // parser
        regex_search(op, matchExp, regExp);
        sregex_iterator pos(op.cbegin(), op.cend(), regExp);
        sregex_iterator end;
        
        if (pos == end) {
          vector<vector<int>> tempPattern{};
          istringstream iss(op);
          bool subBarFlag = 0;
          vector<int> subPatt{};
          
          vector<string> results((istream_iterator<string>(iss)), istream_iterator<string>());
          
          for_each(results.begin(), results.end(),
            [&](string i) {
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
