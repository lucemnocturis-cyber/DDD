# Dungeon Dice Duelists - Complete Development Procedure

## How This Works
This document outlines every step needed to fully realize the game. After each step, simply type **"continue"** and I'll execute the next one. Each step builds on the previous, so we'll have a working game that progressively improves.

---

## PHASE 1: MINIMUM PLAYABLE GAME (Steps 1-10)
*Goal: A working game loop where you can place dice, move units, attack enemies, and win/lose*

### Step 1: Font System & Text Rendering
- [ ] Integrate SDL_ttf properly
- [ ] Create TextRenderer class
- [ ] Add a free pixel font (e.g., Press Start 2P)
- [ ] Render text on screen (gold, wave, HP)
**Deliverable**: Text displays correctly in-game

### Step 2: Mouse Input & Cell Selection
- [ ] Track mouse position on grid
- [ ] Highlight hovered cell
- [ ] Click to select cells
- [ ] Right-click to cancel
**Deliverable**: Visual feedback when hovering/clicking grid cells

### Step 3: Main Menu Screen
- [ ] Title screen with game logo
- [ ] "New Game" button
- [ ] "Settings" button (placeholder)
- [ ] "Quit" button
- [ ] Button hover/click states
**Deliverable**: Functional main menu that starts the game

### Step 4: Player Dice Panel (Left Side)
- [ ] Display available dice as cards
- [ ] Show dice class name and tier
- [ ] Show current rolled face stats
- [ ] Click to select a dice
- [ ] "Roll" button to reroll (costs gold)
**Deliverable**: Interactive dice selection panel

### Step 5: Dice Placement System
- [ ] Select dice from panel
- [ ] Show valid placement cells (highlighted)
- [ ] Click to place dice on board
- [ ] Execute unfurl territory claim
- [ ] Spawn unit from dice
**Deliverable**: Can place dice and claim territory

### Step 6: Unit Selection & Movement
- [ ] Click unit to select it
- [ ] Show unit stats panel
- [ ] Highlight valid movement cells
- [ ] Click to move unit
- [ ] Movement animation (lerp between cells)
**Deliverable**: Units can move around the board

### Step 7: Combat System
- [ ] Show valid attack targets (red highlight)
- [ ] Click enemy to attack
- [ ] Calculate and apply damage
- [ ] Damage number popup
- [ ] Death and removal
- [ ] Award EXP and gold on kill
**Deliverable**: Functional combat with visual feedback

### Step 8: Enemy Turn & Basic AI
- [ ] End turn button
- [ ] Enemy units take actions
- [ ] Move toward nearest player unit
- [ ] Attack if in range
- [ ] Visual delay between enemy actions
**Deliverable**: Enemies fight back

### Step 9: Wave System
- [ ] Wave start with enemy spawns
- [ ] Wave complete detection (all enemies dead)
- [ ] Wave reward screen
- [ ] "Next Wave" button
- [ ] Progressive difficulty
**Deliverable**: Multiple waves of enemies

### Step 10: Win/Lose Conditions
- [ ] Game over when all player units die
- [ ] Victory screen at wave 20
- [ ] Final score display
- [ ] "Play Again" and "Main Menu" buttons
**Deliverable**: Complete game loop from start to finish

---

## PHASE 2: VISUAL POLISH (Steps 11-20)
*Goal: Replace placeholder graphics with proper sprites and effects*

### Step 11: Arena Background
- [ ] Sand/dirt floor texture
- [ ] Stone wall border with brick pattern
- [ ] Corner decorations
- [ ] Subtle grid overlay
**Deliverable**: Professional-looking battlefield

### Step 12: Territory Visuals
- [ ] Player territory (blue tint)
- [ ] Enemy territory (red tint)
- [ ] Neutral cells (no tint)
- [ ] Territory conversion animation
**Deliverable**: Clear ownership visualization

### Step 13: Mage Sprite (Player & Enemy)
- [ ] Design 48x48 pixel art sprite
- [ ] Purple robes, wizard hat
- [ ] Glowing staff with particles
- [ ] Idle animation (2-4 frames)
- [ ] Blue tint for enemy variant
**Deliverable**: Animated mage sprites

### Step 14: Soldier Sprite (Player & Enemy)
- [ ] Design 48x48 pixel art sprite
- [ ] Plate armor, kite shield
- [ ] Sword and red cape
- [ ] Idle animation (2-4 frames)
- [ ] Blue tint for enemy variant
**Deliverable**: Animated soldier sprites

### Step 15: Rogue Sprite (Player & Enemy)
- [ ] Design 48x48 pixel art sprite
- [ ] Dark hood, glowing eyes
- [ ] Dual daggers
- [ ] Shadow wisps effect
- [ ] Blue tint for enemy variant
**Deliverable**: Animated rogue sprites

### Step 16: Dice Card UI
- [ ] Card frame design
- [ ] Class icon/portrait
- [ ] Stat display layout
- [ ] Face number indicator (1-6)
- [ ] Selected state glow
**Deliverable**: Polished dice card visuals

### Step 17: HP Bars & Status Icons
- [ ] Gradient HP bar
- [ ] HP bar border/frame
- [ ] Damage flash effect
- [ ] Status effect icons (poison, stun)
- [ ] Buff/debuff indicators
**Deliverable**: Clear unit status display

### Step 18: Damage Numbers
- [ ] Floating damage text
- [ ] Rise and fade animation
- [ ] Color coding (red=damage, green=heal)
- [ ] Critical hit styling
**Deliverable**: Satisfying damage feedback

### Step 19: Button & Panel Styling
- [ ] Fire Emblem-style button design
- [ ] Panel frames and backgrounds
- [ ] Hover and click states
- [ ] Disabled state
**Deliverable**: Cohesive UI theme

### Step 20: Screen Transitions
- [ ] Fade in/out between states
- [ ] Wave start banner
- [ ] Turn change indicator
- [ ] Smooth UI animations
**Deliverable**: Polished screen flow

---

## PHASE 3: FULL CONTENT (Steps 21-30)
*Goal: All 39 classes, abilities, and progression systems*

### Step 21: Tier 1 Mage Classes
- [ ] Elementalist (sprites + abilities)
- [ ] Warlock (sprites + abilities)
- [ ] Sage (sprites + abilities)
**Deliverable**: 3 mage promotions playable

### Step 22: Tier 1 Soldier Classes
- [ ] Knight (sprites + abilities)
- [ ] Paladin (sprites + abilities)
- [ ] Berserker (sprites + abilities)
**Deliverable**: 3 soldier promotions playable

### Step 23: Tier 1 Rogue Classes
- [ ] Assassin (sprites + abilities)
- [ ] Ranger (sprites + abilities)
- [ ] Thief (sprites + abilities)
**Deliverable**: 3 rogue promotions playable

### Step 24: Promotion System UI
- [ ] Promotion available indicator
- [ ] Promotion selection screen
- [ ] Show 3 promotion options
- [ ] Preview stats and abilities
- [ ] Confirm promotion
**Deliverable**: Full promotion flow

### Step 25: Shop System
- [ ] Shop screen UI
- [ ] Buy new dice (tier 0-3)
- [ ] Reroll current dice
- [ ] Gold cost display
- [ ] Purchase confirmation
**Deliverable**: Functional shop

### Step 26: Ability System - Attacks
- [ ] Fireball (2x2 AoE)
- [ ] Ice Shard (single + freeze)
- [ ] Lightning (line attack)
- [ ] Backstab (bonus damage)
- [ ] Ability cooldown tracking
**Deliverable**: Offensive abilities work

### Step 27: Ability System - Support
- [ ] Heal (restore HP)
- [ ] Shield (temp defense)
- [ ] Taunt (aggro control)
- [ ] Buff/debuff system
**Deliverable**: Support abilities work

### Step 28: Ability UI
- [ ] Ability buttons on unit panel
- [ ] Cooldown display
- [ ] Target selection mode
- [ ] AoE preview overlay
**Deliverable**: Abilities usable via UI

### Step 29: Tier 2 Classes (15 total)
- [ ] Design sprites for all Tier 2
- [ ] Implement unique abilities
- [ ] Balance stats
**Deliverable**: Mid-game content complete

### Step 30: Tier 3 Classes (12 total)
- [ ] Design sprites for all Tier 3
- [ ] Implement ultimate abilities
- [ ] Balance as "legendary" units
**Deliverable**: End-game content complete

---

## PHASE 4: AUDIO & EFFECTS (Steps 31-35)
*Goal: Sound effects, music, and particle systems*

### Step 31: Sound Effects - UI
- [ ] Button click
- [ ] Button hover
- [ ] Dice roll sound
- [ ] Menu open/close
- [ ] Error/invalid action
**Deliverable**: Responsive UI audio

### Step 32: Sound Effects - Combat
- [ ] Melee attack hit
- [ ] Ranged attack (shoot + hit)
- [ ] Magic cast sounds
- [ ] Unit death
- [ ] Level up jingle
**Deliverable**: Satisfying combat audio

### Step 33: Music Tracks
- [ ] Main menu theme
- [ ] Battle theme (looping)
- [ ] Boss battle theme
- [ ] Victory fanfare
- [ ] Defeat theme
**Deliverable**: Full soundtrack

### Step 34: Particle Effects
- [ ] Magic spell particles
- [ ] Hit impact sparks
- [ ] Heal effect
- [ ] Territory claim pulse
- [ ] Unit spawn effect
**Deliverable**: Visual juice

### Step 35: Screen Effects
- [ ] Screen shake on big hits
- [ ] Flash on damage
- [ ] Slow motion on kills
- [ ] Vignette effect
**Deliverable**: Impactful feedback

---

## PHASE 5: STEAM INTEGRATION (Steps 36-40)
*Goal: Achievements, leaderboards, cloud saves*

### Step 36: Steamworks SDK Setup
- [ ] Integrate Steam API
- [ ] Initialize on startup
- [ ] Handle Steam overlay
- [ ] Steam app ID configuration
**Deliverable**: Steam connection working

### Step 37: Achievements
- [ ] Define 20+ achievements
- [ ] First victory
- [ ] Wave milestones (5, 10, 15, 20)
- [ ] Class unlocks
- [ ] Secret achievements
- [ ] Achievement popup UI
**Deliverable**: Achievements unlock

### Step 38: Leaderboards
- [ ] Highest wave reached
- [ ] Best score
- [ ] Fastest wave 10 clear
- [ ] Submit scores to Steam
- [ ] View leaderboard UI
**Deliverable**: Competitive leaderboards

### Step 39: Cloud Saves
- [ ] Save game state to JSON
- [ ] Upload to Steam Cloud
- [ ] Download on game start
- [ ] Sync conflict handling
**Deliverable**: Progress persists across devices

### Step 40: Steam Store Assets
- [ ] Capsule images (460x215, 231x87)
- [ ] Screenshots (1920x1080 x5)
- [ ] Store description text
- [ ] System requirements
**Deliverable**: Ready for Steam store

---

## PHASE 6: FINAL POLISH (Steps 41-45)
*Goal: Bug fixes, balance, and release prep*

### Step 41: Balance Pass
- [ ] Playtest all 39 classes
- [ ] Adjust stat curves
- [ ] Tune ability cooldowns
- [ ] Wave difficulty scaling
- [ ] Gold economy balance
**Deliverable**: Fair and fun gameplay

### Step 42: Performance Optimization
- [ ] Profile frame times
- [ ] Optimize rendering
- [ ] Reduce memory allocations
- [ ] 60 FPS on min-spec
**Deliverable**: Smooth performance

### Step 43: Bug Fixing
- [ ] Fix all known issues
- [ ] Edge case handling
- [ ] Crash prevention
- [ ] Memory leak checks
**Deliverable**: Stable game

### Step 44: Tutorial
- [ ] First-time player tutorial
- [ ] Explain dice mechanics
- [ ] Explain territory
- [ ] Explain promotion
- [ ] Skip option
**Deliverable**: New player onboarding

### Step 45: Release Build
- [ ] Windows 64-bit build
- [ ] Linux build
- [ ] macOS build
- [ ] Create installer
- [ ] Final testing
**Deliverable**: GAME COMPLETE! 🎮

---

## Progress Tracker

| Phase | Steps | Status |
|-------|-------|--------|
| Phase 1: Minimum Playable | 1-10 | ⬜ Not Started |
| Phase 2: Visual Polish | 11-20 | ⬜ Not Started |
| Phase 3: Full Content | 21-30 | ⬜ Not Started |
| Phase 4: Audio & Effects | 31-35 | ⬜ Not Started |
| Phase 5: Steam Integration | 36-40 | ⬜ Not Started |
| Phase 6: Final Polish | 41-45 | ⬜ Not Started |

**Current Step: 1 of 45**

---

## Instructions

Type **"continue"** to execute the next step. I will:
1. Write all necessary code
2. Create any required assets
3. Update the project files
4. Provide a testable build
5. Mark the step complete

Let's build this game! 🎲⚔️
