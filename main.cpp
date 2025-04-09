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
const int NUSERS = 24;                      // 活跃用户数量
const int NBITS = 256;                       // 每个用户发送的比特数量
const int SF = 16;                           // 扩频的倍数
const int N = 256;                          // 编码后的码字长度(请根据CodeMode修改,32)
const int FrameLen = N * SF;                // 总的码字的长度
const int Nr = 1;                           // 天线数量
const int L = 32;                           // Polar Code 的 list size

const double SNR_BEGIN = -4;
const double SNR_END = 1;
const int SNR_NUM = 6;

const int NUM_FRAMES = 1000;               // 帧数量
const int NUM_PRINT = 10;                  // 打印显示间隔

const bool IsFading = false;                 // 控制衰落模式
const string CodeMode = "None";             // 控制IDMA的编码方式（"Polar" for polar coded IDMA;"None" for pure IDMA;）
const int BlockLen = 500;                    // 块衰落的长度
const int IDMAitr = 25;                      // IDMA迭代次数


string filename;                            // 数据保存的文件名

vector<double> SNR_dB(SNR_NUM);                                              // SNR值向量（dB）
vector<double> snr(SNR_NUM, 0.0);                                            // snr值（线性）
vector<vector<double>> BER(SNR_NUM, vector<double>(NUM_FRAMES, 0.0));
vector<vector<double>> PUPE(SNR_NUM, vector<double>(NUM_FRAMES, 0.0));       // 初始化BER和PUPE

mutex data_mutex;                           // 全局变量，用于文件写入的互斥锁

atomic<int> cnt(0);                         // 声明全局变量，存储仿真的次数

// 每个线程执行的蒙特卡洛仿真函数
void PolarCodeIDMA() {

    // 声明线程内私有变量
    vector<vector<int>> input_data(NUSERS, vector<int>(NBITS));     // 用户输入的信息
    vector<vector<int>> encoded_data(NUSERS, vector<int>(N));       // 编码后的信息
    vector<vector<double>> noise(Nr, vector<double>(FrameLen));// 高斯信道白噪声
    vector<vector<vector<double>>> FadingCoff(Nr, vector<vector<double>>(NUSERS, vector<double>(FrameLen)));// 信道衰落系数
    vector<vector<double>> modulated_data(NUSERS, vector<double>(N));// 调制后的信息
    vector<vector<int>> ILidx(NUSERS, vector<int>(NBITS));// 随机交织器
    vector<vector<double>> spreaded_data(NUSERS, vector<double>(FrameLen));// 扩频后的数据
    vector<vector<double>> ILData(NUSERS, vector<double>(FrameLen));// 交织后的数据
    vector<vector<double>> RxData(Nr, vector<double>(FrameLen));// 接收机的接收数据
    vector<double> avg_RxData(FrameLen);// 均衡后的接收机数据
    vector<vector<double>> avg_FadingCoff(NUSERS, vector<double>(FrameLen));// 均衡后的衰落系数
    vector<vector<double>> apLLR(NUSERS, vector<double>(FrameLen, 0.0));// ESE输入软信息
    vector<vector<double>> extrLLR(NUSERS, vector<double>(FrameLen, 0.0));// ESE输出软信息

    vector<vector<double>> deSpData(NUSERS, vector<double>(N));// 解扩频后的数据
    vector<vector<double>> deILData(NUSERS, vector<double>(FrameLen));// 解交织后的数据

    vector<vector<double>> extLLR(NUSERS, vector<double>(FrameLen));
    vector<vector<vector<int>>> output_data(SNR_NUM, vector<vector<int>>(NUSERS, vector<int>(NBITS)));// 用户输入的信息

    // 初始化polar编码器、译码器
    int k = static_cast<int>(log2(N));
    PolarCode pc(k, NBITS, 0, 4, true, false, false, false);

    // 初始化程序
    GenMessage(input_data);
    GenNoise(noise);
    GenFadingCoff(FadingCoff);
    GenILidx(ILidx);

    ChannelEncode(input_data, pc, encoded_data);
    Modulate(encoded_data, modulated_data);
    spreader(modulated_data, spreaded_data);
    InterLeaver(spreaded_data, ILidx, ILData);

    // 计算不同SNR下的误码率
    for (int i = 0; i < SNR_NUM; ++i) {
        double noise_variance = 1.0 / snr[i];       // 计算当前snr下的噪声功率

        channel(ILData, FadingCoff, noise, noise_variance, RxData);
        processMIMOData(RxData, FadingCoff, avg_RxData, avg_FadingCoff);

        // IDMA译码器循环
        for (int r = 0; r < IDMAitr; ++r) {

            calESE(avg_RxData, apLLR, avg_FadingCoff, noise_variance, extrLLR);     // 计算ESE
            deInterleaver(extrLLR, ILidx, deILData);                                  // 解交织
            despreader(deILData, deSpData);                                          // 解扩频

            spreader(deSpData, spreaded_data);                                      // 扩频

            for (size_t user = 0; user < NUSERS; ++user) {
                for (size_t i = 0; i < FrameLen; ++i) {
                    extLLR[user][i] = spreaded_data[user][i] - deILData[user][i];   // 迭代之后做差值
                }
            }

            extLLR = spreaded_data;

            InterLeaver(extLLR, ILidx, apLLR);                                       // 交织
        }

        ChannelDecoder(deSpData, output_data, i, pc);

        // hardDecision(deSpData, output_data, i);                                      // 进行硬判决
    }

    // 文件锁作用域，计算误码率和打印误码率功能只能有一个线程在执行
    {
        std::lock_guard<std::mutex> lock(data_mutex);
                                    
        calcError(output_data, input_data, cnt); // 计算误码率
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

void PureIDMA() {

    // 声明线程内私有变量
    vector<vector<int>> input_data(NUSERS, vector<int>(NBITS,0));     // 用户输入的信息
    vector<vector<double>> noise(Nr, vector<double>(FrameLen,0.1));// 高斯信道白噪声
    vector<vector<vector<double>>> FadingCoff(Nr, vector<vector<double>>(NUSERS, vector<double>(FrameLen,0.9)));// 信道衰落系数
    vector<vector<double>> modulated_data(NUSERS, vector<double>(NBITS));// 调制后的信息
    vector<vector<int>> ILidx(NUSERS, vector<int>(FrameLen));// 随机交织器
    vector<vector<double>> spreaded_data(NUSERS, vector<double>(FrameLen));// 扩频后的数据
    vector<vector<double>> ILData(NUSERS, vector<double>(FrameLen));// 交织后的数据
    vector<vector<double>> RxData(Nr, vector<double>(FrameLen));// 接收机的接收数据
    vector<double> avg_RxData(FrameLen);// 均衡后的接收机数据
    vector<vector<double>> avg_FadingCoff(NUSERS, vector<double>(FrameLen));// 均衡后的衰落系数
    vector<vector<double>> apLLR(NUSERS, vector<double>(FrameLen, 0.0));// ESE输入软信息
    vector<vector<double>> extrLLR(NUSERS, vector<double>(FrameLen, 0.0));// ESE输出软信息

    vector<vector<double>> deSpData(NUSERS, vector<double>(N));// 解扩频后的数据
    vector<vector<double>> deILData(NUSERS, vector<double>(FrameLen));// 解交织后的数据

    vector<vector<double>> extLLR(NUSERS, vector<double>(FrameLen));
    vector<vector<vector<int>>> output_data(SNR_NUM, vector<vector<int>>(NUSERS, vector<int>(NBITS)));// 用户输入的信息

    // 初始化程序
    GenMessage(input_data); // 默认发送信息是0
    GenNoise(noise);  // 噪声基数默认设置为0.1
    GenFadingCoff(FadingCoff);  // 衰落系数默认设置为1.0
    GenILidx(ILidx);  // 使用默认的随机交织器

    Modulate(input_data, modulated_data);
    spreader(modulated_data, spreaded_data);
    InterLeaver(spreaded_data, ILidx, ILData);

    // 计算不同SNR下的误码率
    for (int i = 0; i < SNR_NUM; ++i) {
        double noise_variance = 1.0 / snr[i];    // 计算当前snr下的噪声功率

        channel(ILData, FadingCoff, noise, noise_variance, RxData);
        processMIMOData(RxData, FadingCoff, avg_RxData, avg_FadingCoff);

        // CBC译码器（并行）
        for (int r = 0; r < IDMAitr; ++r) {

            calESE(avg_RxData, apLLR, avg_FadingCoff, noise_variance, extrLLR);       // 计算ESE
            deInterleaver(extrLLR, ILidx, deILData);                                  // 解交织
            despreader(deILData, deSpData);                                           // 解扩频

            spreader(deSpData, spreaded_data);                                        // 扩频

            for (size_t user = 0; user < NUSERS; ++user) {
                for (size_t i = 0; i < FrameLen; ++i) {
                    extLLR[user][i] = spreaded_data[user][i] - deILData[user][i];   // 迭代之后做差值
                }
            }

            InterLeaver(extLLR, ILidx, apLLR);                                      // 交织
        }

        // CBC译码器（串行）
        for (int r = 0; r < IDMAitr; r++)
        {
            for (int j = 0; j < NUSERS; j++) {
                // Produce the LLR values for the de-interleaved chip sequence.
                for (int i = 0; i < FrameLen; i++)
                {

                }

            }
        }


        hardDecision(deSpData, output_data,i);                                      // 进行硬判决
    }

    // 文件锁作用域，计算误码率和打印误码率功能只能有一个线程在执行
    {
        std::lock_guard<std::mutex> lock(data_mutex);

        calcError(output_data, input_data, cnt); // 计算误码率
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

//int main() {
//    ThreadPool pool(8);     // 使用的线程数量
//
//    OpenDataFile();         // 打开数据存储文件
//    GenSNR();               // 生成待仿真的SNR向量
//    PrintHeader();          // 打印表头
//
//    if (CodeMode == "None") {
//        // 多线程并行
//        for (int sim = 0; sim < NUM_FRAMES; ++sim) {
//            pool.enqueue(PureIDMA); // 将任务放到线程池里面
//        }
//    }
//    else if (CodeMode == "Polar") {  // 修正了这里的括号
//        // 多线程并行
//        for (int sim = 0; sim < NUM_FRAMES; ++sim) {
//            pool.enqueue(PolarCodeIDMA); // 将任务放到线程池里面
//        }
//    }
//    else {
//        std::cout << "请输入正确的编码方式字段" << std::endl;
//    }
//
//    return 0;
//}

// 无信道编码的主函数

int main() {

    // 公共初始化函数

    OpenDataFile();         // 打开数据存储文件
    GenSNR();               // 生成待仿真的snr向量
    PrintHeader();          // 打印表头

    vector<int> SpreadSeq(SF); // 扩频序列
    vector<vector<int>> ScrambleRule(NUSERS, vector<int>(FrameLen)); // 交织器
    vector<vector<int>> InputData(NUSERS, vector<int>(NBITS, 0));

    vector<vector<vector<int>>> OutputData(SNR_NUM, vector<vector<int>>(NUSERS, vector<int>(NBITS, 0)));


    vector<double> Noise(FrameLen, 0.0);
    vector<vector<double>> H(NUSERS, vector<double>(FrameLen, 1.0));

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

    // 生成输入信号种子

    mt19937 rng(random_device{}());
    uniform_int_distribution<int> bit_dist(0, 1);

    // 生成噪声种子
    random_device rd;                   // 用于种子
    mt19937 generator(rd());            // Mersenne Twister 引擎
    normal_distribution<double> distribution(0.0, 1.0);  // 均值为0，标准差为1的正态分布



    for (int sim = 0; sim < NUM_FRAMES; ++sim) {

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
        for (int q = 0; q < SNR_NUM; ++q) {
            double sigma = 1.0 / snr[q];    // 计算当前snr下的噪声功率

            auto Tx = Transmitter(InputData, ScrambleRule, SpreadSeq);
            auto Rx = Channel(sigma, Noise, H, Tx);
            auto AppLlr = Receiver(sigma, IDMAitr, ScrambleRule, SpreadSeq, H, Rx);

            for (int j = 0; j < NUSERS; j++)
                transform(AppLlr[j].begin(), AppLlr[j].end(), OutputData[q][j].begin(),
                    [](double x) { return (x >= 0.0) ? 0 : 1; });

        }

            // 文件锁作用域，计算误码率和打印误码率功能只能有一个线程在执行
            {

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

    return 0;
}




