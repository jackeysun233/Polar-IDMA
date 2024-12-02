//
// Created by Saurabh Tavildar on 5/17/16.
//

//
// Changed by lzlh on 11/18/24.
//

#ifndef POLARC_POLARCODE_H
#define POLARC_POLARCODE_H


#include <cstdint>
#include <vector>
#include <math.h>
#include <stack>          // std::stack
#include <algorithm>
#include "ThreadPool.h"

enum class NodeType {
    R0 = 0,
    R1 = 1,
    REP = 2,
    SPC = 3,
    NONE = 4,
    PCR = 5,
    RPC = 6
};

struct Node_Type_Index
{
    int begin;
    int end;
    NodeType type;
};


class PolarCode {


public:

    PolarCode(uint8_t num_layers, uint32_t info_length, double epsilon, uint32_t crc_size, bool flag_5G = false, bool memory = false, bool RM = false, bool system_polar = false) :
        _n(num_layers), _info_length(info_length), _design_epsilon(epsilon),
        _crc_size(crc_size), _llr_based_computation(true), _5G(flag_5G), _c(1024), _thread_pool(4), _memory(memory), _start(false), _RM(RM), _system_polar(system_polar)
    {
        _block_length = (uint32_t)(1 << _n);
        _frozen_bits.resize(_block_length);
        _bit_rev_order.resize(_block_length);
        create_bit_rev_order();
        initialize_frozen_bits();
    }

    std::vector<uint8_t> encode(std::vector<uint8_t> info_bits);
    std::vector<uint8_t> sys_encode(std::vector<uint8_t> info_bits);

    std::vector<uint8_t> decode_scl_p1(std::vector<double> p1, std::vector<double> p0, uint32_t list_size);
    std::vector<uint8_t> decode_scl_llr(std::vector<double> llr, uint32_t list_size);

    std::vector<std::vector<double>> get_bler_quick(std::vector<double> ebno_vec, std::vector<uint32_t> list_size);
    std::vector<int> get_unfrozen() {
        std::vector<int> unfrozen(_info_length);
        for (size_t i = 0; i < _info_length; i++) {
            unfrozen[i] = _channel_order_ascending[_block_length - _info_length + i];
        }
        std::sort(unfrozen.begin(), unfrozen.end());
        return unfrozen;
    }
    std::vector<uint8_t> _c;

    std::vector<std::vector<uint8_t>> _crc_matrix;
    std::vector<uint32_t> _channel_order_ascending;
    std::vector<uint32_t> _channel_order_descending;
    bool _path_with_crc_pass;
private:

    bool _5G;
    bool _RM;
    bool _memory;
    bool _start;
    bool _system_polar;

    uint8_t _n;
    uint32_t _info_length;
    uint32_t _block_length;
    uint32_t _crc_size;
    double _design_epsilon;

    ThreadPool _thread_pool;

    std::vector<uint8_t> _frozen_bits;
    /*std::vector<uint32_t> _channel_order_ascending;*/

    std::vector<uint32_t> _bit_rev_order;
    std::vector<int> _Q;

    void initialize_frozen_bits();
    void create_bit_rev_order();

    std::vector<uint8_t> decode_scl();

    // fast
    std::vector<uint8_t> decode_fast_scl();
    void R0();
    void R1();
    void REP();
    void SPC();
    void PCR();
    void RPC();
    std::vector<Node_Type_Index> _nodes_type;


    bool _llr_based_computation;

    std::vector<std::vector<double*>> _arrayPointer_LLR;
    std::vector<double> _pathMetric_LLR;

    uint32_t _list_size;

    std::stack<uint32_t> _inactivePathIndices;
    std::vector<uint32_t > _activePath;
    std::vector<std::vector<double*>> _arrayPointer_P;
    std::vector<std::vector<uint8_t*>> _arrayPointer_C;
    std::vector<uint8_t*> _arrayPointer_Info;
    std::vector<std::vector<uint32_t>> _pathIndexToArrayIndex;
    std::vector<std::stack<uint32_t>> _inactiveArrayIndices;
    std::vector<std::vector<uint32_t>> _arrayReferenceCount;

    void initializeDataStructures();
    uint32_t assignInitialPath();
    uint32_t clonePath(uint32_t);
    void killPath(uint32_t l);

    double* getArrayPointer_P(uint32_t lambda, uint32_t  l);
    double* getArrayPointer_LLR(uint32_t lambda, uint32_t  l);
    uint8_t* getArrayPointer_C(uint32_t lambda, uint32_t  l);

    void recursivelyCalcP(uint32_t lambda, uint32_t phi);
    void recursivelyCalcLLR(uint32_t lambda, uint32_t phi);
    void recursivelyUpdateC(uint32_t lambda, uint32_t phi);

    void continuePaths_FrozenBit(uint32_t phi);
    void continuePaths_UnfrozenBit(uint32_t phi);

    uint32_t findMostProbablePath(bool check_crc);

    bool crc_check(uint8_t* info_bits_padded);
    void info_return(uint8_t* info_bit_padded);

    void compute_weights(std::vector<int>& weights);
    void sort_channel_orders(std::vector<double>& channel_vec, const std::vector<int>& weights);
};


#endif //POLARC_POLARCODE_H
