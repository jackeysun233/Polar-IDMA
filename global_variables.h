#ifndef GLOBAL_VARIABLES_H
#define GLOBAL_VARIABLES_H


#define		STEP(x)		( (x) > 0 ? 0 : 1 )


#include <vector>
#include <string>
#include <atomic>

using namespace std;

// ȫ�ֳ�������
extern const int NUSERS;     // ��Ծ�û�����
extern const int NBITS;      // ÿ���û����͵ı�������
extern const int SF;         // ��Ƶ�ı���
extern const int Nr;         // ���ջ���������
extern const int Nt;         // ���ͻ���������
extern const int N;			 // ���������ֳ���
extern const int FrameLen;   // �ܵ����ֵĳ���
extern const int L;

extern const double EbNoSTART;  // SNR�Ŀ�ʼֵ
extern const double EbNoSTEP;    // SNR�Ľ���ֵ
extern const int EbNoNUM;       // SNR����

extern const int NUM_FRAMES;   // ֡����
extern const int NUM_PRINT;    // ��ӡ��ʾ���
extern const int BlockLen;     // ��˥��ĳ���

extern const string CodeMode;  // ���Ʊ���ģʽ��Polar or None��
extern const bool IsFading;    // ����˥��ģʽ��ȫ�ֿ��أ�

extern std::string filename;   // ���ݱ�����ļ���

// ȫ�ֱ�������
extern std::vector<double> snr_dB;                         // SNRֵ������dB��
extern std::vector<double> ebno_dB;                        // SNRֵ������dB��
extern std::vector<double> snr;							   // SNRֵ�����ԣ�
extern std::vector<double> total_BER;
extern std::vector<double> total_PUPE;

// ����ȫ��Ժ�ӱ���
extern std::atomic<int> cnt;

#endif // GLOBAL_VARIABLES_H
