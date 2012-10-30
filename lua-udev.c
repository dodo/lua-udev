/*
* Copyright (c) 2012 dodo <dodo.the.last@gmail.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*/

#include <lua.h>
#include <lauxlib.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "libudev.h"

#define UDEV_LIB_NAME "udev"
#define UDEV_TABLE_MT_NAME "UDEV_TABLE"
#define UDEV_MONITOR_TABLE_MT_NAME "UDEV_MONITOR_TABLE"
#define UDEV_MT_NAME "UDEV_HANDLE"
#define UDEV_MONITOR_MT_NAME "UDEV_MONITOR_HANDLE"

typedef struct udev Udev;
typedef struct udev_monitor UdevMonitor;

void push_handle(lua_State *L, void *handle, const char *mt_name) {
    lua_createtable(L, 0, 1);
    lua_pushlightuserdata(L, handle);
    lua_setfield(L, -2, "_native");
    luaL_getmetatable(L, mt_name);
    lua_setmetatable(L, -2);
}

void* get_handle(lua_State *L, int index) {
    void* handle = NULL;
    lua_getfield(L, index, "_native");
    if (lua_islightuserdata(L, -1)) {
        handle = lua_touserdata(L, -1);
    } else {
        lua_pushfstring(L, "not an handle %s!", lua_typename(L, lua_type(L, -1)));
        lua_error(L);
    }
    lua_pop(L, 1);
    return handle;
}

void* close_handle(lua_State *L, int index) {
    fprintf(stderr, "close handle\n");fflush(stderr);
    void* handle = get_handle(L, index);
    lua_pushvalue(L, index);
    lua_pushnil(L);
    lua_setfield(L, -2, "_native");
    lua_pop(L, 1);
    return handle;
}

static int handle_error(lua_State *L) {
    lua_pushnil(L);
    lua_pushstring(L, strerror(errno));
    fprintf(stderr, "err:%s\n", lua_tostring(L, -1));
    lua_pushinteger(L, errno);
    return 3;
}

static int new_handle(lua_State *L, void* handle, const char *mt_name) {
    if(handle == NULL) {
        return handle_error(L);
    } else {
        push_handle(L, handle, mt_name);
        return 1;
    }
}

static int __call_new(lua_State *L) {
    int n = lua_gettop(L);    /* number of arguments */
    lua_getfield(L, 1, "new");
    int i;for(i = 2 ; i <= n ; i++) {
        lua_pushvalue(L, i);
    }
    lua_pcall(L, n-1, 1, 0);
    return 1;
}

static int new_udev(lua_State *L) {
    fprintf(stderr, "new udev\n");fflush(stderr);
    return new_handle(L, udev_new(), UDEV_MT_NAME);
}

static int new_udev_monitor(lua_State *L) {
    fprintf(stderr, "new udev monitor\n");fflush(stderr);
    int n = lua_gettop(L);
    return new_handle(L, udev_monitor_new_from_netlink(
        (Udev*)get_handle(L, 1),
        lua_tostring(L, 2)), UDEV_MONITOR_MT_NAME);
}

static int meth_udev_monitor_receive(lua_State, *L) {
    udev_monitor_receive_device((Udev*)get_handle(L, 1));
    return 1;
}

static int meth_udev_getsyspath(lua_State *L) {
    lua_pushstring(L, udev_get_sys_path((Udev*)get_handle(L, 1)));
    return 1;
}

static int meth_udev_getdevpath(lua_State *L) {
    lua_pushstring(L, udev_get_dev_path((Udev*)get_handle(L, 1)));
    return 1;
}

static int meth_udev_close(lua_State *L) {
    udev_unref((Udev*)close_handle(L, 1));
    return 0;
}

static int meth_udev_monitor_close(lua_State *L) {
    udev_monitor_unref((UdevMonitor*)close_handle(L, 1));
    return 0;
}

static int meth_udev_monitor_clear(lua_State *L) {
    lua_pushinteger(L, udev_monitor_filter_remove((UdevMonitor*)get_handle(L, 1)));
    return 1;
}

static int meth_udev_monitor_getfd(lua_State *L) {
    lua_pushinteger(L, udev_monitor_get_fd((UdevMonitor*)get_handle(L, 1)));
    return 1;
}

static int meth_udev_monitor_start(lua_State *L) {
    lua_pushinteger(L, udev_monitor_enable_receiving((UdevMonitor*)get_handle(L, 1)));
    return 1;
}

static int meth_udev_monitor_udate(lua_State *L) {
    lua_pushinteger(L, udev_monitor_filter_update((UdevMonitor*)get_handle(L, 1)));
    return 1;
}

static int meth_udev_monitor_filter_subsystem_devtype(lua_State *L) {
    int n = lua_gettop(L);
    lua_pushinteger(L, udev_monitor_filter_add_match_subsystem_devtype(
        (UdevMonitor*)get_handle(L, 1),
        lua_tostring(L, 2),
        lua_tostring(L, 3)
    ));
    return 1;
}

static int meth_udev_monitor_filter_tag(lua_State *L) {
    int n = lua_gettop(L);
    lua_pushinteger(L, udev_monitor_filter_add_match_tag(
        (UdevMonitor*)get_handle(L, 1),
        lua_tostring(L, 2)
    ));
    return 1;
}




static luaL_Reg lib_funcs[] = {
    {"new", new_udev},
    {NULL, NULL}
};

static luaL_Reg udev_monitor_funcs[] = {
    {"new", new_udev_monitor},
    {NULL, NULL}
};

static luaL_Reg udev_monitor_methods[] = {
    {"filter_subsystem_devtype", meth_udev_monitor_filter_subsystem_devtype},
    {"filter_tag", meth_udev_monitor_filter_tag},
    {"receive", meth_udev_monitor_receive},
    {"update", meth_udev_monitor_udate},
    {"start", meth_udev_monitor_start},
    {"getfd", meth_udev_monitor_getfd},
    {"clear", meth_udev_monitor_clear},
    {"close", meth_udev_monitor_close},
    {NULL, NULL}
};

static luaL_Reg udev_methods[] = {
    {"getsyspath", meth_udev_getsyspath},
    {"getdevpath", meth_udev_getdevpath},
    {"close", meth_udev_close},
    {NULL, NULL}

};

// static luaL_Reg udev_mt_funcs[] = {
//     {"read", handle_read},
//     {"getfd", handle_getfd},
//     {"close", handle_close},
//     {"addwatch", handle_add_watch},
//     {"rmwatch", handle_rm_watch},
//     {NULL, NULL}
// };

#define register_constant(s)\
    lua_pushinteger(L, s);\
    lua_setfield(L, -2, #s);

#define lua_pushcfunctionfield(field, method)\
    lua_pushcfunction(L, method);\
    lua_setfield(L, -2, field);

#define luaL_new_table(methods)\
    lua_createtable(L, 0, sizeof(methods) / sizeof(luaL_Reg) - 1);\
    luaL_register(L, NULL, methods);

#define luaL_register__index(methods)\
    luaL_new_table(methods);\
    lua_setfield(L, -2, "__index");


static void register_udev_monitor(lua_State *L) {
    luaL_newmetatable(L, UDEV_MONITOR_MT_NAME);
    luaL_register__index(udev_monitor_methods);
    lua_pushcfunctionfield("__gc", meth_udev_monitor_close);
    lua_pop(L, 1);

    luaL_new_table(udev_monitor_funcs);
    lua_createtable(L, 0, 1);
    lua_pushcfunctionfield("__call", __call_new);
    lua_setmetatable(L, -2);
}

static void register_udev(lua_State *L) {
    luaL_newmetatable(L, UDEV_MT_NAME);
    luaL_register__index(udev_methods);
    lua_pushcfunctionfield("__gc", meth_udev_close);
    lua_pop(L, 1);

    luaL_register(L, UDEV_LIB_NAME, lib_funcs);
    lua_getglobal(L, UDEV_LIB_NAME);

    lua_getmetatable(L, -1);
    lua_pushcfunctionfield("__call", __call_new);
    lua_setmetatable(L, -2);

    register_udev_monitor(L);
    lua_setfield(L, -2, "monitor");
}

int luaopen_udev(lua_State *L)
{
    register_udev(L);

//     register_constant(IN_ACCESS);
//     register_constant(IN_ATTRIB);
//     register_constant(IN_CLOSE_WRITE);
//     register_constant(IN_CLOSE_NOWRITE);
//     register_constant(IN_CREATE);
//     register_constant(IN_DELETE);
//     register_constant(IN_DELETE_SELF);
//     register_constant(IN_MODIFY);
//     register_constant(IN_MOVE_SELF);
//     register_constant(IN_MOVED_FROM);
//     register_constant(IN_MOVED_TO);
//     register_constant(IN_OPEN);
//     register_constant(IN_ALL_EVENTS);
//     register_constant(IN_MOVE);
//     register_constant(IN_CLOSE);
//     register_constant(IN_DONT_FOLLOW);
//     register_constant(IN_MASK_ADD);
//     register_constant(IN_ONESHOT);
//     register_constant(IN_ONLYDIR);
//     register_constant(IN_IGNORED);
//     register_constant(IN_ISDIR);
//     register_constant(IN_Q_OVERFLOW);
//     register_constant(IN_UNMOUNT);

    return 1;
}
