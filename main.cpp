#include "global_variables.h"
#include "IDMA.h"
#include "Threadpool.h"
#include "INIT.h"
#include <iostream>
#include <string>
using namespace std;

//声明全局变量

const int NUSERS = 2;                       // 活跃用户数量
const int NBITS = 10;                       // 每个用户发送的比特数量
const int SF = 300;                         // 扩频的倍数
const int N = 10;                           // 编码后的码字长度
const int FrameLen = N * SF;                // 总的码字的长度
const int Nr = 1;                           // 天线数量

const double SNR_BEGIN = -40;
const double SNR_END = -20;
const int SNR_NUM = 3;

const int NUM_FRAMES = 100000;              // 帧数量
const int NUM_PRINT = 100;                  // 打印显示间隔

const int BlockLen = 50;                    // 块衰落的长度
const int IDMAitr = 15;                     // IDMA迭代次数

string filename;                            // 数据保存的文件名

vector<double> SNR_dB(SNR_NUM);                                              // SNR值向量（dB）
vector<double> snr(SNR_NUM, 0.0);                                            // snr值（线性）
vector<vector<double>> BER(SNR_NUM, vector<double>(NUM_FRAMES, 0.0));
vector<vector<double>> PUPE(SNR_NUM, vector<double>(NUM_FRAMES, 0.0));       // 初始化BER和PUPE


mutex data_mutex;                           // 全局变量，用于文件写入的互斥锁

atomic<int> cnt(0);                         // 声明全局变量，存储仿真的次数


// 每个线程执行的蒙特卡洛仿真函数
void montecarlosimulation() {

    // 声明线程内私有变量
    vector<vector<int>> input_data(NUSERS, vector<int>(NBITS));     // 用户输入的信息
    vector<vector<double>> noise(Nr, vector<double>(FrameLen));// 高斯信道白噪声
    vector<vector<vector<double>>> FadingCoff(Nr, vector<vector<double>>(NUSERS, vector<double>(FrameLen)));// 信道衰落系数
    vector<vector<double>> modulated_data(NUSERS, vector<double>(NBITS));// 调制后的信息
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

    // 初始化程序
    GenMessage(input_data);
    GenNoise(noise);
    GenFadingCoff(FadingCoff);
    GenILidx(ILidx);

    Modulate(input_data, modulated_data);
    spreader(modulated_data, spreaded_data);
    InterLeaver(spreaded_data, ILidx, ILData);

    //deInterleaver(ILData, ILidx, deILData);
    //despreader(deILData, despreadedData);

    // 计算不同SNR下的误码率
    for (int i = 0; i < SNR_NUM; ++i) {
        double noise_variance = 1.0 / pow(10.0, snr[i] / 10.0);    // 计算当前snr下的噪声功率

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

            InterLeaver(extLLR, ILidx, apLLR);                                      // 交织
        }

        hardDecision(deSpData, output_data, i);                                      // 进行硬判决
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

int main() {

    ThreadPool pool(8);     // 使用的线程数量

    OpenDataFile();         // 打开数据存储文件
    GenSNR();               // 生成待仿真的snr向量

    // 打印表头
    PrintHeader();

    // 多线程并行
    for (int sim = 0; sim < NUM_FRAMES; ++sim) {

        pool.enqueue(montecarlosimulation); // 将任务放到线程池里面


    }
    return 0;
}

//
//int main() {
//
//    // 声明线程内私有变量
//    vector<vector<int>> input_data(NUSERS, vector<int>(NBITS));     // 用户输入的信息
//    vector<vector<double>> noise(Nr, vector<double>(FrameLen));// 高斯信道白噪声
//    vector<vector<vector<double>>> FadingCoff(Nr, vector<vector<double>>(NUSERS, vector<double>(FrameLen)));// 信道衰落系数
//    vector<vector<double>> modulated_data(NUSERS, vector<double>(NBITS));// 调制后的信息
//    vector<vector<int>> ILidx(NUSERS, vector<int>(NBITS));// 随机交织器
//    vector<vector<double>> spreaded_data(NUSERS, vector<double>(FrameLen));// 扩频后的数据
//    vector<vector<double>> ILData(NUSERS, vector<double>(FrameLen));// 交织后的数据
//    vector<vector<double>> RxData(Nr, vector<double>(FrameLen));// 接收机的接收数据
//    vector<double> avg_RxData(FrameLen);// 均衡后的接收机数据
//    vector<vector<double>> avg_FadingCoff(NUSERS, vector<double>(FrameLen));// 均衡后的衰落系数
//    vector<vector<double>> apLLR(NUSERS,vector<double>(FrameLen,0.0));// ESE输入软信息
//    vector<vector<double>> extrLLR(NUSERS, vector<double>(FrameLen, 0.0));// ESE输出软信息
//
//    vector<vector<double>> deSpData(NUSERS, vector<double>(N));// 解扩频后的数据
//    vector<vector<double>> deILData(NUSERS, vector<double>(FrameLen));// 解交织后的数据
//
//    vector<vector<double>> extLLR(NUSERS, vector<double>(FrameLen));
//    vector<vector<vector<int>>> output_data(SNR_NUM, vector<vector<int>>(NUSERS, vector<int>(NBITS)));// 用户输入的信息
//
//    // 初始化程序
//    GenMessage(input_data);
//    GenNoise(noise);
//    GenFadingCoff(FadingCoff);
//    GenILidx(ILidx);
//
//    Modulate(input_data, modulated_data);
//    spreader(modulated_data, spreaded_data);
//    InterLeaver(spreaded_data, ILidx, ILData);
//
//    //deInterleaver(ILData, ILidx, deILData);
//    //despreader(deILData, despreadedData);
//
//    // 计算不同SNR下的误码率
//    for (int i = 0; i < SNR_NUM; ++i) {
//        double noise_variance = 1.0 / pow(10.0, snr[i] / 10.0);    // 计算当前snr下的噪声功率
//
//        channel(ILData, FadingCoff, noise, noise_variance, RxData);
//        processMIMOData(RxData, FadingCoff, avg_RxData, avg_FadingCoff);
//
//        // IDMA译码器循环
//        for (int r = 0; r < IDMAitr; ++r) {
//
//            calESE(avg_RxData, apLLR, avg_FadingCoff, noise_variance, extrLLR);     // 计算ESE
//            deInterleaver(extrLLR,ILidx,deILData);                                  // 解交织
//            despreader(deILData,deSpData);                                          // 解扩频
//
//            spreader(deSpData, spreaded_data);                                      // 扩频
//
//            for (size_t user = 0; user < NUSERS; ++user) {
//                for (size_t i = 0; i < FrameLen; ++i) {
//                    extLLR[user][i] = spreaded_data[user][i] - deILData[user][i];   // 迭代之后做差值
//                }
//            }
//
//            InterLeaver(extLLR, ILidx, apLLR);                                      // 交织
//        }
//
//        hardDecision(deSpData, output_data,i);                                      // 进行硬判决
//    }
//
//    return 0; 
//}



