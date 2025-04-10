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

// ����ȫ�ֱ���
const int NUSERS = 24;                      // ��Ծ�û�����
const int NBITS = 256;                       // ÿ���û����͵ı�������
const int SF = 16;                           // ��Ƶ�ı���
const int N = 256;                          // ���������ֳ���(�����CodeMode�޸�,32)
const int FrameLen = N * SF;                // �ܵ����ֵĳ���
const int Nr = 1;                           // ��������
const int L = 32;                           // Polar Code �� list size

const double EbNoSTART = 5;
const double EbNoSTEP = 1;
const int EbNoNUM = 6;

const int NUM_FRAMES = 1000;               // ֡����
const int NUM_PRINT = 10;                  // ��ӡ��ʾ���

const bool IsFading = false;                 // ����˥��ģʽ
const string CodeMode = "None";             // ����IDMA�ı��뷽ʽ��"Polar" for polar coded IDMA;"None" for pure IDMA;��
const int BlockLen = 500;                    // ��˥��ĳ���
const int IDMAitr = 25;                      // IDMA��������


string filename;                            // ���ݱ�����ļ���

vector<double> snr_dB(EbNoNUM);
vector<double> snr(EbNoNUM, 0.0);
vector<double> ebno_dB(EbNoNUM,0.0);                                        
vector<vector<double>> BER(EbNoNUM, vector<double>(NUM_FRAMES, 0.0));
vector<vector<double>> PUPE(EbNoNUM, vector<double>(NUM_FRAMES, 0.0));       // ��ʼ��BER��PUPE

mutex data_mutex;                           // ȫ�ֱ����������ļ�д��Ļ�����

atomic<int> cnt(0);                         // ����ȫ�ֱ������洢����Ĵ���

// ÿ���߳�ִ�е����ؿ�����溯��
void PolarCodeIDMA() {

    // �����߳���˽�б���
    vector<vector<int>> input_data(NUSERS, vector<int>(NBITS));     // �û��������Ϣ
    vector<vector<int>> encoded_data(NUSERS, vector<int>(N));       // ��������Ϣ
    vector<vector<double>> noise(Nr, vector<double>(FrameLen));// ��˹�ŵ�������
    vector<vector<vector<double>>> FadingCoff(Nr, vector<vector<double>>(NUSERS, vector<double>(FrameLen)));// �ŵ�˥��ϵ��
    vector<vector<double>> modulated_data(NUSERS, vector<double>(N));// ���ƺ����Ϣ
    vector<vector<int>> ILidx(NUSERS, vector<int>(NBITS));// �����֯��
    vector<vector<double>> spreaded_data(NUSERS, vector<double>(FrameLen));// ��Ƶ�������
    vector<vector<double>> ILData(NUSERS, vector<double>(FrameLen));// ��֯�������
    vector<vector<double>> RxData(Nr, vector<double>(FrameLen));// ���ջ��Ľ�������
    vector<double> avg_RxData(FrameLen);// �����Ľ��ջ�����
    vector<vector<double>> avg_FadingCoff(NUSERS, vector<double>(FrameLen));// ������˥��ϵ��
    vector<vector<double>> apLLR(NUSERS, vector<double>(FrameLen, 0.0));// ESE��������Ϣ
    vector<vector<double>> extrLLR(NUSERS, vector<double>(FrameLen, 0.0));// ESE�������Ϣ

    vector<vector<double>> deSpData(NUSERS, vector<double>(N));// ����Ƶ�������
    vector<vector<double>> deILData(NUSERS, vector<double>(FrameLen));// �⽻֯�������

    vector<vector<double>> extLLR(NUSERS, vector<double>(FrameLen));
    vector<vector<vector<int>>> output_data(EbNoNUM, vector<vector<int>>(NUSERS, vector<int>(NBITS)));// �û��������Ϣ

    // ��ʼ��polar��������������
    int k = static_cast<int>(log2(N));
    PolarCode pc(k, NBITS, 0, 4, true, false, false, false);

    // ��ʼ������
    GenMessage(input_data);
    GenNoise(noise);
    GenFadingCoff(FadingCoff);
    GenILidx(ILidx);

    ChannelEncode(input_data, pc, encoded_data);
    Modulate(encoded_data, modulated_data);
    spreader(modulated_data, spreaded_data);
    InterLeaver(spreaded_data, ILidx, ILData);

    // ���㲻ͬSNR�µ�������
    for (int i = 0; i < EbNoNUM; ++i) {
        double noise_variance = 1.0 / snr[i];       // ���㵱ǰsnr�µ���������

        channel(ILData, FadingCoff, noise, noise_variance, RxData);
        processMIMOData(RxData, FadingCoff, avg_RxData, avg_FadingCoff);

        // IDMA������ѭ��
        for (int r = 0; r < IDMAitr; ++r) {

            calESE(avg_RxData, apLLR, avg_FadingCoff, noise_variance, extrLLR);     // ����ESE
            deInterleaver(extrLLR, ILidx, deILData);                                  // �⽻֯
            despreader(deILData, deSpData);                                          // ����Ƶ

            spreader(deSpData, spreaded_data);                                      // ��Ƶ

            for (size_t user = 0; user < NUSERS; ++user) {
                for (size_t i = 0; i < FrameLen; ++i) {
                    extLLR[user][i] = spreaded_data[user][i] - deILData[user][i];   // ����֮������ֵ
                }
            }

            extLLR = spreaded_data;

            InterLeaver(extLLR, ILidx, apLLR);                                       // ��֯
        }

        ChannelDecoder(deSpData, output_data, i, pc);

        // hardDecision(deSpData, output_data, i);                                      // ����Ӳ�о�
    }

    // �ļ��������򣬼��������ʺʹ�ӡ�����ʹ���ֻ����һ���߳���ִ��
    {
        std::lock_guard<std::mutex> lock(data_mutex);
                                    
        calcError(output_data, input_data, cnt); // ����������
        if (cnt % NUM_PRINT == 0) {
            PrintToConsole(cnt);    // �����ӡ����ʱ����ӡһ�ν��
        }
        if (cnt == NUM_FRAMES - 1) {
            PrintToConsole(cnt);
            WriteToFile();          // ���з�����ɺ󣬰ѽ��д�뵽�ļ�����
        }
        cnt++;        // ����������ɺ����һ��
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
            InputData[j][i] = bit_dist(rng);
        }
    }

    // ���㲻ͬSNR�µ�������
    for (int q = 0; q < EbNoNUM; ++q) {
        double sigma = sqrt(1.0 / snr[q]);    // ���㵱ǰsnr�µ���������

        auto Tx = Transmitter(InputData, ScrambleRule, SpreadSeq);
        auto Rx = Channel(sigma, Noise, H, Tx);
        auto AppLlr = Receiver(sigma, IDMAitr, ScrambleRule, SpreadSeq, H, Rx);

        for (int j = 0; j < NUSERS; j++)
            transform(AppLlr[j].begin(), AppLlr[j].end(), OutputData[q][j].begin(),
                [](double x) { return (x >= 0.0) ? 0 : 1; });

    }

    // �ļ��������򣬼��������ʺʹ�ӡ�����ʹ���ֻ����һ���߳���ִ��
    {
        std::lock_guard<std::mutex> lock(data_mutex);
        calcError(OutputData, InputData, cnt); // ����������
        if (cnt % NUM_PRINT == 0) {
            PrintToConsole(cnt);    // �����ӡ����ʱ����ӡһ�ν��
        }
        if (cnt == NUM_FRAMES - 1) {
            PrintToConsole(cnt);
            WriteToFile();          // ���з�����ɺ󣬰ѽ��д�뵽�ļ�����
        }
        cnt++;        // ����������ɺ����һ��
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
            pool.enqueue(PolarCodeIDMA); // ������ŵ��̳߳�����
        }
    }
    else {
        std::cout << "��������ȷ�ı��뷽ʽ�ֶ�" << std::endl;
    }

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




