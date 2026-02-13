# Contributing to epi_sim

Thank you for your interest in contributing to epi_sim!

## Getting Started

### Prerequisites
- C++23 compatible compiler (LLVM/Clang recommended)
- xmake build system
- vcpkg for package management

### Setting Up Development Environment

1. **Clone the repository:**
   ```bash
   git clone <repository-url>
   cd epi_sim
   ```

2. **Install dependencies:**
   ```bash
   xmake config --vcpkg
   xmake
   ```

3. **Run tests:**
   ```bash
   xmake run test
   ```

## Development Workflow

### Building the Project

```bash
# Build all targets
xmake

# Build specific target
xmake build test
xmake build epi_sim

# Clean build
xmake clean
xmake
```

### Running Tests

```bash
# Run the test suite
xmake run test

# Run the main simulation
xmake run epi_sim
```

## Code Conventions

**Important:** This project uses specific coding conventions that are critical for correctness.

👉 **See [AGENTS.md](AGENTS.md) for detailed technical conventions**, including:
- PopData 1-based indexing (critical!)
- Loop patterns and common pitfalls
- Data structure invariants
- Naming conventions

All contributors (human and AI) should read `AGENTS.md` before making changes to the codebase.

## Project Structure

```
epi_sim/
├── src/              # Source files (.h and .cpp)
├── test/             # Test files
├── sample_parameters/  # Parameter files (JSON, CSV)
├── docs/             # Documentation
├── xmake.lua         # Build configuration
└── AGENTS.md         # Technical conventions (READ THIS!)
```

## Making Changes

1. **Read `AGENTS.md`** - Understand the critical conventions
2. **Make your changes** - Follow the conventions in AGENTS.md
3. **Test your changes** - Run the test suite
4. **Document if needed** - Update docs/ if adding features

## Questions?

- Check `AGENTS.md` for technical conventions
- Check `docs/` for design documentation
- Open an issue for questions or discussions

## License

[Add license information here]

