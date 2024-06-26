#include "htmlParser.hpp"

namespace html {

std::vector<std::string> inline_tags = {"b", "big", "i", "small", "tt",
	"abbr", "acronym", "cite", "code", "dfn", "em", "kbd", "strong", "samp",
	"time", "var", "a", "bdo", "br", "img", "map", "object", "q",
	"span", "sub", "sup", "button", "input", "label", "select", "textarea"};

std::vector<std::string> void_tags = {"area", "base", "br", "col", "embed",
	"hr", "img", "input", "link", "meta", "param", "source", "track", "wbr"};

std::vector<std::string> rawtext_tags = {"title", "textarea", "style", "script",
	"noscript", "plaintext", "iframe", "xmp", "noembed", "noframes"};

selector::selector(std::string s) {
	selector_matcher matcher;        // Example: Extract the text content of a specific element

	condition match_condition;
	char c;
	bool reconsume = false;
	int state = SEL_STATE_TAG;
	if(s == "*") {
		matcher.all_match = true;
		matchers.push_back(std::move(matcher));
		return;
	}
	auto it = s.begin();
	auto getc = [&]() -> char {
		if(it != s.end()) {
			return *it++;
		}
		return 0;
	};
	auto save_matcher = [&]() {
		if(!matcher.conditions.empty()) {
			matchers.push_back(std::move(matcher));
		}
	};
	auto save_cond = [&](const std::string& str) {
		if(!str.empty()) {
			if(matcher.conditions.empty()) {
				matcher.conditions.emplace_back();
			}
			matcher.conditions.back().push_back(std::move(match_condition));
		}
	};
	do {
		if(!reconsume) {
			c = getc();
		}
		reconsume = false;
		switch(state) {
			case SEL_STATE_ROUTE:
				if(c == 0 || c == ' ') {
					save_matcher();
					state = SEL_STATE_TAG;
				} else if(c == '[') {
					state = SEL_STATE_ATTR;
				} else if(c == ':') {
					state = SEL_STATE_OPERATOR;
				} else if(c == '.') {
					state = SEL_STATE_CLASS;
				} else if(c == '#') {
					state = SEL_STATE_ID;
				} else if(c == ',') {
					matcher.conditions.emplace_back();
					state = SEL_STATE_TAG;
				}
			break;
			case SEL_STATE_TAG:
				if(c == 0 || c == ' ' || c == '[' || c == ':' || c == '.' || c == '#' || c == ',') {
					save_cond(match_condition.tag_name);
					reconsume = true;
					state = SEL_STATE_ROUTE;
				} else if(IS_UPPERCASE_ALPHA(c)) {
					match_condition.tag_name += std::tolower(c);
				} else {
					match_condition.tag_name += c;
				}
			break;
			case SEL_STATE_CLASS:
				if(c == 0 || c == ' ' || c == '[' || c == ':' || c == '.' || c == '#' || c == ',') {
					save_cond(match_condition.class_name);
					reconsume = true;
					state = SEL_STATE_ROUTE;
				} else {
					match_condition.class_name += c;
				}
			break;
			case SEL_STATE_ID:
				if(c == 0 || c == ' ' || c == '[' || c == ':' || c == '.' || c == '#' || c == ',') {
					save_cond(match_condition.id);
					reconsume = true;
					state = SEL_STATE_ROUTE;
				} else {
					match_condition.id += c;
				}
			break;
			case SEL_STATE_OPERATOR:
				if(c == 0 || c == ' ' || c == '[' || c == ':' || c == '.' || c == '#' || c == ',') {
					save_cond(match_condition.attr_operator);
					reconsume = true;
					state = SEL_STATE_ROUTE;
				} else if(c == '(') {
					state = SEL_STATE_INDEX;
				} else if(IS_UPPERCASE_ALPHA(c)) {
					match_condition.attr_operator += std::tolower(c);
				} else {
					match_condition.attr_operator += c;
				}
			break;
			case SEL_STATE_INDEX:
				if(c == ')') {
					save_cond(match_condition.index);
					state = SEL_STATE_ROUTE;
				} else if(IS_DIGIT(c)) {
					match_condition.index += c;
				}
			break;
			case SEL_STATE_ATTR:
				if(c == ']') {
					save_cond(match_condition.attr);
					state = SEL_STATE_ROUTE;
				} else if(c == '=' || c == '*' || c == '^' || c == '$' || c == '!') {
					reconsume = true;
					state = SEL_STATE_ATTR_OPERATOR;
				} else if(IS_UPPERCASE_ALPHA(c)) {
					match_condition.attr += std::tolower(c);
				} else {
					match_condition.attr += c;
				}
			break;
			case SEL_STATE_ATTR_OPERATOR:
				if(c == '\'') {
					state = SEL_STATE_ATTR_VAL;
				} else {
					match_condition.attr_operator += c;
				}
			break;
			case SEL_STATE_ATTR_VAL:
				if(c == '\'') {
					save_cond(match_condition.attr_operator);
					state = SEL_STATE_ATTR;
				} else {
					match_condition.attr_value += c;
				}
			break;
		}
	} while(c || reconsume);
}

selector::condition::condition(condition&& c)
	: tag_name(std::move(c.tag_name))
	, id(std::move(c.id))
	, class_name(std::move(c.class_name))
	, index(std::move(c.index))
	, attr(std::move(c.attr))
	, attr_value(std::move(c.attr_value))
	, attr_operator(std::move(c.attr_operator)) {
	c.tag_name.clear();
	c.id.clear();
	c.class_name.clear();
	c.index.clear();
	c.attr.clear();
	c.attr_value.clear();
	c.attr_operator.clear();
}

selector::selector_matcher::selector_matcher(selector_matcher&& m)
	: all_match(m.all_match)
	, conditions(std::move(m.conditions)) {
	m.all_match = false;
	m.conditions.clear();
}

bool selector::condition::operator()(const node& d) const {
	if(!tag_name.empty()) {
		return d.tag_name == tag_name;
	}
	if(!id.empty()) {
		auto it = d.attributes.find("id");
		if(it != d.attributes.end()) {
			return it->second == id;
		}
	}
	if(!class_name.empty()) {
		auto it = d.attributes.find("class");
		if(it != d.attributes.end()) {
			return it->second == class_name;
		}
	}
	if(attr_operator == "first") {
		return d.index == 0;
	}
	if(attr_operator == "last") {
		return d.index == d.parent->node_count - 1;
	}
	if(attr_operator == "eq") {
		return d.index == std::stoi(index);
	}
	if(attr_operator == "gt") {
		return d.index > std::stoi(index);
	}
	if(attr_operator == "lt") {
		return d.index < std::stoi(index);
	}
	if(!attr.empty()) {
		auto it = d.attributes.find(attr);
		if(it == d.attributes.end()) {
			return attr_operator == "!=";
		}
		if(attr_operator == "=") {
			return it->second == attr_value;
		} else if(attr_operator == "^=") {
			return it->second.find(attr_value) == 0;
		} else if(attr_operator == "$=") {
			return attr_value.size() <= it->second.size() && it->second.find(attr_value) == it->second.size() - attr_value.size();
		} else if(attr_operator == "!=") {
			return it->second != attr_value;
		} else if(attr_operator == "*=") {
			return it->second.find(attr_value) != std::string::npos;
		}
		return true;
	}
	return false;
}

bool selector::selector_matcher::operator()(const node& d) const {
	if(d.type_node != node_t::tag) {
		return false;
	}
	if(this->all_match) {
		return true;
	}
	for(auto& c : conditions) {
		size_t i = 0;
		for(; i < c.size(); i++) {
			if(!c[i](d)) {
				break;
			}
		}
		if(i == c.size()) {
			return true;
		}
	}
	return false;
}

node::node(node&& d)
	: tag_name(std::move(d.tag_name))
	, content(std::move(d.content))
	, attributes(std::move(d.attributes))
	, parent(std::move(d.parent))
	, children(std::move(d.children))
	, index(d.index)
	, node_count(d.node_count) {
	d.tag_name.clear();
	d.attributes.clear();
	d.content.clear();
	d.parent = nullptr;
	d.children.clear();
	d.index = 0;
	d.node_count = 0;
}

node& node::operator=(node&& d) {
	attributes = std::move(d.attributes);
	tag_name = std::move(d.tag_name);
	content = std::move(d.content);
	parent = std::move(d.parent);
	children = std::move(d.children);
	d.tag_name.clear();
	d.attributes.clear();
	d.content.clear();
	d.parent = nullptr;
	d.children.clear();
	return *this;
}

void node::walk(std::function<bool(node&)> handler) {
	walk(*this, handler);
}

void node::walk(node& d, std::function<bool(node&)> handler) {
	if(handler(d)) {
		for(auto& c : d.children) {
			walk(*c, handler);
		}
	}
}

node_ptr node::select(const selector s, bool nested) {
	auto matched_dom = shared_from_this();
	size_t msize = s.matchers.size();
	size_t i = 0;
	for(auto& matcher : s) {
		auto selectee_dom = std::move(matched_dom);
		matched_dom = std::make_shared<node>();
		walk(*selectee_dom, [&](node& n) {
			if(matcher(n)) {
				if(i < msize - 1) {
					matched_dom->children = n.children;
					return false;
				} else {
					matched_dom->children.push_back(n.shared_from_this());
					return nested;
				}
			}
			return true;
		});
		i++;
	}
	return matched_dom;
}

void node::to_html(std::ostream& out, bool child, bool text, int level, int& deep, char ind, bool& last_is_block, bool& sibling_is_block) const {
	std::streamoff pos = out.tellp();
	if(type_node == node_t::none) {
		for(auto& c : children) {
			c->to_html(out, child, text, 0, deep, ind, last_is_block, sibling_is_block);
		}
	} else if(type_node == node_t::text) {
		if(text && std::any_of(content.begin(), content.end(), [](char c) {
			return !IS_SPACE(c);
		})) {
			auto str = content;
			if(parent && std::find(rawtext_tags.begin(), rawtext_tags.end(), parent->tag_name) == rawtext_tags.end()) {
				str = std::regex_replace(str, std::regex("[\\s]+"), " ");
			}
			if(last_is_block) {
				out << '\n' << std::string(deep, ind);
			}
			out << str;
			last_is_block = false;
		}
	} else if(type_node == node_t::tag) {
		bool old_is_block = last_is_block;
		last_is_block = std::find(inline_tags.begin(), inline_tags.end(), tag_name) == inline_tags.end();
		if(pos && (old_is_block || last_is_block)) {
			out << '\n' << std::string(deep, ind);
			if(level && last_is_block && !sibling_is_block) {
				sibling_is_block = true;
				deep++;
				out << ind;
			}
		}
		out << "<" << tag_name;
		if(!attributes.empty()) {
			for(auto& a : attributes) {
				out << ' ' << a.first << "=\"" << a.second << "\"";
			}
		}
		if(self_closing) {
			out << " />";
		} else {
			out << ">";
			if(child) {
				bool last_is_block_n = false;
				bool sibling_is_block_n = false;
				for(auto& c : children) {
					c->to_html(out, child, text, level + 1, deep, ind, last_is_block_n, sibling_is_block_n);
				}
				if(sibling_is_block_n) {
					if(deep > 0) {
						deep--;
					}
					out << '\n' << std::string(deep, ind);
				}
			}
			out << "</" << tag_name << ">";
		}
	} else if(type_node == node_t::comment) {
		if(last_is_block) {
			out << '\n' << std::string(deep, ind);
		}
		out << "<!--" << content << "-->";
		last_is_block = false;
	} else if(type_node == node_t::doctype) {
		out << "<!DOCTYPE " << content << ">";
		last_is_block = true;
		sibling_is_block = true;
	}
}

void node::to_raw_html(std::ostream& out, bool child, bool text) const {
	if(type_node == node_t::none) {
		for(auto& c : children) {
			c->to_raw_html(out, child, text);
		}
	} else if(type_node == node_t::text) {
		if(text && std::any_of(content.begin(), content.end(), [](char c) {
			return !IS_SPACE(c);
		})) {
			auto str = content;
			if(parent && std::find(rawtext_tags.begin(), rawtext_tags.end(), parent->tag_name) == rawtext_tags.end()) {
				str = std::regex_replace(str, std::regex("[\\s]+"), " ");
			}
			out << str;
		}
	} else if(type_node == node_t::tag) {
		out << "<" << tag_name;
		if(!attributes.empty()) {
			for(auto& a : attributes) {
				out << ' ' << a.first << "=\"" << a.second << "\"";
			}
		}
		if(self_closing) {
			out << " />";
		} else {
			out << ">";
			if(child) {
				for(auto& c : children) {
					c->to_raw_html(out, child, text);
				}
			}
			out << "</" << tag_name << ">";
		}
	} else if(type_node == node_t::comment) {
		out << "<!--" << content << "-->";
	} else if(type_node == node_t::doctype) {
		out << "<!DOCTYPE " << content << ">";
	}
}

std::string node::to_html(char ind, bool child, bool text) const {
	std::stringstream ret;
	bool last_is_block_n = false;
	bool sibling_is_block_n = false;
	int deep = 0;
	to_html(ret, child, text, 0, deep, ind, last_is_block_n, sibling_is_block_n);
	return ret.str();
}

std::string node::to_raw_html(bool child, bool text) const {
	std::stringstream ret;
	to_raw_html(ret, child, text);
	return ret.str();
}

void node::to_text(std::ostream& out, bool& is_block) const {
	std::streamoff pos = out.tellp();
	if(type_node == node_t::none) {
		for(auto& c : children) {
			c->to_text(out, is_block);
		}
	} else if(type_node == node_t::text) {
		if(is_block) {
			if(pos) {
				out << '\n';
			}
			is_block = false;
		}
		out << content;
	} else if(type_node == node_t::tag) {
		if(tag_name == "br") {
			out << '\n';
		}
		bool is_block_n = std::find(inline_tags.begin(), inline_tags.end(), tag_name) == inline_tags.end();
		if(is_block_n) {
			is_block = true;
		}
		for(auto& c : children) {
			c->to_text(out, is_block);
		}
		if(is_block_n) {
			is_block = true;
		}
	}
}

std::string node::to_text(bool raw) const {
	std::stringstream ret;
	bool is_block = false;
	to_text(ret, is_block);
	auto str = ret.str();
	if(raw) {
		str = std::regex_replace(str, std::regex("[\\s]+"), " ");
	}
	return str;
}

std::string node::get_attr(const std::string& attr) const {
	auto it = attributes.find(attr);
	if(it == attributes.end()) {
		return std::string();
	}
	return it->second;
}

void node::set_attr(const std::string& key, const std::string& val) {
	attributes[key] = val;
}

node_ptr node::copy() {
	node new_node;
	copy(this, &new_node);
	new_node.children[0]->parent = nullptr;
	return new_node.children[0];
}

void node::copy(node* n, node* p) {
	auto new_node = std::make_shared<node>();
	new_node->parent = p;
	new_node->type_node = n->type_node;
	new_node->type_tag = n->type_tag;
	new_node->self_closing = n->self_closing;
	new_node->tag_name = n->tag_name;
	new_node->content = n->content;
	new_node->attributes = n->attributes;
	if(new_node->type_node == node_t::tag) {
		new_node->index = p->node_count++;
	}
	for(auto& c : n->children) {
		copy(c.get(), new_node.get());
	}
	p->children.push_back(new_node);
}

node_ptr node::append(const node_ptr& n) {
	if(n->parent) {
		copy(n.get(), this);
	} else {
		n->parent = this;
		if(n->type_node == node_t::tag) {
			n->index = this->node_count++;
		}
		children.push_back(n);
	}
	return shared_from_this();
}

void parser::operator()(node& nodeptr) {
	for(auto& c : callback_node) {
		if(!c.first) {
			c.second(nodeptr);
			continue;
		}
		auto it = c.first.begin();
		if((*it)(nodeptr)) {
			it++;
		}
		if(it == c.first.end()) {
			c.second(nodeptr);
		}
	}
}

parser& parser::set_callback(std::function<void(node&)> cb) {
	callback_node.push_back(std::make_pair(selector(), cb));
	return *this;
}

parser& parser::set_callback(const selector selector, std::function<void(node&)> cb) {
	callback_node.push_back(std::make_pair(selector, cb));
	return *this;
}

parser& parser::set_callback(std::function<void(err_t, node&)> cb) {
	callback_err.push_back(cb);
	return *this;
}

void parser::clear_callbacks() {
	callback_node.clear();
	callback_err.clear();
}

void parser::handle_node() {
	if(new_node->type_node == node_t::tag) {
		if(new_node->type_tag == tag_t::open) {
			new_node->index = current_ptr->node_count++;
			current_ptr->children.push_back(new_node);
			if(!new_node->self_closing) {
				if(std::find(void_tags.begin(), void_tags.end(), new_node->tag_name) != void_tags.end()) {
					new_node->self_closing = true;
				} else if(std::find(rawtext_tags.begin(), rawtext_tags.end(), new_node->tag_name) != rawtext_tags.end()) {
					current_ptr = new_node.get();
					state = STATE_RAWTEXT;
				} else {
					current_ptr = new_node.get();
				}
			}
			(*this)(*new_node);
		} else if(new_node->type_tag == tag_t::close) {
			auto _current_ptr = current_ptr;
			std::vector<node*> not_closed;
			while(_current_ptr->parent && _current_ptr->tag_name != new_node->tag_name) {
				not_closed.push_back(_current_ptr);
				_current_ptr = _current_ptr->parent;
			}
			if(_current_ptr->parent && _current_ptr->tag_name == new_node->tag_name) {
				for(auto& c : callback_err) {
					for(auto n : not_closed) {
						c(err_t::tag_not_closed, *n);
					}
				}
				if(!new_node->content.empty()) {
					auto text_node = std::make_shared<node>(current_ptr);
					text_node->type_node = node_t::text;
					text_node->content = std::move(new_node->content);
					new_node->content.clear();
					current_ptr->children.push_back(text_node);
				}
				current_ptr = _current_ptr->parent;
				(*this)(*new_node);
			}
		}
	} else if(new_node->type_node == node_t::text) {
		if(!new_node->content.empty()) {
			current_ptr->children.push_back(new_node);
			(*this)(*new_node);
		}
	} else {
		current_ptr->children.push_back(new_node);
		(*this)(*new_node);
	}
	new_node = std::make_shared<node>(current_ptr);
	new_node->type_node = node_t::text;
}

node_ptr html::parser::parse(const std::string& html) {
	bool eof = false;
	char c;
	bool reconsume = false;
	auto it = html.begin();
	auto getc = [&]() -> char {
		if(it != html.end()) {
			return *it++;
		}
		eof = true;
		return 0;
	};
	auto get_string = [&](std::string str) {
		for(std::string::size_type i = 0; i < str.size(); i++) {
			if(i) {
				c = getc();
			}
			if(eof || str[i] != c) {
				it -= i;
				return false;
			}
		}
		return true;
	};
	state = STATE_DATA;
	auto _parent = std::make_shared<node>();
	current_ptr = _parent.get();
	new_node = std::make_shared<node>(current_ptr);
	new_node->type_node = node_t::text;
	std::string k;
	do {
		if(!reconsume) {
			c = getc();
		}
		reconsume = false;
		if(eof) {
			break;
		}
		switch(state) {
			case STATE_DATA: // 0
				if(c == '<') {
					state = STATE_TAG_OPEN;
				} else {
					new_node->content += c;
				}
			break;
			case STATE_RAWTEXT: // 3
				if(c == '<') {
					state = STATE_RAWTEXT_LESS_THAN_SIGN;
				} else if(c == 0x00) {
					new_node->content += '_';
				} else {
					new_node->content += c;
				}
			break;
			case STATE_TAG_OPEN: // 6
				if(c == '!') {
					state = STATE_MARKUP_DEC_OPEN_STATE;
				} else if(c == '/') {
					state = STATE_END_TAG_OPEN;
				} else if(IS_ALPHA(c)) {
					state = STATE_TAG_NAME;
					handle_node();
					new_node->type_node = node_t::tag;
					new_node->type_tag = tag_t::open;
					reconsume = true;
				} else if(c == '?') {
					state = STATE_BOGUS_COMMENT;
					handle_node();
					new_node->type_node = node_t::comment;
					reconsume = true;
				} else {
					new_node->content += '<';
					reconsume = true;
					state = STATE_DATA;
				}
			break;
			case STATE_END_TAG_OPEN: // 7
				if(IS_ALPHA(c)) {
					state = STATE_TAG_NAME;
					handle_node();
					new_node->type_node = node_t::tag;
					new_node->type_tag = tag_t::close;
					reconsume = true;
				} else if(c == '>') {
					state = STATE_DATA;
				} else {
					state = STATE_BOGUS_COMMENT;
					handle_node();
					new_node->type_node = node_t::comment;
					reconsume = true;
				}
			break;
			case STATE_TAG_NAME: // 8
				if(IS_SPACE(c)) {
					state = STATE_BEFORE_ATTRIBUTE_NAME;
				} else if(c == '/') {
					state = STATE_SELF_CLOSING;
				} else if(c == '>') {
					state = STATE_DATA;
					handle_node();
				} else if(IS_UPPERCASE_ALPHA(c)) {
					new_node->tag_name += std::tolower(c);
				} else if(c == 0x00) {
					new_node->tag_name += '_';
				} else {
					new_node->tag_name += c;
				}
			break;
			case STATE_RAWTEXT_LESS_THAN_SIGN: // 12
				if(c == '/') {
					state = STATE_RAWTEXT_END_TAG_OPEN;
				} else {
					new_node->content += '<';
					reconsume = true;
					state = STATE_RAWTEXT;
				}
			break;
			case STATE_RAWTEXT_END_TAG_OPEN: // 13
				if(IS_ALPHA(c)) {
					new_node->type_node = node_t::tag;
					new_node->type_tag = tag_t::close;
					reconsume = true;
					state = STATE_RAWTEXT_END_TAG_NAME;
				} else {
					new_node->content += '<';
					new_node->content += '/';
					reconsume = true;
					state = STATE_RAWTEXT;
				}
			break;
			case STATE_RAWTEXT_END_TAG_NAME: { // 14
				bool anything_else = true;
				if(IS_SPACE(c)) {
					if(new_node->tag_name == current_ptr->tag_name) {
						state = STATE_BEFORE_ATTRIBUTE_NAME;
						anything_else = false;
					}
				} else if(c == '/') {
					if(new_node->tag_name == current_ptr->tag_name) {
						state = STATE_SELF_CLOSING;
						anything_else = false;
					}
				} else if(c == '>') {
					if(new_node->tag_name == current_ptr->tag_name) {
						state = STATE_DATA;
						handle_node();
						anything_else = false;
					}
				} else if(IS_UPPERCASE_ALPHA(c)) {
					new_node->tag_name += std::tolower(c);
					anything_else = false;
				} else if(IS_LOWERCASE_ALPHA(c)) {
					new_node->tag_name += c;
					anything_else = false;
				}
				if(anything_else) {
					new_node->content += '<';
					new_node->content += '/';
					new_node->content += new_node->tag_name;
					new_node->tag_name.clear();
					reconsume = true;
					state = STATE_RAWTEXT;
				}
			}
			break;
			case STATE_BEFORE_ATTRIBUTE_NAME: // 32
				if(IS_SPACE(c)) {
					// skip
				} else if(c == '/' || c == '>') {
					reconsume = true;
					state = STATE_AFTER_ATTRIBUTE_NAME;
				} else if(c == '=') {
					k = c;
					state = STATE_ATTRIBUTE_NAME;
				} else {
					k.clear();
					reconsume = true;
					state = STATE_ATTRIBUTE_NAME;
				}
			break;
			case STATE_ATTRIBUTE_NAME: // 33
				if(IS_SPACE(c) || c == '/' || c == '>') {
					new_node->attributes[k];
					reconsume = true;
					state = STATE_AFTER_ATTRIBUTE_NAME;
				} else if(c == '=') {
					new_node->attributes[k];
					state = STATE_BEFORE_ATTRIBUTE_VALUE;
				} else if(c == 0x00) {
					k += '_';
				} else if(c == '\'' || c == '"' || c == '<') {
					k += c;
				} else {
					k += std::tolower(c);
				}
			break;
			case STATE_AFTER_ATTRIBUTE_NAME: // 34
				if(IS_SPACE(c)) {
					// skip
				} else if(c == '/') {
					state = STATE_SELF_CLOSING;
				} else if(c == '=') {
					state = STATE_BEFORE_ATTRIBUTE_VALUE;
				} else if(c == '>') {
					state = STATE_DATA;
					handle_node();
				} else {
					k.clear();
					reconsume = true;
					state = STATE_ATTRIBUTE_NAME;
				}
			break;
			case STATE_BEFORE_ATTRIBUTE_VALUE: // 35
				if(IS_SPACE(c)) {
					// skip
				} else if(c == '"') {
					state = STATE_ATTRIBUTE_VALUE_DOUBLE;
				} else if(c == '\'') {
					state = STATE_ATTRIBUTE_VALUE_SINGLE;
				} else if(c == '>') {
					state = STATE_DATA;
					handle_node();
				} else {
					reconsume = true;
					state = STATE_ATTRIBUTE_VALUE_UNQUOTED;
				}
			break;
			case STATE_ATTRIBUTE_VALUE_DOUBLE: // 36
				if(c == '"') {
					state = STATE_AFTER_ATTRIBUTE_VALUE_QUOTED;
				} else if(c == 0x00) {
					new_node->attributes[k] += '_';
				} else {
					new_node->attributes[k] += c;
				}
			break;
			case STATE_ATTRIBUTE_VALUE_SINGLE: // 37
				if(c == '\'') {
					state = STATE_AFTER_ATTRIBUTE_VALUE_QUOTED;
				} else if(c == 0x00) {
					new_node->attributes[k] += '_';
				} else {
					new_node->attributes[k] += c;
				}
			break;
			case STATE_ATTRIBUTE_VALUE_UNQUOTED: // 38
				if(IS_SPACE(c)) {
					state = STATE_BEFORE_ATTRIBUTE_NAME;
				} else if(c == '>') {
					state = STATE_DATA;
					handle_node();
				} else if(c == 0x00) {
					new_node->attributes[k] += '_';
				} else if(c == '"' || c == '\'' || c == '<' || c == '=' || c == '`') {
					new_node->attributes[k] += c;
				} else {
					new_node->attributes[k] += c;
				}
			break;
			case STATE_AFTER_ATTRIBUTE_VALUE_QUOTED: // 39
				if(IS_SPACE(c)) {
					state = STATE_BEFORE_ATTRIBUTE_NAME;
				} else if(c == '/') {
					state = STATE_SELF_CLOSING;
				} else if(c == '>') {
					state = STATE_DATA;
					handle_node();
				} else {
					reconsume = true;
					state = STATE_BEFORE_ATTRIBUTE_NAME;
				}
			break;
			case STATE_SELF_CLOSING: // 40
				if(c == '>') {
					new_node->self_closing = true;
					state = STATE_DATA;
					handle_node();
				} else {
					reconsume = true;
					state = STATE_BEFORE_ATTRIBUTE_NAME;
				}
			break;
			case STATE_BOGUS_COMMENT: // 41
				if(c == '>') {
					state = STATE_DATA;
					handle_node();
				} else if(c == 0x00) {
					new_node->content += '_';
				} else {
					new_node->content += c;
				}
			break;
			case STATE_MARKUP_DEC_OPEN_STATE: // 42
				if(get_string("--")) {
					state = STATE_COMMENT_START;
					handle_node();
					new_node->type_node = node_t::comment;
				} else if(get_string("DOCTYPE")) {
					state = STATE_BEFORE_DOCTYPE_NAME;
					handle_node();
					new_node->type_node = node_t::doctype;
				} else {
					state = STATE_BOGUS_COMMENT;
					handle_node();
					new_node->type_node = node_t::comment;
					new_node->bogus_comment = true;
					new_node->content += c;
				}
			break;
			case STATE_COMMENT_START: // 43
				if(c == '-') {
					state = STATE_COMMENT_START_DASH;
				} else if(c == '>') {
					state = STATE_DATA;
					handle_node();
				} else {
					reconsume = true;
					state = STATE_COMMENT;
				}
			break;
			case STATE_COMMENT_START_DASH: // 44
				if(c == '-') {
					state = STATE_COMMENT_END;
				} else if(c == '>') {
					state = STATE_DATA;
					handle_node();
				} else {
					new_node->content += '-';
					state = STATE_COMMENT;
				}
			break;
			case STATE_COMMENT: // 45
				if(c == '-') {
					state = STATE_COMMENT_END_DASH;
				} else if(c == 0x00) {
					new_node->content += '_';
				} else {
					new_node->content += c;
				}
			break;
			case STATE_COMMENT_END_DASH: // 50
				if(c == '-') {
					state = STATE_COMMENT_END;
				} else {
					new_node->content += '-';
					state = STATE_COMMENT;
				}
			break;
			case STATE_COMMENT_END: // 51
				if(c == '>') {
					state = STATE_DATA;
					handle_node();
				} else if(c == '-') {
					new_node->content += c;
				} else {
					new_node->content += '-';
					new_node->content += '-';
					reconsume = true;
					state = STATE_COMMENT;
				}
			break;
			case STATE_BEFORE_DOCTYPE_NAME: // 54
				if(IS_SPACE(c)) {
					// skip
				} else if(c == '>') {
					state = STATE_DATA;
					handle_node();
				} else if(c == 0x00) {
					new_node->content += '_';
					state = STATE_DOCTYPE_NAME;
				} else {
					new_node->content += c;
					state = STATE_DOCTYPE_NAME;
				}
			break;
			case STATE_DOCTYPE_NAME: // 55
				if(c == '>') {
					state = STATE_DATA;
					handle_node();
				} else if(c == 0x00) {
					new_node->content += '_';
				} else {
					new_node->content += c;
				}
			break;
		}
	} while(c || reconsume);
	new_node->type_node = node_t::text;
	handle_node();
	return _parent;
}

std::shared_ptr<html::node> utils::make_node(node_t type, const std::string& str, const std::map<std::string, std::string>& attributes) {
	auto node = std::make_shared<html::node>();
	node->type_node = type;
	if(type == node_t::tag) {
		node->tag_name = str;
		if(std::find(void_tags.begin(), void_tags.end(), str) != void_tags.end()) {
			node->self_closing = true;
		}
		if(!attributes.empty()) {
			node->attributes = attributes;
		}
	} else {
		node->content = str;
	}
	return node;
}

}