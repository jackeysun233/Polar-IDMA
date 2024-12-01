// INIT.h
#ifndef INIT_H
#define INIT_H

#include <vector>
#include <string>
using namespace std;

// º¯ÊýÉùÃ÷
void GenSNR();
void GenMessage(vector<vector<int>>& input_data);
void GenNoise(vector<vector<double>>& noise_parity);
void GenFadingCoff(vector<vector<vector<double>>>& FadingCoff);
string GetCurrentDate();
string GetCurrentTimeString();
void OpenDataFile();
void PrintHeader();
void PrintToConsole(int sim);
void WriteToFile();

#endif // INIT_H
