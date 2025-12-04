# Configuration Guide

## Configuration File

The web server supports configuration through an INI file. The config file must be named the same as the executable with a `.ini` extension.

**Examples:**
- `showdocs` executable → `showdocs.ini`
- `showdocs.exe` executable → `showdocs.exe.ini` or `showdocs.ini`
- `myserver` executable → `myserver.ini`

## Configuration Options

### Port
The port number the server will listen on.

```ini
Port=8080
```

**Default:** 8080  
**Command line override:** `./showdocs --port 3000`

### RootDir
The root directory from which files will be served. Can be absolute or relative path.

```ini
# Current directory (default)
RootDir=

# Relative path
RootDir=./html

# Absolute path (Linux/macOS)
RootDir=/var/www/html

# Absolute path (Windows - use forward slashes or escaped backslashes)
RootDir=C:/web/public
# or
RootDir=C:\\web\\public
```

**Default:** Current directory (empty value)  
**Note:** Files like `index.html`, `404.html`, and `exit.html` will be served from this directory.

### ExecStart
A command to execute once the web server has successfully started listening. Useful for automatically opening a browser or running initialization scripts.

The server supports both generic and platform-specific commands. **Platform-specific commands take priority** over the generic `ExecStart` when running on the respective platform.

```ini
# Generic command (used if platform-specific is not set)
ExecStart=echo "Server is ready!"

# Windows-specific command (takes priority on Windows)
ExecStart_Win=start http://localhost:8080

# Linux-specific command (takes priority on Linux)
ExecStart_Linux=xdg-open http://localhost:8080

# macOS-specific command (takes priority on macOS)
ExecStart_MacOS=open http://localhost:8080
```

**Supported keys:**
- `ExecStart` - Generic command (fallback for all platforms)
- `ExecStart_Win` or `ExecStart_Windows` - Windows-specific
- `ExecStart_Linux` - Linux-specific
- `ExecStart_MacOS` or `ExecStart_Darwin` - macOS-specific

**Priority order:**
1. Platform-specific command (e.g., `ExecStart_Win` on Windows)
2. Generic `ExecStart` command
3. No command (if all are empty)

**Examples:**

Cross-platform config with platform-specific commands:
```ini
ExecStart_Win=start http://localhost:8080
ExecStart_Linux=xdg-open http://localhost:8080
ExecStart_MacOS=open http://localhost:8080
```

Generic command that works everywhere:
```ini
ExecStart=echo "Server started on port 8080"
```

Run a custom script:
```ini
ExecStart=./init-server.sh
```

**Default:** Empty (no command executed)  
**Platform notes:**
- Windows: Uses `CreateProcess()` API
- Linux/macOS: Uses `system()` call

## Example Configuration Files

### Minimal Configuration
```ini
[Server]
Port=8080
RootDir=
ExecStart=
```

### Development Configuration
```ini
[Server]
Port=3000
RootDir=./public
ExecStart_Win=start http://localhost:3000
ExecStart_Linux=xdg-open http://localhost:3000
ExecStart_MacOS=open http://localhost:3000
```

### Production Configuration
```ini
[Server]
Port=80
RootDir=/var/www/html
ExecStart=
### Windows Configuration
```ini
[Server]
Port=8080
RootDir=C:/websites/mysite
ExecStart_Win=start http://localhost:8080
```

### Cross-Platform Configuration
```ini
[Server]
Port=8080
RootDir=./www
# Each platform will use its appropriate command
ExecStart_Win=start http://localhost:8080
ExecStart_Linux=xdg-open http://localhost:8080
ExecStart_MacOS=open http://localhost:8080
```tDir=C:/websites/mysite
ExecStart=start http://localhost:8080
```

## Cross-Platform Compatibility

The server is fully cross-platform and handles path separators automatically:
- **Windows:** Uses backslashes (`\`) internally
- **Linux/macOS:** Uses forward slashes (`/`) internally
- **Config file:** Can use either forward slashes or escaped backslashes

## Usage

1. **With config file:**
   ```bash
   ./showdocs
   ```
   The server will automatically look for `showdocs.ini` in the same directory.

2. **Override port via command line:**
   ```bash
   ./showdocs --port 3000
   ```
   This overrides the port setting in the config file.

3. **No config file:**
   If no config file is found, the server uses default values:
   - Port: 8080
   - RootDir: Current directory
   - ExecStart: None
