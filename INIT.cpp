#include <vector>
#include <random>
#include "global_variables.h"

using namespace std;

// 计算SNR的值
void GenSNR() {
    // 计算 SNR 的步长
    double step = (SNR_NUM > 1) ? (SNR_END - SNR_BEGIN) / (SNR_NUM - 1) : 0.0;

    // 初始化 SNR 值向量
    vector<double> SNR_values(SNR_NUM, 0.0);

    // 计算 SNR 的具体取值
    for (int i = 0; i < SNR_NUM; ++i) {
        SNR_dB[i] = SNR_BEGIN + i * step;                // SNR 的 dB 值
        SNR_values[i] = pow(10, SNR_dB[i] / 10.0);       // 转换为线性单位
    }
}

// 生成用户信息
void GenMessage(vector<vector<int>>& input_data) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> bit_dist(0, 1);

    // 生成随机比特流
    for (int i = 0; i < NBITS; ++i)
    {
        for (int j = 0; j < NUSERS; ++j)
        {
            input_data[j][i] = bit_dist(rng);
        }
    }
}

// 生成噪声功率为1的噪声向量
void GenNoise(vector<vector<double>>& noise_parity) {
    // 初始化随机数生成器和分布
    random_device rd;                   // 用于种子
    mt19937 generator(rd());            // Mersenne Twister 引擎
    normal_distribution<double> distribution(0.0, 1.0);  // 均值为0，标准差为1的正态分布

    // 填充 noise_parity 的每个元素
    for (int i = 0; i < Nr; ++i) {
        for (int j = 0; j < FrameLen; ++j) {
            noise_parity[i][j] = distribution(generator);
        }
    }
}

// 生成衰落系数向量（需要修改）
void GenFadingCoff(vector<vector<vector<double>>>& FadingCoff) {
    // 检查条件 NUSERS * NBITS < M
    if (NUSERS * NBITS > M) {
        throw invalid_argument("错误：NUSERS * NBITS 必须小于等于 M。");
    }

    // 初始化随机数生成器
    mt19937 rng(random_device{}());

    // 计算每个块的衰落系数矩阵的大小
    int num_blocks = FrameLen / BlockLen; // 完整块的数量
    int remaining_slots = FrameLen % BlockLen; // 剩余部分
    if (remaining_slots > 0) {
        num_blocks++; // 如果有剩余部分，增加一个块
    }

    // 初始化 FadingCoff 矩阵，大小为 Nr x NUSERS x FrameLen
    FadingCoff.resize(Nr, vector<vector<double>>(NUSERS, vector<double>(FrameLen, 0.0)));

    // 为每个天线和每个用户生成衰落系数
    for (int nr = 0; nr < Nr; ++nr) {
        for (int user = 0; user < NUSERS; ++user) {
            // 每个块的衰落系数
            uniform_real_distribution<double> uniformDist(0.0, 1.0);
            for (int block = 0; block < num_blocks; ++block) {
                double U = uniformDist(rng);
                double h = sqrt(-2.0 * log(U)); // 瑞利分布的生成方式

                // 填充对应的衰落系数
                for (int i = block * BLOCK_LEN; i < (block + 1) * BLOCK_LEN && i < FrameLen; ++i) {
                    FadingCoff[nr][user][i] = h; // 用生成的衰落系数填充每个用户的每个符号
                }
            }
        }
    }
}
