# Bitmap Font Usage

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
