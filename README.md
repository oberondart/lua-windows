# lua-windows — Windows API for Lua

A Lua 5.4 extension written in C that exposes core Windows API functions for memory management, process creation, and UI. Built using the Lua C API and compiled as a native Windows DLL.

Flee bots :< !
---

## Requirements

- **Lua 5.4** — [luabinaries.sourceforge.net](https://luabinaries.sourceforge.net)
- **TCC** (Tiny C Compiler) or **MinGW-w64** (GCC for Windows)
- **Windows 10** or later

---

## Building

### With TCC

```cmd
tcc -shared -o winapi.dll lua-windows.c -I"C:\lua\include" -L"C:\lua" -llua54 -luser32 -lkernel32
```

### With MinGW-w64

```cmd
gcc -shared -o winapi.dll lua-windows.c -I"C:/lua/include" -L"C:/lua" -llua54 -luser32 -lkernel32
```

Place `winapi.dll` in the same folder as your Lua script.

---

## Loading the Module

```lua
local winapi = require("winapi")
```

---

## API Reference

### messageBox

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

**Example:**
```lua
-- Simple OK dialog
winapi.messageBox("Hello!", "My App")

-- Yes/No dialog
local MB_YESNO        = 0x04
local MB_ICONQUESTION = 0x20
local IDYES           = 6

local result = winapi.messageBox("Continue?", "Confirm", MB_YESNO + MB_ICONQUESTION)
if result == IDYES then
    print("User said yes")
end
```

---

### buffer

Allocates a temporary zeroed C buffer and returns its contents as a Lua string.

```lua
local data = winapi.buffer(size)
```

| Argument | Type | Required |
|---|---|---|
| size | integer | yes |

**Example:**
```lua
local data = winapi.buffer(256)
print(#data) -- 256
```

---

### getProcessHeap

Returns a handle to the default process heap. Every Windows process has one automatically — it exists before your program's first line runs.

```lua
local heap = winapi.getProcessHeap()
```

Takes no arguments. Returns a heap handle (light userdata). Never call `heapDestroy` on this handle — Windows owns it.

**Example:**
```lua
local heap = winapi.getProcessHeap()
local block = winapi.allocHeap(heap, 0, 256)
winapi.freeHeap(heap, block)
```

---

### createHeap

Creates a new private heap isolated from the rest of the process. All allocations inside it can be freed in one `heapDestroy` call.

```lua
local heap = winapi.createHeap(options, init_size, max_size)
```

| Argument | Type | Required | Default |
|---|---|---|---|
| options | integer | no | HEAP_CREATE_ENABLE_EXECUTE |
| init_size | integer | yes | — |
| max_size | integer | yes | — |

Pass `0` for `max_size` to allow the heap to grow freely. Returns a heap handle (light userdata).

**Example:**
```lua
-- Growable heap with 4KB initial size
local heap = winapi.createHeap(0, 4096, 0)

-- Fixed heap capped at 1MB
local heap = winapi.createHeap(0, 4096, 1024 * 1024)
```

---

### allocHeap

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

**Example:**
```lua
local heap  = winapi.getProcessHeap()
local block = winapi.allocHeap(heap, 0, 512)
```

---

### freeHeap

Releases an allocated block back to the heap it came from.

```lua
local ok = winapi.freeHeap(heap, block)
```

| Argument | Type | Required |
|---|---|---|
| heap | light userdata | yes |
| block | light userdata | yes |

Returns `true` on success. The heap handle must be the same one used in `allocHeap`. Passing the wrong heap corrupts both heaps silently.

**Example:**
```lua
local heap  = winapi.getProcessHeap()
local block = winapi.allocHeap(heap, 0, 512)
winapi.freeHeap(heap, block)
```

---

### heapDestroy

Destroys an entire private heap and frees every allocation inside it in one OS call.

```lua
local ok = winapi.heapDestroy(heap)
```

| Argument | Type | Required |
|---|---|---|
| heap | light userdata | yes |

Returns `true` on success. Do **not** call this on the handle returned by `getProcessHeap`.

**Example:**
```lua
local heap   = winapi.createHeap(0, 4096, 0)
local block1 = winapi.allocHeap(heap, 0, 128)
local block2 = winapi.allocHeap(heap, 0, 256)

-- frees block1, block2, and the heap itself in one call
winapi.heapDestroy(heap)
```

---

### virtualAlloc

Allocates pages of memory directly from the OS, bypassing any heap. Always aligned to a page boundary (4096 bytes).

```lua
local mem = winapi.virtualAlloc(address, size, alloc_type, protect)
```

| Argument | Type | Required | Default |
|---|---|---|---|
| address | nil / light userdata / integer | no | nil (OS chooses) |
| size | integer | yes | — |
| alloc_type | integer | no | MEM_COMMIT \| MEM_RESERVE |
| protect | integer | no | PAGE_READWRITE |

Returns a pointer to the allocated region (light userdata). Size must be greater than zero.

**Example:**
```lua
-- Let Windows choose the address
local mem = winapi.virtualAlloc(nil, 4096)

-- Request a specific address
local mem = winapi.virtualAlloc(0x10000000, 4096)

-- Reserve without committing (no physical memory yet)
local MEM_RESERVE   = 0x2000
local PAGE_NOACCESS = 0x01
local mem = winapi.virtualAlloc(nil, 1024 * 1024, MEM_RESERVE, PAGE_NOACCESS)
```

---

### virtualFree

Releases a region allocated by `virtualAlloc`.

```lua
local ok = winapi.virtualFree(address, size, free_type)
```

| Argument | Type | Required | Default |
|---|---|---|---|
| address | light userdata | yes | — |
| size | integer | no | 0 |
| free_type | integer | no | MEM_RELEASE |

Returns `true` on success.

> **Important:** When using `MEM_RELEASE` (default), size **must** be `0`. When using `MEM_DECOMMIT`, size must be greater than `0`.

**Example:**
```lua
local mem = winapi.virtualAlloc(nil, 4096)

-- Release entirely — size must be 0
winapi.virtualFree(mem)

-- Decommit only — size required
local MEM_DECOMMIT = 0x4000
winapi.virtualFree(mem, 4096, MEM_DECOMMIT)
```

---

### createProcess

Launches a new Windows process. Accepts an options table and returns a table of handles and IDs.

```lua
local info = winapi.createProcess(options)
```

**Options table:**

| Field | Type | Required | Default |
|---|---|---|---|
| command | string | yes | — |
| flags | integer | no | 0 |
| inherit | boolean | no | false |
| workingDir | string | no | nil (inherit from parent) |

**Returns a table with four fields:**

| Field | Type | Description |
|---|---|---|
| processHandle | light userdata | Handle to the new process |
| threadHandle | light userdata | Handle to the new process's main thread |
| processId | integer | PID visible in Task Manager |
| threadId | integer | Main thread ID |

> **Important:** Always close `processHandle` and `threadHandle` with `closeHandle` when done. Leaving them open leaks OS resources.

**Example:**
```lua
local info = winapi.createProcess({ command = "notepad.exe" })

print("PID: " .. info.processId)

-- always close handles when done
winapi.closeHandle(info.processHandle)
winapi.closeHandle(info.threadHandle)
```

**With flags:**
```lua
local CREATE_NEW_CONSOLE = 0x10

local info = winapi.createProcess({
    command    = "cmd.exe /C dir C:\\",
    flags      = CREATE_NEW_CONSOLE,
    workingDir = "C:\\"
})

winapi.closeHandle(info.processHandle)
winapi.closeHandle(info.threadHandle)
```

---

### closeHandle

Closes a Windows handle returned by `createProcess` or any other function that produces a handle. Must be called on `processHandle` and `threadHandle` from `createProcess` when you are done with them.

```lua
local ok = winapi.closeHandle(handle)
```

| Argument | Type | Required |
|---|---|---|
| handle | light userdata | yes |

Returns `true` on success.

**Example:**
```lua
local info = winapi.createProcess({ command = "notepad.exe" })
winapi.closeHandle(info.processHandle)
winapi.closeHandle(info.threadHandle)
```

---

## Full Example

```lua
local winapi = require("mywinapi")

-- -------------------------------------------------------
-- MessageBox
-- -------------------------------------------------------
local MB_YESNO        = 0x04
local MB_ICONQUESTION = 0x20
local IDYES           = 6

local answer = winapi.messageBox("Run memory test?", "mywinapi", MB_YESNO + MB_ICONQUESTION)

if answer == IDYES then

    -- -------------------------------------------------------
    -- Default process heap
    -- -------------------------------------------------------
    local heap  = winapi.getProcessHeap()
    local block = winapi.allocHeap(heap, 0, 1024)
    winapi.freeHeap(heap, block)
    print("heap test passed")

    -- -------------------------------------------------------
    -- Private heap — destroy frees everything at once
    -- -------------------------------------------------------
    local myHeap = winapi.createHeap(0, 4096, 0)
    local b1     = winapi.allocHeap(myHeap, 0, 128)
    local b2     = winapi.allocHeap(myHeap, 0, 256)
    winapi.heapDestroy(myHeap)
    print("private heap test passed")

    -- -------------------------------------------------------
    -- Virtual memory
    -- -------------------------------------------------------
    local mem = winapi.virtualAlloc(nil, 4096)
    winapi.virtualFree(mem)
    print("virtual memory test passed")

    -- -------------------------------------------------------
    -- Process creation
    -- -------------------------------------------------------
    local info = winapi.createProcess({ command = "notepad.exe" })
    print("launched notepad, PID: " .. info.processId)
    winapi.closeHandle(info.processHandle)
    winapi.closeHandle(info.threadHandle)
    print("process test passed")

    winapi.messageBox("All tests passed!", "mywinapi")
end
```

---

## Key Concepts

### The Lua Stack
Every value passed between Lua and C goes through a virtual stack. Arguments come in at positions `1, 2, 3...` and return values are pushed before returning. The number returned from each C function tells Lua how many values were pushed.

### Light Userdata
Windows `HANDLE` and pointer values are stored in Lua as light userdata — an opaque pointer Lua holds but does not garbage collect. Always validate with `lua_type(L, n) == LUA_TLIGHTUSERDATA` before use.

### Validate Before You Push
Always check arguments and API return values before pushing anything onto the Lua stack. A bad value reaching a Windows API call does not produce a clean error — it corrupts memory silently.

### Handle Ownership
Handles returned by `createProcess` must be closed with `closeHandle`. The process heap handle from `getProcessHeap` must never be destroyed. Private heaps from `createHeap` must be destroyed with `heapDestroy` when done.

---

## License

MIT
