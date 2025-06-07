#ifndef NET_H
#define NET_H

#include <vector>
#include <iostream>
#include "Node.h"
using namespace std;

class Node;

class Net{
    public:
        Net(int index);
        virtual ~Net();
        void add_node(Node *node);
        int get_index() { return this->index; }
        vector<Node *> &get_nodes() { return this->nodes; }
        int count_0; // 划分 0 中的顶点数
        int count_1; // 划分 1 中的顶点数
    
        int index;
        vector<Node *> nodes;
};

#endif