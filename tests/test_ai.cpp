#include <gtest/gtest.h>
#include "Gameplay/Dice.h"

namespace DDD {
namespace Test {

TEST(DiceTest, CreateBaseDice) {
    auto mage = DiceFactory::CreateDice("Mage");
    ASSERT_NE(mage, nullptr);
    EXPECT_EQ(mage->GetClassName(), "Mage");
    EXPECT_EQ(mage->GetTier(), 0);
    EXPECT_EQ(mage->GetCost(), 1);
}

TEST(DiceTest, DiceRoll) {
    auto dice = DiceFactory::CreateDice("Soldier");
    ASSERT_NE(dice, nullptr);
    
    int index = dice->GetCurrentFaceIndex();
    EXPECT_GE(index, 0);
    EXPECT_LE(index, 5);
    
    // Roll again - should still be valid
    dice->Roll();
    index = dice->GetCurrentFaceIndex();
    EXPECT_GE(index, 0);
    EXPECT_LE(index, 5);
}

TEST(DiceTest, CreateUnit) {
    auto dice = DiceFactory::CreateDice("Rogue");
    ASSERT_NE(dice, nullptr);
    
    auto unit = dice->CreateUnit();
    ASSERT_NE(unit, nullptr);
    EXPECT_EQ(unit->GetClassName(), "Rogue");
    EXPECT_GT(unit->GetStats().hp, 0);
    EXPECT_GT(unit->GetStats().atk, 0);
}

TEST(DiceTest, PromotionOptions) {
    auto options = DiceFactory::GetPromotionOptions("Mage");
    EXPECT_EQ(options.size(), 3);
    
    options = DiceFactory::GetPromotionOptions("UnknownClass");
    EXPECT_TRUE(options.empty());
}

} // namespace Test
} // namespace DDD
