#include <vector>
#include <cmath>
#include <random>
#include <iostream>
#include "IDMA.h"
#include "PolarCode.h"
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
        repCode[i] =  1 - 2 * (i % 2);  // 1-2*mod(0:(SF-1), 2)
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
    despreadedData.assign(NUSERS, vector<double>(N, 0));

    // 对每个用户的数据进行反扩频处理
    for (size_t i = 0; i < NUSERS; ++i) {
        for (size_t j = 0; j < N; ++j) {
            for (size_t k = 0; k < SF; ++k) {
                despreadedData[i][j] += spreadedData[i][j * SF + k] * (1 - 2 * static_cast<double>(k % 2));  // 伪随机扩频序列
            }
        }
    }
}






void calESE(const vector<double>& R,
    const vector<vector<double>>& apLLR,
    const vector<vector<double>>& H,
    double noiseVar,
    vector<vector<double>>& extrLLR) {

    size_t H_rows = H.size();        // H 的行数
    size_t H_cols = H[0].size();     // H 的列数

    // 初始化均值和方差
    vector<vector<double>> avgX(H_rows, vector<double>(H_cols));
    vector<vector<double>> varX(H_rows, vector<double>(H_cols));

    // 计算均值和方差
    for (size_t i = 0; i < H_cols; ++i) {
        for (size_t j = 0; j < H_rows; ++j) {
            avgX[j][i] = tanh(apLLR[j][i] / 2);  // 使用 apLLR[j][i] 来计算 avgX
            varX[j][i] = 1 - avgX[j][i] * avgX[j][i];  // 计算方差
        }
    }

    // 计算接收信号 r 的均值和方差
    vector<double> avgR(H_cols, 0.0);
    vector<double> varR(H_cols, 0.0);
    for (size_t i = 0; i < H_cols; ++i) {
        for (size_t j = 0; j < H_rows; ++j) {
            avgR[i] += avgX[j][i] * H[j][i];  // 根据 H 的列计算 avgR
            varR[i] += varX[j][i] * H[j][i] * H[j][i];  // 计算 varR
        }
        varR[i] += noiseVar;  // 加上噪声方差
    }

    // 计算 ESE (Extrinsic Log-Likelihood Ratio)
    extrLLR.resize(H_rows, vector<double>(H_cols)); // extrLLR 是二维数组
    for (size_t i = 0; i < H_cols; ++i) {
        for (size_t j = 0; j < H_rows; ++j) {
            // 计算 a 和 b
            double a = R[i] - avgR[i] + H[j][i] * avgX[j][i];
            double b = varR[i] - H[j][i] * H[j][i] * varX[j][i];

            // 计算 extrLLR
            extrLLR[j][i] = 2 * H[j][i] * (a / b);
        }
    }
}



// BPSK 调制
void Modulate(const vector<vector<int>>& input_data, vector<vector<double>>& modulated_data) {


    // 生成 BPSK 调制信号
    for (int user = 0; user < NUSERS; ++user) {
        for (int bit = 0; bit < input_data[0].size(); ++bit) {
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

// Channel函数：负责接收数据处理（衰落、叠加噪声）
void channel(const vector<vector<double>>& ILData,
    const vector<vector<vector<double>>>& FadingCoff,
    const vector<vector<double>>& noise,
    double noise_variance,
    vector<vector<double>>& RxData) {

    // 对每个天线进行处理
    for (int nr = 0; nr < Nr; ++nr) {
        // 初始化接收数据
        vector<double> receivedData(FrameLen, 0.0);

        // 对每个用户进行信号叠加
        for (int user = 0; user < NUSERS; ++user) {
            // 加上衰落系数h，FadingCoff[nr][user]为该天线和用户的衰落系数
            for (int i = 0; i < FrameLen; ++i) {
                receivedData[i] += FadingCoff[nr][user][i] * ILData[user][i];
            }
        }

        // 为接收到的数据添加噪声，考虑噪声功率的调整
        for (int i = 0; i < FrameLen; ++i) {
            receivedData[i] += sqrt(noise_variance) * noise[nr][i]; // 调整噪声幅值
        }

        // 将接收到的数据存储到RxData中
        for (int i = 0; i < FrameLen; ++i) {
            RxData[nr][i] = receivedData[i];
        }
    }
}  

// 处理MIMO数据，计算接收数据和衰落系数的平均值
void processMIMOData(const vector<vector<double>>& RxData,
    const vector<vector<vector<double>>>& FadingCoff,
    vector<double>& avg_RxData,
    vector<vector<double>>& avg_FadingCoff) {

    // 对接收数据进行平均处理
    for (int i = 0; i < FrameLen; ++i) {
        double sum = 0.0;
        for (int nr = 0; nr < Nr; ++nr) {
            sum += RxData[nr][i];
        }
        avg_RxData[i] = sum / Nr;  // 求平均值
    }

    // 对每个符号时刻的FadingCoff进行平均处理
    for (int user = 0; user < NUSERS; ++user) {
        for (int i = 0; i < FrameLen; ++i) {
            double sum = 0.0;
            for (int nr = 0; nr < Nr; ++nr) {
                sum += FadingCoff[nr][user][i];
            }
            avg_FadingCoff[user][i] = sum / Nr;  // 求平均值
        }
    }
}

void hardDecision(const vector<vector<double>>& deSpData,
    vector<vector<vector<int>>>& output_data,
    int i) {  
    for (size_t user = 0; user < NUSERS; ++user) {
        for (size_t bit = 0; bit < NBITS; ++bit) {
            // 对每个接收到的符号进行硬判决
            if (deSpData[user][bit] > 0) {
                output_data[i][user][bit] = 1;  // 判决为0
            }
            else {
                output_data[i][user][bit] = 0;  // 判决为1
            }
        }
    }
}


void calcError(const vector<vector<vector<int>>>& output_data,
    const vector<vector<int>>& input_data,
    int sim) {
    int total_bits = NUSERS * NBITS;  // 总比特数量

    for (int snr = 0; snr < SNR_NUM; ++snr) {
        int bit_errors = 0;  // 当前 SNR 的比特错误数量
        int block_errors = 0; // 当前 SNR 的块错误数量

        // 对每个用户进行误差计算
        for (int user = 0; user < NUSERS; ++user) {
            int user_bit_errors = 0; // 当前用户的比特错误数量

            // 获取 output_data 中的判决数据和 input_data 中的真实数据
            const vector<int>& estimated_codeword = output_data[snr][user];
            const vector<int>& true_codeword = input_data[user];

            // 逐位比较 estimated_codeword 和 true_codeword
            for (int bit = 0; bit < NBITS; ++bit) {
                if (estimated_codeword[bit] != true_codeword[bit]) {
                    user_bit_errors++; // 记录比特错误
                }
            }

            // 如果存在比特错误，则计入块错误数量
            if (user_bit_errors > 0) {
                block_errors++;
            }

            // 累加比特错误数量
            bit_errors += user_bit_errors;
        }

        // 计算并更新 BER 和 PUPE
        BER[snr][sim] = static_cast<double>(bit_errors) / total_bits;
        PUPE[snr][sim] = static_cast<double>(block_errors) / NUSERS;
    }
}

// 调用PolarCode类进行信道编码
void ChannelEncode(const vector<vector<int>>& input_data, PolarCode& pc, vector<vector<int>>& encoded_data) {
    // 遍历每个用户
    for (int k = 0; k < NUSERS; ++k) {
        // 将输入数据转换为 uint8_t 类型
        vector<uint8_t> tmp(input_data[k].size());
        transform(input_data[k].begin(), input_data[k].end(), tmp.begin(), [](int x) {
            return static_cast<uint8_t>(x);
            });

        // 获取编码后的数据
        vector<uint8_t> encoded = pc.encode(tmp);

        // 将编码后的数据转换为 int 类型并存储到 encoded_data 中
        for (size_t i = 0; i < encoded.size(); ++i) {
            encoded_data[k][i] = static_cast<int>(encoded[i]);  // 转换为 int 并存储
        }
    }
}

// Polar的译码器
void ChannelDecoder(const vector<vector<double>>& deSpData,
    vector<vector<vector<int>>>& output_data,
    int i, PolarCode& pc) {
    for (size_t user = 0; user < NUSERS; ++user) {
        // 调用PolarCode的SCL译码器
        auto decoded = pc.decode_scl_llr(deSpData[user], L); // 假设list_size为8

        for (size_t bit = 0; bit < NBITS; ++bit) {
            // 将译码结果转换为int并存入output_data
            output_data[i][user][bit] = static_cast<int>(decoded[bit]);
        }
    }
}


vector<vector<double>> Transmitter(
const vector<vector<int>>& InputData,
const vector<vector<int>>& ScrambleRule,
const vector<int>& SpreadSeq
)
{
    int i, j, nuser;
    
    double tmp;

    vector<vector<double>> Chip(NUSERS, vector<double>(FrameLen));
    vector<vector<double>> Tx(NUSERS, vector<double>(FrameLen));


    // Spreading process.
    for (int j = 0; j < NUSERS; j++) for (int i = 0; i < FrameLen; i++)
    {    
        tmp = 1 - (2 * InputData[j][i]);
        for (int s= 0; s < SF; s++) Chip[j][i] = tmp * SpreadSeq[s];
    }

    // Interleaving process.
    for (int j = 0; j < NUSERS; j++) for (int i = 0; i < FrameLen; i++)
        Tx[nuser][i] = Chip[nuser][ScrambleRule[nuser][i]];

    return Tx;
}


vector<double> Channel(double sigma, 
    const vector<double>& Noise, 
    const vector<vector<double>>& H,
    const vector<vector<double>>& Tx) {

    vector<double> Rx(FrameLen);

    for (int i = 0; i < FrameLen; i++)
    {
        // Additive white Gaussian noise.
        Rx[i] = sigma * Noise[i];

        // Multiple access channel and energy scaling.
        for (int j = 0; j < NUSERS; j++) Rx[i] += H[j][i] * Tx[j][i];
    }

}

vector<vector<double>> Receiver(
    const double sigma,
    const int IDMAitr,
    const vector<vector<int>>& ScrambleRule,
    const vector<vector<double>>& H,
    const vector<double> Rx
) {

    // 定义并初始化 TotalMean 和 TotalVar
    vector<double> TotalMean(FrameLen, 0.0); // 总均值，初始化为 0
    vector<double> TotalVar(FrameLen, sigma * sigma); // 总方差，初始化为 sigma^2

    // 定义并初始化 Mean 和 Var
    vector<vector<double>> Mean(NUSERS, vector<double>(FrameLen, 0.0)); // 用户均值，初始化为 0
    vector<vector<double>> Var(NUSERS, vector<double>(FrameLen, 0.0));  // 用户方差，初始化为 0



    // 初始化均值方差
    for (int i = 0; i < FrameLen; i++)
    {
        TotalMean[i] = 0;
        TotalVar[i] = sigma * sigma;
    }
    for (int j = 0; j < NUSERS; j++) for (int i = 0; i < FrameLen; i++)
    {
        Mean[j][i] = 0;
        Var[j][i] = H[j][i] * H[j][i];
        TotalVar[i] += H[j][i] * H[j][i];
    }


    for (int it = 0; it < IDMAitr; it++) for (int j = 0; j < NUSERS; j++)
    {
        // Produce the LLR values for the de-interleaved chip sequence.
        for (int i = 0; i < FrameLen; i++)
        {
            TotalMean[i] -= Mean[j][i];
            TotalVar[i] -= Var[j][i];
            Chip[j][ScrambleRule[j][i]] = 2 * H[j][i] * (Rx[i] - TotalMean[i]) / TotalVar[i];
        }

        // De-spreading operation.(TODO)
        for (int k = 0; k < NBITS; k++)
        {
            
            for (int s = 0; s < SF; s++) appllr[k] += SpreadSeq[s] * chip[m++];
        }

        // Feed the AppLlr to decoder, if there is a FEC codec in the system.

        // Spreading operation: Produce the extrinsic LLR for each chip
        for (m = i = 0; i < _DataLen; i++) for (j = 0; j < _SpreadLen; j++, m++)
            Ext[nuser][m] = SpreadSeq[j] * AppLlr[nuser][i] - Chip[nuser][m];

        // Update the statistic variable together with the interleaving process.
        for (i = 0; i < _ChipLen; i++)
        {
            Mean[nuser][i] = H[nuser] * tanh(Ext[nuser][ScrambleRule[nuser][i]] / 2);
            Var[nuser][i] = H2[nuser] - Mean[nuser][i] * Mean[nuser][i];
            TotalMean[i] += Mean[nuser][i];
            TotalVar[i] += Var[nuser][i];
        }
    }
}