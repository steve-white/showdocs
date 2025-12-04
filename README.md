# Overview

This is a simple light weight web server implemented in C that can be used to serve documentation locally.
It serves content from a particular root folder and optionally runs a command after start-up, such as opening a browser to view the documentation.

It supports Windows, MacOS, Linux. 

Binary sizes:
- Windows ~285KB
- Linux ~22KB
- MacOS ~36KB

The `/docs` directory in this repository contains example documentation that can be served. The bunbled `index.html` includes a third-party library, [docsify](https://docsify.js.org), which renders the `README.md` markdown documentation to HTML on the fly, including mermaid diagram support.

A typical usecase would be to add the `showdocs` binary to a git repo and serve local markdown content, without the need to pre-render documentation to HTML.

See the releases section for pre-built binaries.

Example output:

![Documentation example](doc-example-1.png)

# Building

Prerequisites:
- make, gcc

Run: `make`

# Configuration

Edit `showdocs.ini` and amend as necessary.
See [CONFIG.md](CONFIG.md) for more info.

A `404.html` should exist in the same folder, which will be served if a link is not found.

# How to Run

```
./showdocs [--port 8080]
```

If no config file is present (i.e `showdocs.ini`), then it will serve content from the current folder under port 8080.

# Example

- Clone this repository
- Obtain the respective OS release from Releases and place the binary in the folder root.
- Run `./showdocs{.exe}`
- The process should load custom configuration in `./showdocs.ini` and a browser window should open `http://localhost:8088`, showing example markdown+mermaid documentation rendered in HTML.