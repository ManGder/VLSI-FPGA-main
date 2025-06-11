// solution.h
#ifndef SOLUTION_H
#define SOLUTION_H

#include "Graph.h"
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <map>

class Solution {
public:
    Solution();
    ~Solution();
    
    // 读取电路文件并构造网表
    void read_benchmark(const std::string& filename, Graph& graph);
    
    // 使用FM算法进行图划分
    std::vector<int> partition(Graph& graph);
    
private:
    // FM算法基础结构
    struct FMState {
        std::vector<int> partitions;    // 节点分区状态，0或1
        std::vector<bool> locked;       // 节点锁定状态
        std::vector<int> gains;         // 节点移动增益
        std::vector<int> external_cost; // 节点外部连接代价
        std::vector<int> internal_cost; // 节点内部连接代价
        int cut_size;                   // 当前割边数
        std::vector<int> node_counts;   // 每个分区的节点数量
        
        FMState(int node_count) : 
            partitions(node_count), locked(node_count, false),
            gains(node_count), external_cost(node_count, 0), 
            internal_cost(node_count, 0), cut_size(0) {
            node_counts.resize(2, 0);
        }
    };
    
    // 增益桶数据结构
    class GainBucket {
    private:
        std::map<int, std::list<int>> gain_buckets; // 按增益值排序的桶
        std::vector<std::list<int>::iterator> node_locations; // 节点在桶中的位置
        
    public:
        // 构造函数
        explicit GainBucket(int node_count);
        
        // 将节点插入到桶中
        void insert(int node_id, int gain);
        
        // 从桶中移除节点
        void remove(int node_id);
        
        // 更新节点增益
        void update(int node_id, int old_gain, int new_gain);
        
        // 获取最大增益节点
        int get_max_gain_node(const std::vector<bool>& locked) const;
        
        // 清空所有桶
        void clear();
        
        // 检查桶是否为空
        bool empty() const;
    };
    
    // 初始化随机分区
    std::vector<int> initialize_partition(Graph& graph);
    
    // 计算初始状态
    void initialize_fm_state(Graph& graph, FMState& state);
    
    // 计算初始节点增益
    void compute_initial_gains(Graph& graph, FMState& state);
    
    // 单轮FM优化
    std::vector<int> fm_pass(Graph& graph, const std::vector<int>& initial_partition);
    
    // 多轮FM优化
    std::vector<int> multi_pass_fm(Graph& graph, const std::vector<int>& initial_partition, int max_passes = 10);
    
    // 多起点FM优化
    std::vector<int> multi_start_fm(Graph& graph, int num_starts = 5);
    
    // 检查平衡约束
    bool is_balanced(const FMState& state, int node_id = -1);
    
    // 计算移动节点后的增益变化
    void update_gains_after_move(Graph& graph, FMState& state, GainBucket& bucket, int moved_node);
    
    // 计算割边数量
    int calculate_cut_size(Graph& graph, const std::vector<int>& partition);
    
    // 精确更新割边数
    int update_cut_size(Graph& graph, const FMState& state, int node_id, int old_part);
};

#endif // SOLUTION_H