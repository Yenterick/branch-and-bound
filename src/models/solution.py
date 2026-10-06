from ctypes import *

"""
typedef struct Solution {
    int* route;
    double cost;
    unsigned long long explored;
    int is_optimal;
} Solution;
"""


class Solution(Structure):
    _fields_ = [
        ("route", POINTER(c_int)),
        ("cost", c_double),
        ("explored", c_ulonglong),
        ("is_optimal", c_int),
    ]


SolutionPtr = POINTER(Solution)
