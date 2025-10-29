# Display Modules

This directory contains all display modules for the Jeep Sensor Hub UI. Each module is a self-contained unit that provides a specific set of functionality and can be displayed on the home screen or in detail views.

## Overview

### Current System Architecture

Display modules use a **standardized interface** (`display_module_t`) that allows the main application to manage all modules uniformly:

1. **Automatic Registration**: Modules declare a `const display_module_t` structure that registers themselves
2. **Centralized Management**: All modules are registered in `shared/module_interface.c` and managed automatically
3. **Lifecycle Management**: Three key functions handle module lifecycle:
   - `init()` - Called once at startup
   - `update()` - Called every frame (data updates only, no UI)
   - `cleanup()` - Called at shutdown

### Key Concepts

- **Module Interface**: Every module must implement `display_module_t` with `name`, `init`, `update`, and `cleanup` fields
- **Data Storage**: Module data is typically stored in `app_data_store`, not in the module itself
- **Configuration**: Module settings are stored in `device_state` for persistence
- **UI Separation**: UI creation/destruction is handled separately by the screen system
- **No display_module_base**: The old `display_module_base_t` system is no longer used; modules are simpler

## Module Structure

Each display module follows a standardized structure and implements a consistent interface for initialization, updates, and cleanup.

### Directory Layout

```
displayModules/
├── README.md                   # This file
├── shared/                     # Shared components and utilities
│   ├── current_view/           # Current view management
│   ├── gauges/                 # Gauge components (bar graphs, etc.)
│   ├── modals/                 # Modal dialogs (alerts, timeline, etc.)
│   ├── utils/                  # Utility functions
│   ├── views/                  # Shared view components
│   ├── display_module_base.h   # Base lifecycle interface
│   └── module_interface.h      # Module registration interface
└── power-monitor/              # Example module
	├── config/                 # Module-specific configuration
	├── views/                  # Module-specific views
	├── power-monitor.h         # Public interface
	├── power-monitor.c         # Implementation
	└── gauge_types.h          # Type definitions
```

## Adding a New Module

### 1. Create Module Directory

Create a new directory for your module:

```bash
mkdir src/displayModules/your-module-name
```

### 2. Required Files

Each module must have these files:

#### `your-module-name.h` - Public Interface
```c
#ifndef YOUR_MODULE_NAME_H
#define YOUR_MODULE_NAME_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Module data structure (typically stored in app_data_store)
typedef struct {
	// Your module's data fields
	float some_value;
	bool some_flag;
	// ... other fields
} your_module_data_t;

// Module lifecycle functions
void your_module_init(void);        // Initialize module (called once at startup)
void your_module_cleanup(void);     // Cleanup module (called at shutdown)
your_module_data_t* your_module_get_data(void);  // Get module data from app_data_store

#ifdef __cplusplus
}
#endif

#endif // YOUR_MODULE_NAME_H
```

#### `your-module-name.c` - Implementation

```c
#include "your-module-name.h"
#include "../shared/module_interface.h"  // Required for module registration
#include "../../app_data_store.h"        // For data storage
#include "../../state/device_state.h"    // For configuration

// Module interface implementation - these are called by the module system
static void your_module_module_init(void)
{
	printf("[I] your_module: Module initializing via standardized interface\n");

	// Initialize module-specific data and defaults
	your_module_init();

	// Set up default configuration values in device_state
	// (see Configuration section below)
}

static void your_module_module_update(void)
{
	// This function is called every frame/tick
	// Update module data here (no UI creation/destruction)
	// Example:
	your_module_data_t* data = your_module_get_data();
	if (data) {
		// Update data values from sensors, calculations, etc.
		data->some_value = /* calculate or read value */;
	}
}

static void your_module_module_cleanup(void)
{
	printf("[I] your_module: Module cleaning up via standardized interface\n");
	your_module_cleanup();
}

// Module registration - this is how the module system finds your module
const display_module_t your_module_module = {
	.name = "your-module-name",           // Module identifier
	.init = your_module_module_init,      // Initialization function
	.update = your_module_module_update,  // Update function (called every frame)
	.cleanup = your_module_module_cleanup // Cleanup function
};
```

### 3. Register Module

Add your module to the registry in `shared/module_interface.c`:

```c
// Forward declarations for module interfaces
extern const display_module_t power_monitor_module;
extern const display_module_t your_module_module;  // Add this line

// Registry of all display modules
static const display_module_t* registered_modules[] = {
	&power_monitor_module,
	&your_module_module,  // Add this line
	// Add other modules here as they are created
};
```

### 4. Add to Home Screen (Optional)

If your module should appear on the home screen as a widget, you need to:

1. **Implement a home screen render function** that creates the UI in a given container:
   ```c
   void your_module_show_in_container_home(lv_obj_t* container)
   {
       // Create your module's home screen widget UI in the container
       // This is typically a simplified/mini view of your module
   }
   ```

2. **Register in home screen module registry** in `screens/home_screen/home_screen.c`:
   ```c
   // In the module registry
   static const module_registry_entry_t module_registry[] = {
       {"power-monitor", {power_monitor_show_in_container_home}},
       {"your-module-name", {your_module_show_in_container_home}},  // Add this line
       // ... other modules
   };
   ```

3. **Implement detail screen support** (if needed) by providing functions:
   ```c
   void your_module_show_detail_screen(void);     // Show full detail view
   void your_module_destroy_detail_screen(void);  // Clean up detail view
   ```

## Module Lifecycle

The module system manages the lifecycle automatically through the `display_module_t` interface:

### Initialization Phase
1. **`init()` function** - Called once at application startup
   - Initialize module data structures
   - Set up default configuration values in device_state
   - Initialize any required resources
   - Register with data sources if needed
   - Example: `power_monitor_module_init()` → `power_monitor_init()`

### Runtime Phase
2. **`update()` function** - Called every frame/tick during main loop
   - Update module data from sensors or calculations
   - **Important**: Do NOT create or destroy UI elements here
   - Only update data values and handle data processing
   - Example: `power_monitor_module_update()` updates gauge histories

### Cleanup Phase
3. **`cleanup()` function** - Called at application shutdown
   - Clean up module resources
   - Free allocated memory
   - Unregister from data sources
   - Example: `power_monitor_module_cleanup()` → `power_monitor_cleanup()`

### UI Lifecycle (Separate from Module Lifecycle)

UI creation/destruction is handled separately by the screen system (home screen, detail screen). Modules expose functions like `your_module_show_in_container()` that are called by the screen system when the module UI needs to be displayed.

## Shared Components

### Gauges (`shared/gauges/`)

#### Bar Graph Gauge
```c
#include "../shared/gauges/bar_graph_gauge/bar_graph_gauge.h"

// Initialize gauge
bar_graph_gauge_t my_gauge;
bar_graph_gauge_init(&my_gauge, container, x, y, width, height, bar_width, bar_gap);

// Configure gauge
bar_graph_gauge_configure_advanced(&my_gauge,
	BAR_GRAPH_MODE_POSITIVE_ONLY,  // or BAR_GRAPH_MODE_BIPOLAR
	min_value, max_value, baseline_value,
	alert_low, alert_high,
	color_normal, color_alert, color_warning
);

// Add data points
bar_graph_gauge_add_data_point(&my_gauge, value);
```

### Modals (`shared/modals/`)

#### Alerts Modal
```c
#include "../shared/modals/alerts_modal/alerts_modal.h"

// Define gauge configurations
const alerts_modal_gauge_config_t my_gauge_configs[] = {
	{
		.name = "MY GAUGE",
		.unit = "V",
		.raw_min_value = 0.0f,
		.raw_max_value = 20.0f,
		.fields = {
			[FIELD_ALERT_LOW] = {.name = "LOW", .min_value = 0.0f, .max_value = 20.0f, .default_value = 10.0f, .is_baseline = false},
			[FIELD_ALERT_HIGH] = {.name = "HIGH", .min_value = 0.0f, .max_value = 20.0f, .default_value = 15.0f, .is_baseline = false},
			// ... other fields
		},
		.has_baseline = true
	}
};

// Create modal config
const alerts_modal_config_t my_alerts_config = {
	.gauge_count = 1,
	.gauges = my_gauge_configs,
	.get_state_value = my_get_state_value,
	.set_state_value = my_set_state_value
};
```

#### Timeline Modal
```c
#include "../shared/modals/timeline_modal/timeline_modal.h"

// Define timeline configurations
const timeline_modal_gauge_config_t my_timeline_configs[] = {
	{
		.name = "my_gauge",
		.display_name = "My Gauge",
		.unit = "V",
		.get_timeline_duration = my_get_timeline_duration,
		.set_timeline_duration = my_set_timeline_duration
	}
};

// Create modal config
const timeline_modal_config_t my_timeline_config = {
	.gauge_count = 1,
	.gauges = my_timeline_configs
};
```

### Utilities (`shared/utils/`)

#### Number Formatting
```c
#include "../shared/utils/number_formatting/number_formatting.h"

// Format and display a number
number_formatting_config_t config = {
	.label = my_label,
	.number_alignment = LABEL_ALIGN_RIGHT,
	.warning_alignment = LABEL_ALIGN_LEFT,
	.color = PALETTE_WHITE,
	.font = &lv_font_montserrat_16,
	.show_warning = false,
	.show_error = false
};

format_and_display_number(value, &config);
```

#### Warning Icon
```c
#include "../shared/utils/warning_icon/warning_icon.h"

// Create warning icon
create_warning_icon(parent, label, 30, LABEL_ALIGN_RIGHT);

// Hide warning icon
hide_warning_icon(parent);
```

### Current View Management (`shared/current_view/`)

For modules with multiple views:

```c
#include "../shared/current_view/current_view_manager.h"

// Initialize view manager
current_view_manager_init(3);  // Number of views

// Get current view index
int current_index = current_view_manager_get_index();

// Cycle to next view
current_view_manager_cycle_to_next();

// Check if cycling is in progress
bool is_cycling = current_view_manager_is_cycling_in_progress();
```

## Configuration

### Device State Integration

Modules should store their configuration in the device state system:

```c
#include "../../state/device_state.h"

// Set configuration values
device_state_set_float("your_module.some_setting", 12.5f);
device_state_set_int("your_module.alert_threshold", 100);
device_state_set_bool("your_module.enabled", true);

// Get configuration values
float value = device_state_get_float("your_module.some_setting");
int threshold = device_state_get_int("your_module.alert_threshold");
bool enabled = device_state_get_bool("your_module.enabled");
```

### Default Values

Initialize default values in your module's init function:

```c
static void your_module_init_defaults(void) {
	typedef struct {
		const char* path;
		double value;
	} your_module_default_t;

	your_module_default_t defaults[] = {
		{"your_module.some_setting", 12.5},
		{"your_module.alert_threshold", 100},
		{"your_module.enabled", 1.0},
	};

	for (size_t i = 0; i < sizeof(defaults) / sizeof(defaults[0]); i++) {
		if (!device_state_path_exists(defaults[i].path)) {
			device_state_set_value(defaults[i].path, defaults[i].value);
		}
	}
}
```

## Data Management

### App Data Store Integration

Modules should use the app data store for their data:

```c
#include "../../app_data_store.h"

// Get module data
your_module_data_t* data = app_data_store_get_module_data("your-module-name");

// Update data
data->some_value = new_value;
data->some_flag = true;
```

### LERP Data Integration

For real-time data that needs smoothing:

```c
#include "../../data/lerp_data/lerp_data.h"

// Define LERP data structure
typedef struct {
	lerp_value_t some_sensor;
	lerp_value_t another_sensor;
} lerp_your_module_data_t;

// Get current LERP data
lerp_your_module_data_t lerp_data;
lerp_data_get_current(&lerp_data);

// Get smoothed value
float smoothed_value = lerp_value_get_display(&lerp_data.some_sensor);
```

## Best Practices

### 1. Separation of Concerns
- **Data**: Store in app data store, not in module
- **UI**: Handle in module lifecycle functions
- **Configuration**: Store in device state

### 2. Error Handling
- Always check for NULL pointers
- Validate LVGL object validity with `lv_obj_is_valid()`
- Use proper error logging

### 3. Memory Management
- Clean up resources in destroy functions
- Use static allocation when possible
- Avoid memory leaks in UI creation

### 4. Performance
- Minimize UI creation/destruction in render functions
- Use efficient data structures
- Cache frequently accessed values

### 5. Modularity
- Keep module-specific code in module directory
- Use shared components when possible
- Minimize dependencies between modules

## Example: Complete Module

See `power-monitor/` for a complete example of a module implementation with:
- Standardized module interface (`power_monitor_module`)
- Multiple views (voltage grid, amperage grid, power grid, single value views)
- Gauge integration with history
- Modal dialogs (alerts, timeline)
- Data persistence via app_data_store and device_state
- View cycling using current_view_manager
- Home screen integration
- Detail screen with full functionality

### Key Implementation Points from power-monitor:

1. **Module Registration** (at end of `power-monitor.c`):
   ```c
   const display_module_t power_monitor_module = {
       .name = "power-monitor",
       .init = power_monitor_module_init,
       .update = power_monitor_module_update,
       .cleanup = power_monitor_module_cleanup
   };
   ```

2. **Module Init Function**:
   - Sets up defaults in device_state
   - Initializes data structures
   - Sets up view management

3. **Module Update Function**:
   - Updates gauge histories every frame
   - Updates data values
   - Handles view refresh needs
   - **No UI creation/destruction**

4. **Module Cleanup Function**:
   - Cleans up all module resources
   - Frees allocated memory
   - Resets state

## Troubleshooting

### Common Issues

1. **Module not initializing**
   - Check that `display_module_t` structure is declared and exported
   - Verify module is registered in `shared/module_interface.c`
   - Ensure module's `init()` function is properly implemented
   - Check console logs for initialization errors

2. **Module data not updating**
   - Ensure `update()` function is implemented and accessing data correctly
   - Check that data is stored in `app_data_store`
   - Verify `update()` is only updating data, not creating UI

3. **Module not appearing on home screen**
   - Check module registration in `module_interface.c`
   - Verify module is added to home screen registry in `home_screen.c`
   - Ensure `your_module_show_in_container_home()` function is implemented

4. **Memory leaks**
   - Verify all resources are cleaned up in `cleanup()` function
   - Check for proper LVGL object lifecycle (especially timers!)
   - Ensure gauge cleanup functions are called before freeing memory
   - Watch for double-free bugs (destroy functions that call free after cleanup functions)

5. **Configuration not persisting**
   - Ensure values are saved to device_state, not just in-memory
   - Check default value initialization in `init()` function
   - Verify device_state paths are correct

6. **Module crashes on update**
   - Ensure `update()` function does NOT create or destroy UI elements
   - Check for NULL pointer access
   - Verify data structures are properly initialized in `init()`

### Debug Tips

- Use `printf()` statements for debugging
- Check LVGL object validity before operations
- Monitor memory usage during development
- Test module lifecycle thoroughly
