# Configuration Reference

ACME Web Server uses YAML configuration files for flexible and readable configuration management.

## Configuration File Structure

```yaml
# acme.yaml - Main configuration file

server:
  host: 0.0.0.0
  port: 8080
  timeout: 30s
  max_connections: 10000

ssl:
  enabled: true
  cert: /path/to/cert.pem
  key: /path/to/key.pem
  protocols: [TLSv1.2, TLSv1.3]

logging:
  level: info
  format: json
  output: stdout
  
cache:
  enabled: true
  size: 1GB
  ttl: 3600

routes:
  - path: /
    root: ./public
    index: index.html
    
  - path: /api/*
    proxy: http://backend:8080
    
middleware:
  - type: cors
    enabled: true
  - type: rateLimit
    enabled: true
    limit: 100
    window: 60s
```

## Server Configuration

### Basic Settings

```yaml
server:
  # Bind address (0.0.0.0 for all interfaces)
  host: 0.0.0.0
  
  # Port to listen on
  port: 8080
  
  # Request timeout
  timeout: 30s
  
  # Maximum concurrent connections
  max_connections: 10000
  
  # Maximum request body size
  max_body_size: 10MB
  
  # Maximum header size
  max_header_size: 8KB
  
  # Keep-alive timeout
  keep_alive_timeout: 120s
  
  # Graceful shutdown timeout
  shutdown_timeout: 30s
```

### Worker Configuration

```yaml
server:
  # Number of worker threads (0 = auto-detect CPU cores)
  workers: 0
  
  # Worker thread stack size
  worker_stack_size: 2MB
  
  # I/O thread pool size
  io_threads: 4
```

## SSL/TLS Configuration

```yaml
ssl:
  # Enable SSL/TLS
  enabled: true
  
  # Certificate file path (PEM format)
  cert: /etc/acme/ssl/cert.pem
  
  # Private key file path (PEM format)
  key: /etc/acme/ssl/key.pem
  
  # Certificate chain (optional)
  chain: /etc/acme/ssl/chain.pem
  
  # Supported protocols
  protocols:
    - TLSv1.2
    - TLSv1.3
  
  # Cipher suites (TLS 1.2)
  ciphers:
    - ECDHE-ECDSA-AES128-GCM-SHA256
    - ECDHE-RSA-AES128-GCM-SHA256
    - ECDHE-ECDSA-AES256-GCM-SHA384
  
  # TLS 1.3 cipher suites
  ciphersuites:
    - TLS_AES_128_GCM_SHA256
    - TLS_AES_256_GCM_SHA384
  
  # Enable OCSP stapling
  ocsp_stapling: true
  
  # Session cache size
  session_cache_size: 10MB
  
  # Session timeout
  session_timeout: 5m
  
  # Enable HTTP/2
  http2: true
  
  # Redirect HTTP to HTTPS
  redirect_http: true
```

### SNI (Server Name Indication)

```yaml
ssl:
  enabled: true
  
  # Default certificate
  cert: /etc/acme/ssl/default.pem
  key: /etc/acme/ssl/default-key.pem
  
  # SNI certificates
  sni:
    - host: example.com
      cert: /etc/acme/ssl/example.com.pem
      key: /etc/acme/ssl/example.com-key.pem
      
    - host: api.example.com
      cert: /etc/acme/ssl/api.example.com.pem
      key: /etc/acme/ssl/api.example.com-key.pem
```

## Logging Configuration

```yaml
logging:
  # Log level: debug, info, warn, error, fatal
  level: info
  
  # Log format: json, text, logfmt
  format: json
  
  # Output: stdout, stderr, file path
  output: /var/log/acme-server.log
  
  # Log rotation
  rotation:
    enabled: true
    max_size: 100MB
    max_age: 30d
    max_backups: 10
    compress: true
  
  # Access log
  access_log:
    enabled: true
    output: /var/log/acme-access.log
    format: combined  # combined, common, or custom
    
  # Request/response logging
  include_request_body: false
  include_response_body: false
  
  # Sensitive data masking
  mask_headers:
    - Authorization
    - Cookie
    - X-API-Key
```

## Cache Configuration

```yaml
cache:
  # Enable caching
  enabled: true
  
  # In-memory cache size
  size: 1GB
  
  # Default TTL
  ttl: 3600
  
  # Cache key includes
  vary:
    - Accept
    - Accept-Encoding
    - Authorization
  
  # External cache (Redis)
  redis:
    enabled: false
    host: localhost
    port: 6379
    db: 0
    password: ""
    pool_size: 10
  
  # Cache rules
  rules:
    - path: /api/*
      ttl: 300
      vary: [Accept, Authorization]
      
    - path: /static/*
      ttl: 86400  # 24 hours
      
    - path: /no-cache/*
      enabled: false
```

## Routes Configuration

### Static File Serving

```yaml
routes:
  - path: /
    type: static
    root: ./public
    index: index.html
    
    # Directory listing
    directory_listing: false
    
    # Default files to try
    try_files:
      - index.html
      - index.htm
    
    # File extensions to serve
    extensions:
      - .html
      - .css
      - .js
      - .png
      - .jpg
    
    # Cache headers
    cache_control: public, max-age=3600
    
    # Compression
    compression:
      enabled: true
      min_size: 1KB
      types:
        - text/html
        - text/css
        - application/javascript
```

### Reverse Proxy

```yaml
routes:
  - path: /api/*
    type: proxy
    
    # Single backend
    backend: http://backend:8080
    
    # Or load-balanced backends
    backends:
      - url: http://backend1:8080
        weight: 3
      - url: http://backend2:8080
        weight: 2
      - url: http://backend3:8080
        weight: 1
    
    # Load balancing algorithm
    load_balancing: round_robin  # round_robin, least_conn, ip_hash
    
    # Health check
    health_check:
      enabled: true
      path: /health
      interval: 10s
      timeout: 5s
      healthy_threshold: 2
      unhealthy_threshold: 3
    
    # Proxy settings
    proxy_timeout: 30s
    proxy_connect_timeout: 5s
    proxy_read_timeout: 60s
    proxy_write_timeout: 60s
    
    # Headers to add/modify
    headers:
      add:
        X-Forwarded-For: $remote_addr
        X-Real-IP: $remote_addr
        X-Forwarded-Proto: $scheme
      remove:
        - X-Powered-By
    
    # Path rewriting
    rewrite:
      from: ^/api/(.*)$
      to: /$1
```

### WebSocket

```yaml
routes:
  - path: /ws
    type: websocket
    backend: ws://backend:8080
    
    # WebSocket settings
    ping_interval: 30s
    pong_timeout: 10s
    max_message_size: 1MB
    
    # Compression
    compression:
      enabled: true
      level: 6
```

### Redirect

```yaml
routes:
  - path: /old-path
    type: redirect
    redirect_to: /new-path
    status_code: 301  # 301 (permanent) or 302 (temporary)
```

## Middleware Configuration

### CORS

```yaml
middleware:
  - type: cors
    enabled: true
    
    # Allowed origins
    origins:
      - https://app.example.com
      - https://www.example.com
    
    # Or allow all
    allow_all_origins: false
    
    # Allowed methods
    methods:
      - GET
      - POST
      - PUT
      - DELETE
      - OPTIONS
    
    # Allowed headers
    headers:
      - Content-Type
      - Authorization
      - X-Request-ID
    
    # Expose headers
    expose_headers:
      - X-Request-ID
      - X-RateLimit-Remaining
    
    # Credentials
    credentials: true
    
    # Max age
    max_age: 86400
```

### Rate Limiting

```yaml
middleware:
  - type: rateLimit
    enabled: true
    
    # Global limit
    limit: 100
    window: 60s
    
    # Per-route limits
    routes:
      - path: /api/auth/*
        limit: 5
        window: 60s
        
      - path: /api/search
        limit: 10
        window: 60s
    
    # Key extraction
    key_by: ip  # ip, header, cookie
    
    # Or custom header
    # key_by: header
    # key_header: X-API-Key
    
    # Storage
    storage: memory  # memory or redis
    
    # Response headers
    headers:
      enabled: true
      limit_header: X-RateLimit-Limit
      remaining_header: X-RateLimit-Remaining
      reset_header: X-RateLimit-Reset
```

### Authentication

```yaml
middleware:
  - type: auth
    enabled: true
    
    # JWT authentication
    jwt:
      enabled: true
      secret: your-secret-key
      algorithm: HS256
      
      # Token location
      token_lookup:
        - header:Authorization
        - cookie:jwt
      
      # Claims validation
      required_claims:
        - sub
        - exp
      
      # Issuer validation
      issuer: https://auth.example.com
      audience: api.example.com
    
    # Basic authentication
    basic:
      enabled: false
      realm: "Restricted"
      users:
        admin: $2y$10$...  # bcrypt hash
        user: $2y$10$...
    
    # API key authentication
    api_key:
      enabled: false
      header: X-API-Key
      keys:
        - key: abc123...
          name: frontend-app
        - key: def456...
          name: mobile-app
```

### Request/Response Modification

```yaml
middleware:
  - type: headers
    enabled: true
    
    # Add headers to requests
    request:
      add:
        X-Request-ID: $request_id
        X-Client-IP: $remote_addr
      remove:
        - X-Debug
    
    # Add headers to responses
    response:
      add:
        X-Content-Type-Options: nosniff
        X-Frame-Options: SAMEORIGIN
        X-XSS-Protection: 1; mode=block
        Strict-Transport-Security: max-age=31536000
        Content-Security-Policy: default-src 'self'
      remove:
        - Server
        - X-Powered-By
```

## Monitoring Configuration

```yaml
monitoring:
  # Metrics endpoint
  metrics:
    enabled: true
    path: /metrics
    format: prometheus  # prometheus or json
  
  # Health check endpoint
  health:
    enabled: true
    path: /health
    
    # Health checks
    checks:
      - name: disk_space
        type: disk
        threshold: 90%
        
      - name: memory
        type: memory
        threshold: 85%
        
      - name: backend_api
        type: http
        url: http://backend:8080/health
        timeout: 5s
  
  # Tracing
  tracing:
    enabled: true
    provider: opentelemetry
    endpoint: http://jaeger:14268/api/traces
    sample_rate: 0.1  # 10% sampling
```

## Environment Variables

Configuration values can be overridden with environment variables:

```bash
# Server settings
export ACME_SERVER_HOST=0.0.0.0
export ACME_SERVER_PORT=8080

# SSL settings
export ACME_SSL_ENABLED=true
export ACME_SSL_CERT=/path/to/cert.pem
export ACME_SSL_KEY=/path/to/key.pem

# Logging
export ACME_LOG_LEVEL=info
export ACME_LOG_FORMAT=json

# Cache
export ACME_CACHE_ENABLED=true
export ACME_CACHE_SIZE=1GB
```

## Configuration Validation

Validate your configuration before starting:

```bash
acme-server validate --config acme.yaml
```

Example output:
```
✓ Configuration syntax is valid
✓ SSL certificates are valid
✓ All referenced files exist
✓ Port 8080 is available
✓ No conflicts detected

Configuration is valid and ready to use.
```

## Next Steps

- [Getting Started](getting-started.md) - Installation guide
- [Deployment](deployment.md) - Production deployment
- [Performance](performance.md) - Performance tuning
- [Monitoring](monitoring.md) - Observability setup
