#ifndef SOLUTION_H
#define SOLUTION_H

#include <string>
#include <set>
#include "Graph.h"
#include "evaluate.h"

using namespace std;

class Solution{
    public:
        void read_benchmark(Graph &graph, string benchmark_name);
        void my_partition_algorithm(Graph &graph, set<int> &X, set<int> &Y, string output_file);
        
};

#endif