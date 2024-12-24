#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H

#include <vector>
#include <string>

using namespace std;

// 全局常量声明
extern const int NUSERS;     // 活跃用户数量
extern const int NBITS;      // 每个用户发送的比特数量
extern const int SF;         // 扩频的倍数
extern const int Nr;         // 天线数量
extern const int N;			 // 编码后的码字长度
extern const int FrameLen;   // 总的码字的长度
extern const int L;

extern const double SNR_BEGIN;  // SNR的开始值
extern const double SNR_END;    // SNR的结束值
extern const int SNR_NUM;       // SNR数量

extern const int NUM_FRAMES;   // 帧数量
extern const int NUM_PRINT;    // 打印显示间隔
extern const int BlockLen;     // 块衰落的长度

extern const string CodeMode;  // 控制编码模式（Polar or None）
extern const bool IsFading;    // 控制衰落模式（全局开关）

extern std::string filename;   // 数据保存的文件名

// 全局变量声明
extern std::vector<double> SNR_dB;                         // SNR值向量（dB）
extern std::vector<double> snr;							   // SNR值（线性）
extern std::vector<std::vector<double>> BER;               // 存储误码率
extern std::vector<std::vector<double>> PUPE;              // 存储PUPE（假设为某种性能评估）

#endif // GLOBAL_VARIABLES_H
