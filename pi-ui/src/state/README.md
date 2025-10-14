# Device State Management

This module provides persistent JSON-based state management for the Jeep Sensor Hub UI application.

## Overview

The device state system stores application settings and configuration data in a JSON file that persists across application restarts and system reboots. All settings are saved instantly when modified, ensuring no data loss.

## State File Location

The state file is stored in a persistent location determined by the following priority order:

1. **`$XDG_DATA_HOME/jeep_sensor_hub_state.json`** (if `XDG_DATA_HOME` is set)
2. **`$HOME/.local/share/jeep_sensor_hub_state.json`** (default fallback)
3. **`./jeep_sensor_hub_state.json`** (last resort)

### Example Paths
- **Linux**: `/home/username/.local/share/jeep_sensor_hub_state.json`
- **macOS**: `/Users/username/.local/share/jeep_sensor_hub_state.json`
- **Custom XDG**: `/custom/data/path/jeep_sensor_hub_state.json`

## JSON Structure

The state file follows a hierarchical namespace structure:

```json
{
  "global": {
    "brightness_level": 100,
    "auto_save_enabled": true,
    "auto_save_interval_ms": 5000
  },
  "system": {
    "initialized": true,
    "last_save_timestamp": 1760416288
  },
  "power_monitor": {
    "gauge_timeline_settings": {
      "starter_voltage": {
        "current_view": 30,
        "detail_view": 30
      },
      "starter_current": {
        "current_view": 30,
        "detail_view": 30
      }
    },
    "starter_alert_low_voltage_v": 11,
    "starter_alert_high_voltage_v": 14,
    "starter_baseline_voltage_v": 12.6,
    "starter_min_voltage_v": 11,
    "starter_max_voltage_v": 14.4
  }
}
```

## API Functions

### Initialization
```c
void device_state_init(void);        // Initialize state system
void device_state_cleanup(void);     // Cleanup resources
```

### Data Access
```c
// Get values
int device_state_get_int(const char* path);
float device_state_get_float(const char* path);
bool device_state_get_bool(const char* path);

// Set values (saves instantly)
void device_state_set_int(const char* path, int value);
void device_state_set_float(const char* path, float value);
void device_state_set_bool(const char* path, bool value);

// Generic setter (saves instantly)
void device_state_set_value(const char* path, double value);

// Check if path exists
bool device_state_path_exists(const char* path);
```

### File Operations
```c
void device_state_save(void);        // Save to file
void device_state_load(void);        // Load from file
```

## Usage Examples

### Setting Values
```c
// Set power monitor settings
device_state_set_int("power_monitor.starter_alert_low_voltage_v", 10);
device_state_set_float("power_monitor.starter_baseline_voltage_v", 12.8f);
device_state_set_bool("global.auto_save_enabled", false);

// Set nested values
device_state_set_int("power_monitor.gauge_timeline_settings.starter_voltage.current_view", 60);
```

### Getting Values
```c
// Get values with defaults
int low_voltage = device_state_get_int("power_monitor.starter_alert_low_voltage_v");
float baseline = device_state_get_float("power_monitor.starter_baseline_voltage_v");
bool auto_save = device_state_get_bool("global.auto_save_enabled");

// Check if setting exists
if (device_state_path_exists("power_monitor.custom_setting")) {
    // Handle custom setting
}
```

## Behavior

### Instant Save
- **All setter functions save immediately** when called
- **No auto-save timer** - changes are persisted instantly
- **No data loss** - every setting change is immediately written to disk

### Directory Creation
- **Automatic directory creation** - creates parent directories if they don't exist
- **Proper permissions** - directories created with `0755` (rwxr-xr-x)
- **Error handling** - warns if directory creation fails

### Error Handling
- **Graceful fallbacks** - uses default values if file doesn't exist
- **JSON validation** - handles malformed JSON gracefully
- **File I/O errors** - logs errors and continues with defaults

### Initialization Order
1. Create root JSON object with defaults
2. Load existing state from file (if exists)
3. Module-specific defaults are applied by individual modules
4. State is ready for use

## Module Integration

### Power Monitor Example
```c
// In power_monitor_init()
void power_monitor_init_defaults(void) {
    // Only set defaults if they don't exist
    if (!device_state_path_exists("power_monitor.starter_alert_low_voltage_v")) {
        device_state_set_int("power_monitor.starter_alert_low_voltage_v", 11);
    }
    // ... more defaults
}
```

### Settings UI Example
```c
// In settings UI callback
void on_voltage_threshold_changed(int new_value) {
    device_state_set_int("power_monitor.starter_alert_low_voltage_v", new_value);
    // Value is automatically saved to disk
}
```

## File Format

- **Format**: JSON (cJSON library)
- **Encoding**: UTF-8
- **Indentation**: Tabs (matching project style)
- **Comments**: Not supported (pure JSON)

## Dependencies

- **cJSON**: JSON parsing and generation
- **Standard C libraries**: stdio, stdlib, string, unistd, time, sys/stat

## Thread Safety

- **Not thread-safe**: All functions should be called from the main thread
- **LVGL context**: State operations should be called from LVGL context when possible

## Migration from /tmp

The state file was previously stored in `/tmp/jeep_sensor_hub_state.json`, which was cleared on reboot. The new persistent location ensures settings survive system restarts.

## Troubleshooting

### Settings Not Persisting
1. Check file permissions: `ls -la ~/.local/share/jeep_sensor_hub_state.json`
2. Verify directory exists: `ls -la ~/.local/share/`
3. Check application logs for save errors

### File Not Found
- This is normal on first run - defaults will be used
- File will be created when first setting is saved

### Permission Errors
- Ensure user has write access to `~/.local/share/`
- Check if directory creation failed in logs

## Future Enhancements

- **Backup system**: Automatic backup of state file
- **Migration tools**: Convert between different state file formats
- **Validation**: JSON schema validation for state file
- **Compression**: Optional compression for large state files
