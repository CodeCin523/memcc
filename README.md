# memcc

`memcc` (CodeCin's Memory) is a lightweight C library providing fast, "safe", and convenient memory management helpers. It includes utilities for pointer alignment, memory zeroing, value clamping, and several allocator implementations, making low-level memory operations safer and easier to use.

See the documentation for the full list of features.

---

## Optional Macros (build-controlled)

`memcc` provides optional macros to customize behavior. These are now **set via CMake**, not by defining them in source files.

**Macros:**

* **`MEMCC_NO_CHECKS`** – disables runtime checks (`MEMCC_CHECK`)  
* **`MEMCC_DEBUG`** – enables assertions for debugging  
* **`MEMCC_ZERO_ON_ALLOC`** – zero memory when allocating  
* **`MEMCC_ZERO_ON_FREE`** – zero memory when freeing (useful for sensitive data)  

**Set macros for your target:**

```cmake
# Enable debug checks and zero-on-alloc for a target
target_compile_definitions(myapp PRIVATE
    MEMCC_DEBUG
    MEMCC_ZERO_ON_ALLOC
)
````

**Or set macros for all targets linking memcc:**

```cmake
target_compile_definitions(memcc::memcc PUBLIC
    MEMCC_DEBUG
    MEMCC_ZERO_ON_FREE
)
```

> No need to `#define` these macros in your source files anymore.

---

## Usage

### 1. Pull the library

Clone the repository:

```bash
git clone https://github.com/CodeCin523/memcc.git
```

Or add it as a submodule:

```bash
git submodule add https://github.com/CodeCin523/memcc.git external/memcc
```

---

### 2. Include in your CMake project

#### Add subdirectory and link

```cmake
add_subdirectory(path/to/memcc)
add_executable(myapp main.c)

# Link to the memcc library
target_link_libraries(myapp PRIVATE memcc::memcc)
```

Include headers in your source:

```c
#include "memcc.h"
```

---

### 3. Example

```c
#include "memcc.h"

int main(void) {
    int buffer[16];

    // Align pointer to 16 bytes
    void *aligned = memcc_align_forward(buffer, 16);

    // Zero memory if enabled via MEMCC_ZERO_ON_ALLOC
    MEMCC_ZERO(aligned, sizeof(buffer));

    return 0;
}
```

---

## Notes

* `memcc` is header-only; all `static inline` functions compile directly in your code.
* Optional macros are now **build-controlled** and do not require manual `#define`s in your source files.
* Future compiled `.c` functions can be added without changing usage; simply `target_link_libraries(myapp PRIVATE memcc::memcc)`.
