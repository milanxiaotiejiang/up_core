//
// Created by noodles on 25-2-19.
//
#include <pybind11/pybind11.h>

extern "C" {
#include "add.h"
}

namespace py = pybind11;

PYBIND11_MODULE(up_tech, m) {
    m.doc() = "up_tech module for python";

    m.def("add", &add, "计算两个整数的和。"
                       "参数："
                       "a 第一个加数。"
                       "b 第二个加数。"
                       "返回值：两个整数的和。");

}
