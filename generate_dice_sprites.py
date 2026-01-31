#!/usr/bin/env python3
"""
Generate placeholder dice sprites for Dungeon Dice Duelists
Creates dice for all 32 dice types with different rarities and types
"""

from PIL import Image, ImageDraw, ImageFont
import os

# Dice types from the game
DICE_TYPES = {
    # Standard Dice (Common - White)
    "Warrior_D6": {"rarity": "Common", "color": (200, 200, 200), "faces": 6},
    "Mage_D6": {"rarity": "Common", "color": (200, 200, 200), "faces": 6},
    "Ranger_D6": {"rarity": "Common", "color": (200, 200, 200), "faces": 6},
    "Rogue_D6": {"rarity": "Common", "color": (200, 200, 200), "faces": 6},
    "Cleric_D6": {"rarity": "Common", "color": (200, 200, 200), "faces": 6},
    "Tank_D6": {"rarity": "Common", "color": (200, 200, 200), "faces": 6},
    
    # Weighted Dice (Uncommon - Green)
    "Heavy_Warrior": {"rarity": "Uncommon", "color": (100, 200, 100), "faces": 6},
    "Heavy_Mage": {"rarity": "Uncommon", "color": (100, 200, 100), "faces": 6},
    "Heavy_Ranger": {"rarity": "Uncommon", "color": (100, 200, 100), "faces": 6},
    "Heavy_Rogue": {"rarity": "Uncommon", "color": (100, 200, 100), "faces": 6},
    
    # Lucky Dice (Rare - Blue)
    "Lucky_Warrior": {"rarity": "Rare", "color": (100, 150, 255), "faces": 6},
    "Lucky_Mage": {"rarity": "Rare", "color": (100, 150, 255), "faces": 6},
    "Lucky_Ranger": {"rarity": "Rare", "color": (100, 150, 255), "faces": 6},
    "Lucky_Cleric": {"rarity": "Rare", "color": (100, 150, 255), "faces": 6},
    
    # Elemental Dice (Epic - Purple)
    "Fire_Die": {"rarity": "Epic", "color": (200, 100, 200), "faces": 6},
    "Ice_Die": {"rarity": "Epic", "color": (200, 100, 200), "faces": 6},
    "Lightning_Die": {"rarity": "Epic", "color": (200, 100, 200), "faces": 6},
    "Earth_Die": {"rarity": "Epic", "color": (200, 100, 200), "faces": 6},
    "Holy_Die": {"rarity": "Epic", "color": (200, 100, 200), "faces": 6},
    "Shadow_Die": {"rarity": "Epic", "color": (200, 100, 200), "faces": 6},
    
    # Legendary Dice (Legendary - Gold)
    "Dragon_Die": {"rarity": "Legendary", "color": (255, 215, 0), "faces": 6},
    "Phoenix_Die": {"rarity": "Legendary", "color": (255, 215, 0), "faces": 6},
    "Titan_Die": {"rarity": "Legendary", "color": (255, 215, 0), "faces": 6},
    "Celestial_Die": {"rarity": "Legendary", "color": (255, 215, 0), "faces": 6},
    
    # Special Dice (Various)
    "Wild_Die": {"rarity": "Epic", "color": (150, 150, 255), "faces": 6},
    "Cursed_Die": {"rarity": "Rare", "color": (150, 50, 150), "faces": 6},
    "Blessed_Die": {"rarity": "Rare", "color": (255, 255, 150), "faces": 6},
    "Chaos_Die": {"rarity": "Epic", "color": (255, 100, 100), "faces": 6},
    "Balanced_Die": {"rarity": "Uncommon", "color": (150, 150, 150), "faces": 6},
    "Loaded_Die": {"rarity": "Rare", "color": (100, 255, 100), "faces": 6},
    "Prismatic_Die": {"rarity": "Legendary", "color": (200, 150, 255), "faces": 6},
    "Void_Die": {"rarity": "Legendary", "color": (100, 50, 150), "faces": 6},
}

def draw_die_face(draw, x, y, size, color, face_value):
    """Draw a dice face with dots representing the value"""
    # Draw main die body
    draw.rounded_rectangle([x, y, x + size, y + size], radius=8, 
                          fill=color, outline=(255, 255, 255), width=3)
    
    # Add some shading for 3D effect
    shade_color = tuple(max(c - 40, 0) for c in color)
    draw.rectangle([x + 2, y + size - 10, x + size - 2, y + size - 2], 
                  fill=shade_color)
    
    # Draw dots based on face value
    dot_size = 6
    padding = 15
    center = size // 2
    
    dot_positions = {
        1: [(center, center)],
        2: [(padding, padding), (size - padding, size - padding)],
        3: [(padding, padding), (center, center), (size - padding, size - padding)],
        4: [(padding, padding), (size - padding, padding), 
            (padding, size - padding), (size - padding, size - padding)],
        5: [(padding, padding), (size - padding, padding), (center, center),
            (padding, size - padding), (size - padding, size - padding)],
        6: [(padding, padding), (size - padding, padding),
            (padding, center), (size - padding, center),
            (padding, size - padding), (size - padding, size - padding)],
    }
    
    for dx, dy in dot_positions.get(face_value, [(center, center)]):
        draw.ellipse([x + dx - dot_size, y + dy - dot_size,
                     x + dx + dot_size, y + dy + dot_size],
                    fill=(255, 255, 255))

def create_dice_sprite(dice_name, properties, size=128):
    """Create a dice sprite showing a single face"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Draw the die showing face "6" (most common for display)
    draw_die_face(draw, 4, 4, size - 8, properties['color'], 6)
    
    # Add rarity border
    border_colors = {
        "Common": (200, 200, 200),
        "Uncommon": (100, 200, 100),
        "Rare": (100, 150, 255),
        "Epic": (200, 100, 200),
        "Legendary": (255, 215, 0),
    }
    
    border_color = border_colors.get(properties['rarity'], (255, 255, 255))
    draw.rectangle([0, 0, size - 1, size - 1], outline=border_color, width=4)
    
    return img

def create_dice_spritesheet():
    """Create a spritesheet containing all dice sprites"""
    sprite_size = 128
    cols = 8
    rows = 4
    
    sheet_width = cols * sprite_size
    sheet_height = rows * sprite_size
    
    spritesheet = Image.new('RGBA', (sheet_width, sheet_height), (0, 0, 0, 0))
    
    # Arrange dice in grid
    dice_list = list(DICE_TYPES.items())
    for idx, (dice_name, properties) in enumerate(dice_list):
        col = idx % cols
        row = idx // cols
        
        sprite = create_dice_sprite(dice_name, properties, sprite_size)
        x = col * sprite_size
        y = row * sprite_size
        spritesheet.paste(sprite, (x, y))
    
    return spritesheet

def create_individual_dice():
    """Create individual dice sprite files"""
    for dice_name, properties in DICE_TYPES.items():
        sprite = create_dice_sprite(dice_name, properties, 128)
        filename = f"assets/sprites/dice/{dice_name.lower()}.png"
        sprite.save(filename)
        print(f"Created: {filename}")

def create_dice_faces():
    """Create individual face sprites for dice (1-6)"""
    for face in range(1, 7):
        img = Image.new('RGBA', (64, 64), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        draw_die_face(draw, 2, 2, 60, (200, 200, 200), face)
        filename = f"assets/sprites/dice/face_{face}.png"
        img.save(filename)
        print(f"Created: {filename}")

def main():
    print("Generating dice sprites for Dungeon Dice Duelists...")
    
    # Create spritesheet
    spritesheet = create_dice_spritesheet()
    spritesheet.save("assets/sprites/dice/dice_spritesheet.png")
    print("Created: assets/sprites/dice/dice_spritesheet.png")
    
    # Create individual dice
    create_individual_dice()
    
    # Create dice faces
    create_dice_faces()
    
    print(f"\n✓ Generated {len(DICE_TYPES)} dice sprites + 6 face sprites successfully!")

if __name__ == "__main__":
    main()
