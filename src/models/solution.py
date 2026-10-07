from ctypes import *

# Custom imports
from models.tree_node import TreeNodePtr

"""
typedef struct Solution {
    double* x;
    double z;
    int status;
    TreeNode* tree;
    double* tree_values;
    int tree_size;
} Solution;
"""


class Solution(Structure):
    _fields_ = [
        ("x", POINTER(c_double)),
        ("z", c_double),
        ("status", c_int),
        ("tree", TreeNodePtr),
        ("tree_values", POINTER(c_double)),
        ("tree_size", c_int),
    ]


SolutionPtr = POINTER(Solution)
