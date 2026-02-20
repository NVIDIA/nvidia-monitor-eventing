# Device ID Pattern Language - User Guide

## Table of Contents
- [Introduction](#introduction)
- [Purpose and Scope](#purpose-and-scope)
- [Architecture Overview](#architecture-overview)
- [Pattern Syntax](#pattern-syntax)
- [Use Cases and Examples](#use-cases-and-examples)
- [API Reference](#api-reference)
- [Best Practices](#best-practices)
- [Troubleshooting](#troubleshooting)

---

## Introduction

The Device ID Pattern Language is a powerful and flexible system for expressing mappings between device identifiers and their string representations. It allows you to define patterns that can expand into multiple device IDs using bracket notation with various mapping types.

### Key Features
- **Compact Representation**: Express multiple device IDs with a single pattern
- **Flexible Mappings**: Support for identity, shifted, many-to-one, one-to-many, and comma-separated mappings
- **Multi-dimensional**: Handle complex device hierarchies with multiple bracket levels
- **Bidirectional**: Match device strings back to their pattern indexes

---

## Purpose and Scope

### Purpose
The device_id module provides a domain-specific language for:
1. **Device Naming**: Generate consistent device names across large systems
2. **Index Mapping**: Map logical device indexes to physical device numbers
3. **Pattern Matching**: Reverse lookup from device strings to their indexes
4. **Configuration**: Simplify device configuration in monitoring and eventing systems

### Scope
- **In Scope**: Device identifier patterns, index transformations, string generation
- **Out of Scope**: Device discovery, hardware communication, state management

### Use in nvidia-monitor-eventing
Device ID patterns are used throughout the eventing system to:
- Define device paths in configuration files
- Map events to specific devices
- Generate D-Bus object paths
- Associate sensors with their parent devices

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                     Device ID Pattern System                     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────────┐
        │         Pattern Parsing Layer           │
        │  ┌───────────────────────────────────┐  │
        │  │  DeviceIdPattern                  │  │
        │  │  - Parse pattern string           │  │
        │  │  - Extract brackets & text        │  │
        │  │  - Build internal representation  │  │
        │  └───────────────────────────────────┘  │
        └─────────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────────┐
        │      Bracket Mapping Layer              │
        │  ┌───────────────────────────────────┐  │
        │  │  IndexedBracketMap                │  │
        │  │  - Input position tracking        │  │
        │  │  - Bracket-to-input mapping       │  │
        │  └───────────────────────────────────┘  │
        │  ┌───────────────────────────────────┐  │
        │  │  BracketRangeMap                  │  │
        │  │  - Range-to-range mapping         │  │
        │  │  - 1-to-1 and many-to-1 support   │  │
        │  └───────────────────────────────────┘  │
        │  ┌───────────────────────────────────┐  │
        │  │  BracketRange                     │  │
        │  │  - Simple range representation    │  │
        │  └───────────────────────────────────┘  │
        └─────────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────────┐
        │        Evaluation & Matching            │
        │  ┌───────────────────────────────────┐  │
        │  │  eval(PatternIndex)               │  │
        │  │  → Generate device string         │  │
        │  └───────────────────────────────────┘  │
        │  ┌───────────────────────────────────┐  │
        │  │  match(string)                    │  │
        │  │  → Find matching indexes          │  │
        │  └───────────────────────────────────┘  │
        │  ┌───────────────────────────────────┐  │
        │  │  domain()                         │  │
        │  │  → List all valid indexes         │  │
        │  └───────────────────────────────────┘  │
        └─────────────────────────────────────────┘
```

---

## Pattern Syntax

### Basic Syntax Elements

#### 1. **Plain Text**
Text outside brackets is preserved as-is.
```
Example: "GPU_SXM"
Output:  "GPU_SXM"
```

#### 2. **Simple Range** `[start-end]`
Generates sequential numbers from start to end (inclusive).
```
Pattern: "GPU_[0-3]"
Domain:  0, 1, 2, 3
Values:  "GPU_0", "GPU_1", "GPU_2", "GPU_3"
```

#### 3. **Single Value** `[N]`
Represents a single specific value.
```
Pattern: "FPGA_[5]"
Domain:  5
Values:  "FPGA_5"
```

#### 4. **Range Mapping** `[from:to]`
Maps input range to output range (1-to-1 mapping).
```
Pattern: "PCIeRetimer_[1-8:0-7]"
Domain:  1, 2, 3, 4, 5, 6, 7, 8
Values:  "PCIeRetimer_0", "PCIeRetimer_1", ..., "PCIeRetimer_7"
Mapping: input 1→output 0, input 2→output 1, ..., input 8→output 7
```

#### 5. **Many-to-One Mapping** `[range:single]`
Maps multiple inputs to a single output.
```
Pattern: "HSC_[0-3:5]"
Domain:  0, 1, 2, 3
Values:  "HSC_5", "HSC_5", "HSC_5", "HSC_5"
Mapping: inputs 0,1,2,3 all map to output 5
```

#### 6. **One-to-Many Mapping** `[single:range]`
Maps a single input to multiple outputs.
```
Pattern: "GPU_[0:0-1,1:2-3]"
Domain:  0, 1
Values:  "GPU_0", "GPU_1", "GPU_2", "GPU_3"
Mapping: input 0 maps to outputs 0-1; input 1 maps to outputs 2-3
Note: This creates multiple device strings from a single input index
```

#### 7. **Comma-Separated Series** `[range1:map1,range2:map2,...]`
Combines multiple mappings in one bracket.
```
Pattern: "Device_[0-1:0,2-3:1]"
Domain:  0, 1, 2, 3
Values:  "Device_0", "Device_0", "Device_1", "Device_1"
Mapping: 0,1→0; 2,3→1
```

#### 8. **Explicit Input Position** `[pos|mapping]`
Specifies which input dimension this bracket uses.
```
Pattern: "FPGA_[0|0-1:0,2-3:2][0|0-1:1,2-3:3]"
Domain:  0, 1, 2, 3 (single dimension)
Values:  "FPGA_01", "FPGA_01", "FPGA_23", "FPGA_23"
Both brackets read from input position 0
```

#### 9. **Multi-Dimensional Patterns**
Multiple brackets create multi-dimensional indexes.
```
Pattern: "NVSwitch_[0-3]/Port_[0-15]"
Domain:  (0,0), (0,1), ..., (3,15) - 64 combinations
Values:  "NVSwitch_0/Port_0", "NVSwitch_0/Port_1", ...
```

---

## Use Cases and Examples

### Use Case 1: Simple Device Enumeration

**Scenario**: You have 8 GPUs numbered 1-8 and need to generate their names.

**API Usage**:
```cpp
#include "device_id.hpp"
using namespace device_id;

// Create pattern
DeviceIdPattern pattern("GPU_SXM_[1-8]");

// Get all valid input indexes
auto domain = pattern.domainVec();
// Result: [PatternIndex(1), PatternIndex(2), ..., PatternIndex(8)]

// Generate device names
for (const auto& index : domain) {
    std::string deviceName = pattern.eval(index);
    std::cout << deviceName << std::endl;
}
```

**Output**:
```
GPU_SXM_1
GPU_SXM_2
GPU_SXM_3
GPU_SXM_4
GPU_SXM_5
GPU_SXM_6
GPU_SXM_7
GPU_SXM_8
```

**Input/Output**:
- **Input**: PatternIndex(1) through PatternIndex(8)
- **Output**: Strings "GPU_SXM_1" through "GPU_SXM_8"
- **Dimension**: 1 (single bracket)

---

### Use Case 2: Zero-Based to One-Based Index Conversion

**Scenario**: Your hardware uses 0-based indexing but you want 1-based naming.

**API Usage**:
```cpp
DeviceIdPattern pattern("PCIeRetimer_[0-7:1-8]");

// Evaluate with hardware index (0-based)
std::string name0 = pattern.eval(PatternIndex(0));  // "PCIeRetimer_1"
std::string name7 = pattern.eval(PatternIndex(7));  // "PCIeRetimer_8"

// Reverse lookup: find hardware index from name
auto indexes = pattern.match("PCIeRetimer_5");
// Result: [PatternIndex(4)]
```

**Input/Output**:
- **Input**: Hardware index 0-7
- **Output**: User-facing names 1-8
- **Mapping**: Shifted by +1

**Example Table**:
| Hardware Index | Pattern Input | Device Name |
|----------------|---------------|-------------|
| 0 | 0 | PCIeRetimer_1 |
| 1 | 1 | PCIeRetimer_2 |
| 4 | 4 | PCIeRetimer_5 |
| 7 | 7 | PCIeRetimer_8 |

---

### Use Case 3: Many-to-One Mapping (Shared Resources)

**Scenario**: Multiple NVSwitches share the same Hot Swap Controller.
- NVSwitch 0,1 → HSC 8
- NVSwitch 2,3 → HSC 9

**API Usage**:
```cpp
DeviceIdPattern pattern("HSC_[0-1:8,2-3:9]");

// Generate HSC names for each switch
std::string hsc0 = pattern.eval(PatternIndex(0));  // "HSC_8"
std::string hsc1 = pattern.eval(PatternIndex(1));  // "HSC_8"
std::string hsc2 = pattern.eval(PatternIndex(2));  // "HSC_9"
std::string hsc3 = pattern.eval(PatternIndex(3));  // "HSC_9"

// Find all switches using HSC_8
auto switches = pattern.match("HSC_8");
// Result: [PatternIndex(0), PatternIndex(1)]

// Check if mapping is injective (one-to-one)
bool isOneToOne = pattern.isInjective();  // false
```

**Input/Output**:
- **Input**: Switch index 0-3
- **Output**: HSC identifier (8 or 9)
- **Type**: Non-injective (many-to-one)

**Visualization**:
```
Switch 0 ─┐
          ├─→ HSC_8
Switch 1 ─┘

Switch 2 ─┐
          ├─→ HSC_9
Switch 3 ─┘
```

---

### Use Case 4: One-to-Many Mapping (SMA to Device Mapping)

**Scenario**: A System Management Agent (SMA) interface maps to multiple device instances. This pattern is commonly used when one SMA interface manages multiple devices.
- GPU_SMA_0 → GPU_0, GPU_1
- GPU_SMA_1 → GPU_2, GPU_3
- CX_SMA_0 → CX_0, CX_1
- CX_SMA_1 → CX_2, CX_3

**API Usage**:
```cpp
// Example 1: GPU_SMA to GPU mapping
DeviceIdPattern gpuPattern("GPU_[0:0-1,1:2-3]");

// Example 2: CX_SMA to CX mapping
DeviceIdPattern cxPattern("CX_[0:0-1,1:2-3]");

// Pattern has 1 dimension (SMA index)
unsigned dims = gpuPattern.dim();  // 1

// Get domain for SMA dimension
auto smaDomain = gpuPattern.dimDomain(0);  // {0, 1}

// Generate all device names
auto allGpuDevices = gpuPattern.valuesVec();
// Result: ["GPU_0", "GPU_1", "GPU_2", "GPU_3"]

auto allCxDevices = cxPattern.valuesVec();
// Result: ["CX_0", "CX_1", "CX_2", "CX_3"]

// Evaluate specific SMA index
std::string device0 = gpuPattern.eval(PatternIndex(0));  // "GPU_0" or "GPU_1" (one of the outputs)
std::string device1 = gpuPattern.eval(PatternIndex(1));  // "GPU_2" or "GPU_3" (one of the outputs)

// Note: Since this is one-to-many, eval() may return one of multiple possible values
// Use valuesVec() to get all possible outputs

// Match a device name back to SMA index
auto indexes = gpuPattern.match("GPU_0");
// Result: [PatternIndex(0)]  // GPU_SMA_0 maps to GPU_0

auto indexes2 = gpuPattern.match("GPU_2");
// Result: [PatternIndex(1)]  // GPU_SMA_1 maps to GPU_2
```

**Input/Output**:
- **Input**: SMA index 0-1
- **Output**: Multiple device identifiers (devices 0-1 for SMA_0; devices 2-3 for SMA_1)
- **Type**: Non-surjective (one-to-many)

**Visualization**:
```
SMA_0 ──→ Device_0
      └─→ Device_1

SMA_1 ──→ Device_2
      └─→ Device_3
```

---

### Use Case 5: Multi-Dimensional Device Hierarchy

**Scenario**: Each NVSwitch has multiple ports.
- 4 NVSwitches (0-3)
- 16 ports per switch (0-15)

**API Usage**:
```cpp
DeviceIdPattern pattern("NVSwitch_[0-3]/Port_[0-15]");

// Pattern has 2 dimensions
unsigned dims = pattern.dim();  // 2

// Get domain for each dimension
auto switchDomain = pattern.dimDomain(0);  // {0, 1, 2, 3}
auto portDomain = pattern.dimDomain(1);    // {0, 1, ..., 15}

// Total combinations
unsigned total = pattern.domain().size();  // 64 (4 * 16)

// Evaluate specific switch/port combination
std::string path = pattern.eval(PatternIndex(2, 10));
// Result: "NVSwitch_2/Port_10"

// Match a path back to indexes
auto indexes = pattern.match("NVSwitch_3/Port_7");
// Result: [PatternIndex(3, 7)]
```

**Input/Output**:
- **Input**: 2D index (switch, port)
- **Output**: Hierarchical path string
- **Total combinations**: 64

**Example Evaluations**:
| Input Index | Output String |
|-------------|---------------|
| (0, 0) | NVSwitch_0/Port_0 |
| (0, 15) | NVSwitch_0/Port_15 |
| (3, 7) | NVSwitch_3/Port_7 |
| (3, 15) | NVSwitch_3/Port_15 |

---

### Use Case 6: Parallel Brackets (Same Input Position)

**Scenario**: Generate paired device identifiers where both digits change together.
- Input 0,1 → "FPGA_01"
- Input 2,3 → "FPGA_23"

**API Usage**:
```cpp
// Both brackets use input position 0 (explicit |0)
DeviceIdPattern pattern("FPGA_[0|0-1:0,2-3:2][0|0-1:1,2-3:3]");

// Pattern has only 1 dimension (both brackets share position 0)
unsigned dims = pattern.dim();  // 1

// Evaluate at different inputs
std::string name0 = pattern.eval(PatternIndex(0));  // "FPGA_01"
std::string name1 = pattern.eval(PatternIndex(1));  // "FPGA_01"
std::string name2 = pattern.eval(PatternIndex(2));  // "FPGA_23"
std::string name3 = pattern.eval(PatternIndex(3));  // "FPGA_23"
```

**Input/Output**:
- **Input**: Single dimension (0-3)
- **Output**: Two-digit codes
- **Behavior**: Both brackets read from same input position

**Mapping Table**:
| Input | 1st Bracket Output | 2nd Bracket Output | Final String |
|-------|--------------------|--------------------|--------------|
| 0 | 0 | 1 | FPGA_01 |
| 1 | 0 | 1 | FPGA_01 |
| 2 | 2 | 3 | FPGA_23 |
| 3 | 2 | 3 | FPGA_23 |

---

### Use Case 7: Complex Real-World Example

**Scenario**: GPU sensor paths with shifted indexing.
- 8 GPUs (hardware 1-8, display as 0-7)
- Each GPU has DRAM sensor

**API Usage**:
```cpp
DeviceIdPattern pattern(
    "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_[1-8:0-7]/DRAM_0"
);

// Generate all sensor paths
auto paths = pattern.valuesVec();
// Result: 8 paths like:
//   "/xyz/.../GPU_SXM_0/DRAM_0"
//   "/xyz/.../GPU_SXM_1/DRAM_0"
//   ...
//   "/xyz/.../GPU_SXM_7/DRAM_0"

// Match a specific path
auto indexes = pattern.match(
    "/xyz/openbmc_project/inventory/system/processors/GPU_SXM_5/DRAM_0"
);
// Result: [PatternIndex(6)]  // Hardware index 6 displays as 5
```

---

## API Reference

### Core Classes

#### `PatternIndex`
Represents an index tuple for evaluating patterns.

**Constructor**:
```cpp
PatternIndex()                    // Empty index (0-dimensional)
PatternIndex(int arg0, ...)       // Multi-dimensional index
```

**Methods**:
```cpp
unsigned dim() const              // Number of dimensions
int operator[](unsigned i) const  // Access dimension i
bool operator==(const PatternIndex& other) const
bool operator<(const PatternIndex& other) const
```

**Example**:
```cpp
PatternIndex idx1(5);           // 1D: (5)
PatternIndex idx2(2, 10);       // 2D: (2, 10)
PatternIndex idx3(1, 3, 7);     // 3D: (1, 3, 7)

unsigned d = idx2.dim();        // 2
int first = idx2[0];            // 2
int second = idx2[1];           // 10
```

---

#### `DeviceIdPattern`
Main class for working with device ID patterns.

**Constructor**:
```cpp
DeviceIdPattern(const std::string& pattern)
```

**Core Methods**:

##### `eval(const PatternIndex& index) -> std::string`
Generate device string from pattern index.
```cpp
DeviceIdPattern pat("GPU_[1-4]");
std::string name = pat.eval(PatternIndex(3));  // "GPU_3"
```

##### `domain() -> CartesianProductRange`
Get all valid input indexes (as iterable range).
```cpp
for (const auto& idx : pat.domain()) {
    std::string name = pat.eval(idx);
}
```

##### `domainVec() -> std::vector<PatternIndex>`
Get all valid input indexes (as vector).
```cpp
auto indexes = pat.domainVec();
// Result: [PatternIndex(1), PatternIndex(2), ...]
```

##### `values() -> std::vector<std::string>`
Get all possible output strings.
```cpp
auto names = pat.valuesVec();
// Result: ["GPU_1", "GPU_2", "GPU_3", "GPU_4"]
```

##### `match(const std::string& str) -> std::vector<PatternIndex>`
Find all indexes that produce the given string.
```cpp
auto indexes = pat.match("GPU_3");
// Result: [PatternIndex(3)]
```

##### `matches(const std::string& str) -> bool`
Check if string matches pattern.
```cpp
bool isMatch = pat.matches("GPU_3");  // true
bool isMatch2 = pat.matches("GPU_9"); // false
```

##### `dim() -> unsigned`
Get number of input dimensions.
```cpp
DeviceIdPattern pat1("GPU_[1-8]");              // dim() = 1
DeviceIdPattern pat2("Switch_[0-3]/Port_[0-15]"); // dim() = 2
```

##### `dimDomain(unsigned axis) -> PatternInputDomain`
Get valid values for specific dimension.
```cpp
DeviceIdPattern pat("Switch_[0-3]/Port_[0-15]");
auto switches = pat.dimDomain(0);  // {0, 1, 2, 3}
auto ports = pat.dimDomain(1);     // {0, 1, ..., 15}
```

##### `isInjective() -> bool`
Check if mapping is one-to-one.
```cpp
DeviceIdPattern pat1("GPU_[1-8]");      // true (1-to-1)
DeviceIdPattern pat2("HSC_[0-3:5]");    // false (many-to-1)
```

---

### Syntax Classes (Advanced)

#### `syntax::BracketRange`
Represents a simple range like "0-7" or "5".

**Constructor**:
```cpp
BracketRange(DeviceIndex left, DeviceIndex right)
```

**Methods**:
```cpp
unsigned size() const             // Number of elements
auto begin() const                // Iterator to first element
auto end() const                  // Iterator past last element
static BracketRange parse(const StringRange& range)
```

---

#### `syntax::BracketRangeMap`
Represents a range mapping like "0-7:1-8" or "0-3:5".

**Constructor**:
```cpp
BracketRangeMap(BracketRange&& from, BracketRange&& to)
```

**Conversion**:
```cpp
operator BracketMap() const       // Convert to explicit map
```

**Static Methods**:
```cpp
static BracketRangeMap parse(const StringRange& range)
```

---

#### `syntax::BracketMap`
Type alias for `std::map<DeviceIndex, DeviceIndex>`.

**Functions**:
```cpp
BracketMap parseBracketMap(const StringRange& range)
```

---

## Best Practices

### 1. Choose the Right Mapping Type

**Use identity mapping** when hardware and display indexes match:
```cpp
"GPU_[0-7]"  // Hardware 0-7, display 0-7
```

**Use shifted mapping** for index conversion:
```cpp
"GPU_[0-7:1-8]"  // Hardware 0-7, display 1-8
```

**Use many-to-one** for shared resources:
```cpp
"HSC_[0-1:8,2-3:9]"  // Multiple devices share same resource
```

**Use one-to-many** when a single device index maps to multiple output identifiers:
```cpp
"GPU_[0:0-1,1:2-3]"  // GPU 0 maps to devices 0-1, GPU 1 maps to devices 2-3
```

### 2. Pattern Naming Conventions

- Use descriptive prefixes: `GPU_`, `NVSwitch_`, `PCIeRetimer_`
- Use underscores for separation: `GPU_SXM_` not `GPUSXM`
- Be consistent with existing patterns in your system

### 3. Error Handling

Always handle exceptions when parsing patterns:
```cpp
try {
    DeviceIdPattern pattern(userInput);
    // Use pattern...
} catch (const std::runtime_error& e) {
    std::cerr << "Invalid pattern: " << e.what() << std::endl;
}
```

### 4. Performance Considerations

- **Cache patterns**: Don't recreate patterns repeatedly
- **Use domain() for iteration**: More efficient than generating all values
- **Prefer domainVec()**: When you need random access to indexes

```cpp
// Good: Create once, use many times
DeviceIdPattern pattern("GPU_[1-8]");
for (const auto& idx : pattern.domain()) {
    processDevice(pattern.eval(idx));
}

// Bad: Creating pattern in loop
for (int i = 1; i <= 8; ++i) {
    DeviceIdPattern pattern("GPU_[1-8]");  // Wasteful!
    processDevice(pattern.eval(PatternIndex(i)));
}
```

### 5. Testing Patterns

Verify your patterns work as expected:
```cpp
DeviceIdPattern pattern("Device_[0-1:0,2-3:1]");

// Test domain
assert(pattern.domainVec().size() == 4);

// Test evaluation
assert(pattern.eval(PatternIndex(0)) == "Device_0");
assert(pattern.eval(PatternIndex(3)) == "Device_1");

// Test matching
assert(pattern.match("Device_0").size() == 2);  // Non-injective
assert(pattern.isInjective() == false);
```

---

## Troubleshooting

### Common Errors and Solutions

#### Error: "Invalid range sizes"
```
BracketRangeMap: Invalid range sizes - 'from' range has 8 element(s),
'to' range has 4 element(s)...
```

**Cause**: Mismatched range sizes in mapping.

**Solution**: Ensure ranges have equal sizes or 'to' has exactly 1 element:
```cpp
// Wrong:
"Device_[0-7:0-3]"  // 8 elements → 4 elements (invalid)

// Correct:
"Device_[0-7:0-7]"  // 8 → 8 (identity)
"Device_[0-7:1-8]"  // 8 → 8 (shifted)
"Device_[0-7:5]"    // 8 → 1 (many-to-one)
```

---

#### Error: "Invalid range [7-4]"
```
BracketRange: Invalid range [7-4] - left boundary must not exceed right boundary.
```

**Cause**: Range specified backwards.

**Solution**: Ensure left ≤ right:
```cpp
// Wrong:
"Device_[7-4]"

// Correct:
"Device_[4-7]"
```

---

#### Error: "Index out of domain"
```
std::domain_error: Pattern index not in domain
```

**Cause**: Trying to evaluate pattern with invalid index.

**Solution**: Check domain before evaluation:
```cpp
DeviceIdPattern pat("GPU_[1-8]");

// Wrong:
pat.eval(PatternIndex(0));   // 0 not in domain [1-8]
pat.eval(PatternIndex(9));   // 9 not in domain [1-8]

// Correct:
if (pat.dimDomain(0).contains(5)) {
    std::string name = pat.eval(PatternIndex(5));
}
```

---

#### Pattern doesn't match expected strings

**Debugging steps**:

1. **Check the domain**:
```cpp
auto domain = pat.domainVec();
std::cout << "Domain size: " << domain.size() << std::endl;
for (const auto& idx : domain) {
    std::cout << idx << std::endl;
}
```

2. **Check generated values**:
```cpp
auto values = pat.valuesVec();
for (const auto& val : values) {
    std::cout << val << std::endl;
}
```

3. **Test specific evaluations**:
```cpp
for (const auto& idx : pat.domain()) {
    std::cout << idx << " -> " << pat.eval(idx) << std::endl;
}
```

---

### Getting Help

1. **Check pattern syntax**: Review the [Pattern Syntax](#pattern-syntax) section
2. **Review examples**: Find similar use case in [Use Cases](#use-cases-and-examples)
3. **Enable debug output**: Use the pattern's output operators:
```cpp
std::cout << "Pattern: " << pattern << std::endl;
std::cout << "Index: " << index << std::endl;
```

---

## Appendix: Quick Reference

### Pattern Syntax Quick Reference

| Syntax | Description | Example | Domain | Values |
|--------|-------------|---------|--------|--------|
| `[N]` | Single value | `[5]` | 5 | "5" |
| `[N-M]` | Range | `[0-3]` | 0,1,2,3 | "0","1","2","3" |
| `[N-M:P-Q]` | 1-to-1 map | `[0-3:10-13]` | 0,1,2,3 | "10","11","12","13" |
| `[N-M:P]` | Many-to-1 | `[0-3:5]` | 0,1,2,3 | "5","5","5","5" |
| `[N:P-Q]` | 1-to-Many | `[0:0-1,1:2-3]` | 0,1 | "0","1","2","3" |
| `[A,B,C]` | Series | `[0-1:0,2-3:1]` | 0,1,2,3 | "0","0","1","1" |
| `[P\|...]` | Explicit pos | `[0\|1-4]` | 1,2,3,4 | "1","2","3","4" |

### Common Patterns

```cpp
// Simple enumeration
"GPU_[0-7]"

// Zero-to-one based conversion
"Device_[0-7:1-8]"

// Shared resources
"HSC_[0-1:8,2-3:9]"

// 1-to-many mapping
"GPU_SMA_[0:0-1,1:2-3]"

// Multi-dimensional
"Switch_[0-3]/Port_[0-15]"

// Complex D-Bus paths
"/xyz/openbmc_project/inventory/system/GPU_[1-8:0-7]"
```

### API Quick Reference

```cpp
// Create pattern
DeviceIdPattern pat("GPU_[1-8]");

// Get dimensions
unsigned dims = pat.dim();

// Iterate domain
for (const auto& idx : pat.domain()) { ... }

// Evaluate
std::string name = pat.eval(PatternIndex(5));

// Match
auto indexes = pat.match("GPU_5");

// Check match
bool matches = pat.matches("GPU_5");

// Get all values
auto values = pat.valuesVec();

// Check injectivity
bool oneToOne = pat.isInjective();
```

---

**Document Version**: 1.0
**Last Updated**: January 2026
**Maintainer**: NVIDIA Monitor Eventing Team

