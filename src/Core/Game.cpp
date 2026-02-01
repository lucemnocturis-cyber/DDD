#include "Game.h"
#include "Engine.h"
#include "ResourceManager.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/ParticleSystem.h"
#include "../Graphics/ScreenEffects.h"
#include "../Graphics/TransitionManager.h"
#include "../Audio/SoundManager.h"
#include "../Audio/MusicManager.h"
#include "../Gameplay/Board.h"
#include "../Gameplay/TurnManager.h"
#include "../Gameplay/WaveManager.h"
#include "../Gameplay/SelectionManager.h"
#include "../Gameplay/Unit.h"
#include "../Gameplay/Dice.h"
#include "../UI/UIManager.h"
#include "../UI/HUD.h"
#include "../UI/DicePanel.h"
#include "../UI/TutorialSystem.h"
#include "../Utils/Logger.h"

namespace DDD {

Game::Game(Engine& engine)
    : m_engine(engine)
{
}

Game::~Game() = default;

bool Game::Initialize() {
    Logger::Info("Initializing game...");
    
    // Create particle system
    m_particleSystem = std::make_unique<ParticleSystem>();
    
    // Create screen effects
    m_screenEffects = std::make_unique<ScreenEffects>();
    
    // Create transition manager
    m_transitionManager = std::make_unique<TransitionManager>();
    
    // Create gameplay systems
    m_board = std::make_unique<Board>();
    if (!m_board->Initialize()) {
        Logger::Error("Board initialization failed");
        return false;
    }
    
    // Center the board on screen (accounting for UI panels)
    int boardWidth = Board::WIDTH * Board::CELL_SIZE;
    int boardHeight = Board::HEIGHT * Board::CELL_SIZE;
    int offsetX = (1280 - boardWidth) / 2;  // Assuming 1280 width
    int offsetY = 110;  // Leave room for HUD at top
    m_board->SetOffset(offsetX, offsetY);
    
    m_turnManager = std::make_unique<TurnManager>(*this);
    m_turnManager->Initialize();
    
    m_waveManager = std::make_unique<WaveManager>(*this);
    
    // Create selection manager
    m_selectionManager = std::make_unique<SelectionManager>();
    m_selectionManager->SetBoard(m_board.get());
    SetupSelectionCallbacks();
    SetupTurnCallbacks();
    
    // Create UI
    m_uiManager = std::make_unique<UIManager>(*this);
    if (!m_uiManager->Initialize()) {
        Logger::Error("UIManager initialization failed");
        return false;
    }
    
    // Create tutorial system
    m_tutorialSystem = std::make_unique<TutorialSystem>(*this);
    m_tutorialSystem->Initialize();
    
    // Start at main menu
    ChangeState(GameStateType::MainMenu);
    
    Logger::Info("Game initialization complete");
    return true;
}

void Game::SetupSelectionCallbacks() {
    // When a unit is selected
    m_selectionManager->SetOnUnitSelected([this](std::shared_ptr<Unit> unit) {
        Logger::Debug("Unit selected: {}", unit->GetClassName());
        // Clear dice selection when selecting a unit
        if (m_uiManager && m_uiManager->GetDicePanel()) {
            m_uiManager->GetDicePanel()->ClearSelection();
        }
        // Show unit info panel
        if (m_uiManager) {
            m_uiManager->ShowUnitInfo(unit);
        }
    });
    
    // When a unit moves
    m_selectionManager->SetOnUnitMoved([this](std::shared_ptr<Unit> unit, Position newPos) {
        PlaySound(SoundID::UnitMove);
        Logger::Debug("{} moved to ({},{})", unit->GetClassName(), newPos.x, newPos.y);
    });
    
    // When a unit attacks
    m_selectionManager->SetOnUnitAttacked([this](std::shared_ptr<Unit> attacker, std::shared_ptr<Unit> target) {
        // Calculate damage
        int baseDamage = std::max(1, attacker->GetStats().atk - target->GetStats().def);
        
        // Check for critical hit (10% chance, double damage)
        bool isCritical = (rand() % 100) < 10;
        int damage = isCritical ? baseDamage * 2 : baseDamage;
        
        // Play attack sound
        if (isCritical) {
            PlaySound(SoundID::AttackCrit);
            PlaySound(SoundID::DamageCrit);
        } else {
            PlaySound(SoundID::Attack);
            PlaySound(SoundID::Damage);
        }
        
        // Get screen position for effects
        int screenX, screenY;
        m_board->GetCellScreenPosition(target->GetPosition().x, target->GetPosition().y, screenX, screenY);
        int centerX = screenX + Board::CELL_SIZE / 2;
        int centerY = screenY + Board::CELL_SIZE / 2;
        
        // Screen effects based on hit type
        if (m_screenEffects) {
            if (isCritical) {
                m_screenEffects->OnCriticalHit();
            } else {
                m_screenEffects->OnHit();
            }
            
            // Extra effect if player unit is damaged
            if (target->GetOwner() == Owner::Player) {
                m_screenEffects->OnPlayerDamaged();
            }
        }
        
        // Spawn damage particles
        if (m_particleSystem) {
            m_particleSystem->SpawnDamageEffect(
                static_cast<float>(centerX), 
                static_cast<float>(centerY), 
                damage, 
                isCritical
            );
        }
        
        // Show damage number
        if (m_uiManager && m_uiManager->GetHUD()) {
            m_uiManager->GetHUD()->ShowDamageNumber(
                centerX, 
                screenY, 
                damage, 
                isCritical
            );
            
            // Show critical message
            if (isCritical) {
                m_uiManager->GetHUD()->ShowMessage("CRITICAL HIT!", 1.0f);
            }
        }
        
        // Check if target died
        if (target->IsDead()) {
            // Death sound and screen effect
            PlaySound(SoundID::Death);
            if (m_screenEffects) {
                m_screenEffects->OnDeath();
            }
            
            // Spawn death particles
            if (m_particleSystem) {
                SDL_Color unitColor = {150, 150, 150, 255};
                // Get color based on class
                std::string className = target->GetClassName();
                if (className == "Mage" || className == "Elementalist") {
                    unitColor = {100, 100, 220, 255};
                } else if (className == "Soldier" || className == "Knight") {
                    unitColor = {200, 80, 80, 255};
                } else if (className == "Rogue" || className == "Assassin") {
                    unitColor = {80, 180, 100, 255};
                }
                m_particleSystem->SpawnDeathEffect(
                    static_cast<float>(centerX),
                    static_cast<float>(centerY),
                    unitColor
                );
            }
            
            // Calculate rewards
            int expReward = 10 + target->GetLevel() * 5;
            int goldReward = 5 + target->GetLevel() * 3;
            
            // Award EXP to attacker
            attacker->GainExp(expReward);
            
            // Award gold to player if player unit got the kill
            if (attacker->GetOwner() == Owner::Player) {
                AddGold(goldReward);
                AddScore(25 * target->GetLevel());
                PlaySound(SoundID::GoldPickup);
                
                if (m_uiManager && m_uiManager->GetHUD()) {
                    m_uiManager->GetHUD()->ShowMessage("+" + std::to_string(goldReward) + " Gold!", 1.5f);
                }
            }
            
            // Remove dead unit from board
            m_board->RemoveUnit(target);
            m_turnManager->OnUnitDied(target);
            
            Logger::Info("{} killed {}! +{} EXP, +{} Gold", 
                        attacker->GetClassName(), target->GetClassName(), expReward, goldReward);
            
            // Check for level up
            if (attacker->CanPromote()) {
                PlaySound(SoundID::LevelUp);
                if (m_uiManager && m_uiManager->GetHUD()) {
                    m_uiManager->GetHUD()->ShowMessage(attacker->GetClassName() + " ready to promote!", 2.0f);
                }
            }
        }
    });
    
    // When a dice is placed
    m_selectionManager->SetOnDicePlaced([this](std::shared_ptr<Dice> dice, Position pos) {
        // Check if player can afford the dice
        int cost = dice->GetCost();
        if (m_playerGold < cost) {
            Logger::Warning("Not enough gold to place dice (need {}, have {})", cost, m_playerGold);
            if (m_uiManager && m_uiManager->GetHUD()) {
                m_uiManager->GetHUD()->ShowMessage("Not enough gold!", 1.5f);
            }
            return;
        }
        
        // Spend gold
        SpendGold(cost);
        
        // Place the dice on the board
        if (m_board->PlaceDice(dice, pos.x, pos.y, Owner::Player)) {
            // Remove dice from player's hand
            if (m_uiManager && m_uiManager->GetDicePanel()) {
                m_uiManager->GetDicePanel()->RemoveDice(dice);
            }
            
            // Show placement message
            if (m_uiManager && m_uiManager->GetHUD()) {
                m_uiManager->GetHUD()->ShowMessage(dice->GetClassName() + " summoned!", 1.0f);
            }
            
            Logger::Info("Placed {} at ({},{}) for {} gold", 
                        dice->GetClassName(), pos.x, pos.y, cost);
        }
    });
    
    // When selection is cleared
    m_selectionManager->SetOnSelectionCleared([this]() {
        if (m_uiManager) {
            m_uiManager->HideUnitInfo();
        }
    });
}

void Game::SetupTurnCallbacks() {
    // When turn changes
    m_turnManager->SetOnTurnChanged([this](bool isPlayerTurn, int turnNumber) {
        if (m_uiManager && m_uiManager->GetHUD()) {
            std::string msg = isPlayerTurn ? "YOUR TURN" : "ENEMY TURN";
            m_uiManager->GetHUD()->ShowMessage(msg, 1.5f);
        }
        
        // Clear selection when enemy turn starts
        if (!isPlayerTurn && m_selectionManager) {
            m_selectionManager->ClearSelection();
        }
    });
    
    // When enemy attacks
    m_turnManager->SetOnEnemyAttack([this](std::shared_ptr<Unit> attacker, 
                                           std::shared_ptr<Unit> target, int damage) {
        if (m_uiManager && m_uiManager->GetHUD()) {
            int screenX, screenY;
            m_board->GetCellScreenPosition(target->GetPosition().x, target->GetPosition().y, screenX, screenY);
            m_uiManager->GetHUD()->ShowDamageNumber(
                screenX + Board::CELL_SIZE / 2,
                screenY,
                damage,
                false
            );
        }
        
        // Check if player unit died
        if (target->IsDead() && target->GetOwner() == Owner::Player) {
            if (m_uiManager && m_uiManager->GetHUD()) {
                m_uiManager->GetHUD()->ShowMessage(target->GetClassName() + " was defeated!", 2.0f);
            }
        }
    });
}

void Game::OnMouseMove(int x, int y) {
    m_mouseX = x;
    m_mouseY = y;
    
    if (m_currentState == GameStateType::Battle && m_selectionManager) {
        m_selectionManager->Update(x, y, m_board->GetOffsetX(), m_board->GetOffsetY(), m_board->GetCellSize());
    }
}

void Game::OnMouseClick(int button) {
    // Check if tutorial wants to handle the click first
    if (m_tutorialSystem && m_tutorialSystem->IsShowingHint()) {
        if (button == 1) {  // Left click
            if (m_tutorialSystem->OnClick(m_mouseX, m_mouseY)) {
                // Tutorial consumed the click, show next hint in sequence
                if (!m_tutorialSystem->IsShowingHint()) {
                    // Chain to next hint based on what was just dismissed
                    if (!m_tutorialSystem->HasShownHint(HintType::DiceExplanation)) {
                        m_tutorialSystem->ShowHint(HintType::DiceExplanation);
                    } else if (!m_tutorialSystem->HasShownHint(HintType::PlaceDice)) {
                        m_tutorialSystem->ShowHint(HintType::PlaceDice);
                    }
                }
                return;
            }
        }
    }
    
    if (m_currentState == GameStateType::Battle && m_selectionManager) {
        if (button == 1) {  // Left click
            m_selectionManager->OnLeftClick();
        } else if (button == 3) {  // Right click
            m_selectionManager->OnRightClick();
        }
    }
}

void Game::OnKeyPress(int key) {
    // SDL_SCANCODE values
    const int SDL_SCANCODE_SPACE = 44;
    const int SDL_SCANCODE_ESCAPE = 41;
    
    switch (m_currentState) {
        case GameStateType::MainMenu:
            if (key == SDL_SCANCODE_SPACE) {
                // Start new game
                m_currentWave = 1;
                m_playerGold = STARTING_GOLD;
                m_score = 0;
                StartNewWave();
            } else if (key == SDL_SCANCODE_ESCAPE) {
                // Quit (handled by Engine)
            }
            break;
            
        case GameStateType::Battle:
            if (key == SDL_SCANCODE_SPACE) {
                // End turn
                if (m_turnManager->IsPlayerTurn()) {
                    m_turnManager->EndTurn();
                    m_selectionManager->ClearSelection();
                }
            } else if (key == SDL_SCANCODE_ESCAPE) {
                // Cancel selection or pause
                if (m_selectionManager->GetMode() != SelectionMode::Idle) {
                    m_selectionManager->ClearSelection();
                } else {
                    PushState(GameStateType::Paused);
                }
            }
            break;
            
        case GameStateType::Paused:
            if (key == SDL_SCANCODE_ESCAPE) {
                PopState();
            }
            break;
            
        case GameStateType::WaveReward:
            if (key == SDL_SCANCODE_SPACE) {
                StartNewWave();
            }
            break;
            
        case GameStateType::GameOver:
            if (key == SDL_SCANCODE_SPACE) {
                ChangeState(GameStateType::MainMenu);
            }
            break;
            
        default:
            break;
    }
}

void Game::Update(float deltaTime) {
    UpdateState(deltaTime);
}

void Game::Render(Renderer& renderer) {
    RenderState(renderer);
}

void Game::ChangeState(GameStateType newState) {
    if (m_currentState != newState) {
        ExitState(m_currentState);
        m_currentState = newState;
        EnterState(newState);
        Logger::Info("Game state changed to: {}", static_cast<int>(newState));
    }
}

void Game::TransitionToState(GameStateType newState, TransitionType transition) {
    if (m_transitionManager && !m_transitionManager->IsTransitioning()) {
        m_transitionManager->StartTransition(transition, 0.5f, [this, newState]() {
            ChangeState(newState);
        });
    } else {
        // Fallback if no transition manager or already transitioning
        ChangeState(newState);
    }
}

void Game::PushState(GameStateType state) {
    m_stateStack.push(m_currentState);
    m_currentState = state;
    EnterState(state);
    Logger::Info("Pushed state: {}", static_cast<int>(state));
}

void Game::PopState() {
    if (!m_stateStack.empty()) {
        ExitState(m_currentState);
        m_currentState = m_stateStack.top();
        m_stateStack.pop();
        Logger::Info("Popped to state: {}", static_cast<int>(m_currentState));
    }
}

void Game::EnterState(GameStateType state) {
    switch (state) {
        case GameStateType::MainMenu:
            m_uiManager->ShowMainMenu();
            if (g_musicManager) {
                g_musicManager->Play(MusicTrack::Menu);
            }
            break;
            
        case GameStateType::Battle:
            m_board->Reset();
            m_turnManager->StartBattle();
            m_uiManager->ShowBattleUI();
            if (m_selectionManager) {
                m_selectionManager->ClearSelection();
            }
            if (g_musicManager) {
                g_musicManager->Play(MusicTrack::Battle);
            }
            // Show tutorial hints for first game
            if (m_tutorialSystem && !m_tutorialSystem->HasShownHint(HintType::Welcome)) {
                m_tutorialSystem->ShowHint(HintType::Welcome);
            }
            break;
            
        case GameStateType::WaveReward:
            m_uiManager->ShowWaveRewardUI();
            break;
            
        case GameStateType::Shop:
            m_uiManager->ShowShopUI();
            if (g_musicManager) {
                g_musicManager->Play(MusicTrack::Shop);
            }
            break;
            
        case GameStateType::Promotion:
            m_uiManager->ShowPromotionUI();
            break;
            
        case GameStateType::GameOver:
            m_uiManager->ShowGameOverUI(m_lastGameVictory);
            if (g_musicManager) {
                if (m_lastGameVictory) {
                    g_musicManager->Play(MusicTrack::Victory, 0.1f);
                } else {
                    g_musicManager->Play(MusicTrack::Defeat, 0.1f);
                }
            }
            break;
            
        case GameStateType::Settings:
            m_uiManager->ShowSettingsUI();
            break;
            
        case GameStateType::Paused:
            m_uiManager->ShowPauseMenu();
            break;
            
        default:
            break;
    }
}

void Game::ExitState(GameStateType state) {
    switch (state) {
        case GameStateType::Battle:
            break;
        default:
            break;
    }
}

void Game::UpdateState(float deltaTime) {
    // Update screen effects first - this may modify deltaTime for hit pause
    float effectiveDeltaTime = deltaTime;
    if (m_screenEffects) {
        effectiveDeltaTime = m_screenEffects->Update(deltaTime);
    }
    
    // Update transition manager
    if (m_transitionManager) {
        m_transitionManager->Update(deltaTime);
    }
    
    // Update music fading
    if (g_musicManager) {
        g_musicManager->Update(deltaTime);
    }
    
    // Update tutorial system
    if (m_tutorialSystem) {
        m_tutorialSystem->Update(deltaTime);
    }
    
    // UI always updates at full speed
    m_uiManager->Update(deltaTime);
    
    // Update particles (even during hit pause for visual continuity)
    if (m_particleSystem) {
        m_particleSystem->Update(deltaTime);
    }
    
    // Game logic uses effective delta time (paused during hit pause)
    switch (m_currentState) {
        case GameStateType::Battle:
            m_board->Update(effectiveDeltaTime);
            m_turnManager->Update(effectiveDeltaTime);
            
            // Check for contextual tutorial hints
            if (m_tutorialSystem) {
                m_tutorialSystem->CheckContextualHints();
            }
            break;
            
        case GameStateType::MainMenu:
        case GameStateType::WaveReward:
        case GameStateType::Shop:
        case GameStateType::Promotion:
        case GameStateType::GameOver:
        case GameStateType::Settings:
        case GameStateType::Paused:
            break;
            
        default:
            break;
    }
}

void Game::RenderState(Renderer& renderer) {
    switch (m_currentState) {
        case GameStateType::Battle: {
            // Apply screen shake offset to board
            if (m_screenEffects) {
                int shakeX = 0, shakeY = 0;
                m_screenEffects->ApplyShakeOffset(shakeX, shakeY);
                
                // Temporarily adjust board offset
                int origOffsetX, origOffsetY;
                m_board->GetOffset(origOffsetX, origOffsetY);
                m_board->SetOffset(origOffsetX + shakeX, origOffsetY + shakeY);
                
                // Render battlefield with shake
                m_board->Render(renderer);
                
                // Restore original offset
                m_board->SetOffset(origOffsetX, origOffsetY);
            } else {
                m_board->Render(renderer);
            }
            
            // Render selection highlights
            if (m_selectionManager) {
                m_board->RenderSelectionHighlights(
                    renderer,
                    m_selectionManager->GetHoveredCell(),
                    m_selectionManager->IsMouseOverBoard(),
                    m_selectionManager->GetValidMoveCells(),
                    m_selectionManager->GetValidAttackCells(),
                    m_selectionManager->GetValidPlacementCells()
                );
                
                // Render unfurl preview when placing a dice
                if (m_selectionManager->GetMode() == SelectionMode::PlacingDice &&
                    m_selectionManager->IsMouseOverBoard() &&
                    m_selectionManager->GetSelectedDice()) {
                    
                    auto dice = m_selectionManager->GetSelectedDice();
                    Position hovered = m_selectionManager->GetHoveredCell();
                    
                    // Check if this is a valid placement
                    bool isValid = m_board->IsValidPlacement(*dice, hovered.x, hovered.y, Owner::Player);
                    
                    // Also check if player can afford it
                    if (m_playerGold < dice->GetCost()) {
                        isValid = false;
                    }
                    
                    m_board->RenderUnfurlPreview(renderer, hovered, 
                                                  dice->GetCurrentFace().unfurl, isValid);
                }
            }
            
            // Render particles on top of battlefield
            if (m_particleSystem) {
                m_particleSystem->Render(renderer);
            }
            
            // Render screen flash overlay
            if (m_screenEffects) {
                m_screenEffects->RenderFlash(renderer);
            }
            break;
        }
            
        case GameStateType::MainMenu:
        case GameStateType::WaveReward:
        case GameStateType::Shop:
        case GameStateType::Promotion:
        case GameStateType::GameOver:
        case GameStateType::Settings:
        case GameStateType::Paused:
            renderer.SetDrawColor(26, 26, 46, 255);
            break;
            
        default:
            break;
    }
    
    m_uiManager->Render(renderer);
    
    // Render tutorial hints on top of UI
    if (m_tutorialSystem) {
        m_tutorialSystem->Render(renderer);
    }
    
    // Render transition overlay last (on top of everything)
    if (m_transitionManager) {
        m_transitionManager->Render(renderer);
    }
}

bool Game::SpendGold(int amount) {
    if (m_playerGold >= amount) {
        m_playerGold -= amount;
        return true;
    }
    return false;
}

void Game::StartNewWave() {
    // Reset dice rolls at start of wave
    if (m_uiManager && m_uiManager->GetDicePanel()) {
        m_uiManager->GetDicePanel()->ResetRolls();
    }
    
    m_waveManager->StartWave(m_currentWave);
    m_turnManager->StartBattle();
    ChangeState(GameStateType::Battle);
    
    // Play appropriate music
    bool isBossWave = (m_currentWave % 5 == 0);
    if (g_musicManager) {
        if (isBossWave) {
            g_musicManager->Play(MusicTrack::BossBattle);
        } else {
            g_musicManager->Play(MusicTrack::Battle);
        }
    }
    
    // Play wave start sound
    PlaySound(SoundID::WaveStart);
    
    // Show wave start message
    if (m_uiManager && m_uiManager->GetHUD()) {
        std::string msg = "WAVE " + std::to_string(m_currentWave);
        if (isBossWave) {
            msg += " - BOSS WAVE!";
        }
        m_uiManager->GetHUD()->ShowMessage(msg, 2.0f);
    }
    
    Logger::Info("Starting wave {}", m_currentWave);
}

void Game::OnWaveComplete() {
    // Get rewards from WaveManager
    m_waveManager->OnWaveComplete();
    
    int goldReward = m_waveManager->GetWaveGoldReward();
    int scoreReward = m_waveManager->GetWaveScoreReward();
    
    Logger::Info("Wave {} complete! +{} Gold, +{} Score", m_currentWave, goldReward, scoreReward);
    
    // Play wave complete sound
    PlaySound(SoundID::WaveComplete);
    
    // Show completion message
    if (m_uiManager && m_uiManager->GetHUD()) {
        m_uiManager->GetHUD()->ShowMessage("WAVE COMPLETE! +" + std::to_string(goldReward) + " Gold", 2.5f);
    }
    
    m_currentWave++;
    
    if (m_currentWave > MAX_WAVES) {
        OnGameOver(true);
    } else {
        // For now, auto-start next wave after delay
        // TODO: Show wave reward screen with dice choices
        ChangeState(GameStateType::WaveReward);
    }
}

void Game::OnGameOver(bool victory) {
    m_lastGameVictory = victory;
    
    if (victory) {
        PlaySound(SoundID::Victory);
        Logger::Info("Victory! Final score: {}", m_score);
        if (m_uiManager && m_uiManager->GetHUD()) {
            m_uiManager->GetHUD()->ShowMessage("VICTORY! Score: " + std::to_string(m_score), 5.0f);
        }
    } else {
        PlaySound(SoundID::Defeat);
        Logger::Info("Defeat at wave {}. Final score: {}", m_currentWave, m_score);
        if (m_uiManager && m_uiManager->GetHUD()) {
            m_uiManager->GetHUD()->ShowMessage("GAME OVER - Wave " + std::to_string(m_currentWave), 5.0f);
        }
    }
    
    ChangeState(GameStateType::GameOver);
}

void Game::ResetGame() {
    m_currentWave = 1;
    m_playerGold = STARTING_GOLD;
    m_score = 0;
    m_lastGameVictory = false;
    
    // Reset board
    if (m_board) {
        m_board->Reset();
    }
    
    // Clear dice panel
    if (m_uiManager && m_uiManager->GetDicePanel()) {
        m_uiManager->GetDicePanel()->ClearDice();
    }
    
    // Clear selection
    if (m_selectionManager) {
        m_selectionManager->ClearSelection();
    }
    
    Logger::Info("Game reset for new playthrough");
}

} // namespace DDD
