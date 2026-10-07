from ctypes import *

"""
typedef struct TreeNode {
    int id;
    int parent;
    int depth;
    int branch_variable;
    int branch_sign;
    double branch_value;
    int status;
    double z;
} TreeNode;
"""


class TreeNode(Structure):
    _fields_ = [
        ("id", c_int),
        ("parent", c_int),
        ("depth", c_int),
        ("branch_variable", c_int),
        ("branch_sign", c_int),
        ("branch_value", c_double),
        ("status", c_int),
        ("z", c_double),
    ]


TreeNodePtr = POINTER(TreeNode)
