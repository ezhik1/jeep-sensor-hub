# API Documentation

This directory contains comprehensive API documentation for the Jeep Sensor Hub system, including REST API endpoints, WebSocket interfaces, and data formats.

## Overview

The Jeep Sensor Hub provides a comprehensive API for monitoring and controlling vehicle systems, accessing sensor data, and managing system configuration. The API is designed for both local and remote access, with support for real-time data streaming via WebSockets.

## API Architecture

### REST API
- **Base URL**: `http://localhost:3000/api` (development)
- **Authentication**: JWT-based authentication
- **Rate Limiting**: Configurable rate limiting per endpoint
- **Response Format**: JSON with consistent error handling

### WebSocket API
- **URL**: `ws://localhost:3000/ws`
- **Protocol**: JSON message format
- **Channels**: Topic-based subscription system
- **Heartbeat**: 30-second heartbeat for connection monitoring

## Authentication

### JWT Tokens
```json
{
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "refreshToken": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "expiresIn": "24h"
}
```

### Authentication Headers
```
Authorization: Bearer <token>
```

### Token Refresh
```http
POST /api/auth/refresh
Content-Type: application/json

{
  "refreshToken": "<refresh_token>"
}
```

## Rate Limiting

### Limits by Endpoint
- **General API**: 100 requests per 15 minutes
- **Authentication**: 5 requests per 15 minutes
- **Sensor Data**: 1000 requests per 15 minutes
- **File Uploads**: 10 requests per 15 minutes

### Rate Limit Headers
```
X-RateLimit-Limit: 100
X-RateLimit-Remaining: 95
X-RateLimit-Reset: 1640995200
Retry-After: 900
```

## Error Handling

### Standard Error Response
```json
{
  "error": {
    "code": "VALIDATION_ERROR",
    "message": "Invalid input parameters",
    "details": {
      "field": "temperature",
      "issue": "Value must be a number"
    },
    "timestamp": "2024-01-01T12:00:00Z",
    "requestId": "req_123456789"
  }
}
```

### Error Codes
- **VALIDATION_ERROR**: Input validation failed
- **AUTHENTICATION_ERROR**: Authentication required or failed
- **AUTHORIZATION_ERROR**: Insufficient permissions
- **NOT_FOUND_ERROR**: Resource not found
- **CONFLICT_ERROR**: Resource conflict
- **RATE_LIMIT_ERROR**: Rate limit exceeded
- **INTERNAL_ERROR**: Server internal error

## Core Endpoints

### Sensors API

#### Get All Sensor Data
```http
GET /api/sensors
Authorization: Bearer <token>
```

**Response:**
```json
{
  "data": {
    "temperature": [
      {
        "id": "temp_123",
        "value": 85.2,
        "unit": "°C",
        "timestamp": "2024-01-01T12:00:00Z",
        "metadata": {
          "location": "engine",
          "sensor_type": "DS18B20"
        }
      }
    ],
    "pressure": [
      {
        "id": "press_456",
        "value": 45.8,
        "unit": "PSI",
        "timestamp": "2024-01-01T12:00:00Z",
        "metadata": {
          "location": "oil_pressure",
          "sensor_type": "MPX5050"
        }
      }
    ]
  },
  "meta": {
    "total": 2,
    "timestamp": "2024-01-01T12:00:00Z"
  }
}
```

#### Submit Sensor Data
```http
POST /api/sensors
Authorization: Bearer <token>
Content-Type: application/json

{
  "type": "temperature",
  "value": 85.2,
  "unit": "°C",
  "metadata": {
    "location": "engine",
    "sensor_type": "DS18B20"
  }
}
```

#### Get Sensor Statistics
```http
GET /api/sensors/stats/overview
Authorization: Bearer <token>
```

**Response:**
```json
{
  "data": {
    "totalReadings": 15420,
    "sensorTypes": ["temperature", "pressure", "humidity", "voltage"],
    "sensorCounts": {
      "temperature": 5120,
      "pressure": 3840,
      "humidity": 2560,
      "voltage": 3900
    },
    "lastUpdate": "2024-01-01T12:00:00Z"
  }
}
```

### GPS API

#### Get Current Position
```http
GET /api/gps/position
Authorization: Bearer <token>
```

**Response:**
```json
{
  "data": {
    "latitude": 40.7128,
    "longitude": -74.0060,
    "altitude": 10.5,
    "speed": 35.2,
    "heading": 180.0,
    "timestamp": "2024-01-01T12:00:00Z",
    "accuracy": 3.2
  }
}
```

#### Start New Route
```http
POST /api/gps/routes/start
Authorization: Bearer <token>
Content-Type: application/json

{
  "metadata": {
    "name": "Daily Commute",
    "description": "Morning drive to work",
    "tags": ["commute", "work"]
  }
}
```

#### Get Route History
```http
GET /api/gps/routes?startDate=2024-01-01&endDate=2024-01-31
Authorization: Bearer <token>
```

**Response:**
```json
{
  "data": [
    {
      "id": "route_123",
      "startTime": "2024-01-01T08:00:00Z",
      "endTime": "2024-01-01T08:30:00Z",
      "totalDistance": 12.5,
      "totalDuration": 1800000,
      "averageSpeed": 25.0,
      "maxSpeed": 45.2,
      "waypointCount": 150,
      "metadata": {
        "name": "Daily Commute",
        "description": "Morning drive to work"
      }
    }
  ],
  "meta": {
    "total": 1,
    "page": 1,
    "limit": 10
  }
}
```

### Power Management API

#### Get Power Status
```http
GET /api/power/status
Authorization: Bearer <token>
```

**Response:**
```json
{
  "data": {
    "source": "auxiliary",
    "voltage": 12.8,
    "current": 2.5,
    "power": 32.0,
    "batteryLevel": 85,
    "engineState": true,
    "charging": false,
    "lastUpdate": "2024-01-01T12:00:00Z"
  }
}
```

#### Switch Power Source
```http
POST /api/power/source/switch
Authorization: Bearer <token>
Content-Type: application/json

{
  "source": "starter"
}
```

#### Get Battery Health
```http
GET /api/power/battery/health
Authorization: Bearer <token>
```

**Response:**
```json
{
  "data": {
    "health": "good",
    "status": "charged",
    "voltage": 12.8,
    "batteryLevel": 85,
    "charging": false,
    "lastUpdate": "2024-01-01T12:00:00Z"
  }
}
```

### Cooling System API

#### Get Cooling Status
```http
GET /api/cooling/status
Authorization: Bearer <token>
```

**Response:**
```json
{
  "data": {
    "engineTemp": 85.2,
    "coolantTemp": 82.1,
    "oilTemp": 88.5,
    "ambientTemp": 25.0,
    "fanSpeed": 1,
    "fanOverride": false,
    "acState": true,
    "engineLoad": 65,
    "lastUpdate": "2024-01-01T12:00:00Z"
  }
}
```

#### Update Cooling Configuration
```http
PUT /api/cooling/config
Authorization: Bearer <token>
Content-Type: application/json

{
  "lowSpeedThreshold": 80,
  "highSpeedThreshold": 90,
  "criticalThreshold": 100,
  "hysteresis": 3
}
```

#### Set Fan Override
```http
POST /api/cooling/fan/override
Authorization: Bearer <token>
Content-Type: application/json

{
  "override": true,
  "timeout": 300000
}
```

## WebSocket API

### Connection
```javascript
const ws = new WebSocket('ws://localhost:3000/ws');

ws.onopen = () => {
  console.log('Connected to WebSocket');

  // Subscribe to channels
  ws.send(JSON.stringify({
    type: 'subscribe',
    payload: {
      channels: ['sensors', 'gps', 'power', 'cooling']
    }
  }));
};
```

### Message Types

#### Data Updates
```json
{
  "type": "data_update",
  "data": {
    "sensorType": "temperature",
    "value": 85.2,
    "timestamp": "2024-01-01T12:00:00Z"
  },
  "channels": ["sensors"],
  "timestamp": "2024-01-01T12:00:00Z"
}
```

#### System Alerts
```json
{
  "type": "system_alert",
  "alert": {
    "id": "alert_123",
    "level": "warning",
    "message": "Engine temperature approaching threshold",
    "timestamp": "2024-01-01T12:00:00Z"
  }
}
```

#### Heartbeat
```json
{
  "type": "heartbeat",
  "timestamp": "2024-01-01T12:00:00Z",
  "serverTime": "2024-01-01T12:00:00.123Z"
}
```

### Channel Subscriptions

#### Available Channels
- **sensors**: Real-time sensor data updates
- **gps**: GPS position and route updates
- **power**: Power status and consumption updates
- **cooling**: Cooling system status updates
- **alerts**: System alerts and notifications
- **all**: All data updates

#### Subscription Management
```javascript
// Subscribe to specific channels
ws.send(JSON.stringify({
  type: 'subscribe',
  payload: {
    channels: ['sensors', 'gps']
  }
}));

// Unsubscribe from channels
ws.send(JSON.stringify({
  type: 'unsubscribe',
  payload: {
    channels: ['gps']
  }
}));
```

## Data Export

### Export Formats
- **CSV**: Comma-separated values for spreadsheet analysis
- **JSON**: Structured data for programmatic access
- **GPX**: GPS Exchange Format for mapping applications
- **KML**: Keyhole Markup Language for Google Earth

### Export Endpoints

#### Export Sensor Data
```http
GET /api/data/export/sensors?format=csv&startDate=2024-01-01&endDate=2024-01-31
Authorization: Bearer <token>
```

#### Export GPS Routes
```http
GET /api/data/export/gps?format=gpx&routeId=route_123
Authorization: Bearer <token>
```

#### Export All Data
```http
GET /api/data/export/all?format=json&startDate=2024-01-01&endDate=2024-01-31
Authorization: Bearer <token>
```

## System Management

### System Status
```http
GET /api/system/status
Authorization: Bearer <token>
```

**Response:**
```json
{
  "data": {
    "status": "operational",
    "uptime": 86400000,
    "version": "1.0.0",
    "lastRestart": "2024-01-01T00:00:00Z",
    "services": {
      "sensors": "running",
      "gps": "running",
      "power": "running",
      "cooling": "running",
      "websocket": "running"
    }
  }
}
```

### System Configuration
```http
GET /api/system/config
Authorization: Bearer <token>
```

### System Logs
```http
GET /api/system/logs?level=error&limit=100
Authorization: Bearer <token>
```

## Development and Testing

### API Testing
- **Postman Collection**: Available in `/docs/postman/`
- **Swagger Documentation**: Available at `/api/docs`
- **Test Environment**: Separate endpoints for testing

### Mock Data
- **Development Mode**: Generates realistic mock data
- **Testing Endpoints**: Simulate various system states
- **Fault Injection**: Test error handling and edge cases

### Performance Monitoring
- **Response Times**: Track API performance metrics
- **Error Rates**: Monitor API error frequencies
- **Usage Statistics**: Track API usage patterns

## Security Considerations

### Authentication
- **JWT Tokens**: Secure, stateless authentication
- **Token Expiration**: Configurable token lifetimes
- **Refresh Tokens**: Secure token renewal process

### Authorization
- **Role-Based Access**: Different permission levels
- **Resource Isolation**: User data separation
- **API Key Management**: Secure API key handling

### Data Protection
- **HTTPS Only**: Encrypted data transmission
- **Input Validation**: Comprehensive input sanitization
- **Rate Limiting**: Prevent abuse and DoS attacks

## Future Enhancements

### Planned Features
- **GraphQL API**: Alternative to REST for complex queries
- **Webhook Support**: Real-time notifications to external systems
- **API Versioning**: Backward compatibility management
- **Bulk Operations**: Efficient batch data processing

### Integration Support
- **OAuth 2.0**: Third-party authentication
- **API Keys**: Simplified authentication for applications
- **WebSocket Clustering**: Scalable real-time communication
- **Message Queuing**: Reliable message delivery

## Support and Resources

### Documentation
- **Interactive API Docs**: Swagger UI at `/api/docs`
- **Code Examples**: Available in `/docs/examples/`
- **SDK Libraries**: Client libraries for various languages

### Community
- **GitHub Issues**: Bug reports and feature requests
- **Discord Server**: Community discussions and support
- **Email Support**: Direct support for enterprise users

### Updates
- **Changelog**: Available in `/docs/changelog/`
- **Migration Guides**: Version upgrade instructions
- **Deprecation Notices**: API endpoint lifecycle management

## License

This API documentation is licensed under the MIT License. See the LICENSE file for details.
