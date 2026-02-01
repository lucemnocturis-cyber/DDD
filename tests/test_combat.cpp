#include <gtest/gtest.h>
#include "Gameplay/Unit.h"

namespace DDD {
namespace Test {

TEST(CombatTest, TakeDamage) {
    Unit unit;
    UnitStats stats{100, 100, 10, 5, 3, 1};
    unit.SetStats(stats);
    
    // Damage should be reduced by defense
    unit.TakeDamage(10);  // 10 - 5 = 5 actual damage
    EXPECT_EQ(unit.GetStats().hp, 95);
}

TEST(CombatTest, MinimumDamage) {
    Unit unit;
    UnitStats stats{100, 100, 10, 20, 3, 1};
    unit.SetStats(stats);
    
    // Even with high defense, minimum damage is 1
    unit.TakeDamage(5);  // 5 - 20 = -15, but minimum is 1
    EXPECT_EQ(unit.GetStats().hp, 99);
}

TEST(CombatTest, Death) {
    Unit unit;
    UnitStats stats{10, 10, 10, 0, 3, 1};
    unit.SetStats(stats);
    
    unit.TakeDamage(100);
    EXPECT_TRUE(unit.IsDead());
    EXPECT_EQ(unit.GetStats().hp, 0);
}

TEST(CombatTest, Heal) {
    Unit unit;
    UnitStats stats{50, 100, 10, 0, 3, 1};
    unit.SetStats(stats);
    
    unit.Heal(30);
    EXPECT_EQ(unit.GetStats().hp, 80);
    
    // Can't heal above max
    unit.Heal(100);
    EXPECT_EQ(unit.GetStats().hp, 100);
}

} // namespace Test
} // namespace DDD
