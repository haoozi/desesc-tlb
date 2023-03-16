
#include "config.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"


class TLB_test : public ::testing::Test {
protected:
  void SetUp() override {}

  void TearDown() override {
    // Graph_library::sync_all();
  }
}


TEST_F(TLB_test, trivial) { EXPECT_EQ(true, true); }
