# API Reference

ACME Web Server provides a comprehensive REST API for server management and monitoring.

## Base URL

```
http://localhost:8080/api/v1
```

## Authentication

All API endpoints require authentication using JWT tokens or API keys.

### JWT Authentication

```http
GET /api/v1/status
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

### API Key Authentication

```http
GET /api/v1/status
X-API-Key: your-api-key-here
```

## Server Management

### Get Server Status

Returns current server status and health information.

```http
GET /api/v1/status
```

**Response:**
```json
{
  "status": "healthy",
  "uptime": 3600,
  "version": "1.0.0",
  "connections": {
    "active": 1250,
    "total": 50000,
    "max": 100000
  },
  "memory": {
    "used": 2147483648,
    "total": 8589934592,
    "percentage": 25
  },
  "cpu": {
    "usage": 45.5,
    "cores": 8
  }
}
```

### Get Server Metrics

```http
GET /api/v1/metrics
```

**Response:**
```json
{
  "requests": {
    "total": 1000000,
    "rate": 500,
    "errors": 100
  },
  "latency": {
    "p50": 45,
    "p95": 120,
    "p99": 250,
    "avg": 52
  },
  "cache": {
    "hits": 850000,
    "misses": 150000,
    "hit_rate": 85.0
  }
}
```

### Reload Configuration

Reload server configuration without downtime.

```http
POST /api/v1/config/reload
```

**Response:**
```json
{
  "success": true,
  "message": "Configuration reloaded successfully",
  "timestamp": "2025-12-04T10:15:30Z"
}
```

## Route Management

### List Routes

```http
GET /api/v1/routes
```

**Response:**
```json
{
  "routes": [
    {
      "path": "/api/users",
      "method": "GET",
      "type": "proxy",
      "backend": "http://backend:8080",
      "cache": {
        "enabled": true,
        "ttl": 300
      }
    },
    {
      "path": "/static/*",
      "method": "GET",
      "type": "static",
      "root": "./public"
    }
  ]
}
```

### Add Route

```http
POST /api/v1/routes
Content-Type: application/json

{
  "path": "/api/newservice",
  "method": "GET",
  "type": "proxy",
  "backend": "http://newservice:8080"
}
```

### Delete Route

```http
DELETE /api/v1/routes/{routeId}
```

## Cache Management

### Clear Cache

```http
POST /api/v1/cache/clear
Content-Type: application/json

{
  "pattern": "/api/users/*"
}
```

### Get Cache Stats

```http
GET /api/v1/cache/stats
```

**Response:**
```json
{
  "size": 1073741824,
  "entries": 5000,
  "hit_rate": 85.5,
  "evictions": 1000,
  "memory_usage": {
    "used": 536870912,
    "available": 536870912
  }
}
```

## Connection Management

### List Active Connections

```http
GET /api/v1/connections
```

**Response:**
```json
{
  "connections": [
    {
      "id": "conn_123",
      "remote_addr": "192.168.1.100",
      "state": "active",
      "duration": 120,
      "requests": 5,
      "bytes_sent": 102400,
      "bytes_received": 2048
    }
  ],
  "total": 1250
}
```

### Close Connection

```http
DELETE /api/v1/connections/{connectionId}
```

## SSL Certificate Management

### List Certificates

```http
GET /api/v1/ssl/certificates
```

**Response:**
```json
{
  "certificates": [
    {
      "domain": "example.com",
      "issuer": "Let's Encrypt",
      "valid_from": "2025-01-01T00:00:00Z",
      "valid_until": "2025-12-31T23:59:59Z",
      "status": "valid"
    }
  ]
}
```

### Upload Certificate

```http
POST /api/v1/ssl/certificates
Content-Type: application/json

{
  "domain": "example.com",
  "certificate": "-----BEGIN CERTIFICATE-----\n...",
  "private_key": "-----BEGIN PRIVATE KEY-----\n..."
}
```

## Response Codes

| Code | Description |
|------|-------------|
| 200 | Success |
| 201 | Created |
| 204 | No Content |
| 400 | Bad Request |
| 401 | Unauthorized |
| 403 | Forbidden |
| 404 | Not Found |
| 409 | Conflict |
| 429 | Too Many Requests |
| 500 | Internal Server Error |
| 503 | Service Unavailable |

## Error Response Format

```json
{
  "error": {
    "code": "INVALID_REQUEST",
    "message": "Invalid request parameters",
    "details": {
      "field": "port",
      "reason": "must be between 1 and 65535"
    },
    "timestamp": "2025-12-04T10:15:30Z",
    "request_id": "req_5a7b8c9d"
  }
}
```

## Rate Limits

API endpoints are rate limited:

- **Standard**: 100 requests per minute
- **Authenticated**: 1000 requests per minute
- **Admin**: Unlimited

Rate limit headers:
```http
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 950
X-RateLimit-Reset: 1701691230
```

## Next Steps

- [WebSocket API](websocket-api.md) - WebSocket endpoint documentation
- [Configuration](configuration.md) - Configuration options
- [Getting Started](getting-started.md) - Installation guide
