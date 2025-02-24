//
// Created by noodles on 25-2-19.
//
#include "add.h"
#include "servo.h"
#include <gtest/gtest.h>

TEST(AddTest, PositiveNumbers) {
    EXPECT_EQ(3, add(1, 2));
}

TEST(AddTest, NegativeNumbers) {
    EXPECT_EQ(-3, add(-1, -2));
}

TEST(AddTest, Zero) {
    EXPECT_EQ(0, add(0, 0));
}

