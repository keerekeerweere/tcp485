# AGENTS.md

## Build, Lint, and Test Commands

### Build Commands
```bash
# Build the entire project
idf.py build

# Build a specific component
idf.py build COMPONENT=rs485_phy

# Configure and build
idf.py menuconfig
idf.py build
```

### Lint Commands
```bash
# Run static analysis
idf.py check

# Run CMake configuration check
idf.py reconfigure
```

### Test Commands
```bash
# Run all tests (if any exist)
idf.py test

# Run a specific example/test
idf.py -p /dev/ttyUSB0 flash monitor
```

### Running Individual Tests
For running individual tests or components:
```bash
# For a specific example
idf.py -p /dev/ttyUSB0 flash monitor -C "cd examples/dhcp_client && idf.py build"

# Or build and run specific example manually
cd examples/dhcp_client && idf.py build && idf.py flash monitor
```

## Code Style Guidelines

### Imports
- All header files must include proper include guards
- Use angle brackets for system headers: `#include <stdio.h>`
- Use quotes for local headers: `#include "rs485_frame.h"`
- Group includes by type: system headers, then local headers

### Formatting
- Use 4-space indents (no tabs)
- No trailing whitespace
- Blank lines between logical sections of code
- Place opening braces on the same line as control statements
- Use consistent spacing around operators
- Maximum line length: 100 characters

### Types and Naming Conventions
- Variable names: `snake_case`
- Function names: `snake_case`
- Constants: `UPPER_CASE`
- Struct/typedef names: `snake_case`
- Global variables: `g_` prefix
- Static variables: `_` prefix
- Macro names: `UPPER_CASE_WITH_UNDERSCORES`

### Error Handling
- All functions returning `esp_err_t` must validate inputs
- Use `ESP_RETURN_ON_ERROR` macro for early exit on error
- Handle all return codes appropriately
- Log meaningful error messages with `ESP_LOGE`

### Documentation Standards
- Use Doxygen-style comments for public APIs
- Document all exported functions with parameters and return values
- Include brief description of module purpose in header files
- Use consistent terminology throughout the codebase

### Code Organization
- Split logic into logical components
- Keep functions short and focused on a single responsibility
- Use descriptive variable names that indicate their purpose
- Separate declarations and definitions in appropriate header/source files
- Minimize global state and use explicit parameters where possible

### Memory Management
- All memory allocation must be freed appropriately
- Use `malloc`/`free` for dynamic memory
- Prefer stack allocation over heap when possible
- Check return values of memory allocation functions

### ESP-IDF Specific Guidelines
- Follow ESP-IDF coding standards and conventions
- Use ESP-IDF logging macros (`ESP_LOGD`, `ESP_LOGI`, etc.)
- Use ESP-IDF component structure with proper CMakeLists.txt
- Handle driver initialization properly with `ESP_ERROR_CHECK`
- Follow ESP-IDF memory management practices

### Testing Practices
- Components should have unit tests where applicable
- Integration tests should cover main functionality
- Test edge cases and error conditions
- All examples should compile and run without warnings

### Security Considerations
- Validate all inputs to prevent buffer overflows
- Use secure coding practices for embedded systems
- Avoid hardcoded sensitive values in source code
- Implement proper bounds checking in all functions

### Version Control
- Commit messages should be descriptive and follow the format: `<type>(<scope>): <subject>`
- Keep commits small and focused
- Follow the repository's branching strategy
- Include relevant documentation updates with code changes

This guide is designed to help maintain consistency across the codebase for both human developers and automated agents working with this project.