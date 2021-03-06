/*
   Copyright (C) 2003 by David White <dave@whitevine.net>
   Copyright (C) 2005 by Guillaume Melquiond <guillaume.melquiond@gmail.com>
   Copyright (C) 2005 - 2015 by Philippe Plantier <ayin@anathas.org>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/**
 * @file
 * Read/Write & analyze WML- and config-files.
 */


#include "serialization/parser.hpp"

#include "config.hpp"
#include "gettext.hpp"
#include "loadscreen.hpp"
#include "serialization/preprocessor.hpp"
#include "serialization/tokenizer.hpp"
#include "serialization/string_utils.hpp"
#include "serialization/validator.hpp"
#include "filesystem.hpp"

#include <stack>

#include <boost/algorithm/string/replace.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/bzip2.hpp>
#include <boost/iostreams/filter/gzip.hpp>

static const size_t max_recursion_levels = 1000;

namespace {
class parser
{
	parser();
	parser(const parser&);
	parser& operator=(const parser&);
public:
	parser(config& cfg, std::istream& in,
		   abstract_validator * validator = NULL);
	~parser();
	void operator()();

private:
	void parse_element();
	void parse_variable();
	std::string lineno_string(utils::string_map &map, std::string const &lineno,
		const std::string &error_string,
		const std::string &hint_string = "",
		const std::string &debug_string = "");
	void error(const std::string& message, const std::string& pos_format = "");

	config& cfg_;
	tokenizer *tok_;
	abstract_validator *validator_;

	struct element {
		element(config *cfg, std::string const &name,
			int start_line = 0, const std::string &file = "") :
			cfg(cfg), name(name), start_line(start_line), file(file)
		{}

		config* cfg;
		std::string name;
		int start_line;
		std::string file;
	};

	std::stack<element> elements;
};

parser::parser(config &cfg, std::istream &in, abstract_validator * validator)
			   :cfg_(cfg),
			   tok_(new tokenizer(in)),
			   validator_(validator),
			   elements()
{
}


parser::~parser()
{
	delete tok_;
}

void parser::operator()()
{
	cfg_.clear();
	elements.push(element(&cfg_, ""));

	do {
		tok_->next_token();

		switch(tok_->current_token().type) {
		case token::LF:
			continue;
		case '[':
			parse_element();
			break;
		case token::STRING:
			parse_variable();
			break;
		default:
			if (static_cast<unsigned char>(tok_->current_token().value[0]) == 0xEF &&
			    static_cast<unsigned char>(tok_->next_token().value[0])    == 0xBB &&
			    static_cast<unsigned char>(tok_->next_token().value[0])    == 0xBF)
			{
				utils::string_map i18n_symbols;
				std::stringstream ss;
				ss << tok_->get_start_line() << " " << tok_->get_file();
				// some file can enter it.
/*
				VALIDATE(false, lineno_string(i18n_symbols,
										ss.str(),
										"Skipping over a utf8 BOM at $pos"));
*/
			} else {
				error(_("Unexpected characters at line start"));
			}
			break;
		case token::END:
			break;
		}
		loadscreen::increment_progress();
	} while (tok_->current_token().type != token::END);

	// The main element should be there. If it is not, this is a parser error.
	assert(!elements.empty());

	if(elements.size() != 1) {
		utils::string_map i18n_symbols;
		i18n_symbols["tag"] = elements.top().name;
		std::stringstream ss;
		ss << elements.top().start_line << " " << elements.top().file;
		error(lineno_string(i18n_symbols, ss.str(),
				_("Missing closing tag for tag [$tag]"),
				_("expected at $pos")), _("opened at $pos"));
	}
}

void parser::parse_element()
{
	tok_->next_token();
	std::string elname;
	config* current_element = NULL;
	switch(tok_->current_token().type) {
	case token::STRING: // [element]
		elname = tok_->current_token().value;
		if (tok_->next_token().type != ']')
			error(_("Unterminated [element] tag"));
		// Add the element
		if (!preprocessor_checksum) {
			current_element = &(elements.top().cfg->add_child(elname));
		}
		elements.push(element(current_element, elname, tok_->get_start_line(), tok_->get_file()));
		if (validator_){
			validator_->open_tag(elname,tok_->get_start_line(),
								  tok_->get_file());
		}
		break;

	case '+': // [+element]
		if (tok_->next_token().type != token::STRING)
			error(_("Invalid tag name"));
		elname = tok_->current_token().value;
		if (tok_->next_token().type != ']')
			error(_("Unterminated [+element] tag"));

		if (!preprocessor_checksum) {
			// Find the last child of the current element whose name is
			// element
			if (config &c = elements.top().cfg->child(elname, -1)) {
				current_element = &c;
				if (validator_){
					validator_->open_tag(elname,tok_->get_start_line(),
										 tok_->get_file(),true);
				}
			} else {
				current_element = &elements.top().cfg->add_child(elname);
				if (validator_){
					validator_->open_tag(elname,tok_->get_start_line(),
										 tok_->get_file());
				}
			}
		}
		elements.push(element(current_element, elname, tok_->get_start_line(), tok_->get_file()));
		break;

	case '/': // [/element]
		if(tok_->next_token().type != token::STRING)
			error(_("Invalid closing tag name"));
		elname = tok_->current_token().value;
		if(tok_->next_token().type != ']')
			error(_("Unterminated closing tag"));
		if(elements.size() <= 1)
			error(_("Unexpected closing tag"));
		if(elname != elements.top().name) {
			utils::string_map i18n_symbols;
			i18n_symbols["tag1"] = elements.top().name;
			i18n_symbols["tag2"] = elname;
			std::stringstream ss;
			ss << elements.top().start_line << " " << elements.top().file;
			error(lineno_string(i18n_symbols, ss.str(),
					_("Found invalid closing tag [/$tag2] for tag [$tag1]"),
					_("opened at $pos")), _("closed at $pos"));
		}
		if(validator_){
			element & el= elements.top();
			validator_->validate(*el.cfg,el.name,el.start_line,el.file);
			validator_->close_tag();
		}
		elements.pop();
		break;
	default:
		error(_("Invalid tag name"));
	}
}

void parser::parse_variable()
{
	config* cfg = elements.top().cfg;
	VALIDATE(cfg || preprocessor_checksum, null_str);
	std::vector<std::string> variables;
	variables.push_back("");

	while (tok_->current_token().type != '=') {
		switch(tok_->current_token().type) {
		case token::STRING:
			if(!variables.back().empty())
				variables.back() += ' ';
			variables.back() += tok_->current_token().value;
			break;
		case ',':
			if(variables.back().empty()) {
				error(_("Empty variable name"));
			} else {
				variables.push_back("");
			}
			break;
		default:
			error(_("Unexpected characters after variable name (expected , or =)"));
			break;
		}
		tok_->next_token();
	}
	if(variables.back().empty())
		error(_("Empty variable name"));

	t_string_base buffer;
	bool is_QSTRING = false;

	std::vector<std::string>::const_iterator curvar = variables.begin();

	bool ignore_next_newlines = false, previous_string = false;
	while(1) {
		tok_->next_token();
		assert(curvar != variables.end());

		switch (tok_->current_token().type) {
		case ',':
			is_QSTRING = false;
			if ((curvar+1) != variables.end()) {
				if (cfg) {
					if (buffer.translatable())
						(*cfg)[*curvar] = t_string(buffer);
					else
						(*cfg)[*curvar] = buffer.value();
					if(validator_){
						validator_->validate_key (*cfg,*curvar,buffer.value(),
												  tok_->get_start_line (),
												  tok_->get_file ());
					}
				}
				buffer = t_string_base();
				++curvar;
			} else {
				buffer += ",";
			}
			break;
		case '_':
			is_QSTRING = false;
			tok_->next_token();
			switch (tok_->current_token().type) {
			case token::UNTERMINATED_QSTRING:
				error(_("Unterminated quoted string"));
				break;
			case token::QSTRING:
				buffer += t_string_base(tok_->current_token().value, tok_->textdomain());
				break;
			default:
				buffer += "_";
				buffer += tok_->current_token().value;
				break;
			case token::END:
			case token::LF:
				buffer += "_";
				goto finish;
			}
			break;
		case '+':
			ignore_next_newlines = true;
			continue;
		case token::STRING:
			is_QSTRING = false;
			if (previous_string) buffer += " ";
			//nobreak
		default:
			buffer += tok_->current_token().value;
			break;
		case token::QSTRING:
			// is_QSTRING is true require both first and last is token::QSTRING.
			is_QSTRING = buffer.empty();
			buffer += tok_->current_token().value;
			break;
		case token::UNTERMINATED_QSTRING:
			error(_("Unterminated quoted string"));
			break;
		case token::LF:
			if (ignore_next_newlines) continue;
			//nobreak
		case token::END:
			goto finish;
		}

		previous_string = tok_->current_token().type == token::STRING;
		ignore_next_newlines = false;
	}

finish:
	if (cfg) {
		if (buffer.translatable()) {
			(*cfg)[*curvar] = t_string(buffer);
		} else {
			// if value has QSTRING(" "), use config.from_string() explicited.
			if (is_QSTRING) {
				(*cfg)[*curvar].from_string(buffer.value(), true);
			} else {
				(*cfg)[*curvar] = buffer.value();
			}
		}
		if(validator_){
			validator_->validate_key (*cfg,*curvar,buffer.value(),
									  tok_->get_start_line (),
									  tok_->get_file ());
		}
		while (++curvar != variables.end()) {
			(*cfg)[*curvar] = "";
		}
	}
}

/**
 * This function is crap. Don't use it on a string_map with prefixes.
 */
std::string parser::lineno_string(utils::string_map &i18n_symbols,
								  std::string const &lineno,
								  std::string const &error_string,
								  std::string const &hint_string,
								  std::string const &debug_string)
{
	i18n_symbols["pos"] = ::lineno_string(lineno);
	std::string result = error_string;

	if(!hint_string.empty()) {
		result += '\n' + hint_string;
	}

	if(!debug_string.empty()) {
		result += '\n' + debug_string;
	}

	BOOST_FOREACH(utils::string_map::value_type& var, i18n_symbols)
		boost::algorithm::replace_all(result, std::string("$") + var.first, std::string(var.second));
	return result;
}

void parser::error(const std::string& error_type, const std::string& pos_format)
{
	std::string hint_string = pos_format;

	if(hint_string.empty()) {
		hint_string = _("at $pos");
	}

	utils::string_map i18n_symbols;
	i18n_symbols["error"] = error_type;

	std::stringstream ss;
	ss << tok_->get_start_line() << " " << tok_->get_file();

#ifdef DEBUG
	i18n_symbols["value"] = tok_->current_token().value;
	i18n_symbols["previous_value"] = tok_->previous_token().value;

	const std::string& tok_state =
		_("Value: '$value' Previous: '$previous_value'");
#else
	const std::string& tok_state = "";
#endif

	const std::string& message =
		lineno_string(i18n_symbols, ss.str(), "$error", hint_string, tok_state);

	throw twml_exception(message);
}

} // end anon namespace

void read(config &cfg, std::istream &in, abstract_validator * validator)
{
	parser(cfg, in, validator)();
}

void read(config &cfg, const std::string &in, abstract_validator * validator)
{
	std::istringstream ss(in);
	parser(cfg, ss, validator)();
}

template <typename decompressor>
void read_compressed(config &cfg, std::istream &file, abstract_validator * validator)
{
	//an empty gzip file seems to confuse boost on msvc
	//so return early if this is the case
	if (file.peek() == EOF) {
		return;
	}
	boost::iostreams::filtering_stream<boost::iostreams::input> filter;
	filter.push(decompressor());
	filter.push(file);



	// This causes especially gzip_error (and the corresponding bz2 error), std::ios_base::failure to be thrown here.
	// save_index_class::data expects that and config_cache::read_cache and other functions are also capable of catching.
	// Note that parser(cuff, filter,validator)(); -> tokenizer::tokenizer can throw exeptions too (meaning this functions did already throw these exceptions before this patch).
	// We try to fix https://svn.boost.org/trac/boost/ticket/5237 by not creating empty gz files.
	filter.exceptions(filter.exceptions() | std::ios_base::badbit);

	/*
	 * It sometimes seems the file is not empty but still no real data.
	 * Filter that case here. It might be previous test is no longer required
	 * but simply keep it.
	 */

	// on msvc filter.peek() != EOF does not imply filter.good().
	// we never create empty compressed gzip files because boosts gzip fails at doing that.
	// but empty compressed bz2 files are possible.
	if(filter.peek() == EOF) {
		// Empty compressed file or error at reading a compressed file.
		return;
	}
	

	if (!filter.good()) {
		// filter.peek() != EOF but !filter.good(), this indicates a malformed gz stream, and can make wesnoth crash.
	}

	parser(cfg, filter,validator)();
}

/// might throw a std::ios_base::failure especially a gzip_error
void read_gz(config &cfg, std::istream &file, abstract_validator * validator)
{
	read_compressed<boost::iostreams::gzip_decompressor>(cfg, file, validator);
}

/// might throw a std::ios_base::failure especially bzip2_error 
void read_bz2(config &cfg, std::istream &file, abstract_validator * validator)
{
	read_compressed<boost::iostreams::bzip2_decompressor>(cfg, file, validator);
}

namespace { // helpers for write_key_val().
	/**
	 * Copies a string fragment and converts it to a suitable format for WML.
	 * (I.e., quotes are doubled.)
	 */
	std::string escaped_string(const std::string::const_iterator &begin,
	                           const std::string::const_iterator &end)
	{
		std::string res;
		std::string::const_iterator iter = begin;
		while ( iter != end ) {
			const char c = *iter;
			res.append(c == '"' ? 2 : 1, c);
			++iter;
		}
		return res;
	}
	/**
	 * Copies a string and converts it to a suitable format for WML.
	 * (I.e., quotes are doubled.)
	 */
	inline std::string escaped_string(const std::string &value)
	{
		return escaped_string(value.begin(), value.end());
	}

	class write_key_val_visitor
	{
		std::ostream &out_;
		const unsigned level_;
		std::string &textdomain_;
		const std::string &key_;

	public:
		write_key_val_visitor(std::ostream &out, unsigned level,
			std::string &textdomain, const std::string &key, const config::attribute_value &value)
			: out_(out), level_(level), textdomain_(textdomain), key_(key)
		{
			int type = value.type();
			if (type == config::attribute_value::type_bool || type == config::attribute_value::type_integer || type == config::attribute_value::type_double) {
				indent(); 
				out_ << key_ << '=' << value.str();
			} else if (type == config::attribute_value::type_string) {
				operator_str(value.str_ref());

			} else if (type == config::attribute_value::type_tstring) {
				operator_tstr(value.t_str_ref());

			} else {
				VALIDATE(type == config::attribute_value::type_blank, null_str);
				operator_blank();
			}
		}

		// Specialized visitors for things that go in quotes:
		void operator_blank() const
		{ indent(); out_ << key_ << '=' << "\"\""; }

		void operator_str(std::string const &s) const
		{ indent(); out_ << key_ << '=' << '"' << escaped_string(s) << '"'; }

		void operator_tstr(t_string const &s) const;

	private:
		void indent() const
		{ for ( unsigned i = 0; i < level_; ++i )  out_ << '\t'; }
	};

	/**
	 * Writes all the parts of a translatable string.
	 * @note If the first part is translatable and in the wrong textdomain,
	 *       the textdomain change has to happen before the attribute name.
	 *       That is the reason for not outputting the key beforehand and
	 *       letting this function do it.
	 */
	void write_key_val_visitor::operator_tstr(t_string const &value) const
	{
		bool first = true;

		for (t_string::walker w(value); !w.eos(); w.next())
		{
			if (!first)
				out_ << " +\n";

			if (w.translatable() && w.textdomain() != textdomain_) {
				textdomain_ = w.textdomain();
				out_ << "#textdomain " << textdomain_ << '\n';
			}

			indent();

			if (first)
				out_ << key_ << '=';
			else
				out_ << '\t';

			if (w.translatable())
				out_ << '_';

			out_ << '"' << escaped_string(w.begin(), w.end()) << '"';
			first = false;
		}
	}
}//unnamed namespace for write_key_val() helpers.

void write_key_val(std::ostream &out, const std::string &key,
                   const config::attribute_value &value, unsigned level,
                   std::string& textdomain)
{
	// value.apply_visitor(write_key_val_visitor(out, level, textdomain, key));
	write_key_val_visitor(out, level, textdomain, key, value);

	out << '\n';
}

void write_open_child(std::ostream &out, const std::string &child, unsigned int level)
{
	out << std::string(level, '\t') << '[' << child << "]\n";
}

void write_close_child(std::ostream &out, const std::string &child, unsigned int level)
{
	out << std::string(level, '\t') << "[/" << child << "]\n";
}

static void write_internal(config const &cfg, std::ostream &out, std::string& textdomain, size_t tab = 0)
{
	if (tab > max_recursion_levels)
		throw twml_exception("Too many recursion levels in config write");

	BOOST_FOREACH(const config::attribute &i, cfg.attribute_range()) {
		write_key_val(out, i.first, i.second, tab, textdomain);
	}

	BOOST_FOREACH(const config::any_child &item, cfg.all_children_range())
	{
		write_open_child(out, item.key, tab);
		write_internal(item.cfg, out, textdomain, tab + 1);
		write_close_child(out, item.key, tab);
	}
}

void write(std::ostream &out, config const &cfg, unsigned int level, const std::string& def_textdomain)
{
	std::string textdomain = def_textdomain;
	write_internal(cfg, out, textdomain, level);
}

template <typename compressor>
void write_compressed(std::ostream &out, config const &cfg)
{
	boost::iostreams::filtering_stream<boost::iostreams::output> filter;
	filter.push(compressor());
	filter.push(out);

	write(filter, cfg);
	// prevent empty gz files because of https://svn.boost.org/trac/boost/ticket/5237
	filter << "\n";
}

void write_gz(std::ostream &out, config const &cfg)
{
	write_compressed<boost::iostreams::gzip_compressor>(out, cfg);
}

void write_bz2(std::ostream &out, config const &cfg)
{
	write_compressed<boost::iostreams::bzip2_compressor>(out, cfg);
}
