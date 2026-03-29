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
```

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
#include "memcc_stack.h"
#include "memcc_block.h"  // for other allocators
```

> **Important:** If a header file includes an implementation section (controlled by a `*_IMPLEMENTATION` macro), you should define that macro in **only one `.c` file**.  
> All other files that include the header should **not define the macro**, they will see only the declarations, not the implementation.  
> Defining it in multiple files will cause duplicate symbol errors during linking.
> Example:

```c
#define MEMCC_STACK_IMPLEMENTATION
#define MEMCC_BLOCK_IMPLEMENTATION
#include "memcc_stack.h"
#include "memcc_block.h"
```

---

### 3. Example — NMStack Allocator

```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MEMCC_STACK_IMPLEMENTATION
#include "memcc_stack.h"

int main(void) {
    size_t stack_size = 16 * sizeof(uint32_t);

    // Allocate a properly aligned pool
    void *pool = aligned_alloc(alignof(max_align_t), stack_size);
    if (!pool) {
        perror("aligned_alloc failed");
        return 1;
    }

    // Initialize NMStack
    memcc_nmstack_t stack;
    memcc_setup_nmstack(&stack, pool, stack_size);

    void *mark = NULL;

    // Push values into the stack
    for (uint32_t i = 0; i < 4; ++i) {
        uint32_t *ptr = memcc_nmstack_push_type(&stack, 1, uint32_t);
        if (!ptr) {
            printf("Push failed at i=%u\n", i);
            break;
        }
        if(i == 1) mark = ptr; // Take a mark after the second push

        *ptr = i;  // store value in stack
    }

    // Restore stack to mark (demonstrates partial pop)
    memcc_nmstack_restore(&stack, mark);

    // Manual iteration after restore (just for demonstration)
    while (stack.top != pool) {
        stack.top -= sizeof(uint32_t);
        uint32_t val = *(uint32_t *)stack.top;
        printf("Popped: %u\n", val);
    }

    // Clear and teardown
    memcc_nmstack_clear(&stack);
    memcc_teardown_nmstack(&stack);
    free(pool);

    return 0;
}
```
