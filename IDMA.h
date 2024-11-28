#ifndef IDMA_H
#define IDMA_H

#include <vector>

using namespace std;

// Interleaver 函数声明
void InterLeaver(const vector<vector<int>>& Datain, const vector<vector<int>>& ILidx, vector<vector<int>>& ILData);

// deInterleaver 函数声明
void deInterleaver(const vector<vector<int>>& Datain, const vector<vector<int>>& ILidx, vector<vector<int>>& deILData);

// Spreader 函数声明
void spreader(const vector<int>& inputData, const vector<int>& c, vector<int>& spreadedData);

// Despreader 函数声明
void despreader(const vector<int>& spreadedData, const vector<int>& c, vector<int>& despreadedData);

// calESE 函数声明
void calESE(const vector<double>& R, const vector<double>& apLLR, const vector<vector<double>>& H, double noiseVar, vector<double>& extrLLR);

#endif // FUNCTIONS_H