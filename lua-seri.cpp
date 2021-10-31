#include <cstring>
#include <vector>
#include <string>

using std::vector;
using std::string;
using std::to_string;

#include "lua.hpp"
#include "dtoa_milo.h"

// 只处理 number 和 table
inline static string get_lua_value_str(lua_State *L, int index, int type) {
    if (type == LUA_TNUMBER) {
        double num = lua_tonumber(L, index);
        if (trunc(num) == num) {
            return to_string(int(num));
        } else {
            char buffer[1024];
            dtoa_milo(num, buffer);
            return buffer;
        }
    } else {
        return lua_tostring(L, index);
    }
}

static void write_table(lua_State *L, vector<string> &wb, int index) {
    wb.emplace_back("{");
    lua_pushnil(L);
    while (lua_next(L, index) != 0) { // lua_next 有弹出的操作

        // 处理 key, 只处理 number 和 string
        int key_type = lua_type(L, -2);
        assert(key_type == LUA_TNUMBER || key_type == LUA_TSTRING);
        if (key_type == LUA_TNUMBER) {
            wb.emplace_back("[" + get_lua_value_str(L, -2, key_type) + "]=");
        } else if (key_type == LUA_TSTRING) {
            wb.emplace_back(get_lua_value_str(L, -2, key_type) + "=");
        }

        // 处理 value, 只处理 number, string 和 table
        int value_type = lua_type(L, -1);
        assert(value_type == LUA_TNUMBER || value_type == LUA_TSTRING || value_type == LUA_TTABLE);
        if (value_type == LUA_TNUMBER) {
            wb.emplace_back(get_lua_value_str(L, -1, value_type) + ",");
        } else if (value_type == LUA_TSTRING) {
            wb.emplace_back("\"" + get_lua_value_str(L, -1, value_type) + "\",");
        } else if (value_type == LUA_TTABLE) {
            write_table(L, wb, lua_gettop(L));
        }

        lua_pop(L, 1);
    }
    wb.back().pop_back(); // 删掉最后一次加上去的逗号
    wb.emplace_back("}");
}

static int lserialize(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    vector<string> wb;
    write_table(L, wb, 1);
    string res{};
    for (const auto &str: wb) {
        res += str;
    }
    lua_pushstring(L, res.c_str());

    return 1;
}

LUAMOD_API int luaopen_luaseri(lua_State *L) {
    //@formatter:off
    luaL_Reg libs[] = {
            {"serialize", lserialize},
            {NULL, NULL},
    };
    //@formatter:on
    luaL_newlib(L, libs);
    return 1;
}
