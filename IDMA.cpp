#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include "IDMA.h"
#include "global_variables.h"

using namespace std;

void InterLeaver(const vector<vector<double>>& Datain, const vector<vector<int>>& ILidx, vector<vector<double>>& ILData) {
    // ȷ�� ILData �Ѿ������
    for (size_t i = 0; i < NUSERS; ++i) {
        fill(ILData[i].begin(), ILData[i].end(), 0);
    }

    // ��ÿһ�н��н�֯����
    for (size_t i = 0; i < FrameLen; ++i) {
        for (size_t j = 0; j < NUSERS; ++j) {
            // Temporary vector for each user (column)
            ILData[j][i] = Datain[j][ILidx[j][i]];  // ����֯������ݴ���ILData
        }
    }
}


void deInterleaver(const vector<vector<double>>& Datain, const vector<vector<int>>& ILidx, vector<vector<double>>& deILData) {
    // ȷ�� deILData �Ѿ������
    for (size_t i = 0; i < NUSERS; ++i) {
        fill(deILData[i].begin(), deILData[i].end(), 0);
    }

    // ��ÿһ�н��з���֯����
    for (size_t i = 0; i < FrameLen; ++i) {
        for (size_t j = 0; j < NUSERS; ++j) {
            // ���ݽ�֯���������ݻָ���ԭʼλ��
            deILData[j][ILidx[j][i]] = Datain[j][i];
        }
    }
}



void spreader(const vector<vector<double>>& inputData, vector<vector<double>>& spreadedData) {

    // ����α�����Ƶ����
    vector<int> repCode(SF);
    for (size_t i = 0; i < SF; ++i) {
        repCode[i] = 1 - 2 * (i % 2);  // 1-2*mod(0:(SF-1), 2)
    }


    // ��ÿ���û�����Ϣ������Ƶ����
    for (size_t i = 0; i < NUSERS; ++i) {
        for (size_t j = 0; j < N; ++j) {
            // ÿ���û���ÿһ����Ƶ
            for (size_t k = 0; k < SF; ++k) {
                spreadedData[i][j * SF + k] = inputData[i][j] * repCode[k];
            }
        }
    }
}


void despreader(const vector<vector<double>>& spreadedData, vector<vector<double>>& despreadedData) {


    // ��ʼ�� despreadedData ���󣬴�СΪ NUSERS x N
    despreadedData.resize(NUSERS, vector<double>(N, 0));

    // ��ÿ���û������ݽ��з���Ƶ����
    for (size_t i = 0; i < NUSERS; ++i) {
        for (size_t j = 0; j < N; ++j) {
            for (size_t k = 0; k < SF; ++k) {
                despreadedData[i][j] += spreadedData[i][j * SF + k] * (1 - 2 * static_cast<double>(k % 2));  // α�����Ƶ����
            }
        }
    }
}






void calESE(const vector<double>& R, const vector<double>& apLLR, const vector<vector<double>>& H, double noiseVar, vector<double>& extrLLR) {
    size_t H_rows = H.size();
    size_t H_cols = H[0].size();

    // ��������־���H_rows >= H_cols������ת��
    vector<vector<double>> H_transposed(H_cols, vector<double>(H_rows));
    if (H_rows >= H_cols) {
        for (size_t i = 0; i < H_rows; ++i) {
            for (size_t j = 0; j < H_cols; ++j) {
                H_transposed[j][i] = H[i][j];
            }
        }
    }
    else {
        H_transposed = H;
    }

    // ��ʼ����ֵ�ͷ���
    vector<double> avgX(H_cols);
    vector<double> varX(H_cols);
    for (size_t i = 0; i < H_cols; ++i) {
        avgX[i] = tanh(apLLR[i] / 2);
        varX[i] = 1 - avgX[i] * avgX[i];
    }

    // ��������ź� r �ľ�ֵ�ͷ���
    vector<double> avgR(H_cols, 0.0);
    vector<double> varR(H_cols, 0.0);
    for (size_t i = 0; i < H_cols; ++i) {
        for (size_t j = 0; j < H_rows; ++j) {
            avgR[i] += avgX[j] * H_transposed[i][j];
            varR[i] += varX[j] * H_transposed[i][j] * H_transposed[i][j];
        }
        varR[i] += noiseVar;
    }

    // ���� ESE
    extrLLR.resize(H_cols);
    for (size_t i = 0; i < H_cols; ++i) {
        double a = R[i] - avgR[i] + H_transposed[i][i] * avgX[i];
        double b = varR[i] - H_transposed[i][i] * H_transposed[i][i] * varX[i];
        extrLLR[i] = 2 * H_transposed[i][i] * (a / b);
    }
}

// BPSK ����
void Modulate(const vector<vector<int>>& input_data, vector<vector<double>>& modulated_data) {


    // ���� BPSK �����ź�
    for (int user = 0; user < NUSERS; ++user) {
        for (int bit = 0; bit < NBITS; ++bit) {
            // ��ÿ�����ؽ��е���
            modulated_data[user][bit] = (input_data[user][bit] == 0) ? -1.0 : 1.0;
        }
    }
}

// ����ÿ���û��Ľ�֯������
void GenILidx(std::vector<std::vector<int>>& ILidx) {
    // ��� ILidx�������ε���ʱ�ظ�����
    ILidx.clear();

    // ��ʼ�������������
    random_device rd;
    mt19937 rng(rd());

    // ��ÿ���û������������
    for (int x = 0; x < NUSERS; ++x) {
        // ���ɳ�ʼ���� 0, 1, ..., FrameLen-1
        std::vector<int> temp(FrameLen);
        for (int i = 0; i < FrameLen; ++i) {
            temp[i] = i;
        }

        // �����������
        shuffle(temp.begin(), temp.end(), rng);

        // �����Һ���������� ILidx
        ILidx.push_back(temp);
    }
}
