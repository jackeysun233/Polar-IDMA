#include "global_variables.h"
#include "IDMA.h"
#include "Threadpool.h"
#include "INIT.h"
#include <iostream>
#include <string>
using namespace std;

//����ȫ�ֱ���

const int NUSERS = 1;                       // ��Ծ�û�����
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

const int BlockLen = 50;                  // ��˥��ĳ���

string filename;                            // ���ݱ�����ļ���

vector<double> SNR_dB(SNR_NUM);                                              // SNRֵ������dB��
vector<double> snr(SNR_NUM, 0.0);                                            // snrֵ�����ԣ�
vector<vector<double>> BER(SNR_NUM, vector<double>(NUM_FRAMES, 0.0));
vector<vector<double>> PUPE(SNR_NUM, vector<double>(NUM_FRAMES, 0.0));       // ��ʼ��BER��PUPE


mutex data_mutex;                           // ȫ�ֱ����������ļ�д��Ļ�����

atomic<int> cnt(0);                         // ����ȫ�ֱ������洢����Ĵ���


//// ÿ���߳�ִ�е����ؿ�����溯��
//void MonteCarloSimulation() {
//
//    // �����߳���˽�б���
//    vector<vector<int>> input_data(NUSERS, vector<int>(NBITS));     // �û��������Ϣ
//    vector<vector<double>> noise_parity(Nr,vector<double>(FrameLen));// ��˹�ŵ�������
//    vector<vector<vector<double>>> FadingCoff(Nr, vector<vector<double>>(NUSERS, vector<double>(NBITS)));// �ŵ�˥��ϵ��
//    vector<vector<double>> modulated_data(NUSERS, vector<double>(NBITS));// ���ƺ����Ϣ
//    vector<vector<int>> ILidx(NUSERS, vector<int>(NBITS));// �����֯��
//    vector<vector<double>> spreaded_data(NUSERS, vector<double>(FrameLen));// ��Ƶ�������
//    vector<vector<double>> ILData(NUSERS, vector<double>(FrameLen));// ��֯�������
//
//    // ��ʼ������
//    GenMessage(input_data);
//    GenNoise(noise_parity);
//    GenFadingCoff(FadingCoff);
//    GenILidx(ILidx);
//
//    Modulate(input_data, modulated_data);
//    spreader(modulated_data,spreaded_data);
//    InterLeaver(spreaded_data,ILidx, ILData);
//    
//
//    // ���㲻ͬSNR�µ�������
//    for (int i = 0; i < SNR_NUM; ++i) {
//        double noise_variance  = 1.0 / pow(10.0, snr[i] / 10.0);    // ���㵱ǰsnr�µ���������
//
//
//    }
//
//    // �ļ��������򣬼��������ʺʹ�ӡ�����ʹ���ֻ����һ���߳���ִ��
//    {
//        std::lock_guard<std::mutex> lock(data_mutex);
//                                    
//                                    // ����������
//        if (cnt % NUM_PRINT == 0) {
//            PrintToConsole(cnt);    // �����ӡ����ʱ����ӡһ�ν��
//        }
//        if (cnt == NUM_FRAMES - 1) {
//            PrintToConsole(cnt);
//            WriteToFile();          // ���з�����ɺ󣬰ѽ��д�뵽�ļ�����
//        }
//        cnt++;        // ����������ɺ����һ��
//    }
//
//
//}

//int main() {
//
//    ThreadPool pool(8);     // ʹ�õ��߳�����
//
//    OpenDataFile();         // �����ݴ洢�ļ�
//    GenSNR();               // ���ɴ������SNR����
//
//    // ��ӡ��ͷ
//    PrintHeader();
//
//    // ���̲߳���
//    for (int sim = 0; sim < NUM_FRAMES; ++sim) {
//
//        pool.enqueue(MonteCarloSimulation); // ������ŵ��̳߳�����
//
//
//    }
//    return 0;
//}

int main() {

    // �����߳���˽�б���
    vector<vector<int>> input_data(NUSERS, vector<int>(NBITS));     // �û��������Ϣ
    vector<vector<double>> noise_parity(Nr, vector<double>(FrameLen));// ��˹�ŵ�������
    vector<vector<vector<double>>> FadingCoff(Nr, vector<vector<double>>(NUSERS, vector<double>(FrameLen)));// �ŵ�˥��ϵ��
    vector<vector<double>> modulated_data(NUSERS, vector<double>(NBITS));// ���ƺ����Ϣ
    vector<vector<int>> ILidx(NUSERS, vector<int>(NBITS));// �����֯��
    vector<vector<double>> spreaded_data(NUSERS, vector<double>(FrameLen));// ��Ƶ�������
    vector<vector<double>> ILData(NUSERS, vector<double>(FrameLen));// ��֯�������

    vector<vector<double>> despreadedData(NUSERS, vector<double>(N));// ����Ƶ�������
    vector<vector<double>> deILData(NUSERS, vector<double>(FrameLen));// �⽻֯�������

    // ��ʼ������
    GenMessage(input_data);
    GenNoise(noise_parity);
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


    }

    return 0; 
}



