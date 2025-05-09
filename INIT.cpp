#include <vector>
#include <random>
#include <fstream>
#include <iostream>
#include <iomanip>  
#include <string>
#include "global_variables.h"

using namespace std;

// ����SNR��ֵ
void GenSNR() {
    // ���� SNR �Ĳ���
    double step = (SNR_NUM > 1) ? (SNR_END - SNR_BEGIN) / (SNR_NUM - 1) : 0.0;

    //// ��ʼ�� SNR ֵ����
    //vector<double> snr(SNR_NUM, 0.0);

    // ���� SNR �ľ���ȡֵ
    for (int i = 0; i < SNR_NUM; ++i) {
        SNR_dB[i] = SNR_BEGIN + i * step;                // SNR �� dB ֵ
        snr[i] = pow(10, SNR_dB[i] / 10.0);       // ת��Ϊ���Ե�λ
    }
}

// �����û���Ϣ
void GenMessage(vector<vector<int>>& input_data) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> bit_dist(0, 1);

    // �������������
    for (int i = 0; i < NBITS; ++i)
    {
        for (int j = 0; j < NUSERS; ++j)
        {
            input_data[j][i] = bit_dist(rng);
        }
    }
}

// ������������Ϊ1����������
void GenNoise(vector<vector<double>>& noise_parity) {
    // ��ʼ��������������ͷֲ�
    random_device rd;                   // ��������
    mt19937 generator(rd());            // Mersenne Twister ����
    normal_distribution<double> distribution(0.0, 1.0);  // ��ֵΪ0����׼��Ϊ1����̬�ֲ�

    // ��� noise_parity ��ÿ��Ԫ��
    for (int i = 0; i < Nr; ++i) {
        for (int j = 0; j < FrameLen; ++j) {
            noise_parity[i][j] = distribution(generator);
        }
    }
}

// ����˥��ϵ������
void GenFadingCoff(vector<vector<vector<double>>>& FadingCoff) {
    // ������� NUSERS * NBITS < M
    if (NUSERS * NBITS > FrameLen) {
        throw invalid_argument("����NUSERS * NBITS ����С�ڵ��� FrameLen��");
    }

    // ��ʼ�������������
    mt19937 rng(random_device{}());

    // ����ÿ�����˥��ϵ������Ĵ�С
    int num_blocks = FrameLen / BlockLen; // �����������
    int remaining_slots = FrameLen % BlockLen; // ʣ�ಿ��
    if (remaining_slots > 0) {
        num_blocks++; // �����ʣ�ಿ�֣�����һ����
    }

    // ��ʼ�� FadingCoff ���󣬴�СΪ Nr x NUSERS x FrameLen
    FadingCoff.resize(Nr, vector<vector<double>>(NUSERS, vector<double>(FrameLen, 0.0)));

    // ��� IsFading Ϊ false������˥��ϵ������Ϊ 1
    if (!IsFading) {
        for (int nr = 0; nr < Nr; ++nr) {
            for (int user = 0; user < NUSERS; ++user) {
                fill(FadingCoff[nr][user].begin(), FadingCoff[nr][user].end(), 1.0);
            }
        }
    }
    else {
        // Ϊÿ�����ߺ�ÿ���û�����˥��ϵ��
        for (int nr = 0; nr < Nr; ++nr) {
            for (int user = 0; user < NUSERS; ++user) {
                // ÿ�����˥��ϵ��
                uniform_real_distribution<double> uniformDist(0.0, 1.0);
                for (int block = 0; block < num_blocks; ++block) {

                    double h_tmp = 0.0;
                    for (int nt = 0; nt < Nt; ++nt) {
                        double U = uniformDist(rng);
                        double h = sqrt(0.5) * sqrt(-2.0 * log(U)); // �����ֲ������ɷ�ʽ

                        h_tmp += h;
                    }

                    //double U = uniformDist(rng);
                    //double h = sqrt(0.5)*sqrt(-2.0 * log(U)); // �����ֲ������ɷ�ʽ

                    // ����Ӧ��˥��ϵ��
                    for (int i = block * BlockLen; i < (block + 1) * BlockLen && i < FrameLen; ++i) {
                        FadingCoff[nr][user][i] = h_tmp/Nt; // �����ɵ�˥��ϵ�����ÿ���û���ÿ������
                    }
                }
            }
        }
    }
}

// ��ȡ��ǰ����
std::string GetCurrentDate() {
    time_t t = time(nullptr);
    tm current_time;
    localtime_s(&current_time, &t);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y%m%d", &current_time);
    return std::string(buffer);
}

// ��ȡ��ǰʱ���ַ���
std::string GetCurrentTimeString() {
    time_t t = time(nullptr);
    tm current_time;
    localtime_s(&current_time, &t); // ʹ�ð�ȫ�汾localtime_s
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &current_time);
    return std::string(buffer);
}

// ������������ļ�
void OpenDataFile() {
    // ʹ�õ�ǰ���������ļ���
    filename = GetCurrentDate() + "_data.txt";

    // ��������ļ���
    std::ofstream outfile;

    // ����ļ��Ƿ����
    if (std::ifstream(filename).good()) {
        // �ļ����ڣ���׷��ģʽ���ļ�
        outfile.open(filename, std::ios::app);
    }
    else {
        // �ļ������ڣ��������ļ�
        outfile.open(filename);
    }

    // ����ļ��Ƿ�ɹ���
    if (!outfile.is_open()) {
        std::cerr << "�޷������ļ�: " << filename << std::endl;
    }
    outfile.close(); // �ر��ļ�
}

// ��ӡ����̨��ͷ
void PrintHeader() {

    // �ڿ���̨��ӡͷ��Ϣ
    cout << "���濪ʼʱ��: " << GetCurrentTimeString() << "\n";

    cout << left << setw(15) << "�û�����:" << NUSERS << "\n"
        << left << setw(15) << "�û���Ϣ����:" << NBITS << "\n"
        << left << setw(15) << "������������:" << Nr << "\n"
        << left << setw(15) << "���ʷ���:" << SF << "\n"
        << left << setw(15) << "����:" << "K = " << NBITS << ", N = " << N << "\n";
    
    // ����CodeModeֵ��ӡ����ģʽ
    if (CodeMode == "None") {
        std::cout << std::left << std::setw(15) << "����ģʽ:" << "None" << "\n";
    }
    else if (CodeMode == "Polar") {
        std::cout << std::left << std::setw(15) << "����ģʽ:" << "Polar" << "\n";
    }

    // ����IsFadingֵ��ӡ��˥�䳤��
    if (IsFading == true) {
        std::cout << std::left << std::setw(15) << "˥��鳤��:" << BlockLen << "\n";
    }
    else if (IsFading == false) {
        std::cout << std::left << std::setw(15) << "�ŵ�ģ��:" << "AWGN" << "\n";
    }

}

// ��ӡ���ݵ�����̨����̬ˢ�£�
void PrintToConsole(int sim) {
    // ANSI ת�����У������ƶ���굽����̨����
    // �������̨�Ѿ��й̶�������������ʾ���
    // ���������Ҫˢ�´�Լ 10 �У����Ը���ʵ���������
    // ANSI ת�����У��ƶ���굽��ͷ�·�
    const int header_lines = 9; // ��ͷռ�õ����������� SNR �����ͷ�ͷָ���
    cout << "\033[" << header_lines + 1 << "H"; // �ƶ���굽��ͷ�·�

    // ��ӡ�������������
    cout << "��ǰ�������: " << (sim + 1) << "\n";

    // ��ӡ SNR ��ͷ
    cout << left << setw(10) << "SNR";
    for (int i = 0; i < SNR_NUM; ++i) {
        cout << setw(12) << fixed << setprecision(2) << SNR_dB[i];
    }
    cout << "\n" << string(10 + SNR_NUM * 12, '-') << "\n";

    // ��ӡ BER ������
    cout << left << setw(10) << "BER";
    for (int i = 0; i < SNR_NUM; ++i) {
        double avg_BER = 0.0;
        for (int j = 0; j < BER[i].size(); ++j) {
            avg_BER += BER[i][j];
        }
        avg_BER /= (sim + 1); // ����ƽ�� BER
        cout << setw(12) << scientific << setprecision(2) << avg_BER;
    }
    cout << "\n";

    // ��ӡ PUPE ������
    cout << left << setw(10) << "PUPE";
    for (int i = 0; i < SNR_NUM; ++i) {
        double avg_PUPE = 0.0;
        for (int j = 0; j < PUPE[i].size(); ++j) {
            avg_PUPE += PUPE[i][j];
        }
        avg_PUPE /= (sim + 1); // ����ƽ�� PUPE
        cout << setw(12) << scientific << setprecision(2) << avg_PUPE;
    }
    cout << "\n";

    // ��ӡ�ۼƴ����������
    cout << left << setw(10) << "ErrorBits";
    for (int i = 0; i < SNR_NUM; ++i) {
        int total_errors = 0;
        for (int j = 0; j <= sim; ++j) { // �����ۼƴ���ı�������
            total_errors += static_cast<int>(BER[i][j] * NUSERS * NBITS); // ��������ת��Ϊ�����������
        }
        cout << setw(12) << total_errors;
    }
    cout << "\n";

    // ��ӡ�ָ���
    cout << string(10 + SNR_NUM * 12, '=') << "\n\n";
}

// �����ݴ洢�ļ�д������
void WriteToFile() {
    // ���ļ�
    ofstream outfile(filename, ios::app);
    if (!outfile.is_open()) {
        cerr << "�޷����ļ�: " << filename << endl;
        return;
    }

    // д�����ͷ��Ϣ
    outfile << "�������ʱ��: " << GetCurrentTimeString() << "\n";

    outfile << left << setw(15) << "�û�����:" << NUSERS << "\n"
        << left << setw(15) << "�û���Ϣ����:" << NBITS << "\n"
        << left << setw(15) << "������������:" << Nr << "\n"
        << left << setw(15) << "���ʷ���:" << SF << "\n"
        << left << setw(15) << "����:" << "K = " << NBITS << ", N = " << N << "\n"
        << left << setw(15) << "˥��鳤��:" << BlockLen << "\n\n";

    // д�� SNR ��ͷ
    outfile << left << setw(10) << "SNR";
    for (int i = 0; i < SNR_NUM; ++i) {
        outfile << setw(12) << fixed << setprecision(2) << SNR_dB[i];
    }
    outfile << "\n" << string(10 + SNR_NUM * 12, '-') << "\n";

    // д�� BER ������
    outfile << left << setw(10) << "BER";
    for (int i = 0; i < SNR_NUM; ++i) {
        double avg_BER = 0.0;
        for (int j = 0; j < BER[i].size(); ++j) {
            avg_BER += BER[i][j];
        }
        avg_BER /= (NUM_FRAMES + 1); // ����ƽ�� BER
        outfile << setw(12) << fixed << setprecision(8) << avg_BER; // �ļ��������8λС��
    }
    outfile << "\n";

    // д�� PUPE ������
    outfile << left << setw(10) << "PUPE";
    for (int i = 0; i < SNR_NUM; ++i) {
        double avg_PUPE = 0.0;
        for (int j = 0; j < PUPE[i].size(); ++j) {
            avg_PUPE += PUPE[i][j];
        }
        avg_PUPE /= (NUM_FRAMES + 1); // ����ƽ�� PUPE
        outfile << setw(12) << fixed << setprecision(8) << avg_PUPE; // �ļ��������8λС��
    }
    outfile << "\n";

    // д���ۼƴ����������
    outfile << left << setw(10) << "ErrorBits";
    for (int i = 0; i < SNR_NUM; ++i) {
        int total_errors = 0;
        for (int j = 0; j < BER[i].size(); ++j) { // �ۼƴ����������
            total_errors += static_cast<int>(BER[i][j] * NUSERS * NBITS);
        }
        outfile << setw(12) << total_errors;
    }
    outfile << "\n";

    // д��ָ���
    outfile << string(10 + SNR_NUM * 12, '=') << "\n\n";

    outfile.close(); // �ر��ļ�
}
