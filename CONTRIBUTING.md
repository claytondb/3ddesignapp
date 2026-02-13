# Contributing to dc-3ddesignapp

Thank you for your interest in contributing to dc-3ddesignapp! This document provides guidelines and information for contributors.

---

## 📋 Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Project Structure](#project-structure)
- [Coding Standards](#coding-standards)
- [Git Workflow](#git-workflow)
- [Pull Request Process](#pull-request-process)
- [Issue Guidelines](#issue-guidelines)
- [Testing](#testing)
- [Documentation](#documentation)

---

## 📜 Code of Conduct

By participating in this project, you agree to maintain a respectful and inclusive environment. We expect all contributors to:

- Be respectful and considerate in all interactions
- Welcome newcomers and help them get started
- Focus on constructive feedback
- Accept responsibility for mistakes and learn from them

---

## 🚀 Getting Started

### Prerequisites

Before contributing, ensure you have:

1. Read the [README.md](README.md) for project overview
2. Reviewed the [Architecture Guide](docs/ARCHITECTURE.md)
3. Set up your development environment (see below)
4. Familiarized yourself with the codebase

### First Contribution Ideas

Look for issues labeled:
- `good-first-issue` — Simple fixes for newcomers
- `help-wanted` — Areas where we need community help
- `documentation` — Documentation improvements

---

## 💻 Development Setup

### Required Tools

| Tool | Version | Purpose |
|------|---------|---------|
| CMake | 3.21+ | Build system |
| Qt | 6.5+ LTS | UI framework |
| Open CASCADE | 7.8+ | Geometry kernel |
| C++ Compiler | C++17 | MSVC 2022 / Clang 15+ / GCC 11+ |
| Git | 2.30+ | Version control |
| Ninja | 1.10+ | Build tool (recommended) |

### Setup Steps

```bash
# 1. Fork the repository on GitHub

# 2. Clone your fork
git clone https://github.com/YOUR_USERNAME/dc-3ddesignapp.git
cd dc-3ddesignapp

# 3. Add upstream remote
git remote add upstream https://github.com/claytondb/dc-3ddesignapp.git

# 4. Create a development build
cmake --preset debug
cmake --build build/debug --parallel

# 5. Run tests
ctest --test-dir build/debug --output-on-failure

# 6. Create a feature branch
git checkout -b feature/your-feature-name
```

### IDE Setup

**Visual Studio Code** (Recommended):
```json
// .vscode/settings.json
{
  "cmake.configureOnOpen": true,
  "cmake.buildDirectory": "${workspaceFolder}/build/debug",
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools"
}
```

**Qt Creator**:
- Open `CMakeLists.txt` as project
- Configure with Qt 6 kit

**Visual Studio 2022**:
- File → Open → CMake → Select `CMakeLists.txt`
- Select appropriate preset

---

## 📁 Project Structure

```
dc-3ddesignapp/
├── src/                    # Source code
│   ├── app/               # Application entry point
│   ├── core/              # Core systems (commands, scene)
│   ├── geometry/          # Geometry (mesh, solid, surfaces)
│   ├── io/                # Import/export handlers
│   ├── renderer/          # OpenGL rendering
│   ├── sketch/            # 2D sketching system
│   └── ui/                # Qt UI components
├── include/               # Public headers
├── tests/                 # Unit tests
├── docs/                  # Documentation
├── cmake/                 # CMake modules
└── resources/             # Assets (icons, shaders)
```

### Module Responsibilities

| Module | Purpose |
|--------|---------|
| `app/` | Application bootstrap, main window |
| `core/` | Command pattern, undo/redo, scene management |
| `geometry/` | Mesh, solid, NURBS surfaces, primitives |
| `io/` | File format readers/writers |
| `renderer/` | OpenGL viewport, shaders, rendering |
| `sketch/` | 2D sketching, constraints, solver |
| `ui/` | Dialogs, panels, toolbars |

---

## 📝 Coding Standards

### C++ Style Guide

We follow a consistent C++ style. Key points:

**Naming Conventions:**
```cpp
// Classes: PascalCase
class MeshProcessor { };

// Functions/Methods: camelCase
void processVertices();

// Variables: camelCase
int vertexCount;

// Member variables: m_ prefix
int m_vertexCount;

// Constants: UPPER_SNAKE_CASE
constexpr int MAX_VERTICES = 1000000;

// Namespaces: lowercase
namespace geometry { }
```

**File Naming:**
- Headers: `ClassName.h`
- Sources: `ClassName.cpp`
- One class per file (with exceptions for small related classes)

**Code Formatting:**
```cpp
// Braces on same line for control structures
if (condition) {
    doSomething();
} else {
    doOther();
}

// Braces on new line for classes/functions
class MyClass
{
public:
    void myFunction()
    {
        // implementation
    }
};

// Use nullptr, not NULL
Mesh* mesh = nullptr;

// Use auto judiciously
auto vertices = mesh->getVertices();  // OK when type is obvious
int count = mesh->vertexCount();      // Prefer explicit for primitives
```

**Modern C++ Practices:**
- Use `std::unique_ptr` and `std::shared_ptr` for ownership
- Prefer `const` and `constexpr` where applicable
- Use range-based for loops
- Use `std::optional` for optional values
- Use `[[nodiscard]]` for functions that return important values

**Qt-Specific:**
- Use Qt's parent-child ownership model
- Connect signals/slots with modern syntax
- Use `QStringLiteral()` for string literals

### Documentation

```cpp
/**
 * @brief Brief description of the class/function.
 *
 * Detailed description if needed.
 *
 * @param mesh The input mesh to process
 * @param tolerance Processing tolerance (default: 0.001)
 * @return Processed mesh or nullptr on failure
 *
 * @note Any important notes
 * @see RelatedClass
 */
[[nodiscard]] Mesh* processMesh(const Mesh* mesh, double tolerance = 0.001);
```

For complete style guidelines, see [docs/STYLE_GUIDE.md](docs/STYLE_GUIDE.md).

---

## 🔄 Git Workflow

### Branch Naming

```
feature/description    # New features
bugfix/description     # Bug fixes
hotfix/description     # Urgent production fixes
refactor/description   # Code refactoring
docs/description       # Documentation updates
test/description       # Test additions/fixes
```

### Commit Messages

Follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

**Types:**
- `feat` — New feature
- `fix` — Bug fix
- `docs` — Documentation
- `style` — Formatting (no code change)
- `refactor` — Code restructuring
- `test` — Adding tests
- `chore` — Maintenance tasks

**Examples:**
```
feat(mesh): add bilateral smoothing algorithm

fix(io): handle empty faces in OBJ import

docs(readme): update build instructions for macOS

refactor(core): simplify command merging logic
```

### Keeping Your Fork Updated

```bash
# Fetch upstream changes
git fetch upstream

# Rebase your branch onto upstream/main
git checkout main
git rebase upstream/main

# Update your feature branch
git checkout feature/your-feature
git rebase main
```

---

## 📬 Pull Request Process

### Before Submitting

1. **Update your branch** with the latest `main`
2. **Run all tests** and ensure they pass
3. **Add tests** for new functionality
4. **Update documentation** if needed
5. **Self-review** your changes

### PR Checklist

- [ ] Code follows the style guide
- [ ] Self-review completed
- [ ] Tests added/updated
- [ ] Documentation updated
- [ ] Commit messages follow conventions
- [ ] No merge conflicts

### PR Template

```markdown
## Description
Brief description of changes.

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
Describe how you tested the changes.

## Screenshots
If applicable, add screenshots.

## Checklist
- [ ] Code follows style guide
- [ ] Tests pass locally
- [ ] Documentation updated
```

### Review Process

1. Create PR against `main` branch
2. Automated checks run (build, tests, lint)
3. At least one maintainer review required
4. Address review feedback
5. Maintainer merges when approved

---

## 🐛 Issue Guidelines

### Bug Reports

Include:
- **Summary** — Clear, concise description
- **Steps to Reproduce** — Minimal steps to trigger
- **Expected Behavior** — What should happen
- **Actual Behavior** — What actually happens
- **Environment** — OS, Qt version, compiler
- **Screenshots/Logs** — If applicable

### Feature Requests

Include:
- **Problem Statement** — What problem does this solve?
- **Proposed Solution** — Your suggested approach
- **Alternatives** — Other solutions considered
- **Additional Context** — Mockups, examples, etc.

---

## 🧪 Testing

### Running Tests

```bash
# Build with tests
cmake --preset debug
cmake --build build/debug --parallel

# Run all tests
ctest --test-dir build/debug --output-on-failure

# Run specific test
ctest --test-dir build/debug -R "MeshTest"

# Run with verbose output
ctest --test-dir build/debug -V
```

### Writing Tests

We use [Catch2](https://github.com/catchorg/Catch2) for testing:

```cpp
#include <catch2/catch_test_macros.hpp>
#include "geometry/Mesh.h"

TEST_CASE("Mesh vertex operations", "[mesh]")
{
    Mesh mesh;
    
    SECTION("adding vertices")
    {
        mesh.addVertex({0, 0, 0});
        mesh.addVertex({1, 0, 0});
        REQUIRE(mesh.vertexCount() == 2);
    }
    
    SECTION("removing vertices")
    {
        mesh.addVertex({0, 0, 0});
        mesh.removeVertex(0);
        REQUIRE(mesh.vertexCount() == 0);
    }
}
```

### Test Coverage Goals

- Core geometry: 80%+
- Import/Export: 90%+
- UI components: 50%+
- Integration tests for workflows

---

## 📖 Documentation

### Types of Documentation

1. **Code Comments** — Explain *why*, not *what*
2. **API Documentation** — Doxygen-style in headers
3. **Guides** — High-level docs in `docs/`
4. **README** — Project overview and quick start

### Building Documentation

```bash
# Generate Doxygen docs
doxygen docs/Doxyfile

# Open in browser
open docs/html/index.html
```

---

## 🙋 Getting Help

- **GitHub Issues** — Bug reports and feature requests
- **GitHub Discussions** — Questions and ideas
- **Code Review** — PR feedback and mentoring

---

## 🎉 Recognition

Contributors are recognized in:
- Release notes for significant contributions
- README acknowledgments section
- Git history (of course!)

Thank you for contributing to dc-3ddesignapp! 🚀
