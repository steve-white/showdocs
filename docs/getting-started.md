# Getting Started

This guide will help you get ACME Web Server up and running in minutes.

## Prerequisites

Before installing ACME Web Server, ensure you have:

- **Operating System**: Linux (Ubuntu 20.04+, CentOS 8+), macOS 11+, or Windows 10+
- **Memory**: Minimum 2GB RAM (4GB+ recommended for production)
- **Disk Space**: 500MB for installation
- **Ports**: Port 80 (HTTP) and 443 (HTTPS) available

## Installation

### Linux/macOS

```bash
# Download and install
curl -fsSL https://get.acme-server.io | sh

# Verify installation
acme-server --version
```

### Windows

```powershell
# Using PowerShell
iwr -useb https://get.acme-server.io/install.ps1 | iex

# Verify installation
acme-server --version
```

### Docker

```bash
# Pull the official image
docker pull acme/web-server:latest

# Run container
docker run -d -p 80:80 -p 443:443 \
  --name acme-server \
  acme/web-server:latest
```

## First Steps

### 1. Create Configuration File

Create a file named `acme.yaml`:

```yaml
server:
  host: 0.0.0.0
  port: 8080
  
ssl:
  enabled: false
  
logging:
  level: info
  format: json
  
routes:
  - path: /
    root: ./public
    index: index.html
```

### 2. Create Content Directory

```bash
mkdir public
echo "<h1>Welcome to ACME Web Server!</h1>" > public/index.html
```

### 3. Start the Server

```bash
acme-server start --config acme.yaml
```

### 4. Test Your Server

Open your browser and navigate to `http://localhost:8080`

Or use curl:

```bash
curl http://localhost:8080
```

## Basic Commands

```bash
# Start server
acme-server start --config acme.yaml

# Start in background (daemon mode)
acme-server start --config acme.yaml --daemon

# Stop server
acme-server stop

# Restart server
acme-server restart

# Check status
acme-server status

# View logs
acme-server logs --follow

# Validate configuration
acme-server validate --config acme.yaml
```

## Next Steps

- [Configuration Guide](configuration.md) - Learn about advanced configuration options
- [Architecture Overview](architecture.md) - Understand how ACME Web Server works
- [Deployment Guide](deployment.md) - Deploy to production environments
- [API Reference](api-reference.md) - Explore the REST API

## Troubleshooting

### Port Already in Use

If you see "port already in use" error:

```bash
# Check what's using the port
lsof -i :8080  # Linux/macOS
netstat -ano | findstr :8080  # Windows

# Change port in acme.yaml
server:
  port: 8081
```

### Permission Denied

To use ports 80/443, run with appropriate privileges:

```bash
# Linux
sudo acme-server start --config acme.yaml

# Or use capabilities (Linux)
sudo setcap 'cap_net_bind_service=+ep' /usr/local/bin/acme-server
```

## Getting Help

- 📖 [Full Documentation](/)
- 💬 [Community Forum](https://forum.acme-server.io)
- 🐛 [Report Issues](https://github.com/acme/web-server/issues)
