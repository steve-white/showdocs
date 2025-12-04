# Request Flow

This document details how requests flow through ACME Web Server, from initial connection to final response.

## Complete Request Flow

```mermaid
sequenceDiagram
    participant C as Client
    participant CM as Connection Manager
    participant SSL as SSL/TLS Handler
    participant PP as Protocol Parser
    participant MW as Middleware Chain
    participant R as Router
    participant CH as Cache
    participant H as Handler
    participant B as Backend
    participant RM as Response Manager
    
    C->>CM: TCP Connection
    CM->>SSL: Establish SSL/TLS
    SSL->>PP: Encrypted Stream
    
    PP->>PP: Parse HTTP Request
    PP->>MW: Validated Request
    
    MW->>MW: Auth Check
    MW->>MW: Rate Limiting
    MW->>MW: Request Logging
    
    MW->>R: Route Request
    R->>R: Match Route
    
    alt Cache Hit
        R->>CH: Check Cache
        CH->>RM: Cached Response
    else Cache Miss
        R->>H: Execute Handler
        
        alt Static File
            H->>H: Read File
            H->>RM: File Content
        else Dynamic Content
            H->>B: Forward Request
            B->>H: Backend Response
            H->>CH: Store in Cache
            H->>RM: Response Data
        end
    end
    
    RM->>RM: Apply Response Middleware
    RM->>RM: Compress (if enabled)
    RM->>RM: Set Security Headers
    RM->>SSL: Response
    SSL->>CM: Encrypted Response
    CM->>C: HTTP Response
    
    Note over C,CM: Connection Kept Alive or Closed
```

## Detailed Flow Stages

### Stage 1: Connection Establishment

**Duration**: 1-100ms (depending on SSL/TLS)

```mermaid
graph LR
    A[Client SYN] --> B[Server SYN-ACK]
    B --> C[Client ACK]
    C --> D{SSL?}
    D -->|Yes| E[TLS Handshake]
    D -->|No| F[Ready for HTTP]
    E --> F
```

**Steps:**
1. TCP three-way handshake
2. SSL/TLS negotiation (if HTTPS)
   - ClientHello
   - ServerHello + Certificate
   - Key Exchange
   - Finished
3. Connection added to pool

**Optimizations:**
- TCP Fast Open (TFO)
- TLS session resumption
- HTTP/2 connection reuse

### Stage 2: Request Parsing

**Duration**: <1ms

**Process:**
1. Read request line: `GET /api/users HTTP/1.1`
2. Parse headers (until blank line)
3. Validate request format
4. Extract body (if present)

**Validation Checks:**
- Valid HTTP method
- Proper header format
- Content-Length matches body
- Maximum header size (default: 8KB)
- Maximum body size (configurable)

**Example Request:**
```http
GET /api/users?page=1 HTTP/1.1
Host: example.com
User-Agent: Mozilla/5.0
Accept: application/json
Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGc...
```

### Stage 3: Middleware Processing

**Duration**: 1-50ms

Middleware executes in order:

#### 1. Request Logger
```
[2025-12-04 10:15:30] INFO GET /api/users from 192.168.1.100
```

#### 2. Authentication
- Extract token from header/cookie
- Validate signature
- Verify expiration
- Load user context

#### 3. Rate Limiter
- Check request count for IP/user
- Apply sliding window algorithm
- Return 429 if limit exceeded

**Rate Limit Algorithm:**
```
current_count = redis.get("rate:user:123")
if current_count > limit:
    return 429 Too Many Requests
else:
    redis.incr("rate:user:123")
    redis.expire("rate:user:123", window_seconds)
```

#### 4. CORS Handler
```yaml
Access-Control-Allow-Origin: https://app.example.com
Access-Control-Allow-Methods: GET, POST, PUT, DELETE
Access-Control-Allow-Headers: Authorization, Content-Type
```

#### 5. Request Validation
- Validate query parameters
- Sanitize input
- Check content type

### Stage 4: Routing

**Duration**: <1ms

Route matching order:
1. **Exact match**: `/api/users/profile`
2. **Prefix match**: `/api/users/*`
3. **Pattern match**: `/api/users/:id`
4. **Regex match**: `/api/users/(\d+)`
5. **Fallback**: `/*` (if configured)

**Route Configuration Example:**
```yaml
routes:
  - path: /api/users/:id
    method: GET
    handler: getUserById
    middleware: [auth, rateLimit]
    cache:
      enabled: true
      ttl: 300
```

**Path Parameter Extraction:**
```
Request: GET /api/users/42
Params: { id: "42" }
```

### Stage 5: Cache Check

**Duration**: <5ms

**Cache Key Generation:**
```
cache_key = sha256(
    method + path + query_params + 
    headers[vary] + user_context
)
```

**Example:**
```
GET /api/users?role=admin + Accept: application/json
→ cache:5a7b8c9d2e3f4a5b6c7d8e9f0a1b2c3d
```

**Cache Decision Tree:**
```mermaid
graph TD
    A[Check Cache] --> B{Cache Enabled?}
    B -->|No| C[Skip to Handler]
    B -->|Yes| D{Cache Hit?}
    D -->|Yes| E{Cache Valid?}
    D -->|No| C
    E -->|Yes| F[Return Cached Response]
    E -->|No| C
```

### Stage 6: Handler Execution

**Duration**: 5-500ms (highly variable)

#### Static File Handler

```mermaid
graph LR
    A[Request File] --> B{File Exists?}
    B -->|No| C[404 Not Found]
    B -->|Yes| D{If-Modified-Since?}
    D -->|Match| E[304 Not Modified]
    D -->|No Match| F{Range Request?}
    F -->|Yes| G[206 Partial Content]
    F -->|No| H[200 OK Full File]
```

**Static File Optimizations:**
- Zero-copy sendfile()
- ETag generation
- Last-Modified headers
- Gzip precompression

#### Proxy Handler

**Proxy Request Flow:**
```
1. Rewrite request URL
2. Add/modify headers (X-Forwarded-For, etc.)
3. Forward to backend
4. Stream response back to client
5. Store in cache (if cacheable)
```

**Load Balancing:**
```yaml
backends:
  - url: http://backend1:8080
    weight: 3
  - url: http://backend2:8080
    weight: 2
  - url: http://backend3:8080
    weight: 1
```

**Health Check:**
```
Every 10s:
  GET /health → 200 OK (healthy)
  GET /health → 503 Service Unavailable (unhealthy)
```

### Stage 7: Response Processing

**Duration**: 1-20ms

#### Response Middleware

1. **Add Security Headers**
```http
X-Content-Type-Options: nosniff
X-Frame-Options: SAMEORIGIN
Strict-Transport-Security: max-age=31536000
```

2. **Compression**
```
Accept-Encoding: gzip, br
→ Content-Encoding: br (Brotli preferred)
```

3. **Cache Directives**
```http
Cache-Control: public, max-age=3600
ETag: "5a7b8c9d"
Last-Modified: Wed, 04 Dec 2025 10:15:30 GMT
```

4. **Response Logging**
```
[2025-12-04 10:15:31] INFO 200 GET /api/users 245ms 1.2KB
```

### Stage 8: Response Delivery

**Duration**: 1-1000ms (network dependent)

**Chunked Transfer:**
```http
HTTP/1.1 200 OK
Transfer-Encoding: chunked

25
This is the first chunk of data
1A
This is another chunk
0
```

**Connection Management:**
```http
Connection: keep-alive
Keep-Alive: timeout=30, max=100
```

## Error Handling Flow

```mermaid
graph TD
    A[Error Occurs] --> B{Error Type?}
    B -->|Client Error 4xx| C[Log Warning]
    B -->|Server Error 5xx| D[Log Error]
    B -->|Network Error| E[Log Error + Retry]
    
    C --> F[Generate Error Response]
    D --> F
    E --> G{Retry Count < 3?}
    G -->|Yes| H[Retry Request]
    G -->|No| F
    H --> I[Exponential Backoff]
    I --> A
    
    F --> J[Send to Client]
```

**Error Response Format:**
```json
{
  "error": {
    "code": "RESOURCE_NOT_FOUND",
    "message": "User with ID 999 not found",
    "status": 404,
    "timestamp": "2025-12-04T10:15:31Z",
    "request_id": "req_5a7b8c9d2e3f"
  }
}
```

## Performance Metrics

### Typical Latency Breakdown

| Stage | Duration | % of Total |
|-------|----------|------------|
| SSL Handshake (first request) | 50ms | - |
| Request Parsing | 0.5ms | 0.2% |
| Middleware | 5ms | 2% |
| Routing | 0.1ms | 0.04% |
| Cache Check | 2ms | 0.8% |
| Handler Execution | 200ms | 80% |
| Response Processing | 10ms | 4% |
| Network Transfer | 30ms | 12% |
| **Total** | **247.6ms** | **100%** |

### Optimization Targets

- **P50 Latency**: <100ms
- **P95 Latency**: <500ms
- **P99 Latency**: <1000ms
- **Cache Hit Rate**: >80%
- **Error Rate**: <0.1%

## WebSocket Flow

```mermaid
sequenceDiagram
    participant C as Client
    participant S as Server
    
    C->>S: HTTP Upgrade Request
    S->>C: 101 Switching Protocols
    
    loop Message Exchange
        C->>S: WebSocket Frame
        S->>S: Process Message
        S->>C: WebSocket Frame
    end
    
    Note over C,S: Ping/Pong Heartbeat
    C->>S: Ping
    S->>C: Pong
    
    alt Normal Closure
        C->>S: Close Frame
        S->>C: Close Frame
    else Timeout
        S->>C: Close Frame
    end
```

## Next Steps

- [Architecture Overview](architecture.md) - System design
- [Performance Tuning](performance.md) - Optimization guide
- [Monitoring](monitoring.md) - Observability setup
- [Configuration](configuration.md) - Configuration options
