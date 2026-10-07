from ctypes import *

"""
typedef struct Problem {
    int num_variables;
    int num_constraints;
    int maximize;
    double* objective;
    double* coefficients;
    int* signs;
    double* rhs;
} Problem;
"""


class Problem(Structure):
    _fields_ = [
        ("num_variables", c_int),
        ("num_constraints", c_int),
        ("maximize", c_int),
        ("objective", POINTER(c_double)),
        ("coefficients", POINTER(c_double)),
        ("signs", POINTER(c_int)),
        ("rhs", POINTER(c_double)),
    ]


ProblemPtr = POINTER(Problem)
