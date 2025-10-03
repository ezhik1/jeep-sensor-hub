Test the API endpoints to verify data is being served correctly


$ from mock-sensor-hub python main.py

sometimes the services hangs. in which case:

$ taskkill /F /IM python.exe

Access the mock sensor hub at http://localhost:8000
View the API documentation at http://localhost:8000/docs
Test the WebSocket endpoint at ws://localhost:8000/ws/sensors
Get current sensor data at http://localhost:8000/api/sensors/current
