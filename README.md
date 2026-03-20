# lua-windows
A basic Windows API for the Lua programming language.
You should compile it as a DLL. More info in the description below.

---

## Loading the Module

Place `mywinapi.dll` in the same folder as your script, then:

```lua
local winapi = require("mywinapi")
```

---

## messageBox

Shows a Windows popup dialog.

```lua
local result = winapi.messageBox(text, caption, flags)
```

| Argument | Type | Required | Default |
|---|---|---|---|
| text | string | yes | — |
| caption | string | yes | — |
| flags | integer | no | MB_OK (0) |

Returns an integer indicating which button was pressed.

```lua
-- Simple OK box
winapi.messageBox("Hello!", "My App")

-- Yes/No box
local MB_YESNO        = 0x04
local MB_ICONQUESTION = 0x20
local IDYES           = 6

local result = winapi.messageBox("Continue?", "Confirm", MB_YESNO + MB_ICONQUESTION)
if result == IDYES then
    print("User said yes")
end
```

---

## buffer

Allocates a temporary zeroed C buffer and returns its contents as a Lua string.

```lua
local data = winapi.buffer(size)
```

| Argument | Type | Required |
|---|---|---|
| size | integer | yes |

```lua
local data = winapi.buffer(256)
print(#data) -- 256
```

---

## getProcessHeap

Returns a handle to the default process heap. Every process has one automatically.

```lua
local heap = winapi.getProcessHeap()
```

Takes no arguments. Returns a heap handle (light userdata). Never needs to be destroyed.

```lua
local heap = winapi.getProcessHeap()
-- pass heap to allocHeap, freeHeap etc.
```

---

## createHeap

Creates a new private heap.

```lua
local heap = winapi.createHeap(options, init_size, max_size)
```

| Argument | Type | Required | Default |
|---|---|---|---|
| options | integer | no | HEAP_CREATE_ENABLE_EXECUTE |
| init_size | integer | yes | — |
| max_size | integer | yes | — |

Pass `0` for `max_size` to allow the heap to grow freely. Returns a heap handle (light userdata).

```lua
-- growable heap, 4KB initial size
local heap = winapi.createHeap(0, 4096, 0)

-- fixed heap capped at 1MB
local heap = winapi.createHeap(0, 4096, 1024 * 1024)
```

---

## allocHeap

Allocates a block of memory from a heap.

```lua
local block = winapi.allocHeap(heap, flags, size)
```

| Argument | Type | Required | Default |
|---|---|---|---|
| heap | light userdata | yes | — |
| flags | integer | no | HEAP_ZERO_MEMORY |
| size | integer | yes | — |

Returns a pointer to the allocated block (light userdata). Size must be greater than zero.

```lua
local heap  = winapi.getProcessHeap()
local block = winapi.allocHeap(heap, 0, 512)
```

---

## freeHeap

Releases a block back to the heap it was allocated from.

```lua
local ok = winapi.freeHeap(heap, block)
```

| Argument | Type | Required |
|---|---|---|
| heap | light userdata | yes |
| block | light userdata | yes |

Returns true on success. The heap must be the same one used in allocHeap.

```lua
local heap  = winapi.getProcessHeap()
local block = winapi.allocHeap(heap, 0, 512)

-- use the block...

winapi.freeHeap(heap, block)
```

---

## heapDestroy

Destroys an entire private heap and frees all its allocations at once.

```lua
local ok = winapi.heapDestroy(heap)
```

| Argument | Type | Required |
|---|---|---|
| heap | light userdata | yes |

Returns true on success. Do NOT call this on the handle returned by `getProcessHeap` — that heap belongs to Windows.

```lua
local heap   = winapi.createHeap(0, 4096, 0)
local block1 = winapi.allocHeap(heap, 0, 128)
local block2 = winapi.allocHeap(heap, 0, 256)

-- frees block1, block2, and the heap itself in one call
winapi.heapDestroy(heap)
```

---

## virtualAlloc

Allocates pages of memory directly from the OS.

```lua
local mem = winapi.virtualAlloc(address, size, alloc_type, protect)
```

| Argument | Type | Required | Default |
|---|---|---|---|
| address | nil / light userdata / integer | no | nil (OS chooses) |
| size | integer | yes | — |
| alloc_type | integer | no | MEM_COMMIT \| MEM_RESERVE |
| protect | integer | no | PAGE_READWRITE |

Returns a pointer to the allocated region (light userdata). Always aligned to a page boundary (4096 bytes). Size must be greater than zero.

```lua
-- let Windows choose the address
local mem = winapi.virtualAlloc(nil, 4096)

-- request a specific address
local mem = winapi.virtualAlloc(0x10000000, 4096)

-- reserve without committing
local MEM_RESERVE    = 0x2000
local PAGE_NOACCESS  = 0x01
local mem = winapi.virtualAlloc(nil, 1024 * 1024, MEM_RESERVE, PAGE_NOACCESS)
```

---

## virtualFree

Releases a region allocated by virtualAlloc.

```lua
local ok = winapi.virtualFree(address, size, free_type)
```

| Argument | Type | Required | Default |
|---|---|---|---|
| address | light userdata | yes | — |
| size | integer | no | 0 |
| free_type | integer | no | MEM_RELEASE |

Returns true on success. When using MEM_RELEASE (default), size **must** be 0. When using MEM_DECOMMIT, size must be greater than 0.

```lua
local mem = winapi.virtualAlloc(nil, 4096)

-- release entirely (size must be 0)
winapi.virtualFree(mem)

-- decommit only (size required)
local MEM_DECOMMIT = 0x4000
winapi.virtualFree(mem, 4096, MEM_DECOMMIT)
```

---

## Full Example

```lua
local winapi = require("mywinapi")

-- ask the user a question
local MB_YESNO        = 0x04
local MB_ICONQUESTION = 0x20
local IDYES           = 6

local answer = winapi.messageBox("Run memory test?", "mywinapi", MB_YESNO + MB_ICONQUESTION)

if answer == IDYES then

    -- heap allocation
    local heap  = winapi.getProcessHeap()
    local block = winapi.allocHeap(heap, 0, 1024)
    winapi.freeHeap(heap, block)
    print("heap test passed")

    -- private heap
    local myheap = winapi.createHeap(0, 4096, 0)
    local b1     = winapi.allocHeap(myheap, 0, 128)
    local b2     = winapi.allocHeap(myheap, 0, 256)
    winapi.heapDestroy(myheap) -- frees b1 and b2 together
    print("private heap test passed")

    -- virtual memory
    local mem = winapi.virtualAlloc(nil, 4096)
    winapi.virtualFree(mem)
    print("virtual alloc test passed")

    winapi.messageBox("All tests passed!", "mywinapi")
end
```
