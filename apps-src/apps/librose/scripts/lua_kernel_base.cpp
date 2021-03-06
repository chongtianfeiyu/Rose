/*
   Copyright (C) 2014 - 2018 by Chris Beck <render787@gmail.com>
   

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "scripts/lua_kernel_base.hpp"

// #include "game_config.hpp"
#include "game_errors.hpp"
// #include "gui/core/gui_definition.hpp"
// #include "log.hpp"
#include "lua_jailbreak_exception.hpp"  // for lua_jailbreak_exception
#include "random.hpp"
// #include "seed_rng.hpp"
// #include "deprecation.hpp"

#ifdef DEBUG_LUA
#include "scripts/debug_lua.hpp"
#endif

#include "scripts/lua_common.hpp"
#include "scripts/lua_fileops.hpp"
#include "scripts/gui/dialogs/rldialog.hpp"
#include "scripts/vnet.hpp"
#include "scripts/vdata.hpp"
#include "scripts/vprotobuf.hpp"
#include "scripts/vsurface.hpp"
// #include "game_version.hpp"                  // for do_version_check, etc
// #include "picture.hpp"

// #include "formula/string_utils.hpp"
#include "serialization/string_utils.hpp"
#include "serialization/parser.hpp"
#include "formula_string_utils.hpp"
#include "gettext.hpp"
#include "base_instance.hpp"
#include "qr_code.hpp"
#include "scripts/vconfig.hpp" // for config_variable_set

#include <cstring>
#include <exception>
#include <new>
#include <string>
#include <sstream>
#include <vector>
using namespace std::placeholders;

#include <openssl/aes.h>

#include "lua/lauxlib.h"
#include "lua/lualib.h"

extern int rose_str_dump (lua_State *L);

// Registry key for metatable
static const char * Gen = "name generator";

// Callback implementations

/**
 * Compares 2 version strings - which is newer.
 * - Args 1,3: version strings
 * - Arg 2: comparison operator (string)
 * - Ret 1: comparison result
 */
/*
static int intf_compare_versions(lua_State* L)
{
	char const *v1 = luaL_checkstring(L, 1);

	const VERSION_COMP_OP vop = parse_version_op(luaL_checkstring(L, 2));
	if(vop == OP_INVALID) return luaL_argerror(L, 2, "unknown version comparison operator - allowed are ==, !=, <, <=, > and >=");

	char const *v2 = luaL_checkstring(L, 3);

	const bool result = do_version_check(version_info(v1), vop, version_info(v2));
	lua_pushboolean(L, result);

	return 1;
}
*/
/**
 * Replacement print function -- instead of printing to std::cout, print to the command log.
 * Intended to be bound to this' command_log at registration time.
 */

int lua_kernel_base::intf_print(lua_State* L)
{
	SDL_Log("intf_print called:");
	size_t nargs = lua_gettop(L);

	lua_getglobal(L, "tostring");
	for (size_t i = 1; i <= nargs; ++i) {
		lua_pushvalue(L, -1); // function to call: "tostring"
		lua_pushvalue(L, i); // value to pass through tostring() before printing
		lua_call(L, 1, 1);
		const char * str = lua_tostring(L, -1);
		if (!str) {
			SDL_Log("'tostring' must return a value to 'print'");
			str = "";
		}
		if (i > 1) {
			cmd_log_ << "\t"; //separate multiple args with tab character
		}
		cmd_log_ << str;
		SDL_Log("'%s'", str);
		lua_pop(L, 1); // Pop the output of tostrring()
	}
	lua_pop(L, 1); // Pop 'tostring' global

	cmd_log_ << "\n";
	SDL_Log("");

	return 0;
}

/**
 * Replacement load function. Mostly the same as regular load, but disallows loading binary chunks
 * due to CVE-2018-1999023.
 */

static int intf_load(lua_State* L)
{
	std::string chunk = luaL_checkstring(L, 1);
	const char* name = luaL_optstring(L, 2, chunk.c_str());
	std::string mode = luaL_optstring(L, 3, "t");
	bool override_env = !lua_isnone(L, 4);

	if(mode != "t") {
		return luaL_argerror(L, 3, "binary chunks are not allowed for security reasons");
	}

	int result = luaL_loadbufferx(L, chunk.data(), chunk.length(), name, "t");
	if(result != LUA_OK) {
		lua_pushnil(L);
		// Move the nil as the first return value, like Lua's own load() does.
		lua_insert(L, -2);

		return 2;
	}

	if(override_env) {
		// Copy "env" to the top of the stack.
		lua_pushvalue(L, 4);
		// Set "env" as the first upvalue.
		const char* upvalue_name = lua_setupvalue(L, -2, 1);
		if(upvalue_name == nullptr) {
			// lua_setupvalue() didn't remove the copy of "env" from the stack, so we need to do it ourselves.
			lua_pop(L, 1);
		}
	}

	return 1;
}

// Same for loadstring.

static int intf_loadstring(lua_State* L)
{
	std::string string = luaL_checkstring(L, 1);
	const char* name = luaL_optstring(L, 2, string.c_str());

	// deprecated_message("loadstring()", DEP_LEVEL::FOR_REMOVAL, {1, 15, 0}, "Use load() instead.");

	int result = luaL_loadbufferx(L, string.data(), string.length(), name, "t");
	if(result != LUA_OK) {
		lua_pushnil(L);
		lua_insert(L, -2);
		return 2;
	}

	return 1;
}

// The show lua console callback is similarly a method of lua kernel
int lua_kernel_base::intf_show_lua_console(lua_State *L)
{
	if (cmd_log_.external_log_) {
		std::string message = "There is already an external logger attached to this lua kernel, you cannot open the lua console right now.";
		log_error(message.c_str());
		cmd_log_ << message << "\n";
		return 0;
	}

	return 0;
	// return lua_gui2::show_lua_console(L, this);
}
/*
static int impl_name_generator_call(lua_State *L)
{
	name_generator* gen = static_cast<name_generator*>(lua_touserdata(L, 1));
	lua_pushstring(L, gen->generate().c_str());
	return 1;
}

static int impl_name_generator_collect(lua_State *L)
{
	name_generator* gen = static_cast<name_generator*>(lua_touserdata(L, 1));
	gen->~name_generator();
	return 0;
}

static int intf_name_generator(lua_State *L)
{
	std::string type = luaL_checkstring(L, 1);
	name_generator* gen = nullptr;
	try {
		if(type == "markov" || type == "markov_chain") {
			std::vector<std::string> input;
			if(lua_istable(L, 2)) {
				input = lua_check<std::vector<std::string>>(L, 2);
			} else {
				input = utils::parenthetical_split(luaL_checkstring(L, 2), ',');
			}
			int chain_sz = luaL_optinteger(L, 3, 2);
			int max_len = luaL_optinteger(L, 4, 12);
			gen = new(L) markov_generator(input, chain_sz, max_len);
			// Ensure the pointer didn't change when cast
			assert(static_cast<void*>(gen) == dynamic_cast<markov_generator*>(gen));
		} else if(type == "context_free" || type == "cfg" || type == "CFG") {
			if(lua_istable(L, 2)) {
				std::map<std::string, std::vector<std::string>> data;
				for(lua_pushnil(L); lua_next(L, 2); lua_pop(L, 1)) {
					if(lua_type(L, -2) != LUA_TSTRING) {
						lua_pushstring(L, "CFG generator: invalid nonterminal name (must be a string)");
						return lua_error(L);
					}
					if(lua_isstring(L, -1)) {
						data[lua_tostring(L,-2)] = utils::split(lua_tostring(L,-1),'|');
					} else if(lua_istable(L, -1)) {
						data[lua_tostring(L,-2)] = lua_check<std::vector<std::string>>(L, -1);
					} else {
						lua_pushstring(L, "CFG generator: invalid noterminal value (must be a string or list of strings)");
						return lua_error(L);
					}
				}
				if(!data.empty()) {
					gen = new(L) context_free_grammar_generator(data);
				}
			} else {
				gen = new(L) context_free_grammar_generator(luaL_checkstring(L, 2));
			}
			if(gen) {
				assert(static_cast<void*>(gen) == dynamic_cast<context_free_grammar_generator*>(gen));
			}
		} else {
			return luaL_argerror(L, 1, "should be either 'markov_chain' or 'context_free'");
		}
	}
	catch (const name_generator_invalid_exception& ex) {
		lua_pushstring(L, ex.what());
		return lua_error(L);
	}

	// We set the metatable now, even if the generator is invalid, so that it
	// will be properly collected if it was invalid.
	luaL_getmetatable(L, Gen);
	lua_setmetatable(L, -2);

	return 1;
}
*/
/**
* Returns a random numer, same interface as math.random.
*/
/*
static int intf_random(lua_State *L)
{
	if (lua_isnoneornil(L, 1)) {
		double r = static_cast<double>(randomness::generator->next_random());
		double r_max = static_cast<double>(std::numeric_limits<uint32_t>::max());
		lua_push(L, r / (r_max + 1));
		return 1;
	}
	else {
		int32_t min;
		int32_t max;
		if (lua_isnumber(L, 2)) {
			min = lua_check<int32_t>(L, 1);
			max = lua_check<int32_t>(L, 2);
		}
		else {
			min = 1;
			max = lua_check<int32_t>(L, 1);
		}
		if (min > max) {
			return luaL_argerror(L, 1, "min > max");
		}
		lua_push(L, randomness::generator->get_random_int(min, max));
		return 1;
	}
}

static int intf_wml_matches_filter(lua_State* L)
{
	config cfg = luaW_checkconfig(L, 1);
	config filter = luaW_checkconfig(L, 2);
	lua_pushboolean(L, cfg.matches(filter));
	return 1;
}
*/
/**
* Logs a message
* Arg 1: (optional) Logger
* Arg 2: Message
*/
/*
static int intf_log(lua_State *L) {
	const std::string& logger = lua_isstring(L, 2) ? luaL_checkstring(L, 1) : "";
	std::string msg = lua_isstring(L, 2) ? luaL_checkstring(L, 2) : luaL_checkstring(L, 1);
	if(msg.empty() || msg.back() != '\n') {
		msg += '\n';
	}

	if(logger == "err" || logger == "error") {
		LOG_STREAM(err, log_user) << msg;
	} else if(logger == "warn" || logger == "wrn" || logger == "warning") {
		LOG_STREAM(warn, log_user) << msg;
	} else if((logger == "debug" || logger == "dbg")) {
		LOG_STREAM(debug, log_user) << msg;
	} else {
		LOG_STREAM(info, log_user) << msg;
	}
	return 0;
}
*/
/**
 * Logs a deprecation message. See deprecation.cpp for details
 * Arg 1: Element to be deprecated.
 * Arg 2: Deprecation level.
 * Arg 3: Version when element may be removed.
 * Arg 4: Additional detail message.
 */
/*
static int intf_deprecated_message(lua_State* L) {
	const std::string elem = luaL_checkstring(L, 1);
	// This could produce an invalid deprecation level, but that possibility is handled in deprecated_message()
	const DEP_LEVEL level = DEP_LEVEL(luaL_checkinteger(L, 2));
	const std::string ver_str = lua_isnoneornil(L, 3) ? "" : luaL_checkstring(L, 3);
	const std::string detail = luaW_checktstring(L, 4);
	const version_info ver = ver_str.empty() ? game_config::version : ver_str;
	const std::string msg = deprecated_message(elem, level, ver, detail);
	if(level < DEP_LEVEL::INDEFINITE || level >= DEP_LEVEL::REMOVED) {
		// Invalid deprecation level or level 4 deprecation should raise an interpreter error
		lua_push(L, msg);
		return lua_error(L);
	}
	return 0;
}
*/
/**
* Gets the dimension of an image.
* - Arg 1: string.
* - Ret 1: width.
* - Ret 2: height.
*/
/*
static int intf_get_image_size(lua_State *L) {
	char const *m = luaL_checkstring(L, 1);
	image::locator img(m);
	if(!img.file_exists()) return 0;
	surface s = get_image(img);
	lua_pushinteger(L, s->w);
	lua_pushinteger(L, s->h);
	return 2;
}
*/
/**
* Returns the time stamp, exactly as [set_variable] time=stamp does.
* - Ret 1: integer
*/
static int intf_get_time_stamp(lua_State *L) {
	lua_pushinteger(L, SDL_GetTicks());
	return 1;
}

static int intf_format(lua_State* L)
{
	config cfg = luaW_checkconfig(L, 2);
	config_variable_set variables(cfg);
	if (lua_isstring(L, 1)) {
		std::string str = lua_tostring(L, 1);
		luaW_pushtstring(L, utils::interpolate_variables_into_string(str, variables));
		return 1;
	}
	t_string str = luaW_checktstring(L, 1);
	luaW_pushtstring(L, utils::interpolate_variables_into_tstring(str, variables));
	return 1;
}

/*
template<bool conjunct>
static int intf_format_list(lua_State* L)
{
	const t_string empty = luaW_checktstring(L, 1);
	auto values = lua_check<std::vector<t_string>>(L, 2);
	lua_push(L, (conjunct ? utils::format_conjunct_list : utils::format_disjunct_list)(empty, values));
	return 1;
}
*/

static int intf_webrtc_post(lua_State* L)
{
	gui2::vdialog* vdlg = static_cast<gui2::vdialog*>(luaL_checkudata(L, 1, gui2::vdialog::metatableKey));
	int msg = lua_tointeger(L, 2);
	config cfg = luaW_checkconfig(L, 3);

	gui2::trldialog::tmsg_data_cfg* pdata = new gui2::trldialog::tmsg_data_cfg(cfg);
	rtc::Thread::Current()->Post(RTC_FROM_HERE, &vdlg->dialog(), msg, pdata);
	return 0;
}

/**
* Dumps a wml table or userdata wml object into a pretty string.
* - Arg 1: wml table or vconfig userdata
* - Ret 1: string
*/
static int intf_wml_tostring(lua_State* L) {
	const config& arg = luaW_checkconfig(L, 1);
	std::ostringstream stream;
	write(stream, arg);
	lua_pushstring(L, stream.str().c_str());
	return 1;
}

static int intf_log(lua_State* L)
{
	const char* str = luaL_checkstring(L, 1);
	SDL_Log("[%u]%s", SDL_GetTicks(), str);
	return 0;
}

static int intf_breakpoint(lua_State* L)
{
	SDL_Log("for debug. app can insert breakpoint at this line");
	return 0;
}

static int intf_get_ticks(lua_State* L)
{
	lua_pushinteger(L, SDL_GetTicks());
	return 1;
}

static int intf_gettext(lua_State* L)
{
	const char* textdomain = luaL_checkstring(L, 1);
	const char* msgid = luaL_checkstring(L, 2);

	lua_pushstring(L, dsgettext(textdomain, msgid));
	return 1;
}

static int intf_vgettext(lua_State* L)
{
	const char* textdomain = luaL_checkstring(L, 1);
	const char* msgid = luaL_checkstring(L, 2);
	vconfig vcfg = luaW_checkvconfig(L, 3);
	utils::string_map symbols;
	for (const config::attribute& a: vcfg.get_config().attribute_range()) {
		symbols[a.first] = a.second.str(); 
	}

	lua_pushstring(L, vgettext(textdomain, msgid, symbols).c_str());
	return 1;
}

static int intf_format_time_date(lua_State* L)
{
	time_t t = lua_tointeger(L, 1);

	lua_pushstring(L, format_time_date(t).c_str());
	return 1;
}

static int intf_format_elapse_hms(lua_State* L)
{
	time_t elapse = lua_tointeger(L, 1);

	lua_pushstring(L, format_elapse_hms(elapse).c_str());
	return 1;
}

namespace utils {
void lua_split(lua_State* L, std::string const &val, const char c, const int flags)
{
	lua_newtable(L);
	int index = 0;

	std::string::const_iterator i1 = val.begin();
	std::string::const_iterator i2;
	if (flags & STRIP_SPACES) {
		while (i1 != val.end() && portable_isspace(*i1))
			++i1;
	}
	i2=i1;
			
	while (i2 != val.end()) {
		if (*i2 == c) {
			std::string new_val(i1, i2);
			if (flags & STRIP_SPACES)
				strip_end(new_val);
			if (!(flags & REMOVE_EMPTY) || !new_val.empty()) {
				lua_pushstring(L, new_val.c_str());
				lua_rawseti(L, -2, ++ index);
				// res.push_back(new_val);
			}
			++i2;
			if (flags & STRIP_SPACES) {
				while (i2 != val.end() && portable_isspace(*i2))
					++i2;
			}

			i1 = i2;
		} else {
			++i2;
		}
	}

	std::string new_val(i1, i2);
	if (flags & STRIP_SPACES)
		strip_end(new_val);
	if (!(flags & REMOVE_EMPTY) || !new_val.empty()) {
		lua_pushstring(L, new_val.c_str());
		lua_rawseti(L, -2, ++ index);
		// res.push_back(new_val);
	}

	// return res;
}
}

static int intf_split(lua_State* L)
{
	const char* src = luaL_checkstring(L, 1);
	int c = lua_tointeger(L, 2);
	int flags = luaL_optinteger(L, 3, utils::REMOVE_EMPTY | utils::STRIP_SPACES);
	utils::lua_split(L, src, c, flags);
	return 1;
}

static int intf_normalize_path(lua_State* L)
{
	const char* src = luaL_checkstring(L, 1);
	bool dosstyle = luaW_toboolean(L, 2);

	const std::string dst = utils::normalize_path(src, dosstyle);
	lua_pushlstring(L, dst.c_str(), dst.size());
	return 1;
}

static int intf_change_string(lua_State* L)
{
	const std::string src = luaL_checkstring(L, 1);
	int code = luaL_checkinteger(L, 2);
	std::string dst;

	if (code == STRCT_EXTRACT_DIRECTORY) {
		dst = extract_directory(src);
	} else {
		VALIDATE(false, "This version doesn't support");
	}
	lua_pushlstring(L, dst.c_str(), dst.size());
	return 1;
}

static int intf_is_format(lua_State* L)
{
	const std::string src = luaL_checkstring(L, 1);
	int code = luaL_checkinteger(L, 2);
	
	bool ret = false;
	if (code == STRFMT_UUID) {
		bool line = lua_isnoneornil(L, 3)? false: luaW_toboolean(L, 3);
		ret = utils::is_guid(src, line);
	} else {
		VALIDATE(false, "This version doesn't support");
	}
	lua_pushboolean(L, ret);
	return 1;
}

static int intf_pack_rect(lua_State* L)
{
	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	int w = lua_tointeger(L, 3);
	int h = lua_tointeger(L, 4);

	uint64_t result = lua_pack_rect(x, y, w, h);
	lua_pushinteger(L, result);
	return 1;
}

static int intf_unpack_rect(lua_State* L)
{
	uint64_t u64 = lua_tointeger(L, 1);
	uint32_t lo32 = posix_lo32(u64);
	uint32_t hi32 = posix_hi32(u64);

	int x = (short)posix_lo16(lo32);
	int y = (short)posix_hi16(lo32);
	int w = (short)posix_lo16(hi32);
	int h = (short)posix_hi16(hi32);

	lua_pushinteger(L, x);
	lua_pushinteger(L, y);
	lua_pushinteger(L, w);
	lua_pushinteger(L, h);
	return 4;
}

static int intf_pack_point(lua_State* L)
{
	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);

	uint64_t result = lua_pack_point(x, y);
	lua_pushinteger(L, result);
	return 1;
}

static int intf_unpack_point(lua_State* L)
{
	uint64_t u64 = lua_tointeger(L, 1);
	SDL_Point point = lua_unpack_point(u64);

	lua_pushinteger(L, point.x);
	lua_pushinteger(L, point.y);
	return 2;
}

static int intf_calculate_adaption_ratio_size(lua_State* L)
{
	int outer_w = lua_tointeger(L, 1);
	int outer_h = lua_tointeger(L, 2);
	int inner_w = lua_tointeger(L, 3);
	int inner_h = lua_tointeger(L, 4);

	tpoint result = calculate_adaption_ratio_size(outer_w, outer_h, inner_w, inner_h);
	lua_pushinteger(L, result.x);
	lua_pushinteger(L, result.y);
	return 2;
}

/**
 * Creates a vhttp_agent containing the WML table.
 * - Arg 1: WML table.
 * - Ret 1: vconfig userdata.
 */
static int intf_tovhttp_agent(lua_State *L)
{
	vconfig vcfg = luaW_checkvconfig(L, 1);
	luaW_pushvhttp_agent(L, vcfg.get_config());
	return 1;
}

static int intf_tovtcpcmgr(lua_State *L)
{
	// vconfig vcfg = luaW_checkvconfig(L, 1);
	luaW_pushvtcpcmgr(L);
	return 1;
}

static int intf_tovdata(lua_State *L)
{
	int initial_size = luaL_optinteger(L, 1, nposm);
	luaW_pushvrwdata(L, initial_size);
	return 1;
}

static int intf_tovprotobuf(lua_State *L)
{
	const char* lua_bundleid = luaL_checkstring(L, 1);
	const char* filepath = luaL_checkstring(L, 2);
	luaW_pushvprotobuf(L, get_aplt_user_data_dir(lua_bundleid) + "/" + filepath);
	return 1;
}

static int intf_tovsurface(lua_State *L)
{
	std::string image;
	if (lua_type(L, 1) == LUA_TSTRING) {
		image = luaL_checkstring(L, 1);
	}
	luaW_pushvsurface(L, image);
	return 1;
}

static int intf_tovtexture(lua_State *L)
{
	int type = lua_type(L, 1);
	if (type == LUA_TNONE) {
		luaW_pushvtexture(L);

	} else if (type == LUA_TUSERDATA) {
		vsurface* v = static_cast<vsurface *>(lua_touserdata(L, 1));
		luaW_pushvtexture(L, v->get());  // value is a userdata with wrong metatable

	} else {
		VALIDATE(false, null_str);
	}

	return 1;
}

static int intf_write_qrcode(lua_State *L)
{
	const char* text = luaL_checkstring(L, 1);
	int width = lua_tointeger(L, 2);
	int height = lua_tointeger(L, 3);
	vsurface *v = static_cast<vsurface *>(luaL_checkudata(L, 4, vsurface::metatableKey));
	VALIDATE(width > 0 && height > 0 && v != nullptr, null_str);

	surface surf = generate_qr(text, width, height);
	v->reset(&surf);
	return 0;
}

static int intf_aes_internal(lua_State *L, bool encrypt)
{
	int type = luaL_checkinteger(L, 1);
	VALIDATE(type == AES256, null_str);
	size_t l = 0;
	const char* key = luaL_checklstring(L, 2, &l);
	VALIDATE(l == 32, null_str);
	const char* iv = luaL_checklstring(L, 3, &l);
	VALIDATE(l == AES_BLOCK_SIZE, null_str);
	vdata* in = static_cast<vdata *>(lua_touserdata(L, 4));

	vdata* out = luaW_pushvrwdata(L, nposm);
	if (encrypt) {
		utils::aes256_encrypt((const uint8_t*)key, (const uint8_t*)iv, in->data_ptr(), in->data_vsize(), out->data_ptr()); 
	} else {
		utils::aes256_decrypt((const uint8_t*)key, (const uint8_t*)iv, in->data_ptr(), in->data_vsize(), out->data_ptr()); 
	}
	out->set_data_vsize(in->data_vsize());
	
	return 1;
}

static int intf_aes_encrypt(lua_State *L)
{
	return intf_aes_internal(L, true);
}

static int intf_aes_decrypt(lua_State *L)
{
	return intf_aes_internal(L, false);
}

static int intf_get_rendered_text(lua_State *L)
{
	const char* text = luaL_checkstring(L, 1);
	int maximum_width = lua_tointeger(L, 2);
	int font_size = lua_tointeger(L, 3);
	int color = lua_tointeger(L, 4);
	vsurface* v = static_cast<vsurface *>(lua_touserdata(L, 5));
	VALIDATE(maximum_width >= 0 && font_size > 0 && v != nullptr, null_str);

	if (text[0] != '\0') {
		surface surf = font::get_rendered_text(text, maximum_width, font_size, uint32_to_color(color));
		v->reset(&surf);
	} else {
		v->reset(nullptr);
	}
	return 0;
}

static int intf_render_surface(lua_State *L)
{
	uint64_t renderer = lua_tointeger(L, 1);
	vsurface* v = static_cast<vsurface *>(lua_touserdata(L, 2));
	SDL_Rect src, dst;
	SDL_Rect* srcrect = nullptr;
	SDL_Rect* dstrect = nullptr;
	if (lua_type(L, 3) == LUA_TNUMBER) {
		uint64_t u64n = lua_tointeger(L, 3);
		src = lua_unpack_rect(u64n);
		srcrect = &src;

	}
	if (lua_type(L, 4) == LUA_TNUMBER) {
		uint64_t u64n = lua_tointeger(L, 4);
		dst = lua_unpack_rect(u64n);
		dstrect = &dst;

	}

	render_surface(get_renderer(), v->get(), srcrect, dstrect);

	return 0;
}

static int intf_SDL_RenderCopy(lua_State *L)
{
	uint64_t renderer = lua_tointeger(L, 1);
	vtexture* v = static_cast<vtexture *>(lua_touserdata(L, 2));
	SDL_Rect src, dst;
	SDL_Rect* srcrect = nullptr;
	SDL_Rect* dstrect = nullptr;
	if (lua_type(L, 3) == LUA_TNUMBER) {
		uint64_t u64n = lua_tointeger(L, 3);
		src = lua_unpack_rect(u64n);
		srcrect = &src;

	}
	if (lua_type(L, 4) == LUA_TNUMBER) {
		uint64_t u64n = lua_tointeger(L, 4);
		dst = lua_unpack_rect(u64n);
		dstrect = &dst;

	}

	SDL_RenderCopy(get_renderer(), v->get().get(), srcrect, dstrect);
	return 0;
}

// End Callback implementations

// Template which allows to push member functions to the lua kernel base into lua as C functions, using a shim
typedef int (lua_kernel_base::*member_callback)(lua_State *L);

template <member_callback method>
int dispatch(lua_State *L) {
	return ((lua_kernel_base::get_lua_kernel<lua_kernel_base>(L)).*method)(L);
}

#include "lua/lobject.h"
static void test(lua_State* L)
{
	// stackDump(L, "test, pre");
	size_t s = sizeof(Udata);
	t_string *t = new(L) t_string;
	// stackDump(L, "test, post");
	t->t_string::~t_string();
	int ii = 0;
}

bool lua_kernel_base::lua_closing = false;

// Ctor, initialization
lua_kernel_base::lua_kernel_base()
 : mState(luaL_newstate())
 , cmd_log_()
{
	get_lua_kernel_base_ptr(mState) = this;
	lua_State *L = mState;

	cmd_log_ << "Initializing " << my_name() << "...\n";

	// Open safe libraries.
	// Debug and OS are not, but most of their functions will be disabled below.
	cmd_log_ << "Adding standard libs...\n";

	static const luaL_Reg safe_libs[] {
		{ "",       luaopen_base   },
		{ "table",  luaopen_table  },
		{ "string", luaopen_string },
		{ "math",   luaopen_math   },
		{ "coroutine",   luaopen_coroutine   },
		{ "debug",  luaopen_debug  },
		{ "os",     luaopen_os     },
		{ "utf8",	luaopen_utf8   }, // added in Lua 5.3
		{ nullptr, nullptr }
	};
	for (luaL_Reg const *lib = safe_libs; lib->func; ++lib)
	{
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);  /* remove lib */
	}

	// Disable functions from os which we don't want.
	lua_getglobal(L, "os");
	lua_pushnil(L);
	while(lua_next(L, -2) != 0) {
		lua_pop(L, 1);
		char const* function = lua_tostring(L, -1);
		if(strcmp(function, "clock") == 0 || strcmp(function, "date") == 0
			|| strcmp(function, "time") == 0 || strcmp(function, "difftime") == 0) continue;
		lua_pushnil(L);
		lua_setfield(L, -3, function);
	}
	lua_pop(L, 1);

	// Disable functions from debug which we don't want.
	lua_getglobal(L, "debug");
	lua_pushnil(L);
	while(lua_next(L, -2) != 0) {
		lua_pop(L, 1);
		char const* function = lua_tostring(L, -1);
		if(strcmp(function, "traceback") == 0 || strcmp(function, "getinfo") == 0) continue;	//traceback is needed for our error handler
		lua_pushnil(L);										//getinfo is needed for ilua strict mode
		lua_setfield(L, -3, function);
	}
	lua_pop(L, 1);

	// Delete dofile and loadfile.
	lua_pushnil(L);
	lua_setglobal(L, "dofile");
	lua_pushnil(L);
	lua_setglobal(L, "loadfile");

	// Store the error handler.
	cmd_log_ << "Adding error handler...\n";

	push_error_handler(L);


	// Create the gettext metatable.
	cmd_log_ << lua_common::register_gettext_metatable(L);

	// Create the tstring metatable.
	cmd_log_ << lua_common::register_tstring_metatable(L);

	lua_settop(L, 0);

	// Define the CPP_function metatable ( so we can override print to point to a C++ member function, add "show_dialog" for this kernel, etc. )
	cmd_log_ << "Adding boost function proxy...\n";
/*
	lua_cpp::register_metatable(L);
*/
	// Add some callback from the rose lib
	cmd_log_ << "Registering basic rose API...\n";

	static luaL_Reg const callbacks[] {
		// { "compare_versions",         &intf_compare_versions         		},
		{ "debug",                    &intf_wml_tostring                           },
		{ "log",					  &intf_log                           },
		{ "cpp_breakpoint",           &intf_breakpoint       },
		{ "get_ticks",                &intf_get_ticks       },
		{ "gettext",				  &intf_gettext   		},
		{ "vgettext2",				  &intf_vgettext   		},
		{ "format_time_date",         &intf_format_time_date},
		{ "format_elapse_hms",        &intf_format_elapse_hms},
		{ "split",                    &intf_split           },
		{ "normalize_path",           &intf_normalize_path  },
		{ "change_string",            &intf_change_string },
		{ "is_format",                &intf_is_format },
		{ "pack_rect",                &intf_pack_rect       },
		{ "unpack_rect",              &intf_unpack_rect     },
		{ "pack_point",               &intf_pack_point       },
		{ "unpack_point",             &intf_unpack_point     },
		{ "calculate_adaption_ratio_size", &intf_calculate_adaption_ratio_size},
		// { "deprecated_message",       &intf_deprecated_message              },
		{ "have_file",                &lua_fileops::intf_have_file          },
		// { "read_file",                &lua_fileops::intf_read_file          },
		{ "textdomain",               &lua_common::intf_textdomain   		},
		{ "tovconfig",                &lua_common::intf_tovconfig		},
		{ "tovhttp_agent",            &intf_tovhttp_agent		},
		{ "tovtcpcmgr",               &intf_tovtcpcmgr		},
		{ "tovdata",                  &intf_tovdata		},
		{ "tovprotobuf",              &intf_tovprotobuf },
		{ "tovsurface",               &intf_tovsurface },
		{ "tovtexture",               &intf_tovtexture },
		{ "write_qrcode",             &intf_write_qrcode },
		{ "aes_encrypt",	          &intf_aes_encrypt },
		{ "aes_decrypt",	          &intf_aes_decrypt },
		{ "get_rendered_text",        &intf_get_rendered_text },
		{ "render_surface",           &intf_render_surface },
		{ "SDL_RenderCopy",           &intf_SDL_RenderCopy },
		// { "get_dialog_value",         &lua_gui2::intf_get_dialog_value		},
		// { "set_dialog_active",        &lua_gui2::intf_set_dialog_active		},
		// { "set_dialog_visible",       &lua_gui2::intf_set_dialog_visible    },
		// { "add_dialog_tree_node",     &lua_gui2::intf_add_dialog_tree_node	},
		// { "add_widget_definition",    &lua_gui2::intf_add_widget_definition },
		// { "set_dialog_callback",      &lua_gui2::intf_set_dialog_callback	},
		// { "set_dialog_canvas",        &lua_gui2::intf_set_dialog_canvas		},
		// { "set_dialog_focus",         &lua_gui2::intf_set_dialog_focus      },
		// { "set_dialog_markup",        &lua_gui2::intf_set_dialog_markup		},
		// { "set_dialog_value",         &lua_gui2::intf_set_dialog_value		},
		// { "remove_dialog_item",       &lua_gui2::intf_remove_dialog_item    },
		{ "dofile",                   &dispatch<&lua_kernel_base::intf_dofile>           },
		{ "require",                  &dispatch<&lua_kernel_base::intf_require>          },
		{ "kernel_type",              &dispatch<&lua_kernel_base::intf_kernel_type>          },
		// { "show_dialog",              &lua_gui2::show_dialog   },
		// { "show_menu",                &lua_gui2::show_menu  },
		// { "show_message_dialog",      &lua_gui2::show_message_dialog },
		// { "show_popup_dialog",        &lua_gui2::show_popup_dialog   },
		// { "show_story",               &lua_gui2::show_story          },
		{ "show_lua_console",	      &dispatch<&lua_kernel_base::intf_show_lua_console> },
		// { "compile_formula",          &lua_formula_bridge::intf_compile_formula},
		// { "eval_formula",             &lua_formula_bridge::intf_eval_formula},
		// { "name_generator",           &intf_name_generator           },
		// { "random",                   &intf_random                   },
		// { "wml_matches_filter",       &intf_wml_matches_filter       },
		// { "log",                      &intf_log                      },
		// { "get_image_size",           &intf_get_image_size           },
		{ "get_time_stamp",           &intf_get_time_stamp           },
		{ "format",                   &intf_format                   },
		// { "format_conjunct_list",     &intf_format_list<true>        },
		// { "format_disjunct_list",     &intf_format_list<false>       },
		{ "webrtc_post",                   &intf_webrtc_post            },
		{ nullptr, nullptr }
	};

	lua_getglobal(L, "rose");
	if (!lua_istable(L,-1)) {
		lua_newtable(L);
	}
	luaL_setfuncs(L, callbacks, 0);
	//lua_cpp::set_functions(L, cpp_callbacks, 0);
	lua_setglobal(L, "rose");

	register_rose_const_value();

	//
	// now lua_getop(L) == 1
	//

	// Override the print function
	cmd_log_ << "Redirecting print function...\n";

	lua_getglobal(L, "print");
	lua_setglobal(L, "std_print"); //storing original impl as 'std_print'
	lua_settop(L, 0); //clear stack, just to be sure

	lua_pushcfunction(L, &dispatch<&lua_kernel_base::intf_print>);
	lua_setglobal(L, "print");

	//
	// now lua_getop(L) == 0
	//

	lua_pushcfunction(L, intf_load);
	lua_setglobal(L, "load");
	lua_pushcfunction(L, intf_loadstring);
	lua_setglobal(L, "loadstring");

	cmd_log_ << "Initializing package repository...\n";
	// Create the package table.
	lua_getglobal(L, "rose");
	lua_newtable(L);
	lua_setfield(L, -2, "package");
	lua_pop(L, 1);
	lua_settop(L, 0);
	lua_pushstring(L, "lua/package.lua");
	int res = intf_require(L);
	if(res != 1) {
		cmd_log_ << "Error: Failed to initialize package repository. Falling back to less flexible C++ implementation.\n";
	}

	// Get some callbacks for map locations
	cmd_log_ << "Adding map table...\n";

	static luaL_Reg const map_callbacks[] {
		// { "get_direction",		&lua_map_location::intf_get_direction         		},
		// { "vector_sum",			&lua_map_location::intf_vector_sum			},
		// { "vector_diff",			&lua_map_location::intf_vector_diff			},
		// { "vector_negation",		&lua_map_location::intf_vector_negation			},
		// { "rotate_right_around_center",	&lua_map_location::intf_rotate_right_around_center	},
		// { "tiles_adjacent",		&lua_map_location::intf_tiles_adjacent			},
		// { "get_adjacent_tiles",		&lua_map_location::intf_get_adjacent_tiles		},
		// { "distance_between",		&lua_map_location::intf_distance_between		},
		// { "get_in_basis_N_NE",		&lua_map_location::intf_get_in_basis_N_NE		},
		// { "get_relative_dir",		&lua_map_location::intf_get_relative_dir		},
		{ nullptr, nullptr }
	};

	// Create the map_location table.
	lua_getglobal(L, "rose");
	lua_newtable(L);
	luaL_setfuncs(L, map_callbacks, 0);
	lua_setfield(L, -2, "map");
	lua_pop(L, 1);

	// Create the game_config variable with its metatable.
	cmd_log_ << "Adding game_config table...\n";

	lua_getglobal(L, "rose");
	lua_newuserdata(L, 0);
	lua_createtable(L, 0, 3);
	lua_pushcfunction(L, &dispatch<&lua_kernel_base::impl_game_config_get>);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, &dispatch<&lua_kernel_base::impl_game_config_set>);
	lua_setfield(L, -2, "__newindex");
	lua_pushstring(L, "game config");
	lua_setfield(L, -2, "__metatable");
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "game_config");
	lua_pop(L, 1);

	// Add mersenne twister rng wrapper
	cmd_log_ << "Adding rng tables...\n";
/*
	lua_rng::load_tables(L);
*/
	cmd_log_ << "Adding name generator metatable...\n";

	luaL_newmetatable(L, Gen);
	static luaL_Reg const generator[] {
		// { "__call", &impl_name_generator_call},
		// { "__gc", &impl_name_generator_collect},
		{ nullptr, nullptr}
	};
	luaL_setfuncs(L, generator, 0);

	// Create formula bridge metatables
/*
	cmd_log_ << lua_formula_bridge::register_metatables(L);
*/
	// Loading ilua:
	cmd_log_ << "Loading ilua...\n";

	lua_settop(L, 0);
	luaW_getglobal(L, "rose", "require");
	lua_pushstring(L, "lua/ilua.lua");
	if(protected_call(1, 1)) {
		//run "ilua.set_strict()"
		lua_pushstring(L, "set_strict");
		lua_gettable(L, -2);
		if (!this->protected_call(0,0, std::bind(&lua_kernel_base::log_error, this, _1, _2))) {
			cmd_log_ << "Failed to activate strict mode.\n";
		} else {
			cmd_log_ << "Activated strict mode.\n";
		}

		lua_setglobal(L, "ilua"); //save ilua table as a global
	} else {
		cmd_log_ << "Error: failed to load ilua.\n";
	}

	// test(L);

	lua_settop(L, 0);
}

lua_kernel_base::~lua_kernel_base()
{
	lua_closing = true;
	lua_close(mState);
}

void lua_kernel_base::register_rose_const_value()
{
	lua_State *L = mState;

	tstack_size_lock lock(L, 0);
	lua_getglobal(L, "rose");

	std::map<std::string, int> values;
	values.insert(std::make_pair("STRCT_EXTRACT_DIRECTORY", STRCT_EXTRACT_DIRECTORY));

	values.insert(std::make_pair("STRFMT_UUID", STRFMT_UUID));

	// path type
	values.insert(std::make_pair("PATHTYPE_ABS", PATHTYPE_ABS));
	values.insert(std::make_pair("PATHTYPE_RES", PATHTYPE_RES));
	values.insert(std::make_pair("PATHTYPE_USERDATA", PATHTYPE_USERDATA));

	values.insert(std::make_pair("SPLIT_REMOVE_EMPTY", utils::REMOVE_EMPTY));
	values.insert(std::make_pair("SPLIT_STRIP_SPACES", utils::STRIP_SPACES));

	values.insert(std::make_pair("AES128", AES128));
	values.insert(std::make_pair("AES192", AES192));
	values.insert(std::make_pair("AES256", AES256));

	for (std::map<std::string, int>::const_iterator it = values.begin(); it != values.end(); ++ it) {
		lua_pushinteger(L, it->second);
		lua_setfield(L, -2, it->first.c_str());
	}
	lua_pop(L, 1);
}

void lua_kernel_base::log_error(char const * msg, char const * context)
{
	SDL_Log("%s: %s", context, msg);
}

void lua_kernel_base::throw_exception(char const * msg, char const * context)
{
	VALIDATE(false, null_str);
	// throw game::lua_error(msg, context);
}

bool lua_kernel_base::protected_call(int nArgs, int nRets)
{
	error_handler eh = std::bind(&lua_kernel_base::log_error, this, _1, _2 );
	return this->protected_call(nArgs, nRets, eh);
}

bool lua_kernel_base::load_string(char const * prog)
{
	return false;
/*
	error_handler eh = std::bind(&lua_kernel_base::log_error, this, _1, _2 );
	return this->load_string(prog, eh);
*/
}

bool lua_kernel_base::protected_call(int nArgs, int nRets, error_handler e_h)
{
	return this->protected_call(mState, nArgs, nRets, e_h);
}

bool lua_kernel_base::protected_call(lua_State * L, int nArgs, int nRets, error_handler e_h)
{
	int errcode = luaW_pcall_internal(L, nArgs, nRets);

	if (errcode != LUA_OK) {
		char const * msg = lua_tostring(L, -1);

		std::string context = "When executing, ";
		if (errcode == LUA_ERRRUN) {
			context += "Lua runtime error: ";
		} else if (errcode == LUA_ERRERR) {
			context += "Lua error in attached debugger: ";
		} else if (errcode == LUA_ERRMEM) {
			context += "Lua out of memory error: ";
		} else if (errcode == LUA_ERRERR) {
			context += "Lua error in garbage collection metamethod: ";
		} else {
			context += "unknown lua error: ";
		}
		if(lua_isstring(L, -1)) {
			context +=  msg ? msg : "null string";
		} else {
			context += lua_typename(L, lua_type(L, -1));
		}

		lua_pop(L, 1);

		e_h(context.c_str(), "Lua Error");

		return false;
	}
	return true;
}

bool lua_kernel_base::load_string(char const * prog, error_handler e_h)
{

	// pass 't' to prevent loading bytecode which is unsafe and can be used to escape the sandbox.
	// todo: maybe allow a 'name' parameter to give better error messages.
	// int errcode = luaL_loadbufferx(mState, prog, strlen(prog), /*name*/ prog, "t");
/*	if (errcode != LUA_OK) {
		char const * msg = lua_tostring(mState, -1);
		std::string message = msg ? msg : "null string";

		std::string context = "When parsing a string to lua, ";

		if (errcode == LUA_ERRSYNTAX) {
			context += " a syntax error";
		} else if(errcode == LUA_ERRMEM){
			context += " a memory error";
		} else if(errcode == LUA_ERRGCMM) {
			context += " an error in garbage collection metamethod";
		} else {
			context += " an unknown error";
		}

		lua_pop(mState, 1);

		e_h(message.c_str(), context.c_str());

		return false;
	}
*/
	return true;
}

void lua_kernel_base::run_lua_tag(const config& cfg)
{
/*
	int nArgs = 0;
	if (const config& args = cfg.child("args")) {
		luaW_pushconfig(this->mState, args);
		++nArgs;
	}
	this->run(cfg["code"].str().c_str(), nArgs);
*/
}
// Call load_string and protected call. Make them throw exceptions.
//
void lua_kernel_base::throwing_run(const char * prog, int nArgs)
{
/*
	cmd_log_ << "$ " << prog << "\n";
	error_handler eh = std::bind(&lua_kernel_base::throw_exception, this, _1, _2 );
	this->load_string(prog, eh);
	lua_insert(mState, -nArgs - 1);
	this->protected_call(nArgs, 0, eh);
*/
}

// Do a throwing run, but if we catch a lua_error, reformat it with signature for this function and log it.
void lua_kernel_base::run(const char * prog, int nArgs)
{
/*
	try {
		this->throwing_run(prog, nArgs);
	} catch (const game::lua_error & e) {
		cmd_log_ << e.what() << "\n";
		lua_kernel_base::log_error(e.what(), "In function lua_kernel::run()");
	}
*/
}

// Tests if a program resolves to an expression, and pretty prints it if it is, otherwise it runs it normally. Throws exceptions.
void lua_kernel_base::interactive_run(char const * prog) {
	std::string experiment = "ilua._pretty_print(";
	experiment += prog;
	experiment += ")";
/*
	error_handler eh = std::bind(&lua_kernel_base::throw_exception, this, _1, _2 );

	try {
		// Try to load the experiment without syntax errors
		this->load_string(experiment.c_str(), eh);
	} catch (const game::lua_error &) {
		this->throwing_run(prog, 0);	// Since it failed, fall back to the usual throwing_run, on the original input.
		return;
	}
	// experiment succeeded, now run but log normally.
	cmd_log_ << "$ " << prog << "\n";
	this->protected_call(0, 0, eh);
*/
}
/**
 * Loads and executes a Lua file.
 * - Arg 1: string containing the file name.
 * - Ret *: values returned by executing the file body.
 */
int lua_kernel_base::intf_dofile(lua_State* L)
{
	luaL_checkstring(L, 1);
	lua_rotate(L, 1, -1);
	if (lua_fileops::load_file(L) != 1) return 0;
	//^ should end with the file contents loaded on the stack. actually it will call lua_error otherwise, the return 0 is redundant.
	lua_rotate(L, 1, 1);
	// Using a non-protected call here appears to fix an issue in plugins.
	// The protected call isn't technically necessary anyway, because this function is called from Lua code,
	// which should already be in a protected environment.
	lua_call(L, lua_gettop(L) - 1, LUA_MULTRET);
	return lua_gettop(L);
}

/**
 * Loads and executes a Lua file, if there is no corresponding entry in rose.package.
 * Stores the result of the script in rose.package and returns it.
 * - Arg 1: string containing the file name.
 * - Ret 1: value returned by the script.
 */
int lua_kernel_base::intf_require(lua_State* L)
{
	const char * m = luaL_checkstring(L, 1);
	if(!m) {
		return luaL_argerror(L, 1, "found a null string argument to rose require");
	}

	// Check if there is already an entry.

	lua_getglobal(L, "rose");
	lua_pushstring(L, "package");
	lua_rawget(L, -2);
	lua_pushvalue(L, 1);
	lua_rawget(L, -2);
	if(!lua_isnil(L, -1)) {
		return 1;
	}
	lua_pop(L, 1);
	lua_pushvalue(L, 1);
	// stack is now [packagename] [rose] [package] [packagename]

	if(lua_fileops::load_file(L) != 1) {
		// should end with the file contents loaded on the stack. actually it will call lua_error otherwise, the return 0 is redundant.
		// stack is now [packagename] [rose] [package] [chunk]
		return 0;
	}

	SDL_Log("require: loaded a file, now calling it");

	if (!this->protected_call(L, 0, 1, std::bind(&lua_kernel_base::log_error, this, _1, _2))) {
		// historically if rose.require fails it just yields nil and some logging messages, not a lua error
		return 0;
    }
	// stack is now [packagename] [rose] [package] [results]

	lua_pushvalue(L, 1);
	lua_pushvalue(L, -2);
	// stack is now [packagename] [rose] [package] [results] [packagename] [results]
	// Add the return value to the table.

	lua_settable(L, -4);

	// stack is now [packagename] [rose] [package] [results]
	return 1;
}
int lua_kernel_base::intf_kernel_type(lua_State* L)
{
/*
	lua_push(L, my_name());
*/
	return 1;
}
int lua_kernel_base::impl_game_config_get(lua_State* L)
{
/*
	char const *m = luaL_checkstring(L, 2);
	return_int_attrib("base_income", game_config::base_income);
	return_int_attrib("village_income", game_config::village_income);
	return_int_attrib("village_support", game_config::village_support);
	return_int_attrib("poison_amount", game_config::poison_amount);
	return_int_attrib("rest_heal_amount", game_config::rest_heal_amount);
	return_int_attrib("recall_cost", game_config::recall_cost);
	return_int_attrib("kill_experience", game_config::kill_experience);
	return_string_attrib("version", game_config::version);
	return_bool_attrib("debug", game_config::debug);
	return_bool_attrib("debug_lua", game_config::debug_lua);
	return_bool_attrib("mp_debug", game_config::mp_debug);
*/
	return 0;
}
int lua_kernel_base::impl_game_config_set(lua_State* L)
{
	std::string err_msg = "unknown modifiable property of game_config: ";
	err_msg += luaL_checkstring(L, 2);
	return luaL_argerror(L, 2, err_msg.c_str());
}
/**
 * Loads the "package" package into the Lua environment.
 * This action is inherently unsafe, as Lua scripts will now be able to
 * load C libraries on their own, hence granting them the same privileges
 * as the rose binary itself.
 */
void lua_kernel_base::load_package()
{
	lua_State *L = mState;
	lua_pushcfunction(L, luaopen_package);
	lua_pushstring(L, "package");
	lua_call(L, 1, 0);
}

void lua_kernel_base::load_core()
{
	lua_State* L = mState;
	lua_settop(L, 0);
	cmd_log_ << "Loading core...\n";
	luaW_getglobal(L, "rose", "require");
	lua_pushstring(L, "lua/core.lua");
	if(!protected_call(1, 1)) {
		cmd_log_ << "Error: Failed to load core.\n";
	}
	lua_settop(L, 0);
}

void lua_kernel_base::load_lua(const std::string& lua)
{
	VALIDATE(lua.find("lua/") == 0, null_str);

	lua_State* L = mState;
	lua_settop(L, 0);
	luaW_getglobal(L, "rose", "require");
	lua_pushstring(L, lua.c_str());
	if(!protected_call(1, 1)) {
		cmd_log_ << "Error: Failed to load core.\n";
	}
	lua_settop(L, 0);
}

/**
 * Gets all the global variable names in the Lua environment. This is useful for tab completion.
 */
std::vector<std::string> lua_kernel_base::get_global_var_names()
{
	std::vector<std::string> ret;

	lua_State *L = mState;

	int idx = lua_gettop(L);
	lua_getglobal(L, "_G");
	lua_pushnil(L);

	while (lua_next(L, idx+1) != 0) {
		if (lua_isstring(L, -2)) {
			ret.push_back(lua_tostring(L,-2));
		}
		lua_pop(L,1);
	}
	lua_settop(L, idx);
	return ret;
}

/**
 * Gets all attribute names of an extended variable name. This is useful for tab completion.
 */
std::vector<std::string> lua_kernel_base::get_attribute_names(const std::string & input)
{
	std::vector<std::string> ret;
	std::string base_path = input;
	size_t last_dot = base_path.find_last_of('.');
	std::string partial_name = base_path.substr(last_dot + 1);
	base_path.erase(last_dot);
	std::string load = "return " + base_path;

	lua_State* L = mState;
	int save_stack = lua_gettop(L);
	int result = luaL_loadstring(L, load.c_str());
	if(result != LUA_OK) {
		// This isn't at error level because it's a really low priority error; it just means the user tried to tab-complete something that doesn't exist.
		SDL_Log("Error when attempting tab completion:");
		SDL_Log("%s", luaL_checkstring(L, -1));
		// Just return an empty list; no matches were found
		lua_settop(L, save_stack);
		return ret;
	}
/*
	luaW_pcall(L, 0, 1);
	if(lua_istable(L, -1) || lua_isuserdata(L, -1)) {
		int top = lua_gettop(L);
		int obj = lua_absindex(L, -1);
		if(luaL_getmetafield(L, obj, "__tab_enum") == LUA_TFUNCTION) {
			lua_pushvalue(L, obj);
			lua_pushlstring(L, partial_name.c_str(), partial_name.size());
			luaW_pcall(L, 2, 1);
			ret = lua_check<std::vector<std::string>>(L, -1);
		} else if(lua_type(L, -1) != LUA_TTABLE) {
			SDL_Log("Userdata missing __tab_enum meta-function for tab completion");
			lua_settop(L, save_stack);
			return ret;
		} else {
			lua_settop(L, top);
			// Metafunction not found, so use lua_next to enumerate the table
			for(lua_pushnil(L); lua_next(L, obj); lua_pop(L, 1)) {
				if(lua_type(L, -2) == LUA_TSTRING) {
					std::string attr = lua_tostring(L, -2);
					if(attr.empty()) {
						continue;
					}
					if(!isalpha(attr[0]) && attr[0] != '_') {
						continue;
					}
					if(std::any_of(attr.begin(), attr.end(), [](char c){
						return !isalpha(c) && !isdigit(c) && c != '_';
					})) {
						continue;
					}
					if(attr.substr(0, partial_name.size()) == partial_name) {
						ret.push_back(base_path + "." + attr);
					}
				}
			}
		}
	}
*/
	lua_settop(L, save_stack);
	return ret;
}

lua_kernel_base*& lua_kernel_base::get_lua_kernel_base_ptr(lua_State *L)
{
	#ifdef __GNUC__
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wold-style-cast"
	#endif
	return *reinterpret_cast<lua_kernel_base**>(lua_getextraspace(L));
	#ifdef __GNUC__
		#pragma GCC diagnostic pop
	#endif
}

uint32_t lua_kernel_base::get_random_seed()
{
	return 0;
	// return seed_rng::next_seed();
}

void lua_kernel_base::call_lua_breakpoint()
{
	lua_State* L = mState;
	tstack_size_lock lock(L, 0);

	luaW_getglobal(L, "rose", "lua_breakpoint");
	protected_call(0, 0);
}

extern const TValue* lua_toTValue (lua_State *L, int idx);
void lua_kernel_base::dump_stack(const std::string& scene)
{
	lua_State* L = mState;
	int i;
	int top = lua_gettop(L);
	SDL_Log("---%s, stack's size %i---", scene.c_str(), top);
	for (i = 1; i <= top; i ++) {
		int t = lua_type(L, i);
		// char buf[32];
		switch(t) {
		case LUA_TSTRING:
			SDL_Log("[#%i]{%p}TSTRING, '%s'", i, lua_toTValue(L, i), lua_tostring(L, i));
			break;
		case LUA_TBOOLEAN:
			SDL_Log("[#%i]{%p}TBOOLEAN, %s", i, lua_toTValue(L, i), lua_toboolean(L, i)? "true": "false");
			break;
		case LUA_TNUMBER:
			if (lua_isinteger(L, i)) {
				SDL_Log("[#%i]{%p}TNUMBER.integer, %lld", i, lua_toTValue(L, i), lua_tointeger(L, i));
			} else {
				SDL_Log("[#%i]{%p}TNUMBER.float, %7.2f", i, lua_toTValue(L, i), lua_tonumber(L, i));
			}
			break;
		case LUA_TFUNCTION:
			SDL_Log("[#%i]{%p}function", i, lua_toTValue(L, i));
			break;
		default:
			SDL_Log("[#%i]{%p}%s", i, lua_toTValue(L, t), lua_typename(L, t));
			break;
		}
	}
	SDL_Log("-------------------------");
}
