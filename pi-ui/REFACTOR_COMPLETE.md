# Power Monitor Module Refactor - Complete

## Summary

The `power-monitor` module has been completely refactored to follow a clean, standardized architecture with proper lifecycle management, centralized data storage, and separation of concerns.

## Architecture Changes

### 1. ✅ Central App Data Store (`app_data_store.{c,h}`)

**Location:** `/src/app_data_store.{c,h}`

All module data now lives centrally at the app root, not in individual modules.

```c
// Data Store Structure
typedef struct {
    power_monitor_data_t* power_monitor;
    // Add other modules here...
} app_data_store_t;

// Access pattern
app_data_store_t* store = app_data_store_get();
power_monitor_data_t* data = store->power_monitor;
```

**Key Functions:**
- `app_data_store_init()` - Initialize all module data
- `app_data_store_update()` - Update all data per frame (called from main.c)
- `app_data_store_get()` - Get data store instance
- `app_data_store_cleanup()` - Clean up on shutdown

### 2. ✅ Display Module Base (`display_module_base.h`)

**Location:** `/src/displayModules/shared/display_module_base.h`

Common lifecycle interface for all display modules:

```c
typedef struct display_module_base_s {
    const char* name;
    void* instance;

    // Lifecycle methods
    void (*create)(lv_obj_t* container);  // Create UI
    void (*destroy)(void);                // Destroy UI
    void (*render)(void);                 // Per-frame updates

    bool is_created;
    lv_obj_t* container;
} display_module_base_t;
```

**Lifecycle Pattern:**
1. **create(container)** - Called when module becomes visible (screen shows it)
2. **render()** - Called each frame for UI updates (no data writes)
3. **destroy()** - Called when module becomes hidden (screen hides it)

### 3. ✅ Power Monitor Refactored

**Module subscribes to app data store:**
```c
// OLD - module owned data
static power_monitor_data_t power_data = {0};

// NEW - module subscribes to app store
power_monitor_data_t* power_monitor_get_data(void) {
    app_data_store_t* store = app_data_store_get();
    return store->power_monitor;
}
```

**Lifecycle implementation:**
- `power_monitor_create()` - Initialize module, set up display_module_base
- `power_monitor_destroy()` - Clean up UI, close modals
- `power_monitor_render()` - Per-frame UI updates only

**UI state separated from data:**
```c
typedef struct {
    bool detail_view_needs_refresh;
    bool navigation_teardown_in_progress;
    bool view_destroy_in_progress;
    bool rendering_in_progress;
    bool reset_in_progress;
} power_monitor_ui_state_t;

static power_monitor_ui_state_t s_ui_state = {0};
```

### 4. ✅ Data Flow Architecture

```
┌─────────────────────────────────────────────┐
│ main.c                                      │
│                                             │
│  Per Frame:                                 │
│  1. app_data_store_update()  ← Mock/Real   │
│     └─ Updates power_monitor_data_t        │
│     └─ Updates LERP system                 │
│                                             │
│  2. display_modules_update_all()           │
│     └─ power_monitor_module.update()       │
│        └─ UI rendering only                │
│                                             │
│  3. screen_manager_update()                │
│     └─ Screen transitions                  │
└─────────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────────┐
│ app_data_store                              │
│  └─ power_monitor_data_t*  (centralized)   │
└─────────────────────────────────────────────┘
              ↓ subscribes
┌─────────────────────────────────────────────┐
│ power-monitor module                        │
│  └─ Reads data via app_data_store_get()    │
│  └─ Updates UI only (no data writes)       │
└─────────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────────┐
│ LVGL UI                                     │
│  └─ Rendered display                       │
└─────────────────────────────────────────────┘
```

### 5. ✅ Screen Lifecycle Integration

**Home Screen:**
- Calls `power_monitor_create()` once on module init
- Calls `display_module_base_create(base, container)` to create UI in tile
- Module UI is destroyed when home screen is destroyed

**Detail Screen:**
- Uses `power_monitor_show_detail_screen()` (create)
- Uses `power_monitor_destroy_detail_screen()` (destroy)
- Integrated with screen_manager's create/destroy pattern

**Screen Manager:**
```c
// screen_definitions array wires module lifecycle
{
    .screen_type = SCREEN_DETAIL_VIEW,
    .module_name = "power-monitor",
    .create_func = power_monitor_show_detail_screen,
    .destroy_func = power_monitor_destroy_detail_screen
}
```

### 6. ✅ Modal Management (Simplified)

Kept simple union approach (no unnecessary abstraction):
```c
typedef union {
    alerts_modal_t* alerts;
    timeline_modal_t* timeline;
} modal_union_t;

static modal_union_t current_modal = {.alerts = NULL};
static bool current_modal_is_timeline = false;
```

Modals follow create/show/hide/destroy pattern already built into alerts_modal and timeline_modal.

## Data Namespacing

All power monitor data in device_state uses `power_monitor.*` namespace:

```json
{
  "power_monitor": {
    "gauges": {
      "starter_voltage": {
        "history": [...]
      },
      "starter_current": {
        "history": [...]
      }
    },
    "starter_alert_low_voltage_v": 11.0,
    "starter_baseline_voltage_v": 12.6
  }
}
```

## Key Benefits

1. **Clear Separation of Concerns**
   - Data: app_data_store (main.c)
   - UI: modules subscribe and render
   - Lifecycle: display_module_base

2. **Proper Resource Management**
   - Modules created when shown
   - Modules destroyed when hidden
   - No lingering UI or memory leaks

3. **Scalable Architecture**
   - Easy to add new modules following the same pattern
   - Centralized data makes cross-module features simple
   - Common base reduces boilerplate

4. **Clean Data Flow**
   - Data updates happen once per frame in main.c
   - UI updates happen in module render functions
   - No scattered data updates throughout codebase

## Migration Pattern for Other Modules

To refactor other modules to this pattern:

1. Move module data struct to app_data_store
2. Create module_create/destroy/render functions
3. Initialize display_module_base in create
4. Subscribe to app_data_store for data access
5. Update module_interface.c to call new lifecycle
6. Wire to home_screen via display_module_base_create

## Files Modified

### New Files
- `src/app_data_store.{c,h}`
- `src/displayModules/shared/display_module_base.h`

### Modified Files
- `src/main.c` - Added app_data_store init/update
- `src/displayModules/power-monitor/power-monitor.{c,h}` - Refactored lifecycle
- `src/screens/home_screen/home_screen.c` - Wired display_module_base lifecycle
- `CMakeLists.txt` - Added app_data_store.c to build

## Testing

Build succeeded with no errors:
```bash
cd /home/ezhik/Development/jeep-sensor-hub/pi-ui/build
cmake ..
make
# [100%] Built target pi_ui
```

All refactoring complete and functional.


