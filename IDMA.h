#ifndef IDMA_H
#define IDMA_H

#include <vector>
#include "PolarCode.h"

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
void calESE(const vector<double>& R, const vector<vector<double>>& apLLR, const vector<vector<double>>& H, double noiseVar, vector<vector<double>>& extrLLR);

// BPSK����
void Modulate(const vector<vector<int>>& input_data, vector<vector<double>>& modulated_data);

// ����ÿ���û��Ľ�֯������
void GenILidx(std::vector<std::vector<int>>& ILidx);

// Channel����������������ݴ���˥�䡢����������
void channel(const vector<vector<double>>& ILData,
    const vector<vector<vector<double>>>& FadingCoff,
    const vector<vector<double>>& noise,
    double noise_variance,
    vector<vector<double>>& RxData);

// ����MIMO���ݣ�����������ݺ�˥��ϵ����ƽ��ֵ
void processMIMOData(const vector<vector<double>>& RxData,
    const vector<vector<vector<double>>>& FadingCoff,
    vector<double>& avg_RxData,
    vector<vector<double>>& avg_FadingCoff);

// Ӳ�о�����
void hardDecision(const vector<vector<double>>& deSpData, vector<vector<vector<int>>>& output_data, int i);

// ����ESE
void calcError(const vector<vector<vector<int>>>& output_data,
    const vector<vector<int>>& input_data,
    int sim);

// ����PolarCode������ŵ�����
void ChannelEncode(const vector<vector<int>>& input_data, PolarCode& pc, vector<vector<int>>& encoded_data);

// ����PolarCode������ŵ�����
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