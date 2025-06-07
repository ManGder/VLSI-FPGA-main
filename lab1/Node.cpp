#include "Node.h"

Node::Node(int index):index(index), partition(-1) {}

Node::~Node(){}

void Node::add_net(Net *net) {
    this->nets.push_back(net);
}