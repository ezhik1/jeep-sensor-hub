## Timeline Modal

A configurable modal for setting gauge data visualization timeline durations. The modal provides 5 timeline options that control how often gauge data updates and how data flows across the gauge display.

### Timeline Options

- **30s**: 30 seconds - Updates every 1 second
- **1min**: 1 minute - Updates every 2 seconds
- **30min**: 30 minutes - Updates every 1 minute
- **1h**: 1 hour - Updates every 2 minutes
- **3h**: 3 hours - Updates every 6 minutes

### Gauge Configuration

The modal supports up to 6 gauges with configurable names, units, and enabled states:

1. **STARTER (V)** - Starter battery voltage
2. **HOUSE (V)** - House battery voltage
3. **SOLAR (W)** - Solar input power
4. **ENGINE (°C)** - Engine temperature
5. **OIL (PSI)** - Oil pressure
6. **FUEL (%)** - Fuel level

### Features

- **Button-style Timeline Options**: Custom clickable containers that look like buttons
- **Visual Selection Feedback**: Selected timeline option is highlighted in yellow/orange
- **Gauge List Display**: Shows all configured gauges with their names and units
- **Callback Integration**: Notifies system when timeline duration changes
- **Consistent Styling**: Follows same layout and highlighting treatments as alerts modal

### Usage

```c
// Create timeline modal
timeline_modal_t* modal = timeline_modal_create(&timeline_modal_config, on_close_callback);

// Show modal
timeline_modal_show(modal);

// Get current duration
int duration = timeline_modal_get_duration(modal);

// Set duration programmatically
timeline_modal_set_duration(modal, 60); // 1 minute
```

### Data Flow

When a timeline option is selected, the modal calculates the appropriate update interval for gauge data:

- **Full Width Traversal**: Data points flow from one end of the gauge to the other in exactly the selected time duration
- **Update Frequency**: Calculated based on gauge width and selected duration
- **Real-time Updates**: Gauge bars update at the calculated interval to maintain smooth data flow

### Integration

The timeline modal integrates with the gauge system to:
- Set gauge update frequencies
- Control data point collection rates
- Manage gauge visualization timelines
- Provide consistent user experience across all gauges
