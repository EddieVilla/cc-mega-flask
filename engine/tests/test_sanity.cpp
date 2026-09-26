#include <gtest/gtest.h>

#include "me/version.hpp"

TEST(Sanity, Builds) { EXPECT_TRUE(true); }

TEST(Sanity, VersionIsSet) { EXPECT_EQ(me::version(), "0.1.0"); }
