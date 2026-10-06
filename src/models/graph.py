from ctypes import *

# Custom imports
from models.node import NodePtr

"""
typedef struct Graph {
    Node* nodes;
    unsigned int size;
} Graph;
"""


class Graph(Structure):
    _fields_ = [("nodes", NodePtr), ("size", c_uint)]


GraphPtr = POINTER(Graph)
