#!/bin/bash

# Setup script for Jeep Sensor Hub UI dependencies
echo "Setting up dependencies for Jeep Sensor Hub UI..."

# Check if lv_drivers exists
if [ ! -d "lv_drivers" ]; then
	echo "Cloning lv_drivers repository..."
	git clone https://github.com/lvgl/lv_drivers.git
	if [ $? -eq 0 ]; then
		echo "✓ lv_drivers cloned successfully"
	else
		echo "✗ Failed to clone lv_drivers"
		exit 1
	fi
else
	echo "✓ lv_drivers already exists"
fi

# Check if lvgl exists
if [ ! -d "lv_drivers" ]; then
	echo "Cloning lv_drivers repository..."
	git clone https://github.com/lvgl/lvgl.git
	if [ $? -eq 0 ]; then
		echo "✓ LVGL cloned successfully"
	else
		echo "✗ Failed to clone LVGL"
		exit 1
	fi
else
	echo "✓ LVGL already exists"
fi

# Check if lv_drv_conf.h exists
if [ ! -f "lv_drv_conf.h" ]; then
	echo "Creating lv_drv_conf.h from template..."
	cp lv_drivers/lv_drv_conf_template.h lv_drv_conf.h
	if [ $? -eq 0 ]; then
		echo "✓ lv_drv_conf.h created successfully"
		echo "Note: You may need to configure lv_drv_conf.h for your specific setup"
	else
		echo "✗ Failed to create lv_drv_conf.h"
		exit 1
	fi
else
	echo "✓ lv_drv_conf.h already exists"
fi

# Configure lv_drv_conf.h for SDL support
echo "Configuring lv_drv_conf.h for SDL support..."
sed -i 's/#if 0/#if 1/' lv_drv_conf.h
sed -i 's/# define USE_SDL 0/# define USE_SDL 1/' lv_drv_conf.h
sed -i 's/#  define SDL_VER_RES     320/#  define SDL_VER_RES     800/' lv_drv_conf.h
sed -i 's/#  define SDL_WINDOW_TITLE "TFT Simulator"/#  define SDL_WINDOW_TITLE "Jeep Sensor Hub - Raspberry Pi"/' lv_drv_conf.h
# Add fullscreen configuration
if ! grep -q "SDL_FULLSCREEN" lv_drv_conf.h; then
	sed -i '/#  define SDL_DUAL_DISPLAY/a\\n/* Fullscreen mode */\n#  define SDL_FULLSCREEN              1' lv_drv_conf.h
fi
echo "✓ lv_drv_conf.h configured for SDL support with fullscreen"

echo ""
echo "Dependencies setup complete!"
echo "You can now build the project with:"
echo "  mkdir build && cd build"
echo "  cmake .. && make"
