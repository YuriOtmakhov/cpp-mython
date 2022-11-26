#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <iostream>

using namespace std;

namespace parse {

bool operator==(const Token& lhs, const Token& rhs) {
    using namespace token_type;

    if (lhs.index() != rhs.index()) {
        return false;
    }
    if (lhs.Is<Char>()) {
        return lhs.As<Char>().value == rhs.As<Char>().value;
    }
    if (lhs.Is<Number>()) {
        return lhs.As<Number>().value == rhs.As<Number>().value;
    }
    if (lhs.Is<String>()) {
        return lhs.As<String>().value == rhs.As<String>().value;
    }
    if (lhs.Is<Id>()) {
        return lhs.As<Id>().value == rhs.As<Id>().value;
    }
    return true;
}

bool operator!=(const Token& lhs, const Token& rhs) {
    return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const Token& rhs) {
    using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

    VALUED_OUTPUT(Number);
    VALUED_OUTPUT(Id);
    VALUED_OUTPUT(String);
    VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

    UNVALUED_OUTPUT(Class);
    UNVALUED_OUTPUT(Return);
    UNVALUED_OUTPUT(If);
    UNVALUED_OUTPUT(Else);
    UNVALUED_OUTPUT(Def);
    UNVALUED_OUTPUT(Newline);
    UNVALUED_OUTPUT(Print);
    UNVALUED_OUTPUT(Indent);
    UNVALUED_OUTPUT(Dedent);
    UNVALUED_OUTPUT(And);
    UNVALUED_OUTPUT(Or);
    UNVALUED_OUTPUT(Not);
    UNVALUED_OUTPUT(Eq);
    UNVALUED_OUTPUT(NotEq);
    UNVALUED_OUTPUT(LessOrEq);
    UNVALUED_OUTPUT(GreaterOrEq);
    UNVALUED_OUTPUT(None);
    UNVALUED_OUTPUT(True);
    UNVALUED_OUTPUT(False);
    UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

    return os << "Unknown token :("sv;
}

Lexer::Lexer(std::istream& input) : input_(input) {
//    auto it = std::istreambuf_iterator<char>(input_);
//    current_token_ = ParseInput(it);
    NextToken();
}

const Token& Lexer::CurrentToken() const {
//    if (!current_token_)
//        NextToken();
    return current_token_;
    //throw std::logic_error("Not implemented"s);
}

Token ReadNumber (std::istreambuf_iterator<char>& it) {
    int numder = 0;
    for (auto end = std::istreambuf_iterator<char>(); it != end; ++it)
        if (*it >= '0' && *it <= '9')
            numder = numder*10 + (*it - '0');
        else
            break;

    return token_type::Number{numder};
}

inline Token ReadString (std::istreambuf_iterator<char>& it) {
    std::string str;
    auto end = std::istreambuf_iterator<char>();
    for (char separator = *it++; *it != separator; ++it) {
        if (it == end)
            throw std::logic_error("String parsing error"s);

        if (*it == '\\') {
            ++it;
            if (it == end)
                throw std::logic_error("String parsing error"s);

            switch (*it) {
                case 'n':
                    str.push_back('\n');
                    break;
                case 't':
                    str.push_back('\t');
                    break;
                case 'r':
                    str.push_back('\r');
                    break;
                case '"':
                    str.push_back('"');
                    break;
                case '\\':
                    str.push_back('\\');
                    break;
                case '\'':
                    str.push_back('\'');
                    break;
                default:
                    throw std::logic_error("Unrecognized escape sequence \\"s + *it);
            }
        } else if (*it == '\n' || *it == '\r')
            throw std::logic_error("Unexpected end of line"s);
        else
            str.push_back(*it);

    }
    ++it;
    return token_type::String{str};
}

std::string ReadWord (std::istreambuf_iterator<char>& it) {
    std::string str;
    for (; (*it >= '0' && *it <= '9') ||
            (*it >= 'A' && *it <= 'Z') ||
            (*it >= 'a' && *it <= 'z') ||
            *it  == '_';
             ++it)
        str.push_back(*it);
    return str;
}

Token Lexer::ParseInput(std::istreambuf_iterator<char>& it) {
//    if (it == std::istreambuf_iterator<char>())
//        return token_type::Eof();

    for (;*it == ' '; ++it)
        if (current_token_.Is<token_type::Newline>())
            ++curr_dent_;

    if (*it == '#')
        for (auto end = std::istreambuf_iterator<char>();*it != '\n'; ++it)
            if (it == end)
                return token_type::Eof();

    if (*it == '\n'){
        ++it;
        curr_dent_ = 0;
        if (!current_token_.Is<token_type::Newline>())
            return token_type::Newline();
        else
            return ParseInput(it);
    }

    if (curr_dent_ > old_dent_) {
        old_dent_ += 2;
        return token_type::Indent();
    }else if (curr_dent_ < old_dent_) {
        old_dent_ -= 2;
        return token_type::Dedent();
    }

    if (it == std::istreambuf_iterator<char>())
        return token_type::Eof();

    if (*it == '\'' || *it == '"' )
        return ReadString(it);

    if (*it >= '0' && *it <= '9')
        return ReadNumber(it);

    if ( *it == '-' ||
        *it == '+' ||
        *it == '*' ||
        *it == '/' ||
        *it == ',' ||
        *it == '.' ||
        *it == '(' ||
        *it == ')' ||
        *it == ':'
        )
        return token_type::Char{*it++};

    switch (*it) {
        case '=':
        ++it;
        if (*it == '=') {
            ++it;
            return token_type::Eq();
        }else
            return token_type::Char{'='};

        case '<':
        ++it;
        if (*it == '=') {
            ++it;
            return token_type::LessOrEq();
        }else
            return token_type::Char{'<'};

        case '>':
        ++it;
        if (*it == '=') {
            ++it;
            return token_type::GreaterOrEq();
        }else
            return token_type::Char{'>'};

        case '!':
        ++it;
        if (*it == '=') {
            ++it;
            return token_type::NotEq();
        }else
            throw std::logic_error("Uncorected simbol '!'"s);

        default:
        std::string name = ReadWord(it);
        if (name == "class"s)
            return token_type::Class();

        if (name == "return"s)
            return token_type::Return();

        if (name == "if"s)
            return token_type::If();

        if (name == "else"s)
            return token_type::Else();

        if (name == "def"s)
            return token_type::Def();

        if (name == "print"s)
            return token_type::Print();

        if (name == "and"s)
            return token_type::And();

        if (name == "or"s)
            return token_type::Or();

        if (name == "not"s)
            return token_type::Not();

        if (name == "None"s)
            return token_type::None();

        if (name == "True"s)
            return token_type::True();

        if (name == "False"s)
            return token_type::False();

        return token_type::Id{name};

    }

}

Token Lexer::NextToken() {
//    if (input_.eof())
//        return token_type::Eof();
//    std::cerr<<current_token_<<std::endl;

    auto it = std::istreambuf_iterator<char>(input_);
    if (it == std::istreambuf_iterator<char>()) {
        if (current_token_.Is<token_type::Eof>())
            return token_type::Eof();
        else if (current_token_.Is<token_type::Newline>() || current_token_.Is<token_type::Dedent>()){
            if (curr_dent_ < old_dent_) {
                old_dent_ -= 2;
                current_token_ = token_type::Dedent();
                return token_type::Dedent();
            }

            current_token_ = token_type::Eof();
            return token_type::Eof();
        }else {
            current_token_ =token_type::Newline();// token_type::Eof();
            return token_type::Newline();
        }
    }
    current_token_ = ParseInput(it);
    return current_token_;



    //throw std::logic_error("Not implemented"s);
}

}  // namespace parse
