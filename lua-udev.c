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
#define UDEV_MT_NAME "UDEV_HANDLE"
#define UDEV_DEVICE_MT_NAME "UDEV_DEVICE_HANDLE"
#define UDEV_MONITOR_MT_NAME "UDEV_MONITOR_HANDLE"
#define UDEV_ENUMERATE_MT_NAME "UDEV_ENUMERATE_HANDLE"

#define DEFAULT_SYS_PATH "/sys"
#define DEFAULT_DEV_PATH "/dev"

typedef struct udev Udev;
typedef struct udev_device UdevDevice;
typedef struct udev_monitor UdevMonitor;
typedef struct udev_enumerate UdevEnumerate;
typedef struct udev_list_entry UdevListEntry;

const char* lowerCase(lua_State *L, const char *p) {
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "lower");
    lua_pushstring(L, p);
    lua_pcall(L, 1, 1, 0);
    const char* r = lua_tostring(L, -1);
    lua_pop(L, 2); // r + string
    return r;
}

const char* upperCase(lua_State *L, const char *p) {
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "upper");
    lua_pushstring(L, p);
    lua_pcall(L, 1, 1, 0);
    const char* r = lua_tostring(L, -1);
    lua_pop(L, 2); // r + string
    return r;
}

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
        lua_pushfstring(L, "not an handle (%s)!", lua_typename(L, lua_type(L, -1)));
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

static int list_entry2table(lua_State *L, UdevListEntry *list) {
    UdevListEntry *entry;
    lua_newtable(L);
    udev_list_entry_foreach(entry, list) {
        lua_pushstring(L, lowerCase(L, udev_list_entry_get_name(entry)));
        lua_pushstring(L, udev_list_entry_get_value(entry));
        lua_settable(L, -3);
    }
    return 1;
}

static int list_entry2itable(lua_State *L, UdevListEntry *list) {
    int i = 1;
    UdevListEntry *entry;
    lua_newtable(L);
    udev_list_entry_foreach(entry, list) {
        lua_pushinteger(L, i++);
        lua_pushstring(L, udev_list_entry_get_name(entry));
        lua_settable(L, -3);
    }
    return 1;
}

static int list_sysattr2table(lua_State *L, UdevDevice *device) {
    int i = 1;
    UdevListEntry *entry;
    lua_newtable(L);
    udev_list_entry_foreach(entry, udev_device_get_sysattr_list_entry(device)) {
        const char* name = udev_list_entry_get_name(entry);
        lua_pushstring(L, name);
        lua_pushstring(L, udev_device_get_sysattr_value(device, name));
        lua_settable(L, -3);
    }
    return 1;
}

static int new_udev(lua_State *L) {
    fprintf(stderr, "new udev\n");fflush(stderr);
    return new_handle(L, udev_new(), UDEV_MT_NAME);
}

static int new_udev_device(lua_State *L, UdevDevice *device) {
    fprintf(stderr, "new udev device\n");fflush(stderr);
    return new_handle(L, device, UDEV_DEVICE_MT_NAME);
}

static int new_udev_monitor(lua_State *L) {
    fprintf(stderr, "new udev monitor\n");fflush(stderr);
    return new_handle(L, udev_monitor_new_from_netlink(
        (Udev*)get_handle(L, 1),
        lua_tostring(L, 2)), UDEV_MONITOR_MT_NAME);
}

static int new_udev_enumerate(lua_State *L) {
    fprintf(stderr, "new udev enumerate\n");fflush(stderr);
    return new_handle(L, udev_enumerate_new(
        (Udev*)get_handle(L, 1)), UDEV_ENUMERATE_MT_NAME);
}

// static int new_udev_device_from_device_id(lua_State *L) {
//     return new_udev_device(L, udev_device_new_from_device_id(
//         (Udev*)get_handle(L, 1),
//         lua_tostring(L, 2)));
// }

// static int new_udev_device_from_devnum(lua_State *L) {
//     return new_udev_device(L, udev_device_new_from_devnum(
//         (Udev*)get_handle(L, 1),
//         lua_tostring(L, 2),
//         lua_tointeger(L, 3)));
// }

static int new_udev_device_from_environment(lua_State *L) {
    return new_udev_device(L, udev_device_new_from_environment(
        (Udev*)get_handle(L, 1)));
}

static int new_udev_device_from_subsystem_sysname(lua_State *L) {
    return new_udev_device(L, udev_device_new_from_subsystem_sysname(
        (Udev*)get_handle(L, 1),
        lua_tostring(L, 2),
        lua_tostring(L, 3)));
}

static int new_udev_device_from_syspath(lua_State *L) {
    return new_udev_device(L, udev_device_new_from_syspath(
        (Udev*)get_handle(L, 1),
        lua_tostring(L, 2)));
}

static int meth_udev_monitor_receive(lua_State *L) {
    return new_udev_device(L, udev_monitor_receive_device(
        (UdevMonitor*)get_handle(L, 1)));
}

static int meth_udev_getsyspath(lua_State *L) {
	char* sysPath;
	
	sysPath = getenv("SYSFS_PATH");
	
	if (sysPath == NULL) {
		sysPath = DEFAULT_SYS_PATH;
	}
	
    lua_pushstring(L, sysPath);
    return 1;
}

static int meth_udev_getdevpath(lua_State *L) {
	char* devPath;
	
	devPath = getenv("UDEV_ROOT");
	
	if (devPath == NULL) {
			devPath = DEFAULT_DEV_PATH;
	}
	
    lua_pushstring(L, devPath);
    return 1;
}

static int meth_udev_close(lua_State *L) {
    udev_unref((Udev*)close_handle(L, 1));
    return 0;
}

static int meth_udev_device_close(lua_State *L) {
    udev_device_unref((UdevDevice*)close_handle(L, 1));
    return 0;
}

static int meth_udev_enumerate_close(lua_State *L) {
    udev_enumerate_unref((UdevEnumerate*)close_handle(L, 1));
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
    lua_pushinteger(L, udev_monitor_filter_add_match_subsystem_devtype(
        (UdevMonitor*)get_handle(L, 1),
        lua_tostring(L, 2),
        lua_tostring(L, 3)
    ));
    return 1;
}

static int meth_udev_monitor_filter_tag(lua_State *L) {
    lua_pushinteger(L, udev_monitor_filter_add_match_tag(
        (UdevMonitor*)get_handle(L, 1),
        lua_tostring(L, 2)
    ));
    return 1;
}

static int meth_udev_device_getdevtype(lua_State *L) {
    lua_pushstring(L, udev_device_get_devtype((UdevDevice*)get_handle(L, 1)));
    return 1;
}


static int meth_udev_device_getsyspath(lua_State *L) {
    lua_pushstring(L, udev_device_get_syspath((UdevDevice*)get_handle(L, 1)));
    return 1;
}


static int meth_udev_device_getsysname(lua_State *L) {
    lua_pushstring(L, udev_device_get_sysname((UdevDevice*)get_handle(L, 1)));
    return 1;
}


static int meth_udev_device_getdriver(lua_State *L) {
    lua_pushstring(L, udev_device_get_driver((UdevDevice*)get_handle(L, 1)));
    return 1;
}


static int meth_udev_device_getaction(lua_State *L) {
    lua_pushstring(L, udev_device_get_action((UdevDevice*)get_handle(L, 1)));
    return 1;
}


static int meth_udev_device_getsubsystem(lua_State *L) {
    lua_pushstring(L, udev_device_get_subsystem((UdevDevice*)get_handle(L, 1)));
    return 1;
}


static int meth_udev_device_getdevpath(lua_State *L) {
    lua_pushstring(L, udev_device_get_devpath((UdevDevice*)get_handle(L, 1)));
    return 1;
}


static int meth_udev_device_getsysnum(lua_State *L) {
    lua_pushstring(L, udev_device_get_sysnum((UdevDevice*)get_handle(L, 1)));
    return 1;
}


static int meth_udev_device_getdevnode(lua_State *L) {
    lua_pushstring(L, udev_device_get_devnode((UdevDevice*)get_handle(L, 1)));
    return 1;
}


static int meth_udev_device_getdevnum(lua_State *L) {
    lua_pushinteger(L, udev_device_get_devnum((UdevDevice*)get_handle(L, 1)));
    return 1;
}

static int meth_udev_device_getseqnum(lua_State *L) {
    lua_pushnumber(L, udev_device_get_seqnum((UdevDevice*)get_handle(L, 1)));
    return 1;
}

static int meth_udev_device_getparent(lua_State *L) {
    return new_udev_device(L, udev_device_get_parent((UdevDevice*)get_handle(L, 1)));
}

static int meth_udev_device_getparent_with_subsystem_devtype(lua_State *L) {
    return new_udev_device(L, udev_device_get_parent_with_subsystem_devtype(
        (UdevDevice*)get_handle(L, 1),
        lua_tostring(L, 2),
        lua_tostring(L, 3)));
}

static int meth_udev_device_getproperties(lua_State *L) {
    return list_entry2table(L, udev_device_get_properties_list_entry(
        (UdevDevice*)get_handle(L, 1)));
}

static int meth_udev_device_getsysattrs(lua_State *L) {
    return list_sysattr2table(L, (UdevDevice*)get_handle(L, 1));
}

static int meth_udev_device_hastag(lua_State *L) {
    lua_pushinteger(L, udev_device_has_tag(
        (UdevDevice*)get_handle(L, 1),
        lua_tostring(L, 2)));
    return 1;
}

static int meth_udev_enumerate_match_subsystem(lua_State *L) {
    lua_pushinteger(L, udev_enumerate_add_match_subsystem(
        (UdevEnumerate*)get_handle(L, 1),
        lua_tostring(L, 2)
    ));
    return 1;
}

static int meth_udev_enumerate_nomatch_subsystem(lua_State *L) {
    lua_pushinteger(L, udev_enumerate_add_nomatch_subsystem(
        (UdevEnumerate*)get_handle(L, 1),
        lua_tostring(L, 2)
    ));
    return 1;
}

static int meth_udev_enumerate_match_sysattr(lua_State *L) {
    lua_pushinteger(L, udev_enumerate_add_match_sysattr(
        (UdevEnumerate*)get_handle(L, 1),
        lua_tostring(L, 2),
        lua_tostring(L, 3)
    ));
    return 1;
}

static int meth_udev_enumerate_nomatch_sysattr(lua_State *L) {
    lua_pushinteger(L, udev_enumerate_add_nomatch_sysattr(
        (UdevEnumerate*)get_handle(L, 1),
        lua_tostring(L, 2),
        lua_tostring(L, 3)
    ));
    return 1;
}

static int meth_udev_enumerate_match_property(lua_State *L) {
    lua_pushinteger(L, udev_enumerate_add_match_property(
        (UdevEnumerate*)get_handle(L, 1),
        upperCase(L, lua_tostring(L, 2)),
        lua_tostring(L, 3)
    ));
    return 1;

}

static int meth_udev_enumerate_match_tag(lua_State *L) {
    lua_pushinteger(L, udev_enumerate_add_match_tag(
        (UdevEnumerate*)get_handle(L, 1),
        lua_tostring(L, 2)
    ));
    return 1;

}

static int meth_udev_enumerate_match_sysname(lua_State *L) {
    lua_pushinteger(L, udev_enumerate_add_match_sysname(
        (UdevEnumerate*)get_handle(L, 1),
        lua_tostring(L, 2)
    ));
    return 1;

}

static int meth_udev_enumerate_match_parent(lua_State *L) {
    lua_pushinteger(L, udev_enumerate_add_match_parent(
        (UdevEnumerate*)get_handle(L, 1),
        (UdevDevice*)get_handle(L, 1)
    ));
    return 1;

}

static int meth_udev_enumerate_match_initialized(lua_State *L) {
    lua_pushinteger(L, udev_enumerate_add_match_is_initialized(
        (UdevEnumerate*)get_handle(L, 1)
    ));
    return 1;

}


static int meth_udev_enumerate_addsyspath(lua_State *L) {
    lua_pushinteger(L, udev_enumerate_add_syspath(
        (UdevEnumerate*)get_handle(L, 1),
        lua_tostring(L, 2)
    ));
    return 1;

}

static int meth_udev_enumerate_scan_devices(lua_State *L) {
    lua_pushinteger(L, udev_enumerate_scan_devices(
        (UdevEnumerate*)get_handle(L, 1)
    ));
    return 1;

}

static int meth_udev_enumerate_scan_subsystems(lua_State *L) {
    lua_pushinteger(L, udev_enumerate_scan_subsystems(
        (UdevEnumerate*)get_handle(L, 1)
    ));
    return 1;
}

static int meth_udev_enumerate_getlist(lua_State *L) {
    return list_entry2itable(L, udev_enumerate_get_list_entry(
        (UdevEnumerate*)get_handle(L, 1)));
}


static luaL_Reg lib_funcs[] = {
    {"new", new_udev},
    {NULL, NULL}
};

static luaL_Reg udev_monitor_funcs[] = {
    {"new", new_udev_monitor},
    {NULL, NULL}
};

static luaL_Reg udev_enumerate_funcs[] = {
    {"new", new_udev_enumerate},
    {NULL, NULL}
};

static luaL_Reg udev_device_funcs[] = {
    {"new_from_syspath", new_udev_device_from_syspath},
//     {"new_from_devnum", new_udev_device_from_devnum},
    {"new_from_subsystem_sysname", new_udev_device_from_subsystem_sysname},
//     {"new_from_device_id", new_udev_device_from_device_id},
    {"new_from_from_evironment", new_udev_device_from_environment},
    {NULL, NULL}
};

static luaL_Reg udev_device_methods[] = {
    {"getproperties", meth_udev_device_getproperties},
    {"getsysattrs", meth_udev_device_getsysattrs},
    {"getparent", meth_udev_device_getparent},
    {"getparent_with_subsystem_devtype", meth_udev_device_getparent_with_subsystem_devtype},
    {"getdevtype", meth_udev_device_getdevtype},
    {"getdevpath", meth_udev_device_getdevpath},
    {"getsyspath", meth_udev_device_getsyspath},
    {"getsysname", meth_udev_device_getsysname},
    {"getsysnum", meth_udev_device_getsysnum},
    {"getdevnode", meth_udev_device_getdevnode},
    {"getdriver", meth_udev_device_getdriver},
    {"getdevnum", meth_udev_device_getdevnum},
    {"getaction", meth_udev_device_getaction},
    {"getseqnum", meth_udev_device_getseqnum},
    {"hastag", meth_udev_device_hastag},
    {"close", meth_udev_device_close},
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

static luaL_Reg udev_enumerate_methods[] = {
    {"addsyspath", meth_udev_enumerate_addsyspath},
    {"nomatch_sysattr", meth_udev_enumerate_nomatch_sysattr},
    {"nomatch_subsystem", meth_udev_enumerate_nomatch_subsystem},
    {"match_subsystem", meth_udev_enumerate_match_subsystem},
    {"match_sysattr", meth_udev_enumerate_match_sysattr},
    {"match_property", meth_udev_enumerate_match_property},
    {"match_tag", meth_udev_enumerate_match_tag},
    {"match_sysname", meth_udev_enumerate_match_sysname},
    {"match_parent", meth_udev_enumerate_match_parent},
    {"match_initialized", meth_udev_enumerate_match_initialized},
    {"scan_subsystems", meth_udev_enumerate_scan_subsystems},
    {"scan_devices", meth_udev_enumerate_scan_devices},
    {"getlist", meth_udev_enumerate_getlist},
    {"close", meth_udev_enumerate_close},
    {NULL, NULL}

};

static luaL_Reg udev_methods[] = {
    {"getsyspath", meth_udev_getsyspath},
    {"getdevpath", meth_udev_getdevpath},
    {"close", meth_udev_close},
    {NULL, NULL}

};

#define lua_pushcfunctionfield(field, method)\
    lua_pushcfunction(L, method);\
    lua_setfield(L, -2, field);

#define luaL_new_table(methods)\
    lua_createtable(L, 0, sizeof(methods) / sizeof(luaL_Reg) - 1);\
    luaL_register(L, NULL, methods);

#define luaL_register__index(methods)\
    luaL_new_table(methods);\
    lua_setfield(L, -2, "__index");


static void register_udev_device(lua_State *L) {
    luaL_newmetatable(L, UDEV_DEVICE_MT_NAME);
    luaL_register__index(udev_device_methods);
    lua_pushcfunctionfield("__gc", meth_udev_device_close);
    lua_pop(L, 1);

    luaL_new_table(udev_device_funcs);
}

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

static void register_udev_enumerate(lua_State *L) {
    luaL_newmetatable(L, UDEV_ENUMERATE_MT_NAME);
    luaL_register__index(udev_enumerate_methods);
    lua_pushcfunctionfield("__gc", meth_udev_enumerate_close);
    lua_pop(L, 1);

    luaL_new_table(udev_enumerate_funcs);
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

    register_udev_device(L);
    lua_setfield(L, -2, "device");

    register_udev_monitor(L);
    lua_setfield(L, -2, "monitor");

    register_udev_enumerate(L);
    lua_setfield(L, -2, "enumerate");
}

int luaopen_udev(lua_State *L)
{
    register_udev(L);

    return 1;
}
