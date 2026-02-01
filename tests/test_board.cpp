#include <gtest/gtest.h>
#include "Gameplay/Board.h"

namespace DDD {
namespace Test {

TEST(BoardTest, Initialization) {
    Board board;
    EXPECT_TRUE(board.Initialize());
}

TEST(BoardTest, ValidPosition) {
    Board board;
    board.Initialize();
    
    EXPECT_TRUE(board.IsValidPosition(0, 0));
    EXPECT_TRUE(board.IsValidPosition(Board::WIDTH - 1, Board::HEIGHT - 1));
    EXPECT_FALSE(board.IsValidPosition(-1, 0));
    EXPECT_FALSE(board.IsValidPosition(Board::WIDTH, 0));
}

TEST(BoardTest, Distance) {
    Board board;
    board.Initialize();
    
    EXPECT_EQ(board.GetDistance({0, 0}, {0, 0}), 0);
    EXPECT_EQ(board.GetDistance({0, 0}, {1, 0}), 1);
    EXPECT_EQ(board.GetDistance({0, 0}, {1, 1}), 2);
    EXPECT_EQ(board.GetDistance({0, 0}, {3, 4}), 7);
}

} // namespace Test
} // namespace DDD
