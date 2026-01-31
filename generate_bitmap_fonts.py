#!/usr/bin/env python3
"""
Generate bitmap fonts for Dungeon Dice Duelists
Creates pixel-perfect bitmap fonts when TTF fonts are unavailable
"""

from PIL import Image, ImageDraw, ImageFont
import json
import os

def create_pixel_font(size=16):
    """Create a pixel-style font using PIL drawing"""
    
    # Character set (printable ASCII)
    chars = ' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~'
    
    char_width = size
    char_height = size
    chars_per_row = 16
    num_rows = (len(chars) + chars_per_row - 1) // chars_per_row
    
    # Create font sheet
    img = Image.new('RGBA', (char_width * chars_per_row, char_height * num_rows), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    # Simple 8x8 pixel patterns for each character
    patterns = get_character_patterns()
    
    for i, char in enumerate(chars):
        col = i % chars_per_row
        row = i // chars_per_row
        x = col * char_width
        y = row * char_height
        
        # Draw character pattern
        pattern = patterns.get(char, patterns.get('?', []))
        draw_character(draw, x, y, pattern, char_width, char_height)
    
    return img, chars

def get_character_patterns():
    """Define pixel patterns for characters (8x8 grid)"""
    patterns = {
        ' ': [],
        '0': [(1,0,6,0), (0,1,0,6), (6,1,6,6), (1,7,6,7)],
        '1': [(2,0,2,7), (1,1,3,1)],
        '2': [(0,0,6,0), (6,1,6,3), (0,4,6,4), (0,5,0,7), (0,7,6,7)],
        '3': [(0,0,6,0), (6,1,6,6), (0,7,6,7), (3,3,6,3)],
        '4': [(0,0,0,4), (0,4,6,4), (6,0,6,7)],
        '5': [(0,0,6,0), (0,1,0,3), (0,3,6,3), (6,4,6,6), (0,7,6,7)],
        '6': [(1,0,6,0), (0,1,0,6), (0,3,6,3), (6,4,6,6), (1,7,6,7)],
        '7': [(0,0,6,0), (6,0,6,7)],
        '8': [(1,0,5,0), (0,1,0,3), (6,1,6,3), (1,3,5,3), (0,4,0,6), (6,4,6,6), (1,7,5,7)],
        '9': [(1,0,5,0), (0,1,0,3), (6,1,6,6), (1,3,6,3), (1,7,5,7)],
        'A': [(2,0,4,0), (1,1,1,6), (5,1,5,6), (1,3,5,3), (0,7,6,7)],
        'B': [(0,0,5,0), (0,1,0,6), (0,3,5,3), (0,7,5,7), (6,1,6,2), (6,4,6,6)],
        'C': [(1,0,6,0), (0,1,0,6), (1,7,6,7)],
        'D': [(0,0,5,0), (0,1,0,6), (0,7,5,7), (6,1,6,6)],
        'E': [(0,0,6,0), (0,1,0,6), (0,3,5,3), (0,7,6,7)],
        'F': [(0,0,6,0), (0,1,0,7), (0,3,5,3)],
        'G': [(1,0,6,0), (0,1,0,6), (4,4,6,4), (6,5,6,6), (1,7,6,7)],
        'H': [(0,0,0,7), (6,0,6,7), (0,3,6,3)],
        'I': [(2,0,4,0), (3,1,3,6), (2,7,4,7)],
        'J': [(0,0,6,0), (4,1,4,6), (0,7,4,7)],
        'K': [(0,0,0,7), (6,0,1,5), (1,4,6,7)],
        'L': [(0,0,0,7), (0,7,6,7)],
        'M': [(0,0,0,7), (6,0,6,7), (0,0,3,3), (6,0,3,3)],
        'N': [(0,0,0,7), (6,0,6,7), (0,0,6,7)],
        'O': [(1,0,5,0), (0,1,0,6), (6,1,6,6), (1,7,5,7)],
        'P': [(0,0,5,0), (0,1,0,7), (6,1,6,3), (0,4,5,4)],
        'Q': [(1,0,5,0), (0,1,0,6), (6,1,6,6), (1,7,5,7), (4,5,6,7)],
        'R': [(0,0,5,0), (0,1,0,7), (6,1,6,3), (0,4,5,4), (4,5,6,7)],
        'S': [(1,0,6,0), (0,1,0,2), (1,3,5,3), (6,4,6,6), (0,7,5,7)],
        'T': [(0,0,6,0), (3,1,3,7)],
        'U': [(0,0,0,6), (6,0,6,6), (1,7,5,7)],
        'V': [(0,0,3,7), (6,0,3,7)],
        'W': [(0,0,0,7), (6,0,6,7), (0,7,3,4), (6,7,3,4)],
        'X': [(0,0,6,7), (6,0,0,7)],
        'Y': [(0,0,3,3), (6,0,3,3), (3,4,3,7)],
        'Z': [(0,0,6,0), (6,1,0,6), (0,7,6,7)],
        '+': [(3,1,3,6), (1,3,5,3)],
        '-': [(1,3,5,3)],
        '*': [(3,1,3,6), (1,3,5,3), (1,1,5,5), (1,5,5,1)],
        '/': [(6,0,0,7)],
        '=': [(1,2,5,2), (1,5,5,5)],
        '.': [(3,6,3,6)],
        ',': [(3,6,3,7), (2,7,2,7)],
        ':': [(3,2,3,2), (3,5,3,5)],
        ';': [(3,2,3,2), (3,5,3,6)],
        '!': [(3,0,3,4), (3,6,3,6)],
        '?': [(1,0,5,0), (6,1,6,2), (3,3,3,4), (3,6,3,6)],
        '(': [(4,0,2,2), (2,3,2,4), (4,7,2,5)],
        ')': [(2,0,4,2), (4,3,4,4), (2,7,4,5)],
        '[': [(2,0,4,0), (2,1,2,6), (2,7,4,7)],
        ']': [(2,0,4,0), (4,1,4,6), (2,7,4,7)],
        '<': [(5,1,2,3), (5,5,2,3)],
        '>': [(1,1,4,3), (1,5,4,3)],
        '%': [(1,0,1,0), (2,1,4,5), (5,6,5,6)],
        '#': [(1,1,1,5), (5,1,5,5), (0,2,6,2), (0,4,6,4)],
        '@': [(1,0,5,0), (0,1,0,6), (6,1,6,4), (3,3,5,3), (3,4,3,5), (1,7,5,7)],
    }
    
    # Lowercase (simplified versions)
    patterns.update({
        'a': [(1,2,5,2), (6,3,6,6), (1,7,5,7), (6,2,6,2)],
        'b': [(0,0,0,7), (0,3,5,3), (6,4,6,6), (1,7,5,7)],
        'c': [(1,3,5,3), (0,4,0,6), (1,7,5,7)],
        'd': [(6,0,6,7), (1,3,5,3), (0,4,0,6), (1,7,5,7)],
        'e': [(1,3,5,3), (0,4,0,6), (0,5,6,5), (1,7,5,7)],
        'f': [(3,0,5,0), (2,1,2,7), (1,3,4,3)],
        'g': [(1,3,5,3), (6,3,6,8), (0,4,0,6), (1,7,5,7), (1,9,5,9)],
        'h': [(0,0,0,7), (0,3,5,3), (6,4,6,7)],
        'i': [(3,1,3,1), (3,3,3,7)],
        'j': [(4,1,4,1), (4,3,4,8), (1,9,3,9)],
        'k': [(0,0,0,7), (5,3,1,5), (1,5,5,7)],
        'l': [(3,0,3,7)],
        'm': [(0,3,0,7), (0,3,2,3), (3,4,3,7), (4,3,6,3), (6,4,6,7)],
        'n': [(0,3,0,7), (0,3,5,3), (6,4,6,7)],
        'o': [(1,3,5,3), (0,4,0,6), (6,4,6,6), (1,7,5,7)],
        'p': [(0,3,0,9), (0,3,5,3), (6,4,6,6), (1,7,5,7)],
        'q': [(6,3,6,9), (1,3,5,3), (0,4,0,6), (1,7,5,7)],
        'r': [(0,3,0,7), (0,3,4,3), (5,3,6,3)],
        's': [(1,3,5,3), (0,4,0,4), (1,5,5,5), (6,6,6,6), (1,7,5,7)],
        't': [(2,1,2,7), (1,3,4,3), (3,7,5,7)],
        'u': [(0,3,0,6), (6,3,6,7), (1,7,5,7)],
        'v': [(0,3,3,7), (6,3,3,7)],
        'w': [(0,3,0,7), (6,3,6,7), (0,7,2,5), (6,7,4,5)],
        'x': [(0,3,6,7), (6,3,0,7)],
        'y': [(0,3,3,6), (6,3,3,6), (3,7,3,9), (1,9,2,9)],
        'z': [(0,3,6,3), (5,4,1,6), (0,7,6,7)],
    })
    
    return patterns

def draw_character(draw, x, y, pattern, width, height):
    """Draw a character from its pattern"""
    scale_x = width / 8
    scale_y = height / 8
    
    for line in pattern:
        if len(line) == 4:
            x1, y1, x2, y2 = line
            draw.line([
                (x + x1 * scale_x, y + y1 * scale_y),
                (x + x2 * scale_x, y + y2 * scale_y)
            ], fill=(255, 255, 255, 255), width=max(1, int(scale_x * 0.6)))
        elif len(line) == 2:
            px, py = line
            draw.point((x + px * scale_x, y + py * scale_y), fill=(255, 255, 255, 255))

def create_font_metadata(chars, char_width, char_height):
    """Create metadata for the bitmap font"""
    metadata = {
        "type": "bitmap_font",
        "char_width": char_width,
        "char_height": char_height,
        "chars_per_row": 16,
        "characters": chars,
        "char_map": {}
    }
    
    for i, char in enumerate(chars):
        col = i % 16
        row = i // 16
        metadata["char_map"][char] = {
            "index": i,
            "x": col * char_width,
            "y": row * char_height,
            "width": char_width,
            "height": char_height
        }
    
    return metadata

def main():
    print("=" * 60)
    print("Bitmap Font Generator for Dungeon Dice Duelists")
    print("=" * 60)
    
    font_dir = "assets/fonts"
    os.makedirs(font_dir, exist_ok=True)
    
    # Create fonts in multiple sizes
    sizes = {
        "small": 12,
        "medium": 16,
        "large": 24,
        "title": 32
    }
    
    for size_name, size in sizes.items():
        print(f"\nGenerating {size_name} font ({size}px)...")
        
        img, chars = create_pixel_font(size)
        filename = f"{font_dir}/bitmap_font_{size_name}.png"
        img.save(filename)
        print(f"  ✓ Created {filename}")
        
        # Create metadata
        metadata = create_font_metadata(chars, size, size)
        metadata_file = f"{font_dir}/bitmap_font_{size_name}.json"
        with open(metadata_file, 'w') as f:
            json.dump(metadata, f, indent=2)
        print(f"  ✓ Created {metadata_file}")
    
    # Create font configuration
    config = {
        "font_type": "bitmap",
        "available_fonts": {
            "game_font": "bitmap_font_medium.png",
            "title_font": "bitmap_font_title.png",
            "small_font": "bitmap_font_small.png",
            "large_font": "bitmap_font_large.png"
        },
        "default_font": "game_font",
        "note": "Bitmap fonts generated as fallback. Replace with TTF when available."
    }
    
    config_file = f"{font_dir}/font_config.json"
    with open(config_file, 'w') as f:
        json.dump(config, f, indent=2)
    print(f"\n✓ Created {config_file}")
    
    # Create usage instructions
    instructions = f"{font_dir}/BITMAP_FONT_USAGE.md"
    with open(instructions, 'w') as f:
        f.write("""# Bitmap Font Usage

## Generated Fonts

This directory contains bitmap fonts generated as a fallback solution.

### Available Fonts:
- `bitmap_font_small.png` (12px) - Small UI text
- `bitmap_font_medium.png` (16px) - Main game font
- `bitmap_font_large.png` (24px) - Large UI text
- `bitmap_font_title.png` (32px) - Titles and headers

### Using in Code:

```cpp
// Load bitmap font
BitmapFont font;
font.Load("assets/fonts/bitmap_font_medium.png", 
          "assets/fonts/bitmap_font_medium.json");

// Render text
font.RenderText(renderer, "Hello World", x, y);
```

### Replacing with TTF:

When TTF fonts become available:
1. Download Roboto or similar free font
2. Place .ttf file in this directory
3. Update ResourceManager to use TTF instead
4. Bitmap fonts serve as fallback

### Character Set:

Supports all printable ASCII characters (32-126):
- Letters: A-Z, a-z
- Numbers: 0-9
- Symbols: !@#$%^&*()_+-=[]{}|;:'",.<>?/

## Advantages:
✓ No external dependencies
✓ Pixel-perfect rendering
✓ Fast loading
✓ Small file size
✓ Retro aesthetic

## Limitations:
✗ Fixed size (no scaling)
✗ No anti-aliasing
✗ Limited character set
✗ No font effects (bold, italic)
""")
    print(f"✓ Created {instructions}")
    
    print("\n" + "=" * 60)
    print("✓ BITMAP FONT GENERATION COMPLETE")
    print("=" * 60)
    print("\nGenerated 4 bitmap fonts:")
    print("  • Small (12px)")
    print("  • Medium (16px)")
    print("  • Large (24px)")
    print("  • Title (32px)")
    print("\nThe game can now render text using bitmap fonts!")
    print("\nNote: For production, replace with TTF fonts for better quality.")

if __name__ == "__main__":
    main()
