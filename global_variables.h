#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H


#define		STEP(x)		( (x) > 0 ? 0 : 1 )


#include <vector>
#include <string>
#include <atomic>

using namespace std;

// 全局常量声明
extern const int NUSERS;     // 活跃用户数量
extern const int NBITS;      // 每个用户发送的比特数量
extern const int SF;         // 扩频的倍数
extern const int Nr;         // 接收机天线数量
extern const int Nt;         // 发送机天线数量
extern const int N;			 // 编码后的码字长度
extern const int FrameLen;   // 总的码字的长度
extern const int L;

extern const double EbNoSTART;  // SNR的开始值
extern const double EbNoSTEP;    // SNR的结束值
extern const int EbNoNUM;       // SNR数量

extern const int NUM_FRAMES;   // 帧数量
extern const int NUM_PRINT;    // 打印显示间隔
extern const int BlockLen;     // 块衰落的长度

extern const string CodeMode;  // 控制编码模式（Polar or None）
extern const bool IsFading;    // 控制衰落模式（全局开关）

extern std::string filename;   // 数据保存的文件名

// 全局变量声明
extern std::vector<double> snr_dB;                         // SNR值向量（dB）
extern std::vector<double> ebno_dB;                        // SNR值向量（dB）
extern std::vector<double> snr;							   // SNR值（线性）
extern std::vector<double> total_BER;
extern std::vector<double> total_PUPE;

// 定义全局院子变量
extern std::atomic<int> cnt;

#endif // GLOBAL_VARIABLES_H
