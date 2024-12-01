#ifndef IDMA_H
#define IDMA_H

#include <vector>

using namespace std;

// Interleaver 函数声明
void InterLeaver(const vector<vector<double>>& Datain, const vector<vector<int>>& ILidx, vector<vector<double>>& ILData);

// deInterleaver 函数声明
void deInterleaver(const vector<vector<double>>& Datain, const vector<vector<int>>& ILidx, vector<vector<double>>& deILData);

// Spreader 函数声明
void spreader(const vector<vector<double>>& inputData, vector<vector<double>>& spreadedData);

// Despreader 函数声明
void despreader(const vector<vector<double>>& spreadedData, vector<vector<double>>& despreadedData);

// calESE 函数声明
void calESE(const vector<double>& R, const vector<double>& apLLR, const vector<vector<double>>& H, double noiseVar, vector<double>& extrLLR);

// BPSK调制
void Modulate(const vector<vector<int>>& input_data, vector<vector<double>>& modulated_data);

// 生成每个用户的交织器索引
void GenILidx(std::vector<std::vector<int>>& ILidx);

#endif // FUNCTIONS_H