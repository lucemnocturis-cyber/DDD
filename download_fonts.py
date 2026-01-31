#!/usr/bin/env python3
"""
Download and configure free fonts for Dungeon Dice Duelists
Downloads from Google Fonts API
"""

import os
import urllib.request
import zipfile
import shutil
from pathlib import Path

# Font directory
FONT_DIR = Path("assets/fonts")

# Google Fonts to download
FONTS = {
    "Roboto": {
        "url": "https://fonts.google.com/download?family=Roboto",
        "variants": ["Roboto-Regular.ttf", "Roboto-Bold.ttf"],
        "description": "Clean, modern sans-serif - Main UI font",
        "sizes": {"small": 14, "medium": 18, "large": 24, "title": 36}
    },
    "RobotoMono": {
        "url": "https://fonts.google.com/download?family=Roboto+Mono",
        "variants": ["RobotoMono-Regular.ttf", "RobotoMono-Bold.ttf"],
        "description": "Monospace font for stats and numbers",
        "sizes": {"small": 12, "medium": 16, "large": 20}
    }
}

def download_font_from_github(font_name, filename, url):
    """Download font directly from GitHub mirror"""
    try:
        print(f"Downloading {font_name} from GitHub...")
        urllib.request.urlretrieve(url, filename)
        print(f"  ✓ Downloaded {filename}")
        return True
    except Exception as e:
        print(f"  ✗ Failed to download {font_name}: {e}")
        return False

def create_fallback_bitmap_font():
    """Create a simple bitmap font as absolute fallback"""
    from PIL import Image, ImageDraw
    
    print("Creating fallback bitmap font...")
    
    # Create a simple 8x8 pixel font for basic ASCII characters
    char_width = 8
    char_height = 12
    chars_per_row = 16
    num_rows = 6  # 96 printable ASCII chars / 16
    
    img = Image.new('L', (char_width * chars_per_row, char_height * num_rows), 0)
    draw = ImageDraw.Draw(img)
    
    # Draw simple pixel patterns for each character (32-127)
    for i in range(96):
        char = chr(32 + i)
        x = (i % chars_per_row) * char_width
        y = (i // chars_per_row) * char_height
        
        # Draw a simple representation (placeholder)
        if char.isalnum():
            # Draw a box with the character (simplified)
            draw.rectangle([x+2, y+2, x+6, y+10], fill=255)
        elif char == ' ':
            pass  # Space is empty
        else:
            # Symbols get a small marker
            draw.rectangle([x+3, y+5, x+5, y+7], fill=255)
    
    img.save(FONT_DIR / "fallback_font.png")
    print("  ✓ Created fallback_font.png")

def download_fonts():
    """Download fonts from Google Fonts or GitHub mirrors"""
    
    # Ensure font directory exists
    FONT_DIR.mkdir(parents=True, exist_ok=True)
    
    print("=" * 60)
    print("Font Download for Dungeon Dice Duelists")
    print("=" * 60)
    
    # Direct GitHub URLs for Roboto fonts (reliable source)
    github_fonts = {
        "Roboto-Regular.ttf": "https://github.com/google/roboto/raw/main/src/hinted/Roboto-Regular.ttf",
        "Roboto-Bold.ttf": "https://github.com/google/roboto/raw/main/src/hinted/Roboto-Bold.ttf",
        "RobotoMono-Regular.ttf": "https://github.com/googlefonts/RobotoMono/raw/main/fonts/ttf/RobotoMono-Regular.ttf",
        "RobotoMono-Bold.ttf": "https://github.com/googlefonts/RobotoMono/raw/main/fonts/ttf/RobotoMono-Bold.ttf",
    }
    
    downloaded = []
    
    for font_file, url in github_fonts.items():
        output_path = FONT_DIR / font_file
        if download_font_from_github(font_file.split('-')[0], output_path, url):
            downloaded.append(font_file)
    
    if downloaded:
        print(f"\n✓ Successfully downloaded {len(downloaded)} font files!")
        
        # Create symbolic links with standard names
        main_font = FONT_DIR / "Roboto-Regular.ttf"
        mono_font = FONT_DIR / "RobotoMono-Regular.ttf"
        
        if main_font.exists():
            # Create standard font links
            for name in ["game_font.ttf", "ui_font.ttf"]:
                link_path = FONT_DIR / name
                if link_path.exists():
                    link_path.unlink()
                try:
                    link_path.symlink_to("Roboto-Regular.ttf")
                    print(f"  ✓ Created {name} -> Roboto-Regular.ttf")
                except OSError:
                    # Symlink failed, copy instead
                    shutil.copy(main_font, link_path)
                    print(f"  ✓ Copied to {name}")
        
        if mono_font.exists():
            link_path = FONT_DIR / "mono_font.ttf"
            if link_path.exists():
                link_path.unlink()
            try:
                link_path.symlink_to("RobotoMono-Regular.ttf")
                print(f"  ✓ Created mono_font.ttf -> RobotoMono-Regular.ttf")
            except OSError:
                shutil.copy(mono_font, link_path)
                print(f"  ✓ Copied to mono_font.ttf")
        
        # Create title font (using bold variant)
        title_font = FONT_DIR / "Roboto-Bold.ttf"
        if title_font.exists():
            link_path = FONT_DIR / "title_font.ttf"
            if link_path.exists():
                link_path.unlink()
            try:
                link_path.symlink_to("Roboto-Bold.ttf")
                print(f"  ✓ Created title_font.ttf -> Roboto-Bold.ttf")
            except OSError:
                shutil.copy(title_font, link_path)
                print(f"  ✓ Copied to title_font.ttf")
    else:
        print("\n✗ No fonts could be downloaded!")
        print("Creating fallback bitmap font...")
        create_fallback_bitmap_font()
    
    return len(downloaded) > 0

def create_font_config():
    """Create font configuration file"""
    config = {
        "fonts": {
            "main": {
                "file": "Roboto-Regular.ttf",
                "sizes": {
                    "small": 14,
                    "medium": 18,
                    "large": 24,
                    "huge": 36
                }
            },
            "title": {
                "file": "Roboto-Bold.ttf",
                "sizes": {
                    "medium": 24,
                    "large": 36,
                    "huge": 48
                }
            },
            "mono": {
                "file": "RobotoMono-Regular.ttf",
                "sizes": {
                    "small": 12,
                    "medium": 16,
                    "large": 20
                }
            }
        }
    }
    
    import json
    config_path = FONT_DIR / "font_config.json"
    with open(config_path, 'w') as f:
        json.dump(config, f, indent=2)
    
    print(f"\n✓ Created font_config.json")

def verify_fonts():
    """Verify fonts are properly installed"""
    print("\n" + "=" * 60)
    print("Font Verification")
    print("=" * 60)
    
    required_fonts = ["game_font.ttf", "title_font.ttf", "mono_font.ttf"]
    all_ok = True
    
    for font_name in required_fonts:
        font_path = FONT_DIR / font_name
        if font_path.exists():
            size = font_path.stat().st_size
            print(f"  ✓ {font_name} ({size:,} bytes)")
        else:
            print(f"  ✗ {font_name} MISSING")
            all_ok = False
    
    # List all fonts
    print(f"\nAll fonts in {FONT_DIR}:")
    for font_file in sorted(FONT_DIR.glob("*.ttf")):
        size = font_file.stat().st_size
        print(f"  - {font_file.name} ({size:,} bytes)")
    
    return all_ok

def main():
    success = download_fonts()
    
    if success:
        create_font_config()
        verify_fonts()
        
        print("\n" + "=" * 60)
        print("✓ FONT INSTALLATION COMPLETE")
        print("=" * 60)
        print("\nFonts installed:")
        print("  • Roboto Regular - Main UI font")
        print("  • Roboto Bold - Titles and headers")
        print("  • Roboto Mono - Stats and numbers")
        print("\nStandard names created:")
        print("  • game_font.ttf")
        print("  • title_font.ttf")
        print("  • mono_font.ttf")
        print("\nThe game is now ready for text rendering!")
    else:
        print("\n" + "=" * 60)
        print("⚠ FONT INSTALLATION INCOMPLETE")
        print("=" * 60)
        print("\nPlease manually download fonts:")
        print("  1. Visit https://fonts.google.com/specimen/Roboto")
        print("  2. Download Roboto")
        print("  3. Extract and copy .ttf files to assets/fonts/")
        print("  4. Rename Roboto-Regular.ttf to game_font.ttf")

if __name__ == "__main__":
    main()
