#include "IDMA.h"
#include <string>
using namespace std;

//声明全局变量

const int NUSERS = 1;//活跃用户数量
const int NBITS = 10;//每个用户发送的比特数量
const int SF = 300;//扩频的倍数
const int N = 32;//编码后的码字长度
const int FrameLen = N * SF;//总的码字的长度
const int Nr = 1;//天线数量

const double SNR_BEGIN = -40;
const double SNR_END = -20;
const int SNR_NUM = 3;

const int NUM_FRAMES = 100000;              // 帧数量
const int NUM_PRINT = 100;                  // 打印显示间隔

const int BlockLen = 1500;// 块衰落的长度

string filename;                            // 数据保存的文件名

vector<double> SNR_dB(SNR_NUM);                                              // SNR值向量（dB）
vector<vector<double>> BER(SNR_NUM, vector<double>(NUM_FRAMES, 0.0));
vector<vector<double>> PUPE(SNR_NUM, vector<double>(NUM_FRAMES, 0.0));       // 初始化BER和PUPE




