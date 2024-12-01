#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include "IDMA.h"
#include "global_variables.h"

using namespace std;

void InterLeaver(const vector<vector<double>>& Datain, const vector<vector<int>>& ILidx, vector<vector<double>>& ILData) {
    // 确保 ILData 已经被清空
    for (size_t i = 0; i < NUSERS; ++i) {
        fill(ILData[i].begin(), ILData[i].end(), 0);
    }

    // 对每一列进行交织操作
    for (size_t i = 0; i < FrameLen; ++i) {
        for (size_t j = 0; j < NUSERS; ++j) {
            // Temporary vector for each user (column)
            ILData[j][i] = Datain[j][ILidx[j][i]];  // 将交织后的数据存入ILData
        }
    }
}


void deInterleaver(const vector<vector<double>>& Datain, const vector<vector<int>>& ILidx, vector<vector<double>>& deILData) {
    // 确保 deILData 已经被清空
    for (size_t i = 0; i < NUSERS; ++i) {
        fill(deILData[i].begin(), deILData[i].end(), 0);
    }

    // 对每一列进行反交织操作
    for (size_t i = 0; i < FrameLen; ++i) {
        for (size_t j = 0; j < NUSERS; ++j) {
            // 根据交织索引将数据恢复到原始位置
            deILData[j][ILidx[j][i]] = Datain[j][i];
        }
    }
}



void spreader(const vector<vector<double>>& inputData, vector<vector<double>>& spreadedData) {

    // 生成伪随机扩频序列
    vector<int> repCode(SF);
    for (size_t i = 0; i < SF; ++i) {
        repCode[i] = 1 - 2 * (i % 2);  // 1-2*mod(0:(SF-1), 2)
    }


    // 对每个用户的信息进行扩频处理
    for (size_t i = 0; i < NUSERS; ++i) {
        for (size_t j = 0; j < N; ++j) {
            // 每个用户的每一行扩频
            for (size_t k = 0; k < SF; ++k) {
                spreadedData[i][j * SF + k] = inputData[i][j] * repCode[k];
            }
        }
    }
}


void despreader(const vector<vector<double>>& spreadedData, vector<vector<double>>& despreadedData) {


    // 初始化 despreadedData 矩阵，大小为 NUSERS x N
    despreadedData.resize(NUSERS, vector<double>(N, 0));

    // 对每个用户的数据进行反扩频处理
    for (size_t i = 0; i < NUSERS; ++i) {
        for (size_t j = 0; j < N; ++j) {
            for (size_t k = 0; k < SF; ++k) {
                despreadedData[i][j] += spreadedData[i][j * SF + k] * (1 - 2 * static_cast<double>(k % 2));  // 伪随机扩频序列
            }
        }
    }
}






void calESE(const vector<double>& R, const vector<double>& apLLR, const vector<vector<double>>& H, double noiseVar, vector<double>& extrLLR) {
    size_t H_rows = H.size();
    size_t H_cols = H[0].size();

    // 如果不是胖矩阵（H_rows >= H_cols），则转置
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

    // 初始化均值和方差
    vector<double> avgX(H_cols);
    vector<double> varX(H_cols);
    for (size_t i = 0; i < H_cols; ++i) {
        avgX[i] = tanh(apLLR[i] / 2);
        varX[i] = 1 - avgX[i] * avgX[i];
    }

    // 计算接收信号 r 的均值和方差
    vector<double> avgR(H_cols, 0.0);
    vector<double> varR(H_cols, 0.0);
    for (size_t i = 0; i < H_cols; ++i) {
        for (size_t j = 0; j < H_rows; ++j) {
            avgR[i] += avgX[j] * H_transposed[i][j];
            varR[i] += varX[j] * H_transposed[i][j] * H_transposed[i][j];
        }
        varR[i] += noiseVar;
    }

    // 计算 ESE
    extrLLR.resize(H_cols);
    for (size_t i = 0; i < H_cols; ++i) {
        double a = R[i] - avgR[i] + H_transposed[i][i] * avgX[i];
        double b = varR[i] - H_transposed[i][i] * H_transposed[i][i] * varX[i];
        extrLLR[i] = 2 * H_transposed[i][i] * (a / b);
    }
}

// BPSK 调制
void Modulate(const vector<vector<int>>& input_data, vector<vector<double>>& modulated_data) {


    // 生成 BPSK 调制信号
    for (int user = 0; user < NUSERS; ++user) {
        for (int bit = 0; bit < NBITS; ++bit) {
            // 对每个比特进行调制
            modulated_data[user][bit] = (input_data[user][bit] == 0) ? -1.0 : 1.0;
        }
    }
}

// 生成每个用户的交织器索引
void GenILidx(std::vector<std::vector<int>>& ILidx) {
    // 清空 ILidx，避免多次调用时重复数据
    ILidx.clear();

    // 初始化随机数生成器
    random_device rd;
    mt19937 rng(rd());

    // 对每个用户生成随机排列
    for (int x = 0; x < NUSERS; ++x) {
        // 生成初始索引 0, 1, ..., FrameLen-1
        std::vector<int> temp(FrameLen);
        for (int i = 0; i < FrameLen; ++i) {
            temp[i] = i;
        }

        // 随机打乱索引
        shuffle(temp.begin(), temp.end(), rng);

        // 将打乱后的索引存入 ILidx
        ILidx.push_back(temp);
    }
}
