#include "IDMA.h"
using namespace std;

//声明全局变量

const int nUsers = 1;//活跃用户数量
const int nBits = 10;//每个用户发送的比特数量
const int nRuns = 50000;//蒙特卡洛仿真的数量
const int SF = 300;//扩频的倍数

const int BlockLen = 1500;// 块衰落的长度


