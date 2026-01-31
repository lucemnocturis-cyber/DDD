#!/usr/bin/env python3
"""
Generate placeholder terrain and particle sprites for Dungeon Dice Duelists
Creates terrain tiles and particle effect textures
"""

from PIL import Image, ImageDraw
import os
import random

# Terrain types from the game
TERRAIN_TYPES = {
    "normal": {"color": (120, 110, 100), "name": "Normal Ground"},
    "grass": {"color": (80, 150, 70), "name": "Grass"},
    "water": {"color": (70, 120, 200), "name": "Water"},
    "lava": {"color": (220, 60, 20), "name": "Lava"},
    "ice": {"color": (180, 230, 255), "name": "Ice"},
    "forest": {"color": (50, 100, 50), "name": "Forest"},
    "desert": {"color": (220, 200, 120), "name": "Desert"},
    "mountain": {"color": (120, 120, 130), "name": "Mountain"},
    "swamp": {"color": (100, 120, 90), "name": "Swamp"},
    "void": {"color": (30, 20, 40), "name": "Void"},
    "sanctified": {"color": (255, 250, 200), "name": "Sanctified"},
    "corrupted": {"color": (100, 50, 100), "name": "Corrupted"},
    "wall": {"color": (80, 80, 90), "name": "Wall"},
    "bridge": {"color": (140, 100, 70), "name": "Bridge"},
    "chasm": {"color": (20, 20, 20), "name": "Chasm"},
}

def create_terrain_tile(tile_type, properties, size=64):
    """Create a terrain tile sprite"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    base_color = properties['color']
    
    # Fill base color
    draw.rectangle([0, 0, size, size], fill=base_color)
    
    # Add texture based on terrain type
    random.seed(hash(tile_type))
    
    if tile_type == "grass":
        # Add grass blades
        for _ in range(20):
            x = random.randint(0, size)
            y = random.randint(0, size)
            shade = random.randint(-20, 20)
            color = tuple(max(0, min(255, c + shade)) for c in base_color)
            draw.line([(x, y), (x, y + 3)], fill=color, width=1)
    
    elif tile_type == "water":
        # Add wave lines
        for i in range(0, size, 8):
            y = i + random.randint(-2, 2)
            shade = 20
            color = tuple(min(255, c + shade) for c in base_color)
            draw.line([(0, y), (size, y)], fill=color, width=1)
    
    elif tile_type == "lava":
        # Add lava cracks
        for _ in range(10):
            x = random.randint(0, size)
            y = random.randint(0, size)
            draw.ellipse([x - 3, y - 3, x + 3, y + 3], fill=(255, 200, 50))
    
    elif tile_type == "ice":
        # Add ice crystals
        for _ in range(5):
            x = random.randint(10, size - 10)
            y = random.randint(10, size - 10)
            points = [(x, y - 5), (x + 3, y), (x, y + 5), (x - 3, y)]
            draw.polygon(points, fill=(200, 240, 255))
    
    elif tile_type == "wall":
        # Add brick pattern
        brick_h = 10
        for row in range(0, size, brick_h):
            offset = (brick_h // 2) if (row // brick_h) % 2 else 0
            for col in range(-brick_h, size, brick_h * 2):
                x = col + offset
                draw.rectangle([x, row, x + brick_h * 2 - 2, row + brick_h - 2],
                             outline=(60, 60, 70), width=1)
    
    else:
        # Generic texture
        for _ in range(100):
            x = random.randint(0, size)
            y = random.randint(0, size)
            shade = random.randint(-15, 15)
            color = tuple(max(0, min(255, c + shade)) for c in base_color)
            draw.point((x, y), fill=color)
    
    # Add grid border
    draw.rectangle([0, 0, size - 1, size - 1], outline=(0, 0, 0, 100), width=1)
    
    return img

def create_particle(size, particle_type):
    """Create a particle sprite"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    center = size // 2
    
    if particle_type == "fire":
        # Fire particle (red-orange-yellow gradient)
        for r in range(center, 0, -2):
            progress = 1 - (r / center)
            red = 255
            green = int(100 + 155 * progress)
            blue = int(50 * (1 - progress))
            alpha = int(255 * (1 - progress * 0.5))
            color = (red, green, blue, alpha)
            draw.ellipse([center - r, center - r, center + r, center + r], fill=color)
    
    elif particle_type == "ice":
        # Ice particle (light blue)
        draw.ellipse([2, 2, size - 2, size - 2], fill=(180, 230, 255, 200))
        # Add sparkle
        draw.line([(center, 0), (center, size)], fill=(255, 255, 255, 255), width=1)
        draw.line([(0, center), (size, center)], fill=(255, 255, 255, 255), width=1)
    
    elif particle_type == "lightning":
        # Lightning particle (yellow-white)
        draw.ellipse([2, 2, size - 2, size - 2], fill=(255, 255, 150, 220))
        draw.ellipse([center - 3, center - 3, center + 3, center + 3], 
                    fill=(255, 255, 255, 255))
    
    elif particle_type == "heal":
        # Heal particle (green with cross)
        draw.ellipse([2, 2, size - 2, size - 2], fill=(100, 255, 100, 200))
        cross_width = 3
        draw.rectangle([center - cross_width, 4, center + cross_width, size - 4], 
                      fill=(255, 255, 255))
        draw.rectangle([4, center - cross_width, size - 4, center + cross_width], 
                      fill=(255, 255, 255))
    
    elif particle_type == "smoke":
        # Smoke particle (gray)
        for r in range(center, 0, -2):
            alpha = int(150 * (1 - r / center))
            color = (100, 100, 100, alpha)
            draw.ellipse([center - r, center - r, center + r, center + r], fill=color)
    
    elif particle_type == "sparkle":
        # Sparkle particle
        draw.ellipse([4, 4, size - 4, size - 4], fill=(255, 255, 200, 255))
        # Star shape
        points = []
        import math
        for i in range(8):
            angle = (math.pi * 2 * i) / 8
            r = center - 2 if i % 2 == 0 else center // 2
            x = center + r * math.cos(angle)
            y = center + r * math.sin(angle)
            points.append((x, y))
        draw.polygon(points, fill=(255, 255, 255, 255))
    
    elif particle_type == "blood":
        # Blood particle (dark red)
        draw.ellipse([2, 2, size - 2, size - 2], fill=(150, 20, 20, 200))
    
    elif particle_type == "poison":
        # Poison particle (purple-green)
        draw.ellipse([2, 2, size - 2, size - 2], fill=(100, 150, 100, 180))
        draw.ellipse([center - 3, center - 3, center + 3, center + 3], 
                    fill=(150, 50, 150, 200))
    
    return img

def create_explosion_frames(size=64, num_frames=8):
    """Create explosion animation frames"""
    frames = []
    
    for frame in range(num_frames):
        img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
        draw = ImageDraw.Draw(img)
        
        center = size // 2
        progress = frame / num_frames
        
        # Expanding circle
        radius = int(center * progress * 1.5)
        
        # Color fades from yellow to red to dark
        if progress < 0.3:
            color = (255, 255, 100, int(255 * (1 - progress)))
        elif progress < 0.6:
            color = (255, 150, 50, int(255 * (1 - progress)))
        else:
            color = (200, 50, 50, int(150 * (1 - progress)))
        
        if radius > 0:
            draw.ellipse([center - radius, center - radius,
                         center + radius, center + radius], fill=color)
        
        frames.append(img)
    
    return frames

def main():
    print("Generating terrain and particle sprites for Dungeon Dice Duelists...")
    
    # Create terrain tiles
    for terrain_type, properties in TERRAIN_TYPES.items():
        tile = create_terrain_tile(terrain_type, properties, 64)
        filename = f"assets/sprites/terrain/{terrain_type}.png"
        tile.save(filename)
        print(f"Created: {filename}")
    
    # Create terrain spritesheet
    tile_size = 64
    cols = 5
    rows = 3
    sheet = Image.new('RGBA', (cols * tile_size, rows * tile_size), (0, 0, 0, 0))
    
    terrain_list = list(TERRAIN_TYPES.items())
    for idx, (terrain_type, properties) in enumerate(terrain_list):
        col = idx % cols
        row = idx // cols
        tile = create_terrain_tile(terrain_type, properties, tile_size)
        sheet.paste(tile, (col * tile_size, row * tile_size))
    
    sheet.save("assets/sprites/terrain/terrain_spritesheet.png")
    print("Created: assets/sprites/terrain/terrain_spritesheet.png")
    
    # Create particle sprites
    particle_types = ["fire", "ice", "lightning", "heal", "smoke", "sparkle", "blood", "poison"]
    for ptype in particle_types:
        for size in [16, 32, 64]:
            particle = create_particle(size, ptype)
            filename = f"assets/sprites/particles/{ptype}_{size}.png"
            particle.save(filename)
            print(f"Created: {filename}")
    
    # Create explosion animation frames
    explosion_frames = create_explosion_frames(64, 8)
    for idx, frame in enumerate(explosion_frames):
        filename = f"assets/sprites/effects/explosion_frame_{idx}.png"
        frame.save(filename)
        print(f"Created: {filename}")
    
    print(f"\n✓ Generated {len(TERRAIN_TYPES)} terrain tiles and {len(particle_types) * 3} particle sprites!")

if __name__ == "__main__":
    main()
