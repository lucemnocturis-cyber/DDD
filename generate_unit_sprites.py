#!/usr/bin/env python3
"""
Generate placeholder unit sprites for Dungeon Dice Duelists
Creates sprites for all 24 unit classes with distinct visual styles
"""

from PIL import Image, ImageDraw, ImageFont
import os

# Unit classes from the game (6 archetypes x 4 tiers = 24 classes)
UNIT_CLASSES = {
    # Warrior Archetype (Red)
    "Squire": {"color": (180, 50, 50), "shape": "square", "icon": "⚔"},
    "Knight": {"color": (200, 40, 40), "shape": "square", "icon": "⚔"},
    "Paladin": {"color": (220, 30, 30), "shape": "square", "icon": "⚔"},
    "Champion": {"color": (255, 20, 20), "shape": "square", "icon": "⚔"},
    
    # Mage Archetype (Blue)
    "Apprentice": {"color": (50, 50, 180), "shape": "diamond", "icon": "✦"},
    "Sorcerer": {"color": (40, 40, 200), "shape": "diamond", "icon": "✦"},
    "Archmage": {"color": (30, 30, 220), "shape": "diamond", "icon": "✦"},
    "Mystic": {"color": (20, 20, 255), "shape": "diamond", "icon": "✦"},
    
    # Ranger Archetype (Green)
    "Scout": {"color": (50, 180, 50), "shape": "triangle", "icon": "➳"},
    "Hunter": {"color": (40, 200, 40), "shape": "triangle", "icon": "➳"},
    "Sniper": {"color": (30, 220, 30), "shape": "triangle", "icon": "➳"},
    "Marksman": {"color": (20, 255, 20), "shape": "triangle", "icon": "➳"},
    
    # Rogue Archetype (Purple)
    "Thief": {"color": (150, 50, 150), "shape": "hexagon", "icon": "🗡"},
    "Assassin": {"color": (170, 40, 170), "shape": "hexagon", "icon": "🗡"},
    "Shadowblade": {"color": (190, 30, 190), "shape": "hexagon", "icon": "🗡"},
    "Phantom": {"color": (220, 20, 220), "shape": "hexagon", "icon": "🗡"},
    
    # Cleric Archetype (Yellow)
    "Acolyte": {"color": (200, 200, 50), "shape": "circle", "icon": "✚"},
    "Priest": {"color": (220, 220, 40), "shape": "circle", "icon": "✚"},
    "Bishop": {"color": (240, 240, 30), "shape": "circle", "icon": "✚"},
    "Saint": {"color": (255, 255, 20), "shape": "circle", "icon": "✚"},
    
    # Tank Archetype (Gray)
    "Guard": {"color": (120, 120, 120), "shape": "octagon", "icon": "🛡"},
    "Defender": {"color": (140, 140, 140), "shape": "octagon", "icon": "🛡"},
    "Fortress": {"color": (160, 160, 160), "shape": "octagon", "icon": "🛡"},
    "Juggernaut": {"color": (180, 180, 180), "shape": "octagon", "icon": "🛡"},
}

def draw_shape(draw, center_x, center_y, size, shape_type, color):
    """Draw different shapes for different unit types"""
    half = size // 2
    
    if shape_type == "square":
        draw.rectangle([center_x - half, center_y - half, 
                       center_x + half, center_y + half], 
                      fill=color, outline=(255, 255, 255), width=2)
    
    elif shape_type == "circle":
        draw.ellipse([center_x - half, center_y - half,
                     center_x + half, center_y + half],
                    fill=color, outline=(255, 255, 255), width=2)
    
    elif shape_type == "diamond":
        points = [
            (center_x, center_y - half),  # top
            (center_x + half, center_y),  # right
            (center_x, center_y + half),  # bottom
            (center_x - half, center_y),  # left
        ]
        draw.polygon(points, fill=color, outline=(255, 255, 255))
    
    elif shape_type == "triangle":
        points = [
            (center_x, center_y - half),  # top
            (center_x + half, center_y + half),  # bottom right
            (center_x - half, center_y + half),  # bottom left
        ]
        draw.polygon(points, fill=color, outline=(255, 255, 255))
    
    elif shape_type == "hexagon":
        import math
        points = []
        for i in range(6):
            angle = math.pi / 3 * i
            x = center_x + half * math.cos(angle)
            y = center_y + half * math.sin(angle)
            points.append((x, y))
        draw.polygon(points, fill=color, outline=(255, 255, 255))
    
    elif shape_type == "octagon":
        import math
        points = []
        for i in range(8):
            angle = math.pi / 4 * i
            x = center_x + half * math.cos(angle)
            y = center_y + half * math.sin(angle)
            points.append((x, y))
        draw.polygon(points, fill=color, outline=(255, 255, 255))

def create_unit_sprite(class_name, properties, size=64):
    """Create a single unit sprite"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    center = size // 2
    
    # Draw the main shape
    draw_shape(draw, center, center, size - 8, properties['shape'], properties['color'])
    
    # Add a smaller inner shape for detail
    inner_color = tuple(min(c + 40, 255) for c in properties['color'])
    draw_shape(draw, center, center, size - 24, properties['shape'], inner_color)
    
    # Add tier indicator (corner dots)
    tier = ["Squire", "Scout", "Thief", "Acolyte", "Guard", "Apprentice"].count(class_name) > 0
    if not tier:
        # Draw tier dots in corner
        dot_size = 4
        for i in range(min(3, len(class_name) % 4)):
            draw.ellipse([size - 12 - i*6, 4, size - 8 - i*6, 8], fill=(255, 255, 255))
    
    return img

def create_unit_spritesheet():
    """Create a spritesheet containing all unit sprites"""
    sprite_size = 64
    cols = 6  # 6 archetypes
    rows = 4  # 4 tiers
    
    sheet_width = cols * sprite_size
    sheet_height = rows * sprite_size
    
    spritesheet = Image.new('RGBA', (sheet_width, sheet_height), (0, 0, 0, 0))
    
    # Arrange units by archetype and tier
    archetypes = [
        ["Squire", "Knight", "Paladin", "Champion"],
        ["Apprentice", "Sorcerer", "Archmage", "Mystic"],
        ["Scout", "Hunter", "Sniper", "Marksman"],
        ["Thief", "Assassin", "Shadowblade", "Phantom"],
        ["Acolyte", "Priest", "Bishop", "Saint"],
        ["Guard", "Defender", "Fortress", "Juggernaut"],
    ]
    
    for col, archetype in enumerate(archetypes):
        for row, class_name in enumerate(archetype):
            sprite = create_unit_sprite(class_name, UNIT_CLASSES[class_name], sprite_size)
            x = col * sprite_size
            y = row * sprite_size
            spritesheet.paste(sprite, (x, y))
    
    return spritesheet

def create_individual_sprites():
    """Create individual sprite files for each unit class"""
    for class_name, properties in UNIT_CLASSES.items():
        sprite = create_unit_sprite(class_name, properties, 64)
        filename = f"assets/sprites/units/{class_name.lower()}.png"
        sprite.save(filename)
        print(f"Created: {filename}")

def main():
    print("Generating unit sprites for Dungeon Dice Duelists...")
    
    # Create spritesheet
    spritesheet = create_unit_spritesheet()
    spritesheet.save("assets/sprites/units/units_spritesheet.png")
    print("Created: assets/sprites/units/units_spritesheet.png")
    
    # Create individual sprites
    create_individual_sprites()
    
    print(f"\n✓ Generated {len(UNIT_CLASSES)} unit sprites successfully!")

if __name__ == "__main__":
    main()
