#include "Net.h"

Net::Net(int index):index(index), count_0(0), count_1(0) {}

Net::~Net() {}

void Net::add_node(Node *node) {
    this->nodes.push_back(node);
}