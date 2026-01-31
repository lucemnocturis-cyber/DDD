#!/usr/bin/env python3
"""
Generate placeholder UI sprites for Dungeon Dice Duelists
Creates buttons, panels, backgrounds, and other UI elements
"""

from PIL import Image, ImageDraw, ImageFont
import os

def create_button(width, height, color, text="", state="normal"):
    """Create a button sprite with different states"""
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Adjust appearance based on state
    if state == "hover":
        color = tuple(min(c + 30, 255) for c in color)
        outline_width = 3
    elif state == "pressed":
        color = tuple(max(c - 30, 0) for c in color)
        outline_width = 4
    else:
        outline_width = 2
    
    # Draw button background with rounded corners
    draw.rounded_rectangle([0, 0, width - 1, height - 1], radius=8,
                          fill=color, outline=(255, 255, 255), width=outline_width)
    
    # Add gradient effect
    for i in range(height // 2):
        alpha = int(50 * (1 - i / (height // 2)))
        overlay_color = (255, 255, 255, alpha)
        draw.rectangle([2, 2 + i, width - 3, 2 + i + 1], fill=overlay_color)
    
    return img

def create_panel(width, height, title="Panel"):
    """Create a panel with title bar"""
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Main panel body
    draw.rounded_rectangle([0, 0, width - 1, height - 1], radius=10,
                          fill=(40, 40, 60, 230), outline=(100, 100, 150), width=3)
    
    # Title bar
    draw.rounded_rectangle([4, 4, width - 5, 34], radius=6,
                          fill=(60, 60, 90), outline=(120, 120, 180), width=2)
    
    return img

def create_background(width, height, style="arena"):
    """Create game background"""
    img = Image.new('RGBA', (width, height), (0, 0, 0, 255))
    draw = ImageDraw.Draw(img)
    
    if style == "arena":
        # Stone floor with grid
        base_color = (80, 70, 60)
        img.paste(base_color, (0, 0, width, height))
        
        # Add texture
        import random
        random.seed(42)
        for _ in range(1000):
            x = random.randint(0, width)
            y = random.randint(0, height)
            shade = random.randint(-10, 10)
            color = tuple(max(0, min(255, c + shade)) for c in base_color)
            draw.point((x, y), fill=color)
        
        # Draw grid lines
        cell_size = 40
        for x in range(0, width, cell_size):
            draw.line([(x, 0), (x, height)], fill=(100, 90, 80, 100), width=1)
        for y in range(0, height, cell_size):
            draw.line([(0, y), (width, y)], fill=(100, 90, 80, 100), width=1)
    
    elif style == "menu":
        # Gradient background for menus
        for y in range(height):
            progress = y / height
            r = int(20 + 30 * progress)
            g = int(20 + 40 * progress)
            b = int(40 + 60 * progress)
            draw.line([(0, y), (width, y)], fill=(r, g, b))
    
    return img

def create_icon(size, icon_type):
    """Create small icons for UI"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    center = size // 2
    
    if icon_type == "heart":
        # Health icon
        points = [
            (center, center + 8),
            (center - 10, center - 5),
            (center - 10, center - 10),
            (center, center - 8),
            (center + 10, center - 10),
            (center + 10, center - 5),
        ]
        draw.polygon(points, fill=(255, 50, 50))
    
    elif icon_type == "sword":
        # Attack icon
        draw.rectangle([center - 3, 5, center + 3, size - 5], fill=(200, 200, 200))
        draw.polygon([
            (center, 3),
            (center - 5, 8),
            (center + 5, 8),
        ], fill=(200, 200, 200))
        draw.rectangle([center - 6, size - 8, center + 6, size - 5], fill=(150, 100, 50))
    
    elif icon_type == "shield":
        # Defense icon
        points = [
            (center, 3),
            (center - 10, 8),
            (center - 10, center + 5),
            (center, size - 3),
            (center + 10, center + 5),
            (center + 10, 8),
        ]
        draw.polygon(points, fill=(150, 150, 200), outline=(255, 255, 255), width=2)
    
    elif icon_type == "boot":
        # Movement icon
        draw.ellipse([center - 8, center - 3, center + 8, center + 10], 
                    fill=(100, 70, 50), outline=(255, 255, 255), width=1)
        draw.rectangle([center - 5, center - 8, center + 5, center + 2], 
                      fill=(120, 90, 70))
    
    elif icon_type == "target":
        # Range icon
        draw.ellipse([5, 5, size - 5, size - 5], outline=(255, 100, 100), width=2)
        draw.ellipse([center - 5, center - 5, center + 5, center + 5], 
                    fill=(255, 100, 100))
        draw.line([(center, 3), (center, size - 3)], fill=(255, 100, 100), width=2)
        draw.line([(3, center), (size - 3, center)], fill=(255, 100, 100), width=2)
    
    elif icon_type == "coin":
        # Gold icon
        draw.ellipse([3, center - 8, size - 3, center + 8], 
                    fill=(255, 215, 0), outline=(200, 150, 0), width=2)
        draw.text((center - 5, center - 6), "$", fill=(200, 150, 0))
    
    return img

def create_cursor(size=32):
    """Create custom cursor sprite"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Arrow cursor
    points = [
        (0, 0),
        (0, size - 8),
        (8, size - 12),
        (12, size),
        (16, size - 2),
        (12, size - 14),
        (size - 8, 16),
    ]
    draw.polygon(points, fill=(255, 255, 255), outline=(0, 0, 0), width=2)
    
    return img

def create_health_bar(width=200, height=24, fill_percent=1.0):
    """Create health bar sprite"""
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Background
    draw.rounded_rectangle([0, 0, width - 1, height - 1], radius=4,
                          fill=(40, 40, 40), outline=(255, 255, 255), width=2)
    
    # Fill based on percentage
    fill_width = int((width - 6) * fill_percent)
    if fill_width > 0:
        # Color gradient from green to red based on health
        if fill_percent > 0.5:
            color = (50, 200, 50)
        elif fill_percent > 0.25:
            color = (200, 200, 50)
        else:
            color = (200, 50, 50)
        
        draw.rounded_rectangle([3, 3, 3 + fill_width, height - 4], radius=2,
                              fill=color)
    
    return img

def main():
    print("Generating UI sprites for Dungeon Dice Duelists...")
    
    # Create buttons in different states
    button_colors = {
        "primary": (80, 120, 200),
        "success": (80, 200, 80),
        "danger": (200, 80, 80),
        "warning": (200, 180, 80),
    }
    
    for name, color in button_colors.items():
        for state in ["normal", "hover", "pressed"]:
            btn = create_button(200, 50, color, state=state)
            filename = f"assets/sprites/ui/button_{name}_{state}.png"
            btn.save(filename)
            print(f"Created: {filename}")
    
    # Create panels
    for width, height, name in [(300, 400, "small"), (500, 600, "medium"), (800, 600, "large")]:
        panel = create_panel(width, height, f"{name.capitalize()} Panel")
        filename = f"assets/sprites/ui/panel_{name}.png"
        panel.save(filename)
        print(f"Created: {filename}")
    
    # Create backgrounds
    arena = create_background(1280, 720, "arena")
    arena.save("assets/sprites/ui/background_arena.png")
    print("Created: assets/sprites/ui/background_arena.png")
    
    menu = create_background(1280, 720, "menu")
    menu.save("assets/sprites/ui/background_menu.png")
    print("Created: assets/sprites/ui/background_menu.png")
    
    # Create icons
    icons = ["heart", "sword", "shield", "boot", "target", "coin"]
    for icon_type in icons:
        icon = create_icon(32, icon_type)
        filename = f"assets/sprites/ui/icon_{icon_type}.png"
        icon.save(filename)
        print(f"Created: {filename}")
    
    # Create cursor
    cursor = create_cursor(32)
    cursor.save("assets/sprites/ui/cursor.png")
    print("Created: assets/sprites/ui/cursor.png")
    
    # Create health bars at different fill levels
    for percent in [1.0, 0.75, 0.5, 0.25]:
        hbar = create_health_bar(200, 24, percent)
        filename = f"assets/sprites/ui/healthbar_{int(percent*100)}.png"
        hbar.save(filename)
        print(f"Created: {filename}")
    
    print("\n✓ Generated UI sprites successfully!")

if __name__ == "__main__":
    main()
