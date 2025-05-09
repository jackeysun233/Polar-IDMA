#include "global_variables.h"
#include "IDMA.h"
#include "Threadpool.h"
#include "INIT.h"
#include "PolarCode.h"
#include <iostream>
#include <string>
#include <bits/stdc++.h>
#include <random>
#include <chrono>
using namespace std;
using namespace std::chrono;

// ����ȫ�ֱ���
const int NUSERS = 1;                       // ��Ծ�û�����
const int NBITS = 10;                       // ÿ���û����͵ı�������
const int SF = 833;                         // ��Ƶ�ı���
const int N = 10;                           // ���������ֳ���(�����CodeMode�޸�,32)
const int FrameLen = N * SF;                // �ܵ����ֵĳ���
const int Nr = 2;                           // ���ջ���������
const int Nt = 1;                           // ���ͻ���������
const int L = 32;                           // Polar Code �� list size

const double EbNoSTART = 7;
const double EbNoSTEP = 1;
const int EbNoNUM = 2;

const int NUM_FRAMES = 10000;               // ֡����
const int NUM_PRINT = 100;                   // ��ӡ��ʾ���

const bool IsFading = true;                 // ����˥��ģʽ
const string CodeMode = "None";             // ����IDMA�ı��뷽ʽ��"Polar" for polar coded IDMA;"None" for pure IDMA;��
const int IDMAitr = 15;                     // IDMA��������

const int BlockLen = 500;                               // ��˥��ĳ���
const int BlockNum = round(FrameLen / BlockLen);        // ˥��������


string filename;                            // ���ݱ�����ļ���

vector<double> snr_dB(EbNoNUM);
vector<double> snr(EbNoNUM, 0.0);
vector<double> ebno_dB(EbNoNUM,0.0);
vector<double> total_BER(EbNoNUM, 0.0);
vector<double> total_PUPE(EbNoNUM, 0.0);

mutex data_mutex;                           // ȫ�ֱ����������ļ�д��Ļ�����

atomic<int> cnt(0);                         // ����ȫ�ֱ������洢����Ĵ���

// ÿ���߳�ִ�е����ؿ�����溯��
void PolarCodeIDMA(
    const vector<int>& SpreadSeq,
    const vector<vector<int>> ScrambleRule
) {

    vector<vector<int>> InputData(NUSERS, vector<int>(NBITS, 0));
    vector<vector<int>> EncodedData(NUSERS, vector<int>(N, 0));

    vector<vector<int>> OutputData(NUSERS, vector<int>(NBITS, 0));
    vector<double> BER(EbNoNUM, 0.0);
    vector<double> PUPE(EbNoNUM, 0.0);

    vector<double> Noise(FrameLen, 0.0);
    vector<vector<double>> H(NUSERS, vector<double>(FrameLen, 1.0));

    // ���������ź�����

    mt19937 rng(random_device{}());
    uniform_int_distribution<int> bit_dist(0, 1);

    // ������������
    random_device rd;                   // ��������
    mt19937 generator(rd());            // Mersenne Twister ����
    normal_distribution<double> distribution(0.0, 1.0);  // ��ֵΪ0����׼��Ϊ1����̬�ֲ�


    // ��������
    for (int i = 0; i < FrameLen; ++i) {
        Noise[i] = distribution(generator);
    }

    // ���������ź�
    for (int i = 0; i < NBITS; ++i)
    {
        for (int j = 0; j < NUSERS; ++j)
        {
            // InputData[j][i] = bit_dist(rng);
            InputData[j][i] = 0;
        }
    }

    // �ŵ�����
    // ��ʼ��polar��������������
    int k = static_cast<int>(log2(N));
    PolarCode pc(k, NBITS, 0, 4, true, false, false, false);

    for (int j = 0; j < NUSERS; ++j) {
        // ����������ת��Ϊ uint8_t ����
        vector<uint8_t> tmp(InputData[j].size());
        transform(InputData[j].begin(), InputData[j].end(), tmp.begin(), [](int x) {
            return static_cast<uint8_t>(x);
            });

        // ��ȡ����������
        vector<uint8_t> encoded = pc.encode(tmp);

        // ������������ת��Ϊ int ���Ͳ��洢�� encoded_data ��
        for (size_t i = 0; i < encoded.size(); ++i) {
            EncodedData[j][i] = static_cast<int>(encoded[i]);  // ת��Ϊ int ���洢
        }
    }
    
    // ���㲻ͬSNR�µ�������
    for (int q = 0; q < EbNoNUM; ++q) {
        double sigma = sqrt(1.0 / snr[q]);    // ���㵱ǰsnr�µ���������

        auto Tx = Transmitter(EncodedData, ScrambleRule, SpreadSeq);
        auto Rx = Channel(sigma, Noise, H, Tx);
        auto AppLlr = Receiver(sigma, IDMAitr, ScrambleRule, SpreadSeq, H, Rx);

        // �ŵ���������
        for (size_t user = 0; user < NUSERS; ++user) {
            // ����PolarCode��SCL������
            auto decoded = pc.decode_scl_llr(AppLlr[user], L); // ����list_sizeΪ8

            transform(decoded.begin(), decoded.end(), OutputData[user].begin(), 
                [](auto x) {return static_cast<int>(x); });
        }

        int errnum = 0;
        double ber = 0.0;
        vector<int> errusr(NUSERS, 0);
        for (int j = 0; j < NUSERS; j++) for (int k = 0; k < NBITS; k++) {
            if (InputData[j][k] != OutputData[j][k]) {
                errnum++;
                errusr[j] = 1;
            }
        }

        ber = static_cast<double>(errnum) / (NUSERS * NBITS);
        double pupe = accumulate(errusr.begin(), errusr.end(), 0) / static_cast<double>(NUSERS);

        BER[q] = ber;
        PUPE[q] = pupe;

    }

    // �ļ��������򣬼��������ʺʹ�ӡ�����ʹ���ֻ����һ���߳���ִ��
    {
        std::lock_guard<std::mutex> lock(data_mutex);

        for (int q = 0; q < EbNoNUM; q++) {
            total_BER[q] = (BER[q] + total_BER[q] * cnt) / static_cast<double>(cnt + 1);
            total_PUPE[q] = (PUPE[q] + total_PUPE[q] * cnt) / static_cast<double>(cnt + 1);
        }

        if (cnt % NUM_PRINT == 0) {
            PrintToConsole();
        }
        cnt++;
    }


}

void PureIDMA(
    const vector<int>& SpreadSeq,
    const vector<vector<int>> ScrambleRule
    ) {

    vector<vector<int>> InputData(NUSERS, vector<int>(NBITS, 0));
    vector<vector<int>> EncodedData(NUSERS, vector<int>(N, 0));

    vector<vector<int>> OutputData(NUSERS, vector<int>(NBITS, 0));
    vector<double> BER(EbNoNUM, 0.0);
    vector<double> PUPE(EbNoNUM, 0.0);

    vector<double> Noise(FrameLen, 0.0);
    vector<vector<double>> H(NUSERS, vector<double>(FrameLen, 1.0));

    // ���������ź�����

    mt19937 rng(random_device{}());
    uniform_int_distribution<int> bit_dist(0, 1);

    // ������������
    random_device rd;                   // ��������
    mt19937 generator(rd());            // Mersenne Twister ����
    normal_distribution<double> distribution(0.0, 1.0);  // ��ֵΪ0����׼��Ϊ1����̬�ֲ�
    normal_distribution<double> rayleigh(0.0, 1.0/sqrt(2.0));  // ��ֵΪ0����׼��Ϊ1����̬�ֲ�

    // ��������
    for (int i = 0; i < FrameLen; ++i) {
        Noise[i] = distribution(generator);
    }

    // ���������ź�
    for (int i = 0; i < NBITS; ++i)
    {
        for (int j = 0; j < NUSERS; ++j)
        {
            InputData[j][i] = bit_dist(rng);
        }
    }

    for (int q = 0; q < EbNoNUM; ++q) {
        double sigma = sqrt(1.0 / snr[q]);    // ���㵱ǰsnr�µ���������

        // �����ŵ�˥��ϵ��

        int diversity = Nt * Nr; // �ŵ��ּ����������ջ�����x���ͻ�����
        int energy = Nr; // ���ӷ��ͻ����ߣ����ӽ��ջ��ռ�����

        if (IsFading) {
            for (int j = 0; j < NUSERS; j++) {
                for (int kk = 0; kk < BlockNum; kk++) {

                    double h_tmp = 0.0;

                    for (int d = 0; d < diversity; ++d) {
                        auto U1 = rayleigh(generator);
                        auto U2 = rayleigh(generator);

                        h_tmp += sqrt(U1 * U1 + U2 * U2);
                    }

                    auto U1 = rayleigh(generator);
                    auto U2 = rayleigh(generator);
                    for (int ii = 0; ii < BlockLen; ii++)
                        H[j][BlockLen * kk + ii] = sqrt(energy) * h_tmp/diversity;
                }
                if (BlockLen * BlockNum < FrameLen) {
                    int RemainLen = FrameLen - BlockLen * BlockNum;

                    double h_tmp = 0.0;

                    for (int d = 0; d < diversity; ++d) {
                        auto U1 = rayleigh(generator);
                        auto U2 = rayleigh(generator);

                        h_tmp += sqrt(U1 * U1 + U2 * U2);
                    }

                    for (int ii = 0; ii < RemainLen; ii++)
                        H[j][BlockLen * BlockNum + ii] = sqrt(energy) * h_tmp/diversity;
                }
            }
        }

        auto Tx = Transmitter(InputData, ScrambleRule, SpreadSeq);
        auto Rx = Channel(sigma, Noise, H, Tx);
        auto AppLlr = Receiver(sigma, IDMAitr, ScrambleRule, SpreadSeq, H, Rx);

        for (int j = 0; j < NUSERS; j++) 
            transform(AppLlr[j].begin(), AppLlr[j].end(), OutputData[j].begin(),
                [](double x) { return (x >= 0.0) ? 0 : 1; });  

        int errnum = 0;
        double ber = 0.0;
        vector<int> errusr(NUSERS, 0);

        for (int j = 0; j < NUSERS; j++) for (int k = 0; k < NBITS; k++) {
            if (InputData[j][k] != OutputData[j][k]) {
                errnum++;
                errusr[j] = 1;
            }
        }

        ber = static_cast<double>(errnum) / (NUSERS * NBITS);
        double pupe = accumulate(errusr.begin(), errusr.end(), 0) / static_cast<double>(NUSERS);

        BER[q] = ber;
        PUPE[q] = pupe;
    }

    // �ļ��������򣬼��������ʺʹ�ӡ�����ʹ���ֻ����һ���߳���ִ��
    {
        std::lock_guard<std::mutex> lock(data_mutex);

        for (int q = 0; q < EbNoNUM; q++) {
            total_BER[q] = (BER[q] + total_BER[q] * cnt) / static_cast<double>(cnt + 1);
            total_PUPE[q] = (PUPE[q] + total_PUPE[q] * cnt) / static_cast<double>(cnt + 1);
        }

        if (cnt % NUM_PRINT == 0) {
            PrintToConsole();
        }
        cnt++;
    }
}



int main() {
    ThreadPool pool(8);     // ʹ�õ��߳�����

    OpenDataFile();         // �����ݴ洢�ļ�
    GenSNR();               // ���ɴ������SNR����
    PrintHeader();          // ��ӡ��ͷ

    vector<int> SpreadSeq(SF); // ��Ƶ����
    vector<vector<int>> ScrambleRule(NUSERS, vector<int>(FrameLen)); // ��֯��
    vector<vector<int>> InputData(NUSERS, vector<int>(NBITS, 0));
    vector<vector<int>> EncodedData(NUSERS, vector<int>(N, 0));

    // ������Ƶ���� {+1, -1, +1, -1, ... }.
    for (int i = 0; i < SF; i++) SpreadSeq[i] = 1 - 2 * (i % 2);

    // ���ɽ�֯������
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
        // ���̲߳���
        for (int sim = 0; sim < NUM_FRAMES; ++sim) {
            pool.enqueue(PureIDMA,SpreadSeq,ScrambleRule); // ������ŵ��̳߳�����
        }
    }
    else if (CodeMode == "Polar") {  // ���������������
        // ���̲߳���
        for (int sim = 0; sim < NUM_FRAMES; ++sim) {
            pool.enqueue(PolarCodeIDMA, SpreadSeq, ScrambleRule); // ������ŵ��̳߳�����
        }
    }
    else {
        std::cout << "��������ȷ�ı��뷽ʽ�ֶ�" << std::endl;
    }

    system("pause");

    return 0;
}

// ���ŵ������������

//int main() {
//
//    // ������ʼ������
//
//    OpenDataFile();         // �����ݴ洢�ļ�
//    GenSNR();               // ���ɴ������snr����
//    PrintHeader();          // ��ӡ��ͷ
//
//    vector<int> SpreadSeq(SF); // ��Ƶ����
//    vector<vector<int>> ScrambleRule(NUSERS, vector<int>(FrameLen)); // ��֯��
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
//    // ������Ƶ���� {+1, -1, +1, -1, ... }.
//    for (int i = 0; i < SF; i++) SpreadSeq[i] = 1 - 2 * (i % 2);
//
//    // ���ɽ�֯������
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
//    // ���������ź�����
//
//    mt19937 rng(random_device{}());
//    uniform_int_distribution<int> bit_dist(0, 1);
//
//    // ������������
//    random_device rd;                   // ��������
//    mt19937 generator(rd());            // Mersenne Twister ����
//    normal_distribution<double> distribution(0.0, 1.0);  // ��ֵΪ0����׼��Ϊ1����̬�ֲ�
//
//
//
//    for (int sim = 0; sim < NUM_FRAMES; ++sim) {
//
//        // ��������
//        for (int i = 0; i < FrameLen; ++i) {
//            Noise[i] = distribution(generator);
//        }
//
//        // ���������ź�
//        for (int i = 0; i < NBITS; ++i)
//        {
//            for (int j = 0; j < NUSERS; ++j)
//            {
//                InputData[j][i] = bit_dist(rng);
//            }
//        }
//
//        // �ŵ�����
//        //if (CodeMode == "Polar") {
//        //    // ��ʼ��polar��������������
//        //    int k = static_cast<int>(log2(N));
//        //    PolarCode pc(k, NBITS, 0, 4, true, false, false, false);
//
//        //    for (int j = 0; j < NUSERS; ++j) {
//        //        // ����������ת��Ϊ uint8_t ����
//        //        vector<uint8_t> tmp(InputData[j].size());
//        //        transform(InputData[j].begin(), InputData[j].end(), tmp.begin(), [](int x) {
//        //            return static_cast<uint8_t>(x);
//        //            });
//
//        //        // ��ȡ����������
//        //        vector<uint8_t> encoded = pc.encode(tmp);
//
//        //        // ������������ת��Ϊ int ���Ͳ��洢�� encoded_data ��
//        //        for (size_t i = 0; i < encoded.size(); ++i) {
//        //            EncodedData[j][i] = static_cast<int>(encoded[i]);  // ת��Ϊ int ���洢
//        //        }
//        //    }
//
//        //}
//
//        // ���㲻ͬSNR�µ�������
//        for (int q = 0; q < EbNoNUM; ++q) {
//            double sigma = sqrt(1.0 / snr[q]);    // ���㵱ǰsnr�µ���������
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
//            // �ļ��������򣬼��������ʺʹ�ӡ�����ʹ���ֻ����һ���߳���ִ��
//            {
//
//                calcError(OutputData, InputData, cnt); // ����������
//                if (cnt % NUM_PRINT == 0) {
//                    PrintToConsole(cnt);    // �����ӡ����ʱ����ӡһ�ν��
//                }
//                if (cnt == NUM_FRAMES - 1) {
//                    PrintToConsole(cnt);
//                    WriteToFile();          // ���з�����ɺ󣬰ѽ��д�뵽�ļ�����
//                }
//                cnt++;        // ����������ɺ����һ��
//            }
//    }
//
//    return 0;
//}




