#include "IDMA.h"
#include <string>
using namespace std;

//����ȫ�ֱ���

const int NUSERS = 1;//��Ծ�û�����
const int NBITS = 10;//ÿ���û����͵ı�������
const int SF = 300;//��Ƶ�ı���
const int N = 32;//���������ֳ���
const int FrameLen = N * SF;//�ܵ����ֵĳ���
const int Nr = 1;//��������

const double SNR_BEGIN = -40;
const double SNR_END = -20;
const int SNR_NUM = 3;

const int NUM_FRAMES = 100000;              // ֡����
const int NUM_PRINT = 100;                  // ��ӡ��ʾ���

const int BlockLen = 1500;// ��˥��ĳ���

string filename;                            // ���ݱ�����ļ���

vector<double> SNR_dB(SNR_NUM);                                              // SNRֵ������dB��
vector<vector<double>> BER(SNR_NUM, vector<double>(NUM_FRAMES, 0.0));
vector<vector<double>> PUPE(SNR_NUM, vector<double>(NUM_FRAMES, 0.0));       // ��ʼ��BER��PUPE




