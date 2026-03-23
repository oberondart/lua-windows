#include <windows.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> 

/*
    MIT License

Copyright (c) 2026 devenoch

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

// use the command below to compile!
// tcc -shared -o mywinapi.dll mywinapi.c -I"C:\Users\Admin\Desktop\lua" -L"C:\Users\Admin\Desktop\lua" -llua54 -luser32 -lkernel32

// this will be our message box function!
static int messageBox(lua_State *L) {
    const char *text = luaL_checkstring(L, 1); // first arg, text for message box
    const char *caption = luaL_checkstring(L, 2); // second arg, caption for message box, simple!

    int flags = (int)luaL_optinteger(L, 3, MB_OK); // default is mb_ok

    int result = MessageBoxA(NULL, text, caption, (UINT)flags);

    lua_pushinteger(L, result);
    return 1;
}

// buffer creator!
static int buffer(lua_State *L) {
    size_t size = (size_t)luaL_checkinteger(L, 1); // get size of buffer for first argument
    char *buf = (char *)malloc(size); // allocate memory
    if (!buf) {
        return luaL_error(L, "allocation failed");
    }

    memset(buf, 0, size);
    lua_pushstring(L, buf);
    free(buf);

    return 1;
}

// heaps here!

static int createHeap(lua_State *L) {
    int options = (int)luaL_optinteger(L, 1, HEAP_CREATE_ENABLE_EXECUTE); // options are the first arguments
    size_t init_size = (size_t)luaL_checkinteger(L, 2); // get size of heap of second argument
    size_t max_size = (size_t)luaL_checkinteger(L, 3); // get max size of heap as third and last argument

    HANDLE hHeap = HeapCreate((DWORD)options, init_size, max_size);
    if (hHeap == NULL) {
        return luaL_error(L, "heap creation failed: %lu", GetLastError());
    }
    lua_pushlightuserdata(L, hHeap);

    return 1; // whenever we return 1, we return one because we pushed one thing
}

static int getProcessHeap(lua_State *L) {
    HANDLE heap = GetProcessHeap();

    if (heap == NULL) {
        return luaL_error(L, "cannot find heap process: %lu", GetLastError());
    }
    lua_pushlightuserdata(L, heap);
    return 1;
}

static int allocHeap(lua_State *L) { // we will use light user data for this example!
    if (lua_type(L, 1) != LUA_TLIGHTUSERDATA) { // checks for type, needs to be LUA_TLIGHTUSERDATA!!!
        return luaL_error(L, "expected heap handle, got %s",
        lua_typename(L, lua_type(L, 1)));
    }
    HANDLE heap = (HANDLE)lua_touserdata(L, 1); // get heap handle as first argument
    
    if (heap == NULL) {
        return luaL_error(L, "heap handle is NULL");
    }
    int options = (int)luaL_optinteger(L, 2, HEAP_ZERO_MEMORY); // options default to HEAP_ZERO_MEMORY
    size_t size = (size_t)luaL_checkinteger(L, 3); // get byte size for heap allocation
    if (size == 0) {
        return luaL_error(L, "allocation size must be greater than zero");
    }

    void* heapalloc = HeapAlloc(heap, (DWORD)options, size); // call function

    if (heapalloc == NULL) {
        return luaL_error(L, "heap allocation failed: %lu", GetLastError());
    }

    lua_pushlightuserdata(L, heapalloc);
    
    return 1;
}

static int freeHeap(lua_State *L) {
    if (lua_type(L, 1) != LUA_TLIGHTUSERDATA) {
        return luaL_error(L, "expected heap handle, got %s",
            lua_typename(L, lua_type(L, 1)));
    }

    HANDLE heap = (HANDLE)lua_touserdata(L, 1); // get handle as first argument
    if (lua_type(L, 2) != LUA_TLIGHTUSERDATA) {
        return luaL_error(L, "expected block pointer, got %s",
            lua_typename(L, lua_type(L, 2)));
    }
    LPVOID block = (LPVOID)lua_touserdata(L, 2);
    if (block == NULL) {
        return luaL_error(L, "block pointer is NULL");
    }
    int options = 0; // options as second argument
    
    if (heap == NULL) {
        return luaL_error(L, "heap handle is NULL");
    }
    
    BOOL result = HeapFree(heap, options, block); // call function

    if (!result) {
        return luaL_error(L, "heap freedom failure: %lu", GetLastError());
    }

    lua_pushboolean(L, 1);
    return 1;
}

static int heapDestroy(lua_State *L) {
    if (lua_type(L, 1) != LUA_TLIGHTUSERDATA) {
        return luaL_error(L, "expected heap handle, got %s",
            lua_typename(L, lua_type(L, 1)));
    }

    HANDLE heap = (HANDLE)lua_touserdata(L, 1); // get handle as first argument, the handle leading to heap to be destroyed

    if (heap == NULL) { // null check
        return luaL_error(L, "heap handle is NULL");
    }
    BOOL result = HeapDestroy(heap);

    if (!result) {
        return luaL_error(L, "heap destruction failed: %lu", GetLastError());
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

static int virtualAlloc(lua_State *L) {
    LPVOID address = NULL; // we're getting the address, but for now, NULL!

if (lua_isnoneornil(L, 1)) {
    // nil or nothing — address stays NULL
    address = NULL;

} else if (lua_type(L, 1) == LUA_TLIGHTUSERDATA) {
    // caller passed a pointer from previous VirtualAlloc
    address = (LPVOID)lua_touserdata(L, 1);

} else if (lua_type(L, 1) == LUA_TNUMBER) {
    // caller passed a raw numeric address
    // uintptr_t widens the integer to pointer size before the cast
    address = (LPVOID)(uintptr_t)lua_tointeger(L, 1);

} else {
    // anything else is a mistake, obviously
    return luaL_error(L, "expected address or nil, got %s",
        lua_typename(L, lua_type(L, 1)));
}
    size_t size = (size_t)luaL_checkinteger(L, 2);
    if (size == 0) {
        return luaL_error(L, "size cannot be zero!");
    }
    DWORD mem_options = (DWORD)luaL_optinteger(L, 3, MEM_COMMIT | MEM_RESERVE);
    DWORD protect_options = (DWORD)luaL_optinteger(L, 4, PAGE_READWRITE);
    LPVOID result = VirtualAlloc(address, size, mem_options, protect_options);

    if (!result) {
        return luaL_error(L, "VirtualAlloc() failed: %lu", GetLastError());
    }

    lua_pushlightuserdata(L, result);
    return 1;
}

static int virtualFree(lua_State *L) {
    if (lua_type(L, 1) != LUA_TLIGHTUSERDATA) {
        return luaL_error(L, "expected address, got %s",
                lua_typename(L, lua_type(L, 1)));
    }
    LPVOID address = (LPVOID)lua_touserdata(L, 1); // address as first argument
    if (address == NULL) {
        return luaL_error(L, "error: address is null");
    }

    size_t size = (size_t)luaL_optinteger(L, 2, 0); // size as second argument, optional

    DWORD free_Type = (DWORD)luaL_optinteger(L, 3, MEM_RELEASE); // free type as third argument, default to MEM_RELEASE

    if (free_Type == MEM_RELEASE && size != 0) {
        return luaL_error(L, "cannot use MEM_RELEASE when size is not equal to 0");
    }
    
    if (free_Type == MEM_DECOMMIT && size == 0) {
        return luaL_error(L, "cannot use MEM_DECOMMIT when size is 0");
    }
    
    BOOL result = VirtualFree(address, size, free_Type); // api call

    if (!result) {
        return luaL_error(L, "VirtualFree failed: %lu", GetLastError());
    }
    
    lua_pushboolean(L, 1);
    return 1;
    
}

// process function! yipe cool stuff :>
static int createProcess(lua_State *L) {
    if (lua_type(L, 1) != LUA_TTABLE) {
        return luaL_error(L, "argument 1 in createProcess() must be table {}");
    }

    // command field
    lua_getfield(L, 1, "command");
    if (lua_type(L, -1) != LUA_TSTRING) {
        return luaL_error(L, "error: command field is required and expects type string");
    }
    const char *command = lua_tostring(L, 1);
    lua_pop(L, 1);

    // flags field
    lua_getfield(L, 1, "flags");
    DWORD flags;
    if (lua_isnil(L, -1)) {
        flags = 0;
    } else {
        flags = (DWORD)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    // inherit field, optional, default to FALSE
    lua_getfield(L, 1, "inherit");
    BOOL inherit;
    if (lua_isnil(L, -1)) {
        inherit = FALSE;
    } else {
        inherit = (BOOL)lua_toboolean(L, -1);
    }
    lua_pop(L, 1);

    // workingDir field, optional, default to NULL
    lua_getfield(L, 1, "workingDir");
    LPCSTR workingDir;
    if (lua_isnil(L, -1)) {
        workingDir = NULL;
    } else {
        workingDir = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    // now, we copy command into a wrtiable malloc() buffer
    size_t commandLen = strlen(command) + 1;
    char *commandBuffer = malloc(commandLen);
    if (commandBuffer == NULL) {
        return luaL_error(L, "error: malloc() failure for command buffer");
    }
    memcpy(commandBuffer, command, commandLen);

    // startup info initaliziting...
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(STARTUPINFO);

    // process info initaliziting... windows takes care of it! for once...
    PROCESS_INFORMATION pi;

    // function call
    BOOL result = CreateProcess(
        NULL,
        commandBuffer,
        NULL,
        NULL,
        inherit,
        flags,
        NULL,
        workingDir,
        &si,
        &pi
    );

    // quickly! free buffer! no leaks on my watch :3
    free(commandBuffer);
    commandBuffer = NULL;

    if (!result) {
        return luaL_error(L, "error: CreateProcess() failure at: %lu", GetLastError());
    }

    lua_newtable(L);

    lua_pushlightuserdata(L, pi.hProcess);
    lua_setfield(L, -2, "processHandle");

    lua_pushlightuserdata(L, pi.hThread);
    lua_setfield(L, -2, "threadHandle");

    lua_pushinteger(L, pi.dwProcessId);
    lua_setfield(L, -2, "processId");

    lua_pushinteger(L, pi.dwThreadId);
    lua_setfield(L, -2, "threadId");

    return 1;
}

// our handle closer function, needed to clean up all our used up handles
static int closeHandle(lua_State *L) {
    if (lua_type(L, 1) != LUA_TLIGHTUSERDATA) {
        return luaL_error(L, "error: expected handle, got %s",
                lua_typename(L, lua_type(L, 1)));
    }

    HANDLE handle = (HANDLE)lua_touserdata(L, 1);

    if (handle == NULL) {
        return luaL_error(L, "error: handled expected, got NULL");
    }

    BOOL result = CloseHandle(handle);

    if (result == FALSE) {
        return luaL_error(L, "error: CloseHandle() failed: %lu", GetLastError());
    }

    lua_pushboolean(L, 1);
    return 1;
}

static const luaL_Reg lib[] = {
    { "messageBox",   messageBox   },
    { "buffer",       buffer       },
    { "createHeap",   createHeap   },
    { "getProcessHeap", getProcessHeap },
    { "allocHeap",    allocHeap    },
    { "freeHeap",     freeHeap     },
    { "heapDestroy",  heapDestroy  },
    { "virtualAlloc", virtualAlloc },
    { "virtualFree", virtualFree },
    { "createProcess", createProcess },
    { "closeHandle", closeHandle}
};

__declspec(dllexport) int luaopen_mywinapi(lua_State *L) {
    luaL_newlib(L, lib);
    return 1;
}
