#include "global_variables.h"
#include "IDMA.h"
#include "Threadpool.h"
#include "INIT.h"
#include <iostream>
#include <string>
using namespace std;

//����ȫ�ֱ���

const int NUSERS = 2;                       // ��Ծ�û�����
const int NBITS = 10;                       // ÿ���û����͵ı�������
const int SF = 300;                         // ��Ƶ�ı���
const int N = 10;                           // ���������ֳ���
const int FrameLen = N * SF;                // �ܵ����ֵĳ���
const int Nr = 1;                           // ��������

const double SNR_BEGIN = -40;
const double SNR_END = -20;
const int SNR_NUM = 3;

const int NUM_FRAMES = 100000;              // ֡����
const int NUM_PRINT = 100;                  // ��ӡ��ʾ���

const int BlockLen = 50;                    // ��˥��ĳ���
const int IDMAitr = 15;                     // IDMA��������

string filename;                            // ���ݱ�����ļ���

vector<double> SNR_dB(SNR_NUM);                                              // SNRֵ������dB��
vector<double> snr(SNR_NUM, 0.0);                                            // snrֵ�����ԣ�
vector<vector<double>> BER(SNR_NUM, vector<double>(NUM_FRAMES, 0.0));
vector<vector<double>> PUPE(SNR_NUM, vector<double>(NUM_FRAMES, 0.0));       // ��ʼ��BER��PUPE


mutex data_mutex;                           // ȫ�ֱ����������ļ�д��Ļ�����

atomic<int> cnt(0);                         // ����ȫ�ֱ������洢����Ĵ���


// ÿ���߳�ִ�е����ؿ�����溯��
void montecarlosimulation() {

    // �����߳���˽�б���
    vector<vector<int>> input_data(NUSERS, vector<int>(NBITS));     // �û��������Ϣ
    vector<vector<double>> noise(Nr, vector<double>(FrameLen));// ��˹�ŵ�������
    vector<vector<vector<double>>> FadingCoff(Nr, vector<vector<double>>(NUSERS, vector<double>(FrameLen)));// �ŵ�˥��ϵ��
    vector<vector<double>> modulated_data(NUSERS, vector<double>(NBITS));// ���ƺ����Ϣ
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
    vector<vector<vector<int>>> output_data(SNR_NUM, vector<vector<int>>(NUSERS, vector<int>(NBITS)));// �û��������Ϣ

    // ��ʼ������
    GenMessage(input_data);
    GenNoise(noise);
    GenFadingCoff(FadingCoff);
    GenILidx(ILidx);

    Modulate(input_data, modulated_data);
    spreader(modulated_data, spreaded_data);
    InterLeaver(spreaded_data, ILidx, ILData);

    //deInterleaver(ILData, ILidx, deILData);
    //despreader(deILData, despreadedData);

    // ���㲻ͬSNR�µ�������
    for (int i = 0; i < SNR_NUM; ++i) {
        double noise_variance = 1.0 / pow(10.0, snr[i] / 10.0);    // ���㵱ǰsnr�µ���������

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

            InterLeaver(extLLR, ILidx, apLLR);                                      // ��֯
        }

        hardDecision(deSpData, output_data, i);                                      // ����Ӳ�о�
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

int main() {

    ThreadPool pool(8);     // ʹ�õ��߳�����

    OpenDataFile();         // �����ݴ洢�ļ�
    GenSNR();               // ���ɴ������snr����

    // ��ӡ��ͷ
    PrintHeader();

    // ���̲߳���
    for (int sim = 0; sim < NUM_FRAMES; ++sim) {

        pool.enqueue(montecarlosimulation); // ������ŵ��̳߳�����


    }
    return 0;
}

//
//int main() {
//
//    // �����߳���˽�б���
//    vector<vector<int>> input_data(NUSERS, vector<int>(NBITS));     // �û��������Ϣ
//    vector<vector<double>> noise(Nr, vector<double>(FrameLen));// ��˹�ŵ�������
//    vector<vector<vector<double>>> FadingCoff(Nr, vector<vector<double>>(NUSERS, vector<double>(FrameLen)));// �ŵ�˥��ϵ��
//    vector<vector<double>> modulated_data(NUSERS, vector<double>(NBITS));// ���ƺ����Ϣ
//    vector<vector<int>> ILidx(NUSERS, vector<int>(NBITS));// �����֯��
//    vector<vector<double>> spreaded_data(NUSERS, vector<double>(FrameLen));// ��Ƶ�������
//    vector<vector<double>> ILData(NUSERS, vector<double>(FrameLen));// ��֯�������
//    vector<vector<double>> RxData(Nr, vector<double>(FrameLen));// ���ջ��Ľ�������
//    vector<double> avg_RxData(FrameLen);// �����Ľ��ջ�����
//    vector<vector<double>> avg_FadingCoff(NUSERS, vector<double>(FrameLen));// ������˥��ϵ��
//    vector<vector<double>> apLLR(NUSERS,vector<double>(FrameLen,0.0));// ESE��������Ϣ
//    vector<vector<double>> extrLLR(NUSERS, vector<double>(FrameLen, 0.0));// ESE�������Ϣ
//
//    vector<vector<double>> deSpData(NUSERS, vector<double>(N));// ����Ƶ�������
//    vector<vector<double>> deILData(NUSERS, vector<double>(FrameLen));// �⽻֯�������
//
//    vector<vector<double>> extLLR(NUSERS, vector<double>(FrameLen));
//    vector<vector<vector<int>>> output_data(SNR_NUM, vector<vector<int>>(NUSERS, vector<int>(NBITS)));// �û��������Ϣ
//
//    // ��ʼ������
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
//    // ���㲻ͬSNR�µ�������
//    for (int i = 0; i < SNR_NUM; ++i) {
//        double noise_variance = 1.0 / pow(10.0, snr[i] / 10.0);    // ���㵱ǰsnr�µ���������
//
//        channel(ILData, FadingCoff, noise, noise_variance, RxData);
//        processMIMOData(RxData, FadingCoff, avg_RxData, avg_FadingCoff);
//
//        // IDMA������ѭ��
//        for (int r = 0; r < IDMAitr; ++r) {
//
//            calESE(avg_RxData, apLLR, avg_FadingCoff, noise_variance, extrLLR);     // ����ESE
//            deInterleaver(extrLLR,ILidx,deILData);                                  // �⽻֯
//            despreader(deILData,deSpData);                                          // ����Ƶ
//
//            spreader(deSpData, spreaded_data);                                      // ��Ƶ
//
//            for (size_t user = 0; user < NUSERS; ++user) {
//                for (size_t i = 0; i < FrameLen; ++i) {
//                    extLLR[user][i] = spreaded_data[user][i] - deILData[user][i];   // ����֮������ֵ
//                }
//            }
//
//            InterLeaver(extLLR, ILidx, apLLR);                                      // ��֯
//        }
//
//        hardDecision(deSpData, output_data,i);                                      // ����Ӳ�о�
//    }
//
//    return 0; 
//}



