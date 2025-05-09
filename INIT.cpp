#include <vector>
#include <random>
#include <fstream>
#include <iostream>
#include <iomanip>  
#include <string>
#include "global_variables.h"

using namespace std;

// 计算SNR的值
void GenSNR() {
    // 计算 SNR 的步长
    double step = (SNR_NUM > 1) ? (SNR_END - SNR_BEGIN) / (SNR_NUM - 1) : 0.0;

    //// 初始化 SNR 值向量
    //vector<double> snr(SNR_NUM, 0.0);

    // 计算 SNR 的具体取值
    for (int i = 0; i < SNR_NUM; ++i) {
        SNR_dB[i] = SNR_BEGIN + i * step;                // SNR 的 dB 值
        snr[i] = pow(10, SNR_dB[i] / 10.0);       // 转换为线性单位
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

// 生成衰落系数向量
void GenFadingCoff(vector<vector<vector<double>>>& FadingCoff) {
    // 检查条件 NUSERS * NBITS < M
    if (NUSERS * NBITS > FrameLen) {
        throw invalid_argument("错误：NUSERS * NBITS 必须小于等于 FrameLen。");
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

    // 如果 IsFading 为 false，所有衰落系数设置为 1
    if (!IsFading) {
        for (int nr = 0; nr < Nr; ++nr) {
            for (int user = 0; user < NUSERS; ++user) {
                fill(FadingCoff[nr][user].begin(), FadingCoff[nr][user].end(), 1.0);
            }
        }
    }
    else {
        // 为每个天线和每个用户生成衰落系数
        for (int nr = 0; nr < Nr; ++nr) {
            for (int user = 0; user < NUSERS; ++user) {
                // 每个块的衰落系数
                uniform_real_distribution<double> uniformDist(0.0, 1.0);
                for (int block = 0; block < num_blocks; ++block) {

                    double h_tmp = 0.0;
                    for (int nt = 0; nt < Nt; ++nt) {
                        double U = uniformDist(rng);
                        double h = sqrt(0.5) * sqrt(-2.0 * log(U)); // 瑞利分布的生成方式

                        h_tmp += h;
                    }

                    //double U = uniformDist(rng);
                    //double h = sqrt(0.5)*sqrt(-2.0 * log(U)); // 瑞利分布的生成方式

                    // 填充对应的衰落系数
                    for (int i = block * BlockLen; i < (block + 1) * BlockLen && i < FrameLen; ++i) {
                        FadingCoff[nr][user][i] = h_tmp/Nt; // 用生成的衰落系数填充每个用户的每个符号
                    }
                }
            }
        }
    }
}

// 获取当前日期
std::string GetCurrentDate() {
    time_t t = time(nullptr);
    tm current_time;
    localtime_s(&current_time, &t);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y%m%d", &current_time);
    return std::string(buffer);
}

// 获取当前时间字符串
std::string GetCurrentTimeString() {
    time_t t = time(nullptr);
    tm current_time;
    localtime_s(&current_time, &t); // 使用安全版本localtime_s
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &current_time);
    return std::string(buffer);
}

// 创建或打开数据文件
void OpenDataFile() {
    // 使用当前日期生成文件名
    filename = GetCurrentDate() + "_data.txt";

    // 创建输出文件流
    std::ofstream outfile;

    // 检查文件是否存在
    if (std::ifstream(filename).good()) {
        // 文件存在，以追加模式打开文件
        outfile.open(filename, std::ios::app);
    }
    else {
        // 文件不存在，创建新文件
        outfile.open(filename);
    }

    // 检查文件是否成功打开
    if (!outfile.is_open()) {
        std::cerr << "无法创建文件: " << filename << std::endl;
    }
    outfile.close(); // 关闭文件
}

// 打印控制台表头
void PrintHeader() {

    // 在控制台打印头信息
    cout << "仿真开始时间: " << GetCurrentTimeString() << "\n";

    cout << left << setw(15) << "用户数量:" << NUSERS << "\n"
        << left << setw(15) << "用户信息长度:" << NBITS << "\n"
        << left << setw(15) << "接收天线数量:" << Nr << "\n"
        << left << setw(15) << "功率分配:" << SF << "\n"
        << left << setw(15) << "编码:" << "K = " << NBITS << ", N = " << N << "\n";
    
    // 根据CodeMode值打印编码模式
    if (CodeMode == "None") {
        std::cout << std::left << std::setw(15) << "编码模式:" << "None" << "\n";
    }
    else if (CodeMode == "Polar") {
        std::cout << std::left << std::setw(15) << "编码模式:" << "Polar" << "\n";
    }

    // 根据IsFading值打印块衰落长度
    if (IsFading == true) {
        std::cout << std::left << std::setw(15) << "衰落块长度:" << BlockLen << "\n";
    }
    else if (IsFading == false) {
        std::cout << std::left << std::setw(15) << "信道模型:" << "AWGN" << "\n";
    }

}

// 打印数据到控制台（动态刷新）
void PrintToConsole(int sim) {
    // ANSI 转义序列，用于移动光标到控制台顶部
    // 假设控制台已经有固定的行数用于显示结果
    // 这里假设需要刷新大约 10 行，可以根据实际情况调整
    // ANSI 转义序列：移动光标到表头下方
    const int header_lines = 9; // 表头占用的行数，包括 SNR 横向表头和分隔线
    cout << "\033[" << header_lines + 1 << "H"; // 移动光标到表头下方

    // 打印仿真次数和数据
    cout << "当前仿真次数: " << (sim + 1) << "\n";

    // 打印 SNR 表头
    cout << left << setw(10) << "SNR";
    for (int i = 0; i < SNR_NUM; ++i) {
        cout << setw(12) << fixed << setprecision(2) << SNR_dB[i];
    }
    cout << "\n" << string(10 + SNR_NUM * 12, '-') << "\n";

    // 打印 BER 数据行
    cout << left << setw(10) << "BER";
    for (int i = 0; i < SNR_NUM; ++i) {
        double avg_BER = 0.0;
        for (int j = 0; j < BER[i].size(); ++j) {
            avg_BER += BER[i][j];
        }
        avg_BER /= (sim + 1); // 计算平均 BER
        cout << setw(12) << scientific << setprecision(2) << avg_BER;
    }
    cout << "\n";

    // 打印 PUPE 数据行
    cout << left << setw(10) << "PUPE";
    for (int i = 0; i < SNR_NUM; ++i) {
        double avg_PUPE = 0.0;
        for (int j = 0; j < PUPE[i].size(); ++j) {
            avg_PUPE += PUPE[i][j];
        }
        avg_PUPE /= (sim + 1); // 计算平均 PUPE
        cout << setw(12) << scientific << setprecision(2) << avg_PUPE;
    }
    cout << "\n";

    // 打印累计错误比特数量
    cout << left << setw(10) << "ErrorBits";
    for (int i = 0; i < SNR_NUM; ++i) {
        int total_errors = 0;
        for (int j = 0; j <= sim; ++j) { // 计算累计错误的比特数量
            total_errors += static_cast<int>(BER[i][j] * NUSERS * NBITS); // 将误码率转换为错误比特数量
        }
        cout << setw(12) << total_errors;
    }
    cout << "\n";

    // 打印分隔线
    cout << string(10 + SNR_NUM * 12, '=') << "\n\n";
}

// 向数据存储文件写入数据
void WriteToFile() {
    // 打开文件
    ofstream outfile(filename, ios::app);
    if (!outfile.is_open()) {
        cerr << "无法打开文件: " << filename << endl;
        return;
    }

    // 写入仿真头信息
    outfile << "仿真结束时间: " << GetCurrentTimeString() << "\n";

    outfile << left << setw(15) << "用户数量:" << NUSERS << "\n"
        << left << setw(15) << "用户信息长度:" << NBITS << "\n"
        << left << setw(15) << "接收天线数量:" << Nr << "\n"
        << left << setw(15) << "功率分配:" << SF << "\n"
        << left << setw(15) << "编码:" << "K = " << NBITS << ", N = " << N << "\n"
        << left << setw(15) << "衰落块长度:" << BlockLen << "\n\n";

    // 写入 SNR 表头
    outfile << left << setw(10) << "SNR";
    for (int i = 0; i < SNR_NUM; ++i) {
        outfile << setw(12) << fixed << setprecision(2) << SNR_dB[i];
    }
    outfile << "\n" << string(10 + SNR_NUM * 12, '-') << "\n";

    // 写入 BER 数据行
    outfile << left << setw(10) << "BER";
    for (int i = 0; i < SNR_NUM; ++i) {
        double avg_BER = 0.0;
        for (int j = 0; j < BER[i].size(); ++j) {
            avg_BER += BER[i][j];
        }
        avg_BER /= (NUM_FRAMES + 1); // 计算平均 BER
        outfile << setw(12) << fixed << setprecision(8) << avg_BER; // 文件输出保留8位小数
    }
    outfile << "\n";

    // 写入 PUPE 数据行
    outfile << left << setw(10) << "PUPE";
    for (int i = 0; i < SNR_NUM; ++i) {
        double avg_PUPE = 0.0;
        for (int j = 0; j < PUPE[i].size(); ++j) {
            avg_PUPE += PUPE[i][j];
        }
        avg_PUPE /= (NUM_FRAMES + 1); // 计算平均 PUPE
        outfile << setw(12) << fixed << setprecision(8) << avg_PUPE; // 文件输出保留8位小数
    }
    outfile << "\n";

    // 写入累计错误比特数量
    outfile << left << setw(10) << "ErrorBits";
    for (int i = 0; i < SNR_NUM; ++i) {
        int total_errors = 0;
        for (int j = 0; j < BER[i].size(); ++j) { // 累计错误比特数量
            total_errors += static_cast<int>(BER[i][j] * NUSERS * NBITS);
        }
        outfile << setw(12) << total_errors;
    }
    outfile << "\n";

    // 写入分隔线
    outfile << string(10 + SNR_NUM * 12, '=') << "\n\n";

    outfile.close(); // 关闭文件
}
