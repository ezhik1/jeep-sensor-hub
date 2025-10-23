
## Alerts Modal

Used to control arbitrary set points for one or more gauges instantiated in any giving displayModule. The modal provides callbacks to update these setpoints for the bar_graphs and alerts treatment on detail_screen, home_screen, and anywhere else the displayModule is used.

### Gauge and Alert Set Points

#### `GAUGE` Values:
These are minimum, baseline, and maximum values used for plotting bar_graphs for individual gauges:

	- `RAW_MIN`: the absolute minimum value of the raw data (configurable in gauge config)
	- `RAW_MAX`: the absolute maximum value of the raw data (configurable in gauge config)
	- `MIN`: default minimum value for plotting values to bar graphs, clamped to the `RAW_MIN` value
	- `MAX`: default maximum value for plotting values to bar graphs, clamped to the `RAW_MAX` value

	- `LOW` Field Value: a settable field value for the low value of the gauge (between the `RAW_MIN` and current `HIGH` value)
	- `HIGH` Field Value: a settable field value for the high value of the gauge (between the current LOW value and `RAW_MAX`)

	- `BASELINE`: default baseline value for plotting values to bar graphs, directly computed as midpoint of `HIGH` and `LOW`

#### `ALERT` Values:
These are low and high setpoints that trigger visual treatment for raw and display values that go out of the specified range.

	- `LOW`: a settable field value for the LOW value used for visual treatment of raw and other numerical display values. Possible range: between the `RAW_MIN` and current `ALERT` `HIGH` value
	- `HIGH`: a settable field value for the high value used for visual treatment of raw and other numerical display values. Possible range: between the current `ALERT` `LOW` value and `RAW_MAX` value
