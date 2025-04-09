#ifndef IDMA_H
#define IDMA_H

#include <vector>
#include "PolarCode.h"

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
void calESE(const vector<double>& R, const vector<vector<double>>& apLLR, const vector<vector<double>>& H, double noiseVar, vector<vector<double>>& extrLLR);

// BPSK调制
void Modulate(const vector<vector<int>>& input_data, vector<vector<double>>& modulated_data);

// 生成每个用户的交织器索引
void GenILidx(std::vector<std::vector<int>>& ILidx);

// Channel函数：负责接收数据处理（衰落、叠加噪声）
void channel(const vector<vector<double>>& ILData,
    const vector<vector<vector<double>>>& FadingCoff,
    const vector<vector<double>>& noise,
    double noise_variance,
    vector<vector<double>>& RxData);

// 处理MIMO数据，计算接收数据和衰落系数的平均值
void processMIMOData(const vector<vector<double>>& RxData,
    const vector<vector<vector<double>>>& FadingCoff,
    vector<double>& avg_RxData,
    vector<vector<double>>& avg_FadingCoff);

// 硬判决函数
void hardDecision(const vector<vector<double>>& deSpData, vector<vector<vector<int>>>& output_data, int i);

// 计算ESE
void calcError(const vector<vector<vector<int>>>& output_data,
    const vector<vector<int>>& input_data,
    int sim);

// 调用PolarCode类进行信道编码
void ChannelEncode(const vector<vector<int>>& input_data, PolarCode& pc, vector<vector<int>>& encoded_data);

// 调用PolarCode类进行信道译码
void ChannelDecoder(const vector<vector<double>>& deSpData,
    vector<vector<vector<int>>>& output_data,
    int i, PolarCode& pc);

vector<vector<double>> Transmitter(
    const vector<vector<int>>& InputData,
    const vector<vector<int>>& ScrambleRule,
    const vector<int>& SpreadSeq
);

vector<double> Channel(double sigma,
    const vector<double>& Noise,
    const vector<vector<double>>& H,
    const vector<vector<double>>& Tx);





#endif // FUNCTIONS_H