#ifndef IDMA_H
#define IDMA_H

#include <vector>

using namespace std;

// Interleaver ��������
void InterLeaver(const vector<vector<double>>& Datain, const vector<vector<int>>& ILidx, vector<vector<double>>& ILData);

// deInterleaver ��������
void deInterleaver(const vector<vector<double>>& Datain, const vector<vector<int>>& ILidx, vector<vector<double>>& deILData);

// Spreader ��������
void spreader(const vector<vector<double>>& inputData, vector<vector<double>>& spreadedData);

// Despreader ��������
void despreader(const vector<vector<double>>& spreadedData, vector<vector<double>>& despreadedData);

// calESE ��������
void calESE(const vector<double>& R, const vector<double>& apLLR, const vector<vector<double>>& H, double noiseVar, vector<double>& extrLLR);

// BPSK����
void Modulate(const vector<vector<int>>& input_data, vector<vector<double>>& modulated_data);

// ����ÿ���û��Ľ�֯������
void GenILidx(std::vector<std::vector<int>>& ILidx);

#endif // FUNCTIONS_H