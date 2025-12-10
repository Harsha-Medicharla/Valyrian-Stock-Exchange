#include <gtest/gtest.h>
#include "MatchingEngine.h"

TEST(test1,subtest1){
    ASSERT_TRUE(1 == 1);
}

int main(int argc,char* argv[]){
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}
