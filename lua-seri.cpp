#include <cstdio>
#include <cstring>
#include <string>

using std::string;
using std::to_string;

#include "lua.hpp"
#include "dtoa_milo.h"

extern "C" {
    LUAMOD_API int luaopen_luaseri(lua_State *L);
}

#define BLOCK_SIZE 128
#define MAX_NUM_LEN 25

struct seri_block {
    struct seri_block *next;
    char buffer[BLOCK_SIZE];
};

struct seri_write_block {
    // 头块
    struct seri_block *head;
    // 当前块
    struct seri_block *current;
    // 当前总长度
    int len;
    // 当前 block 中的写入长度
    int ptr;
};

static struct seri_block *new_block() {
    struct seri_block *b = (seri_block *)malloc(sizeof(struct seri_block));
    b->next = NULL;
    memset(b->buffer, 0, BLOCK_SIZE);
    return b;
}

static void init_write_block(struct seri_write_block *wb) {
    struct seri_block *b = new_block();
    wb->head = b;
    wb->len = 0;
    wb->ptr = 0;
    wb->current = b;
}

static void alloc_block(struct seri_write_block *wb) {
    struct seri_block *b = new_block();
    wb->current->next = b;
    wb->current = b;
    wb->ptr = 0;
}

static void write_data(struct seri_write_block *wb, const char *data) {
    size_t size = strlen(data);
    size_t diff = 0;
    while (diff < size) {
        if (wb->ptr == BLOCK_SIZE) {
            alloc_block(wb);
        }
        int write_size = wb->ptr + (size - diff) <= BLOCK_SIZE ? (size - diff) : (BLOCK_SIZE - wb->ptr);
        memcpy(wb->current->buffer + wb->ptr, data + diff, write_size);
        wb->ptr = wb->ptr + write_size;
        wb->len = wb->len + write_size;
        diff += write_size;
    }
}

// 用来修改队尾
static void change_data(struct seri_write_block *wb, const char *data){
    wb->current->buffer[wb->ptr - 1] = data[0];
}

// 只处理 number 和 table
static string get_lua_value_str(lua_State *L, int index, int type) {
    if (type == LUA_TNUMBER) {
        double num = lua_tonumber(L, index);
        if (trunc(num) == num) {
            return to_string(long(num));
        } else {
            char buffer[MAX_NUM_LEN];
            dtoa_milo(num, buffer);
            return buffer;
        }
    } else {
        return lua_tostring(L, index);
    }
}

static void write_table(lua_State *L, struct seri_write_block *wb, int index) {
    write_data(wb, "{");
    lua_pushnil(L);
    while (lua_next(L, index) != 0) { // lua_next 有弹出的操作

        // 处理 key, 只处理 number 和 string
        int key_type = lua_type(L, -2);
        assert(key_type == LUA_TNUMBER || key_type == LUA_TSTRING);
        if (key_type == LUA_TNUMBER) {
            string res = "[" + get_lua_value_str(L, -2, key_type) + "]=";
            write_data(wb, res.c_str());
        } else if (key_type == LUA_TSTRING) {
            string res = get_lua_value_str(L, -2, key_type) + "=";
            write_data(wb, res.c_str());
        }

        // 处理 value, 只处理 number, string 和 table
        int value_type = lua_type(L, -1);
        assert(value_type == LUA_TNUMBER || value_type == LUA_TSTRING || value_type == LUA_TTABLE);
        if (value_type == LUA_TNUMBER) {
            string res = get_lua_value_str(L, -1, value_type) + ",";
            write_data(wb, res.c_str());
        } else if (value_type == LUA_TSTRING) {
            string res = "\"" + get_lua_value_str(L, -1, value_type) + "\",";
            write_data(wb, res.c_str());
        } else if (value_type == LUA_TTABLE) {
            write_table(L, wb, lua_gettop(L));
            write_data(wb, ",");
        }

        lua_pop(L, 1);
    }
    change_data(wb, "}"); // 将最后的 , 替换为 }
}

// 把内存块中的数据拼接并释放原内存块中的数据
static char *table_str(struct seri_write_block *wb) {
    char *res = (char *)malloc(wb->len + 1);
    int read_size = 0;
    struct seri_block *cur = wb->head;
    while (cur != NULL) {
        int cur_size = cur->next == NULL ? wb->ptr : BLOCK_SIZE;
        memcpy(res + read_size, cur->buffer, cur_size);
        read_size += cur_size;
        struct seri_block *tmp = cur;
        cur = cur->next;
        free(tmp);
    }
    res[wb->len] = '\0';
    wb->head = NULL;
    wb->current = NULL;
    wb->ptr = 0;
    wb->len = 0;
    return res;
}

static int lserialize(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    struct seri_write_block wb;
    init_write_block(&wb);
    write_table(L, &wb, 1);
    char *res = table_str(&wb);
    lua_pushstring(L, res);
    free(res);

    return 1;
}

LUAMOD_API int luaopen_luaseri(lua_State *L) {
    luaL_Reg libs[] = {
        {"serialize", lserialize},
        {NULL, NULL},
    };
    luaL_newlib(L, libs);
    return 1;
}
