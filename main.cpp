#include "global_variables.h"
#include "IDMA.h"
#include "Threadpool.h"
#include "INIT.h"
#include "PolarCode.h"
#include <iostream>
#include <string>
#include <bits/stdc++.h>
#include <random>
using namespace std;

// 声明全局变量
const int NUSERS = 1;                       // 活跃用户数量
const int NBITS = 10;                       // 每个用户发送的比特数量
const int SF = 83;                         // 扩频的倍数
const int N = 10;                           // 编码后的码字长度(请根据CodeMode修改,32)
const int FrameLen = N * SF;                // 总的码字的长度
const int Nr = 1;                           // 天线数量
const int L = 32;                           // Polar Code 的 list size

const double EbNoSTART = 5;
const double EbNoSTEP = 4;
const int EbNoNUM = 6;

const int NUM_FRAMES = 50000;               // 帧数量
const int NUM_PRINT = 100;                   // 打印显示间隔

const bool IsFading = true;                 // 控制衰落模式
const string CodeMode = "None";             // 控制IDMA的编码方式（"Polar" for polar coded IDMA;"None" for pure IDMA;）
const int IDMAitr = 25;                     // IDMA迭代次数

const int BlockLen = 500;                               // 块衰落的长度
const int BlockNum = round(FrameLen / BlockLen);        // 衰落块的数量


string filename;                            // 数据保存的文件名

vector<double> snr_dB(EbNoNUM);
vector<double> snr(EbNoNUM, 0.0);
vector<double> ebno_dB(EbNoNUM,0.0);                                        
vector<vector<double>> BER(EbNoNUM, vector<double>(NUM_FRAMES, 0.0));
vector<vector<double>> PUPE(EbNoNUM, vector<double>(NUM_FRAMES, 0.0));       // 初始化BER和PUPE

mutex data_mutex;                           // 全局变量，用于文件写入的互斥锁

atomic<int> cnt(0);                         // 声明全局变量，存储仿真的次数

// 每个线程执行的蒙特卡洛仿真函数
void PolarCodeIDMA(
    const vector<int>& SpreadSeq,
    const vector<vector<int>> ScrambleRule
) {

    vector<vector<int>> InputData(NUSERS, vector<int>(NBITS, 0));
    vector<vector<int>> EncodedData(NUSERS, vector<int>(N, 0));

    vector<vector<vector<int>>> OutputData(EbNoNUM, vector<vector<int>>(NUSERS, vector<int>(NBITS, 0)));


    vector<double> Noise(FrameLen, 0.0);
    vector<vector<double>> H(NUSERS, vector<double>(FrameLen, 1.0));

    // 生成输入信号种子

    mt19937 rng(random_device{}());
    uniform_int_distribution<int> bit_dist(0, 1);

    // 生成噪声种子
    random_device rd;                   // 用于种子
    mt19937 generator(rd());            // Mersenne Twister 引擎
    normal_distribution<double> distribution(0.0, 1.0);  // 均值为0，标准差为1的正态分布


    // 生成噪声
    for (int i = 0; i < FrameLen; ++i) {
        Noise[i] = distribution(generator);
    }

    // 生成输入信号
    for (int i = 0; i < NBITS; ++i)
    {
        for (int j = 0; j < NUSERS; ++j)
        {
            // InputData[j][i] = bit_dist(rng);
            InputData[j][i] = 0;
        }
    }

    // 信道编码
    // 初始化polar编码器、译码器
    int k = static_cast<int>(log2(N));
    PolarCode pc(k, NBITS, 0, 4, true, false, false, false);

    for (int j = 0; j < NUSERS; ++j) {
        // 将输入数据转换为 uint8_t 类型
        vector<uint8_t> tmp(InputData[j].size());
        transform(InputData[j].begin(), InputData[j].end(), tmp.begin(), [](int x) {
            return static_cast<uint8_t>(x);
            });

        // 获取编码后的数据
        vector<uint8_t> encoded = pc.encode(tmp);

        // 将编码后的数据转换为 int 类型并存储到 encoded_data 中
        for (size_t i = 0; i < encoded.size(); ++i) {
            EncodedData[j][i] = static_cast<int>(encoded[i]);  // 转换为 int 并存储
        }
    }
    
    // 计算不同SNR下的误码率
    for (int q = 0; q < EbNoNUM; ++q) {
        double sigma = sqrt(1.0 / snr[q]);    // 计算当前snr下的噪声功率

        auto Tx = Transmitter(EncodedData, ScrambleRule, SpreadSeq);
        auto Rx = Channel(sigma, Noise, H, Tx);
        auto AppLlr = Receiver(sigma, IDMAitr, ScrambleRule, SpreadSeq, H, Rx);

        // 信道编码译码

        for (size_t user = 0; user < NUSERS; ++user) {
            // 调用PolarCode的SCL译码器
            auto decoded = pc.decode_scl_llr(AppLlr[user], L); // 假设list_size为8

            transform(decoded.begin(), decoded.end(), OutputData[q][user].begin(), 
                [](auto x) {return static_cast<int>(x); });

            //for (size_t bit = 0; bit < NBITS; ++bit) {
            //    // 将译码结果转换为int并存入output_data
            //    OutputData[q][user][bit] = static_cast<int>(decoded[bit]);
            //}
        }
    }

    // 文件锁作用域，计算误码率和打印误码率功能只能有一个线程在执行
    {
        std::lock_guard<std::mutex> lock(data_mutex);
        calcError(OutputData, InputData, cnt); // 计算误码率
        if (cnt % NUM_PRINT == 0) {
            PrintToConsole(cnt);    // 满足打印条件时，打印一次结果
        }
        if (cnt == NUM_FRAMES - 1) {
            PrintToConsole(cnt);
            WriteToFile();          // 所有仿真完成后，把结果写入到文件里面
        }
        cnt++;        // 仅在任务完成后递增一次
    }


}

void PureIDMA(
    const vector<int>& SpreadSeq,
    const vector<vector<int>> ScrambleRule
    ) {

    vector<vector<int>> InputData(NUSERS, vector<int>(NBITS, 0));
    vector<vector<int>> EncodedData(NUSERS, vector<int>(N, 0));

    vector<vector<vector<int>>> OutputData(EbNoNUM, vector<vector<int>>(NUSERS, vector<int>(NBITS, 0)));


    vector<double> Noise(FrameLen, 0.0);
    vector<vector<double>> H(NUSERS, vector<double>(FrameLen, 1.0));

    // 生成输入信号种子

    mt19937 rng(random_device{}());
    uniform_int_distribution<int> bit_dist(0, 1);

    // 生成噪声种子
    random_device rd;                   // 用于种子
    mt19937 generator(rd());            // Mersenne Twister 引擎
    normal_distribution<double> distribution(0.0, 1.0);  // 均值为0，标准差为1的正态分布
    normal_distribution<double> rayleigh(0.0, 1.0/sqrt(2.0));  // 均值为0，标准差为1的正态分布



    // 生成噪声
    for (int i = 0; i < FrameLen; ++i) {
        Noise[i] = distribution(generator);
    }

    // 生成输入信号
    for (int i = 0; i < NBITS; ++i)
    {
        for (int j = 0; j < NUSERS; ++j)
        {
            InputData[j][i] = bit_dist(rng);
        }
    }

    // 计算不同SNR下的误码率
    for (int q = 0; q < EbNoNUM; ++q) {
        double sigma = sqrt(1.0 / snr[q]);    // 计算当前snr下的噪声功率

        // 生成信道衰落系数
        if (IsFading) {
            for (int j = 0; j < NUSERS; j++) {
                for (int kk = 0; kk < BlockNum; kk++) {
                    auto U1 = rayleigh(generator);
                    auto U2 = rayleigh(generator);
                    for (int ii = 0; ii < BlockLen; ii++)
                        H[j][BlockLen * kk + ii] = sqrt(U1 * U1 + U2 * U2);
                }
                if (BlockLen * BlockNum < FrameLen) {
                    int RemainLen = FrameLen - BlockLen * BlockNum;
                    auto U1 = rayleigh(generator);
                    auto U2 = rayleigh(generator);
                    for (int ii = 0; ii < RemainLen; ii++)
                        H[j][BlockLen * BlockNum + ii] = sqrt(U1 * U1 + U2 * U2);
                }
            }
        }

        auto Tx = Transmitter(InputData, ScrambleRule, SpreadSeq);
        auto Rx = Channel(sigma, Noise, H, Tx);
        auto AppLlr = Receiver(sigma, IDMAitr, ScrambleRule, SpreadSeq, H, Rx);

        for (int j = 0; j < NUSERS; j++)
            transform(AppLlr[j].begin(), AppLlr[j].end(), OutputData[q][j].begin(),
                [](double x) { return (x >= 0.0) ? 0 : 1; });

    }

    // 文件锁作用域，计算误码率和打印误码率功能只能有一个线程在执行
    {
        std::lock_guard<std::mutex> lock(data_mutex);
        calcError(OutputData, InputData, cnt); // 计算误码率
        if (cnt % NUM_PRINT == 0) {
            PrintToConsole(cnt);    // 满足打印条件时，打印一次结果
        }
        if (cnt == NUM_FRAMES - 1) {
            PrintToConsole(cnt);
            WriteToFile();          // 所有仿真完成后，把结果写入到文件里面
        }
        cnt++;        // 仅在任务完成后递增一次
    }
}



int main() {
    ThreadPool pool(6);     // 使用的线程数量

    OpenDataFile();         // 打开数据存储文件
    GenSNR();               // 生成待仿真的SNR向量
    PrintHeader();          // 打印表头

    vector<int> SpreadSeq(SF); // 扩频序列
    vector<vector<int>> ScrambleRule(NUSERS, vector<int>(FrameLen)); // 交织器
    vector<vector<int>> InputData(NUSERS, vector<int>(NBITS, 0));
    vector<vector<int>> EncodedData(NUSERS, vector<int>(N, 0));

    // 生成扩频序列 {+1, -1, +1, -1, ... }.
    for (int i = 0; i < SF; i++) SpreadSeq[i] = 1 - 2 * (i % 2);

    // 生成交织器索引
    for (int j = 0; j < NUSERS; j++)
    {
        for (int i = 0; i < FrameLen; i++) ScrambleRule[j][i] = i;

        // Perform this for-loop more times may increase the randomness of the interleaver.
        for (int i = 0; i < FrameLen - 1; i++)
        {
            int q = i + (rand() * rand()) % (FrameLen - i);
            int tmp = ScrambleRule[j][i];
            ScrambleRule[j][i] = ScrambleRule[j][q];
            ScrambleRule[j][q] = tmp;
        }
    }

    if (CodeMode == "None") {
        // 多线程并行
        for (int sim = 0; sim < NUM_FRAMES; ++sim) {
            pool.enqueue(PureIDMA,SpreadSeq,ScrambleRule); // 将任务放到线程池里面
        }
    }
    else if (CodeMode == "Polar") {  // 修正了这里的括号
        // 多线程并行
        for (int sim = 0; sim < NUM_FRAMES; ++sim) {
            pool.enqueue(PolarCodeIDMA, SpreadSeq, ScrambleRule); // 将任务放到线程池里面
        }
    }
    else {
        std::cout << "请输入正确的编码方式字段" << std::endl;
    }

    return 0;
}

// 无信道编码的主函数

//int main() {
//
//    // 公共初始化函数
//
//    OpenDataFile();         // 打开数据存储文件
//    GenSNR();               // 生成待仿真的snr向量
//    PrintHeader();          // 打印表头
//
//    vector<int> SpreadSeq(SF); // 扩频序列
//    vector<vector<int>> ScrambleRule(NUSERS, vector<int>(FrameLen)); // 交织器
//
//    vector<vector<int>> InputData(NUSERS, vector<int>(NBITS, 0));
//    vector<vector<int>> EncodedData(NUSERS, vector<int>(N, 0));
//
//    vector<vector<vector<int>>> OutputData(EbNoNUM, vector<vector<int>>(NUSERS, vector<int>(NBITS, 0)));
//
//
//    vector<double> Noise(FrameLen, 0.0);
//    vector<vector<double>> H(NUSERS, vector<double>(FrameLen, 1.0));
//
//    // 生成扩频序列 {+1, -1, +1, -1, ... }.
//    for (int i = 0; i < SF; i++) SpreadSeq[i] = 1 - 2 * (i % 2);
//
//    // 生成交织器索引
//    for (int j = 0; j < NUSERS; j++)
//    {
//        for (int i = 0; i < FrameLen; i++) ScrambleRule[j][i] = i;
//
//        // Perform this for-loop more times may increase the randomness of the interleaver.
//        for (int i = 0; i < FrameLen - 1; i++)
//        {
//            int q = i + (rand() * rand()) % (FrameLen - i);
//            int tmp = ScrambleRule[j][i];
//            ScrambleRule[j][i] = ScrambleRule[j][q];
//            ScrambleRule[j][q] = tmp;
//        }
//    }
//
//    // 生成输入信号种子
//
//    mt19937 rng(random_device{}());
//    uniform_int_distribution<int> bit_dist(0, 1);
//
//    // 生成噪声种子
//    random_device rd;                   // 用于种子
//    mt19937 generator(rd());            // Mersenne Twister 引擎
//    normal_distribution<double> distribution(0.0, 1.0);  // 均值为0，标准差为1的正态分布
//
//
//
//    for (int sim = 0; sim < NUM_FRAMES; ++sim) {
//
//        // 生成噪声
//        for (int i = 0; i < FrameLen; ++i) {
//            Noise[i] = distribution(generator);
//        }
//
//        // 生成输入信号
//        for (int i = 0; i < NBITS; ++i)
//        {
//            for (int j = 0; j < NUSERS; ++j)
//            {
//                InputData[j][i] = bit_dist(rng);
//            }
//        }
//
//        // 信道编码
//        //if (CodeMode == "Polar") {
//        //    // 初始化polar编码器、译码器
//        //    int k = static_cast<int>(log2(N));
//        //    PolarCode pc(k, NBITS, 0, 4, true, false, false, false);
//
//        //    for (int j = 0; j < NUSERS; ++j) {
//        //        // 将输入数据转换为 uint8_t 类型
//        //        vector<uint8_t> tmp(InputData[j].size());
//        //        transform(InputData[j].begin(), InputData[j].end(), tmp.begin(), [](int x) {
//        //            return static_cast<uint8_t>(x);
//        //            });
//
//        //        // 获取编码后的数据
//        //        vector<uint8_t> encoded = pc.encode(tmp);
//
//        //        // 将编码后的数据转换为 int 类型并存储到 encoded_data 中
//        //        for (size_t i = 0; i < encoded.size(); ++i) {
//        //            EncodedData[j][i] = static_cast<int>(encoded[i]);  // 转换为 int 并存储
//        //        }
//        //    }
//
//        //}
//
//        // 计算不同SNR下的误码率
//        for (int q = 0; q < EbNoNUM; ++q) {
//            double sigma = sqrt(1.0 / snr[q]);    // 计算当前snr下的噪声功率
//
//            auto Tx = Transmitter(InputData, ScrambleRule, SpreadSeq);
//            auto Rx = Channel(sigma, Noise, H, Tx);
//            auto AppLlr = Receiver(sigma, IDMAitr, ScrambleRule, SpreadSeq, H, Rx);
//
//            for (int j = 0; j < NUSERS; j++)
//                transform(AppLlr[j].begin(), AppLlr[j].end(), OutputData[q][j].begin(),
//                    [](double x) { return (x >= 0.0) ? 0 : 1; });
//
//        }
//
//            // 文件锁作用域，计算误码率和打印误码率功能只能有一个线程在执行
//            {
//
//                calcError(OutputData, InputData, cnt); // 计算误码率
//                if (cnt % NUM_PRINT == 0) {
//                    PrintToConsole(cnt);    // 满足打印条件时，打印一次结果
//                }
//                if (cnt == NUM_FRAMES - 1) {
//                    PrintToConsole(cnt);
//                    WriteToFile();          // 所有仿真完成后，把结果写入到文件里面
//                }
//                cnt++;        // 仅在任务完成后递增一次
//            }
//    }
//
//    return 0;
//}




