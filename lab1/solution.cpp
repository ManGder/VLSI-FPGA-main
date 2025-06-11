// solution.cpp
#include "solution.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <numeric>
#include <climits>
#include <ctime>
#include <queue>
#include <chrono>
#include <cassert>

Solution::Solution() {
    // 初始化随机数生成器
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

Solution::~Solution() {}

// 增益桶构造函数
Solution::GainBucket::GainBucket(int node_count) {
    node_locations.resize(node_count);
}

// 将节点插入到桶中
void Solution::GainBucket::insert(int node_id, int gain) {
    // 在对应增益值的桶中插入节点
    gain_buckets[gain].push_front(node_id);
    // 记录节点位置
    node_locations[node_id] = gain_buckets[gain].begin();
}

// 从桶中移除节点
void Solution::GainBucket::remove(int node_id) {
    // 空操作保护
    if (node_locations[node_id] == std::list<int>::iterator{}) return;
    
    // 找到节点所在的桶
    auto it = gain_buckets.begin();
    while (it != gain_buckets.end()) {
        auto& bucket = it->second;
        for (auto node_it = bucket.begin(); node_it != bucket.end(); ++node_it) {
            if (*node_it == node_id) {
                bucket.erase(node_it);
                // 如果桶为空，移除桶
                if (bucket.empty()) {
                    gain_buckets.erase(it);
                }
                // 置空节点位置
                node_locations[node_id] = std::list<int>::iterator{};
                return;
            }
        }
        ++it;
    }
}

// 更新节点增益
void Solution::GainBucket::update(int node_id, int old_gain, int new_gain) {
    // 如果增益没有变化，不需要更新
    if (old_gain == new_gain) return;
    
    // 从旧增益桶移除
    remove(node_id);
    
    // 插入到新增益桶
    insert(node_id, new_gain);
}

// 获取最大增益节点
int Solution::GainBucket::get_max_gain_node(const std::vector<bool>& locked) const {
    // 空桶检查
    if (gain_buckets.empty()) return -1;
    
    // 倒序遍历，寻找最大增益的未锁定节点
    for (auto rit = gain_buckets.rbegin(); rit != gain_buckets.rend(); ++rit) {
        const auto& bucket = rit->second;
        for (int node_id : bucket) {
            if (!locked[node_id]) {
                return node_id;
            }
        }
    }
    
    // 没有找到符合条件的节点
    return -1;
}

// 清空所有桶
void Solution::GainBucket::clear() {
    gain_buckets.clear();
    std::fill(node_locations.begin(), node_locations.end(), std::list<int>::iterator{});
}

// 检查桶是否为空
bool Solution::GainBucket::empty() const {
    return gain_buckets.empty();
}

// 读取电路文件并构造网表
void Solution::read_benchmark(const std::string& filename, Graph& graph) {
    std::ifstream file(filename);

    if(!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        exit(-1);
    }

    int edge_num, node_num;
    std::string line;
    getline(file >> std::ws, line);
    std::istringstream iss(line);
    iss >> edge_num;
    iss >> node_num;

    for(int i = 0; i < edge_num; i++) {
        getline(file, line);
        std::istringstream iss(line);
        int node_id;
        
        Net *net = graph.add_net(i);

        while(iss >> node_id) {
            Node *node = graph.get_or_create_node(node_id - 1);  // 调整索引从0开始
            node->add_net(net);
            net->add_node(node);
        }
    }

    file.close();
}

// 初始化随机分区
std::vector<int> Solution::initialize_partition(Graph& graph) {
    int n = graph.get_nodes().size();
    std::vector<int> partition(n);
    
    // 创建节点索引序列，用于随机打乱
    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    
    // 随机打乱序列
    std::shuffle(indices.begin(), indices.end(), std::mt19937(std::random_device()()));
    
    // 前一半分配给分区0，后一半分配给分区1
    int half = n / 2;
    for (int i = 0; i < n; i++) {
        int node_id = indices[i];
        partition[node_id] = (i < half) ? 0 : 1;
    }
    
    return partition;
}

// 检查平衡约束
bool Solution::is_balanced(const FMState& state, int node_id) {
    int total = state.partitions.size();
    int moving_from = -1, moving_to = -1;
    
    // 如果指定了节点，则模拟该节点的移动
    if (node_id >= 0) {
        moving_from = state.partitions[node_id];
        moving_to = 1 - moving_from;
    }
    
    int count_0 = state.node_counts[0];
    int count_1 = state.node_counts[1];
    
    // 模拟移动
    if (moving_from == 0 && moving_to == 1) {
        count_0--;
        count_1++;
    } else if (moving_from == 1 && moving_to == 0) {
        count_0++;
        count_1--;
    }
    
    // 计算每个分区的比例
    double ratio_0 = static_cast<double>(count_0) / total;
    double ratio_1 = static_cast<double>(count_1) / total;
    
    // 允许的不平衡度（通常为2%）
    double epsilon = 0.02;
    
    return ratio_0 >= (0.5 - epsilon) && ratio_0 <= (0.5 + epsilon) &&
           ratio_1 >= (0.5 - epsilon) && ratio_1 <= (0.5 + epsilon);
}

// 计算初始状态
void Solution::initialize_fm_state(Graph& graph, FMState& state) {
    int n = graph.get_nodes().size();
    
    // 计算每个分区的节点数量
    state.node_counts[0] = std::count(state.partitions.begin(), state.partitions.end(), 0);
    state.node_counts[1] = n - state.node_counts[0];
    
    // 重置所有状态
    std::fill(state.locked.begin(), state.locked.end(), false);
    std::fill(state.external_cost.begin(), state.external_cost.end(), 0);
    std::fill(state.internal_cost.begin(), state.internal_cost.end(), 0);
    
    // 计算初始割边数和节点连接代价
    state.cut_size = 0;
    
    for (Net* net : graph.get_nets()) {
        bool has_part0 = false;
        bool has_part1 = false;
        int nodes_in_part0 = 0;
        int nodes_in_part1 = 0;
        
        // 计算网络中每个分区的节点数
        for (Node* node : net->get_nodes()) {
            int node_id = node->get_index();
            if (state.partitions[node_id] == 0) {
                has_part0 = true;
                nodes_in_part0++;
            } else {
                has_part1 = true;
                nodes_in_part1++;
            }
        }
        
        // 如果网络横跨两个分区，则增加割边数
        if (has_part0 && has_part1) {
            state.cut_size++;
            
            // 更新节点的外部和内部连接代价
            for (Node* node : net->get_nodes()) {
                int node_id = node->get_index();
                int part = state.partitions[node_id];
                
                if (part == 0) {
                    if (nodes_in_part1 > 0) state.external_cost[node_id]++;
                    if (nodes_in_part0 > 1) state.internal_cost[node_id]++;
                } else {
                    if (nodes_in_part0 > 0) state.external_cost[node_id]++;
                    if (nodes_in_part1 > 1) state.internal_cost[node_id]++;
                }
            }
        } else {
            // 如果网络只在一个分区内，更新内部连接代价
            for (Node* node : net->get_nodes()) {
                int node_id = node->get_index();
                int part = state.partitions[node_id];
                
                if ((part == 0 && has_part0) || (part == 1 && has_part1)) {
                    state.internal_cost[node_id]++;
                }
            }
        }
    }
    
    // 计算初始增益：外部连接 - 内部连接
    for (int i = 0; i < n; i++) {
        state.gains[i] = state.external_cost[i] - state.internal_cost[i];
    }
}

// 计算割边数量
int Solution::calculate_cut_size(Graph& graph, const std::vector<int>& partition) {
    int cut_size = 0;
    
    for (Net* net : graph.get_nets()) {
        bool has_part0 = false;
        bool has_part1 = false;
        
        for (Node* node : net->get_nodes()) {
            int idx = node->get_index();
            if (partition[idx] == 0) {
                has_part0 = true;
            } else {
                has_part1 = true;
            }
            
            if (has_part0 && has_part1) {
                cut_size++;
                break;
            }
        }
    }
    
    return cut_size;
}

// 精确更新割边数
int Solution::update_cut_size(Graph& graph, const FMState& state, int node_id, int old_part) {
    int cut_delta = 0;
    int new_part = 1 - old_part;
    
    // 遍历与节点相连的所有网络
    for (Net* net : graph.get_nodes()[node_id]->get_nets()) {
        bool had_old_part = false;
        bool had_new_part = false;
        int nodes_in_old_part = 0;
        int nodes_in_new_part = 0;
        
        // 计算移动前网络中每个分区的节点数
        for (Node* node : net->get_nodes()) {
            int idx = node->get_index();
            if (idx == node_id) continue; // 排除要移动的节点
            
            if (state.partitions[idx] == old_part) {
                had_old_part = true;
                nodes_in_old_part++;
            } else {
                had_new_part = true;
                nodes_in_new_part++;
            }
        }
        
        // 移动前是割边，而移动后不是割边
        if (had_old_part && had_new_part && nodes_in_old_part == 0) {
            cut_delta--;
        }
        // 移动前不是割边，而移动后是割边
        else if (had_old_part && !had_new_part) {
            cut_delta++;
        }
    }
    
    return cut_delta;
}

// 更新移动节点后的增益变化
void Solution::update_gains_after_move(Graph& graph, FMState& state, GainBucket& bucket, int moved_node) {
    int from_part = state.partitions[moved_node];
    int to_part = 1 - from_part;
    
    // 标记已访问的网络，避免重复更新
    std::unordered_set<int> processed_nets;
    
    // 遍历与移动节点相连的所有网络
    for (Net* net : graph.get_nodes()[moved_node]->get_nets()) {
        int net_id = net->get_index();
        if (processed_nets.find(net_id) != processed_nets.end()) continue;
        processed_nets.insert(net_id);
        
        // 统计移动后网络中每个分区的节点数
        int nodes_in_from_part = 0;
        int nodes_in_to_part = 1; // 已经包含了移动的节点
        
        for (Node* node : net->get_nodes()) {
            int node_id = node->get_index();
            if (node_id == moved_node) continue; // 排除已移动的节点
            
            if (state.partitions[node_id] == from_part) {
                nodes_in_from_part++;
            } else {
                nodes_in_to_part++;
            }
        }
        
        // 更新受影响节点的增益
        for (Node* node : net->get_nodes()) {
            int node_id = node->get_index();
            if (node_id == moved_node || state.locked[node_id]) continue;
            
            int old_gain = state.gains[node_id];
            int delta_gain = 0;
            
            if (state.partitions[node_id] == from_part) {
                // 如果当前网络从"有两个分区中的节点"变为"全部在目标分区"
                if (nodes_in_from_part == 1 && nodes_in_to_part > 0) {
                    delta_gain++;
                }
                // 如果当前网络从"全部在原分区"变为"有两个分区中的节点"
                else if (nodes_in_from_part > 1 && nodes_in_to_part == 1) {
                    delta_gain--;
                }
            } else { // state.partitions[node_id] == to_part
                // 如果当前网络从"有两个分区中的节点"变为"全部在原分区"
                if (nodes_in_to_part == 1 && nodes_in_from_part > 0) {
                    delta_gain++;
                }
                // 如果当前网络从"全部在目标分区"变为"有两个分区中的节点"
                else if (nodes_in_to_part > 1 && nodes_in_from_part == 0) {
                    delta_gain--;
                }
            }
            
            // 如果增益有变化，更新增益值和桶
            if (delta_gain != 0) {
                int new_gain = old_gain + delta_gain;
                state.gains[node_id] = new_gain;
                bucket.update(node_id, old_gain, new_gain);
            }
        }
    }
}

// 单轮FM优化
std::vector<int> Solution::fm_pass(Graph& graph, const std::vector<int>& initial_partition) {
    int n = graph.get_nodes().size();
    
    // 初始化FM状态
    FMState state(n);
    state.partitions = initial_partition;
    initialize_fm_state(graph, state);
    
    // 初始化增益桶
    GainBucket bucket(n);
    for (int i = 0; i < n; i++) {
        bucket.insert(i, state.gains[i]);
    }
    
    // 记录移动序列和割边变化
    std::vector<int> move_sequence;
    std::vector<int> cut_sizes;
    cut_sizes.push_back(state.cut_size);
    
    // FM算法主循环
    while (true) {
        // 获取最大增益节点
        int best_node = bucket.get_max_gain_node(state.locked);
        if (best_node == -1) break; // 没有可移动的节点
        
        // 检查移动是否满足平衡约束
        if (!is_balanced(state, best_node)) {
            state.locked[best_node] = true; // 锁定但不移动
            continue;
        }
        
        // 记录原分区和增益
        int from_part = state.partitions[best_node];
        int to_part = 1 - from_part;
        int node_gain = state.gains[best_node];
        
        // 更新切割大小
        state.cut_size -= node_gain;
        cut_sizes.push_back(state.cut_size);
        
        // 移动节点并更新状态
        state.partitions[best_node] = to_part;
        state.node_counts[from_part]--;
        state.node_counts[to_part]++;
        state.locked[best_node] = true;
        move_sequence.push_back(best_node);
        
        // 更新受影响节点的增益
        update_gains_after_move(graph, state, bucket, best_node);
    }
    
    // 找到割边数最小的移动序列位置
    auto min_it = std::min_element(cut_sizes.begin(), cut_sizes.end());
    int best_pos = std::distance(cut_sizes.begin(), min_it);
    int best_cut = *min_it;
    
    // 重新应用移动序列到最佳位置
    std::vector<int> best_partition = initial_partition;
    for (int i = 0; i < best_pos; i++) {
        int node = move_sequence[i];
        best_partition[node] = 1 - best_partition[node];
    }
    
    return best_partition;
}

// 多轮FM优化
std::vector<int> Solution::multi_pass_fm(Graph& graph, const std::vector<int>& initial_partition, int max_passes) {
    std::vector<int> current_partition = initial_partition;
    int current_cut = calculate_cut_size(graph, current_partition);
    
    std::cout << "Multi-pass FM started: Initial cut=" << current_cut << std::endl;
    
    bool improved = true;
    int pass = 0;
    
    while (improved && pass < max_passes) {
        pass++;
        std::cout << "Pass " << pass << ":" << std::endl;
        
        // 执行单轮FM优化
        std::vector<int> new_partition = fm_pass(graph, current_partition);
        int new_cut = calculate_cut_size(graph, new_partition);
        
        // 检查是否有改进
        if (new_cut < current_cut) {
            std::cout << "  Improved: " << current_cut << " -> " << new_cut << std::endl;
            current_cut = new_cut;
            current_partition = new_partition;
            improved = true;
        } else {
            std::cout << "  No improvement, stopping" << std::endl;
            improved = false;
        }
    }
    
    double imbalance = 100.0 * std::abs(2.0 * std::count(current_partition.begin(), current_partition.end(), 0) / 
                                         current_partition.size() - 1.0);
    
    std::cout << "Multi-pass FM completed: Final cut=" << current_cut 
              << ", Passes=" << pass 
              << ", Imbalance=" << imbalance << "%" << std::endl;
    
    return current_partition;
}

// 多起点FM优化
std::vector<int> Solution::multi_start_fm(Graph& graph, int num_starts) {
    std::vector<int> best_partition;
    int best_cut = INT_MAX;
    
    std::cout << "Multi-start FM optimization: Trying " << num_starts << " different starting points" << std::endl;
    
    for (int i = 0; i < num_starts; i++) {
        std::cout << "Starting point " << (i + 1) << "/" << num_starts << ":" << std::endl;
        
        // 创建随机初始划分
        std::vector<int> initial_partition = initialize_partition(graph);
        
        // 执行多轮FM优化
        std::vector<int> result = multi_pass_fm(graph, initial_partition, 20);
        int cut = calculate_cut_size(graph, result);
        
        // 更新最佳结果
        if (cut < best_cut) {
            best_cut = cut;
            best_partition = result;
            std::cout << "Found better solution: Cut=" << best_cut << std::endl;
        }
    }
    
    return best_partition;
}

// 主划分函数
std::vector<int> Solution::partition(Graph& graph) {
    std::cout << "Starting Fiduccia-Mattheyses algorithm..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // 使用多起点策略执行FM算法
    std::vector<int> result = multi_start_fm(graph, 10);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;
    
    int final_cut = calculate_cut_size(graph, result);
    std::cout << "FM Algorithm completed in " << elapsed.count() << " seconds" << std::endl;
    std::cout << "Final cut size: " << final_cut << std::endl;
    
    return result;
}