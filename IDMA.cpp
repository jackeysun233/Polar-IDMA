#include <vector>
#include <cmath>
#include "IDMA.h"

using namespace std;

void InterLeaver(const vector<vector<int>>& Datain, const vector<vector<int>>& ILidx, vector<vector<int>>& ILData) {
    for (size_t i = 0; i < Datain[0].size(); ++i) {
        // Temporary vector for each column in Datain
        vector<int> Temp = Datain[i];
        for (size_t j = 0; j < Temp.size(); ++j) {
            ILData[j][i] = Temp[ILidx[j][i]];
        }
    }
}

void deInterleaver(const vector<vector<int>>& Datain, const vector<vector<int>>& ILidx, vector<vector<int>>& deILData) {
    // Declare temporary vector
    vector<int> tempILData(Datain.size(), 0);

    for (size_t i = 0; i < Datain[0].size(); ++i) {
        // Temporary vector for each column in Datain
        vector<int> Temp = Datain[i];
        for (size_t j = 0; j < Temp.size(); ++j) {
            tempILData[ILidx[j][i]] = Temp[j];
        }
        for (size_t j = 0; j < tempILData.size(); ++j) {
            deILData[j][i] = tempILData[j];
        }
    }
}


void spreader(const vector<int>& inputData, const vector<int>& c, vector<int>& spreadedData) {
    size_t sf = c.size();
    size_t n = inputData.size();

    spreadedData.resize(sf * n);

    // Replicating inputData to match spreading factor
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < sf; ++j) {
            spreadedData[i * sf + j] = inputData[i];
        }
    }

    // Multiplying each symbol with the spreading code
    for (size_t i = 0; i < n * sf; ++i) {
        spreadedData[i] *= c[i % sf];
    }
}

void despreader(const vector<int>& spreadedData, const vector<int>& c, vector<int>& despreadedData) {
    size_t sf = c.size();
    size_t n = spreadedData.size() / sf;

    // Reshaping spreadedData into sf x n matrix
    vector<vector<int>> reshapedData(n, vector<int>(sf));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < sf; ++j) {
            reshapedData[i][j] = spreadedData[i * sf + j];
        }
    }

    // Despreading each symbol
    despreadedData.resize(n, 0);
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < sf; ++j) {
            despreadedData[i] += reshapedData[i][j] * c[j];
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

