// main.cpp
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <chrono>
#include "Graph.h"
#include "solution.h"
#include "evaluate.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <benchmark_file>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    Graph graph;
    Solution solution;
    
    // 读取电路文件并构造网表
    solution.read_benchmark(filename, graph);
    std::cout << "Successfully read circuit file: " << filename << std::endl;
    std::cout << "Node count: " << graph.get_nodes().size() << std::endl;
    std::cout << "Network count: " << graph.get_nets().size() << std::endl;
    
    // 执行图划分
    std::vector<int> partition_result = solution.partition(graph);
    
    // 将划分结果转换为set<int>类型，以适应evaluate.cpp中的函数要求
    std::set<int> partition_0;
    std::set<int> partition_1;
    for (int i = 0; i < partition_result.size(); ++i) {
        if (partition_result[i] == 0) {
            partition_0.insert(i);
        } else {
            partition_1.insert(i);
        }
    }
    
    // 输出分区大小
    std::cout << "Nodes in partition V1: " << partition_0.size() << std::endl;
    std::cout << "Nodes in partition V2: " << partition_1.size() << std::endl;
    
    // 评估划分结果
    int cut_size = calculate_cut(graph, partition_0, partition_1);
    std::cout << "Final cut size: " << cut_size << std::endl;
    
    // 创建结果文件名
    std::string result_filename = filename.substr(0, filename.rfind(".")) + "_partition.txt";
    std::ofstream result_file(result_filename);
    if (!result_file.is_open()) {
        std::cerr << "Failed to create result file" << std::endl;
        return 1;
    }
    
    // 保存划分结果到文件
    for (int i = 0; i < partition_result.size(); ++i) {
        result_file << partition_result[i] << std::endl;
    }
    result_file.close();
    std::cout << "Partition result saved to: " << result_filename << std::endl;
    
    // 将划分结果写入临时文件以便使用evaluate函数
    std::string partition_file = "partition_result.txt";
    std::ofstream outfile(partition_file);
    if (!outfile.is_open()) {
        std::cerr << "Failed to create temporary partition file" << std::endl;
        return 1;
    }
    
    for (int i = 0; i < partition_result.size(); ++i) {
        outfile << partition_result[i] << std::endl;
    }
    outfile.close();
    
    // 使用evaluate函数评估
    int evaluation_result = evaluate(graph, partition_file);
    std::cout << "Evaluation result: " << evaluation_result << std::endl;
    
    return 0;
}