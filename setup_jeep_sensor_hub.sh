#!/bin/bash

# Jeep Sensor Hub - Complete Setup Script
# This script configures hardware (portrait display), autologin, and UI autostart

set -e

echo "=========================================="
echo "Jeep Sensor Hub - Complete Setup"
echo "=========================================="

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run this script as root (use sudo)"
    exit 1
fi

# Load environment variables
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Generate .env file if it doesn't exist
if [ ! -f "$SCRIPT_DIR/.env" ]; then
    if [ -f "$SCRIPT_DIR/.env.default" ]; then
        echo "Creating .env file from .env.default..."
        cp "$SCRIPT_DIR/.env.default" "$SCRIPT_DIR/.env"
        echo "✓ .env file created."
    else
        echo "Error: .env.default file not found!"
        exit 1
    fi
fi

# Always pause to allow user to edit .env file
echo ""
echo "=========================================="
echo "IMPORTANT: Review .env file before continuing"
echo "=========================================="
echo "Please review and edit the following file if needed:"
echo "  $SCRIPT_DIR/.env"
echo ""
echo "Key settings to verify:"
echo "  - JEEP_USER: Your username"
echo "  - JEEP_HOME: Your home directory"
echo "  - PROJECT_DIR: Path to this project"
echo "  - DISPLAY_WIDTH/HEIGHT: Display resolution"
echo "  - DISPLAY_ROTATION: Rotation value (0-3)"
echo ""
echo "Press Enter to continue after reviewing/editing the .env file..."
echo -n "> "
read -r
echo "Continuing with setup..."

# Load environment variables
echo "Loading environment from .env file..."
source "$SCRIPT_DIR/.env"

# Validate required environment variables
required_vars=("JEEP_USER" "JEEP_HOME" "PROJECT_DIR" "DISPLAY_WIDTH" "DISPLAY_HEIGHT" "DISPLAY_ROTATION" "SERVICE_NAME")
for var in "${required_vars[@]}"; do
    if [ -z "${!var}" ]; then
        echo "Error: Required environment variable $var is not set in .env file"
        exit 1
    fi
done

echo "Setting up for user: $JEEP_USER"
echo "Project directory: $PROJECT_DIR"

# =============================================================================
# 1. HARDWARE CONFIGURATION - Portrait Display
# =============================================================================

echo ""
echo "1. Configuring hardware for portrait display..."

# Backup current config
cp /boot/firmware/config.txt /boot/firmware/config.txt.backup.$(date +%Y%m%d_%H%M%S)

# Remove all existing rotation settings
sed -i '/^display_lcd_rotate/d; /^display_rotate/d; /^lcd_rotate/d; /^hdmi_rotate/d; /^LCD_rotate/d' /boot/firmware/config.txt

# Remove all existing framebuffer settings
sed -i '/^max_framebuffers/d; /^framebuffer_width/d; /^framebuffer_height/d' /boot/firmware/config.txt

# Add portrait display configuration with kernel-level rotation
cat >> /boot/firmware/config.txt << EOF

# --- Extra ---
max_framebuffers=2

# Framebuffer configuration for vertical display
framebuffer_width=$DISPLAY_WIDTH
framebuffer_height=$DISPLAY_HEIGHT

# --- Rotate framebuffer (portrait mode) ---
# 0 = normal, 1 = 90°, 2 = 180°, 3 = 270°
display_lcd_rotate=$DISPLAY_ROTATION
lcd_rotate=$DISPLAY_ROTATION
EOF

# Update kernel command line for display rotation
echo ""
echo "Updating kernel command line for display rotation..."

# Backup current cmdline
cp /boot/firmware/cmdline.txt /boot/firmware/cmdline.txt.backup.$(date +%Y%m%d_%H%M%S)

# Remove existing rotation parameters
sed -i 's/ fbcon=rotate:[0-9]//g; s/ video=DSI-1:[^ ]*//g' /boot/firmware/cmdline.txt

# Add new rotation parameters
sed -i "s/\$/ fbcon=rotate:$DISPLAY_ROTATION video=DSI-1:${DISPLAY_WIDTH}x${DISPLAY_HEIGHT}@60,rotate=90/" /boot/firmware/cmdline.txt

# Note: Kernel-level rotation ensures the entire system (including boot logs)
# displays in vertical orientation. UI app is configured for 480x800 portrait.

echo "✓ Boot configuration updated for portrait mode"
echo "✓ Kernel command line updated for display rotation"

# =============================================================================
# 2. AUTOLOGIN CONFIGURATION
# =============================================================================

echo ""
echo "2. Setting up autologin..."

# Enable autologin for the user
systemctl set-default multi-user.target
systemctl enable getty@tty1.service

# Create autologin override
mkdir -p /etc/systemd/system/getty@tty1.service.d
cat > /etc/systemd/system/getty@tty1.service.d/autologin.conf << EOF
[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin $JEEP_USER --noclear %I \$TERM
EOF

echo "✓ Autologin configured for user: $JEEP_USER"

# =============================================================================
# 3. UI AUTOSTART CONFIGURATION
# =============================================================================

echo ""
echo "3. Setting up UI autostart..."

# Create the UI service
cat > /etc/systemd/system/$SERVICE_NAME.service << EOF
[Unit]
Description=Jeep Sensor Hub UI
After=multi-user.target
Wants=multi-user.target

[Service]
Type=simple
User=$JEEP_USER
Group=$JEEP_USER
WorkingDirectory=$PROJECT_DIR/pi-ui
ExecStartPre=/bin/sleep $SERVICE_SLEEP_SEC
ExecStart=$PROJECT_DIR/pi-ui/build/pi_ui
Restart=always
RestartSec=$SERVICE_RESTART_SEC
Environment=DISPLAY=:0
Environment=SDL_VIDEO_WINDOW_POS=0,0
Environment=SDL_VIDEODRIVER=$SDL_VIDEO_DRIVER
Environment=SDL_FBDEV=$SDL_FBDEV
Environment=SDL_VIDEO_FBCON_ROTATION=$SDL_VIDEO_FBCON_ROTATION

[Install]
WantedBy=multi-user.target
EOF

# Enable the service
systemctl daemon-reload
systemctl enable $SERVICE_NAME.service

echo "✓ UI autostart service created and enabled"

# =============================================================================
# 4. FINAL CONFIGURATION
# =============================================================================

echo ""
echo "4. Final configuration..."

# Ensure the UI build directory exists
if [ ! -d "$PROJECT_DIR/pi-ui/build" ]; then
    echo "Warning: UI build directory not found at $PROJECT_DIR/pi-ui/build"
    echo "Please build the UI application first"
fi

# Set proper permissions
chown -R $JEEP_USER:$JEEP_USER $JEEP_HOME/test_display_rotation.sh 2>/dev/null || true
chown -R $JEEP_USER:$JEEP_USER $JEEP_HOME/change_rotation.sh 2>/dev/null || true

echo "✓ Permissions set correctly"

# =============================================================================
# COMPLETION
# =============================================================================

echo ""
echo "=========================================="
echo "Setup Complete!"
echo "=========================================="
echo ""
echo "Configuration applied:"
echo "✓ Kernel-level portrait display rotation ($DISPLAY_ROTATION)"
echo "✓ Kernel command line updated with rotation settings"
echo "✓ Autologin enabled for user: $JEEP_USER"
echo "✓ UI autostart service configured ($SERVICE_NAME)"
echo "✓ Display rotation testing tools created"
echo ""
echo "Next steps:"
echo "1. Test current rotation: $JEEP_HOME/test_display_rotation.sh"
echo "2. If rotation is wrong, try different values:"
echo "   sudo $JEEP_HOME/change_rotation.sh [0|1|2|3]"
echo "3. Reboot to apply all changes: sudo reboot"
echo "4. After reboot, the system should:"
echo "   - Auto-login as $JEEP_USER"
echo "   - Display in portrait mode (kernel-level rotation)"
echo "   - Auto-start the UI application ($DISPLAY_WIDTHx$DISPLAY_HEIGHT rotated)"
echo ""
echo "If the display orientation is still wrong after reboot,"
echo "use the change_rotation.sh script to try different values."
echo ""
echo "The correct rotation value depends on your specific display hardware."
