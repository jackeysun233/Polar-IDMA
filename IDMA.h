#ifndef IDMA_H
#define IDMA_H

#include <vector>

using namespace std;

// Interleaver ��������
void InterLeaver(const vector<vector<int>>& Datain, const vector<vector<int>>& ILidx, vector<vector<int>>& ILData);

// deInterleaver ��������
void deInterleaver(const vector<vector<int>>& Datain, const vector<vector<int>>& ILidx, vector<vector<int>>& deILData);

// Spreader ��������
void spreader(const vector<int>& inputData, const vector<int>& c, vector<int>& spreadedData);

// Despreader ��������
void despreader(const vector<int>& spreadedData, const vector<int>& c, vector<int>& despreadedData);

// calESE ��������
void calESE(const vector<double>& R, const vector<double>& apLLR, const vector<vector<double>>& H, double noiseVar, vector<double>& extrLLR);

#endif // FUNCTIONS_H