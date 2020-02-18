#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <curl/curl.h>
#include "curl.h"

//so that name mangling doesn't mess up function names
static char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

static int curl(lua_State *L){
    const char *link = lua_tostring(L, 1);
    testSpeed(link,L);
    lua_pushstring(L,link);
    return 3;

//   
//   FILE *fp;
//   char results[1035];
//   const char *command = concat(concat("/usr/bin/curl ",page),"| tac");
//   fp = popen(command, "r");
//   while(fgets(results,sizeof(results), fp) != NULL){
//     printf("%s",results);
//   }
  
//   lua_pushstring(L,page);
//   return 3;
}



static int c_swap (lua_State *L) {
    //check and fetch the arguments
    double arg1 = luaL_checknumber (L, 1);
    double arg2 = luaL_checknumber (L, 2);

    //push the results
    lua_pushnumber(L, arg2);
    lua_pushnumber(L, arg1);

    //return number of results
    return 2;
}
static int my_sin (lua_State *L) {
    double arg = luaL_checknumber (L, 1);
    lua_pushnumber(L, sin(arg));
    return 1;
}

//library to be registered
static const struct luaL_Reg mylib [] = {
      {"c_swap", c_swap},
      {"mysin", my_sin}, /* names can be different */
      {"curl", curl},
      {NULL, NULL}  /* sentinel */
    };

//name of this function is not flexible
int luaopen_mylib (lua_State *L){
    luaL_newlib(L, mylib);
    return 1;
}


int main(){
    // Create new Lua state and load the lua libraries
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    //Expose the c_swap function to the lua environment
    lua_pushcfunction(L, c_swap);
    lua_setglobal(L, "c_swap");

    // Tell Lua to execute a lua command
    luaL_dostring(L, "print(c_swap(4, 5))");
    return 0;
}