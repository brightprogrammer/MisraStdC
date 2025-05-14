/// file      : parsers/c.h
/// author    : Siddharth Mishra (admin@brightprogrammer.in)
/// copyright : Copyright (c) 2025, Siddharth Mishra, All rights reserved.
///
/// A C parser, to parse C code into an AST
///

#ifndef MISRA_PARSERS_C_H
#define MISRA_PARSERS_C_H

#include <Misra/Std/Container.h>
#include <Misra/Types.h>

typedef enum Keyword {
    KEYWORD_UNKNOWN = 0, // Not a keyword
    KEYWORD_ALIGNAS,
    KEYWORD_ALIGNOF,
    KEYWORD_AUTO,
    KEYWORD_BOOL,
    KEYWORD_BREAK,
    KEYWORD_CASE,
    KEYWORD_CHAR,
    KEYWORD_CONST,
    KEYWORD_CONSTEXPR,
    KEYWORD_CONTINUE,
    KEYWORD_DEFAULT,
    KEYWORD_DO,
    KEYWORD_DOUBLE,
    KEYWORD_ELSE,
    KEYWORD_ENUM,
    KEYWORD_EXTERN,
    KEYWORD_FALSE,
    KEYWORD_FLOAT,
    KEYWORD_FOR,
    KEYWORD_GOTO,
    KEYWORD_IF,
    KEYWORD_INLINE,
    KEYWORD_INT,
    KEYWORD_LONG,
    KEYWORD_NULLPTR,
    KEYWORD_REGISTER,
    KEYWORD_RESTRICT,
    KEYWORD_RETURN,
    KEYWORD_SHORT,
    KEYWORD_SIGNED,
    KEYWORD_SIZEOF,
    KEYWORD_STATIC,
    KEYWORD_STATIC_ASSERT,
    KEYWORD_STRUCT,
    KEYWORD_SWITCH,
    KEYWORD_THREAD_LOCAL,
    KEYWORD_TRUE,
    KEYWORD_TYPEDEF,
    KEYWORD_TYPEOF,
    KEYWORD_TYPEOF_UNQUAL,
    KEYWORD_UNION,
    KEYWORD_UNSIGNED,
    KEYWORD_VOID,
    KEYWORD_VOLATILE,
    KEYWORD_WHILE,
    KEYWORD_ATOMIC,
    KEYWORD_BITINT,
    KEYWORD_COMPLEX,
    KEYWORD_DECIMAL128,
    KEYWORD_DECIMAL32,
    KEYWORD_DECIMAL64,
    KEYWORD_GENERIC,
    KEYWORD_IMAGINARY,
    KEYWORD_NORETURN,
} Keyword;

typedef enum {
    CONSTANT_TYPE_UNKNOWN = 0,
    CONSTANT_TYPE_INTEGER,
    CONSTANT_TYPE_FLOAT,
    CONSTANT_TYPE_ENUM,
    CONSTANT_TYPE_CHAR,
    CONSTANT_TYPE_PREDEFINED
} ConstantType;

/// Sentence 6 of section 6.4.4.2
typedef enum {
    INTEGER_SUFFIX_NONE = 0,
    INTEGER_SUFFIX_WBU,
    INTEGER_SUFFIX_LLU,
    INTEGER_SUFFIX_LU,
    INTEGER_SUFFIX_U,
    INTEGER_SUFFIX_UL,
    INTEGER_SUFFIX_ULL,
    INTEGER_SUFFIX_UWB,
    INTEGER_SUFFIX_WB,
    INTEGER_SUFFIX_LL,
    INTEGER_SUFFIX_L
} IntegerSuffix;

/// Sentence 5 of section 6.4.4.3
typedef enum {
    FLOAT_SUFFIX_NONE = 0,
    FLOAT_SUFFIX_F,
    FLOAT_SUFFIX_L,
    FLOAT_SUFFIX_DF,
    FLOAT_SUFFIX_DL,
    FLOAT_SUFFIX_DD
} FloatSuffix;

typedef enum {
    CHAR_PREFIX_NONE = 0,
    CHAR_PREFIX_U8,  // unsigned character
    CHAR_PREFIX_U16, // char starts with u or \u escape sequence
    CHAR_PREFIX_U32, // char starts with U, or \U escape sequence
    CHAR_PREFIX_L    // platform-dependent 16 or 32 bit
} CharPrefix;

typedef struct {
    IntegerSuffix suffix;
    i64           i;
} Integer;

typedef struct {
    FloatSuffix suffix;
    fl          f;
} Float;

typedef struct {
    CharPrefix prefix;
    union {
        i8  c;
        u8  u8;
        u16 u16;
        u32 u32;
    };
} Char;

typedef enum {
    PREDEFINED_CONSTANT_UNKNOWN = 0,
    PREDEFINED_CONSTANT_TRUE,
    PREDEFINED_CONSTANT_FALSE,
    PREDEFINED_CONSTANT_NULL,
} PredefinedConstant;

typedef struct {
    ConstantType type;
    union {
        Integer            i;
        Float              f;
        Char               c;
        Str                e;
        PredefinedConstant p;
    };
} Constant;

typedef Vec(Char) String;
typedef struct {
    CharPrefix prefix;
    String     s;
} StringLiteral;

typedef enum {
    PUNCTUATOR_INVALID = 0,
    PUNCTUATOR_LBRACKET,     // [ or <:
    PUNCTUATOR_RBRACKET,     // ] or :>
    PUNCTUATOR_LPAREN,       // (
    PUNCTUATOR_RPAREN,       // )
    PUNCTUATOR_LBRACE,       // { or <%
    PUNCTUATOR_RBRACE,       // } or %>
    PUNCTUATOR_DOT,          // .
    PUNCTUATOR_ARROW,        // ->
    PUNCTUATOR_PLUSPLUS,     // ++
    PUNCTUATOR_MINUSMINUS,   // --
    PUNCTUATOR_AND,          // &
    PUNCTUATOR_STAR,         // *
    PUNCTUATOR_PLUS,         // +
    PUNCTUATOR_MINUS,        // -
    PUNCTUATOR_TILDE,        // ~
    PUNCTUATOR_BANG,         // !
    PUNCTUATOR_DIV,          // /
    PUNCTUATOR_MOD,          // %
    PUNCTUATOR_LSHIFT,       // <<
    PUNCTUATOR_RSHIFT,       // >>
    PUNCTUATOR_LT,           // <
    PUNCTUATOR_GT,           // >
    PUNCTUATOR_LE,           // <=
    PUNCTUATOR_GE,           // <=
    PUNCTUATOR_EQ,           // ==
    PUNCTUATOR_NE,           // !=
    PUNCTUATOR_XOR,          // ^
    PUNCTUATOR_OR,           // |
    PUNCTUATOR_LOGAND,       // &&
    PUNCTUATOR_LOGOR,        // ||
    PUNCTUATOR_QUESTION,     // ?
    PUNCTUATOR_COLON,        // :
    PUNCTUATOR_COLONCOLON,   // ::
    PUNCTUATOR_SEMICOLON,    // ;
    PUNCTUATOR_ELLIPSIS,     // ...
    PUNCTUATOR_ASSIGN,       // =
    PUNCTUATOR_MULASSIGN,    // *=
    PUNCTUATOR_DIVASSIGN,    // /=
    PUNCTUATOR_MODASSIGN,    // %=
    PUNCTUATOR_PLUSASSIGN,   // +=
    PUNCTUATOR_MINUSASSIGN,  // -=
    PUNCTUATOR_LSHIFTASSIGN, // <<=
    PUNCTUATOR_RSHIFTASSIGN, // >>=
    PUNCTUATOR_ANDASSIGN,    // &=
    PUNCTUATOR_XORASSIGN,    // ^=
    PUNCTUATOR_ORASSIGN,     // |=
    PUNCTUATOR_COMMA,        // ,
    PUNCTUATOR_HASH,         // #  or %:
    PUNCTUATOR_HASHHASH,     // ## or %:%:
} Punctuator;

typedef struct {
    bool is_local; // name is inside "" instead of <>
    Str  name;
} HeaderName;

typedef Str PpNumber;

typedef enum {
    STORAGE_CLASS_SPECIFIER_NONE = 0,
    STORAGE_CLASS_SPECIFIER_AUTO,
    STORAGE_CLASS_SPECIFIER_CONSTEXPR,
    STORAGE_CLASS_SPECIFIER_EXTERN,
    STORAGE_CLASS_SPECIFIER_REGISTER
} StorageClassSpecifier;
typedef Vec(StorageClassSpecifier) StorageClassSpecifiers;

typedef struct {
    StorageClassSpecifiers storage_class_specifiers;
    Str                    type_name;
    // TODO: braced initializer
} CompoundLiteral;

typedef enum {
    EXPR_TYPE_INVALID = 0,
    EXPR_TYPE_IDENTIFIER,
    EXPR_TYPE_CONSTANT,
    EXPR_TYPE_IN_PARENS,
    EXPR_TYPE_STRING_LITERAL,
    EXPR_TYPE_GENERIC_SELECTION,

    EXPR_TYPE_ARG_LIST,
    EXPR_TYPE_COMPOUND_LITERAL,

    EXPR_TYPE_ARRAY_ACCESS,
    EXPR_TYPE_CALL,
    EXPR_TYPE_DOT_ACCESS,
    EXPR_TYPE_ARROW_ACCESS,
    EXPR_TYPE_POST_INCREMENT,
    EXPR_TYPE_POST_DECREMENT,

    EXPR_TYPE_PRE_INCREMENT,
    EXPR_TYPE_PRE_DECREMENT,
    EXPR_TYPE_REF,
    EXPR_TYPE_DEREF,
    EXPR_TYPE_PRE_PLUS,
    EXPR_TYPE_PRE_MINUS,
    EXPR_TYPE_NOT,
    EXPR_TYPE_LOGNOT,
    EXPR_TYPE_SIZEOF_EXPR,
    EXPR_TYPE_SIZEOF_TYPE,
    EXPR_TYPE_ALIGNOF_TYPE,

    EXPR_TYPE_CAST,

    EXPR_TYPE_MUL,
    EXPR_TYPE_DIV,
    EXPR_TYPE_MOD,
    EXPR_TYPE_ADD,
    EXPR_TYPE_SUB,
    EXPR_TYPE_LSHIFT,
    EXPR_TYPE_RSHIFT,
    EXPR_TYPE_LT,
    EXPR_TYPE_GT,
    EXPR_TYPE_LE,
    EXPR_TYPE_GE,
    EXPR_TYPE_EQ,
    EXPR_TYPE_NE,
    EXPR_TYPE_AND,
    EXPR_TYPE_XOR,
    EXPR_TYPE_OR,
    EXPR_TYPE_LOGAND,
    EXPR_TYPE_LOGOR,

    EXPR_TYPE_TERNARY,

    EXPR_TYPE_ASSIGN,
    EXPR_TYPE_MUL_ASSIGN,
    EXPR_TYPE_DIV_ASSIGN,
    EXPR_TYPE_MOD_ASSIGN,
    EXPR_TYPE_ADD_ASSIGN,
    EXPR_TYPE_SUB_ASSIGN,
    EXPR_TYPE_LSHIFT_ASSIGN,
    EXPR_TYPE_RSHIFT_ASSIGN,
    EXPR_TYPE_AND_ASSIGN,
    EXPR_TYPE_XOR_ASSIGN,
    EXPR_TYPE_OR_ASSIGN,

    EXPR_TYPE_EXPRS, // a comma separated list of expressions
} ExprType;

typedef struct Expr Expr;
typedef Vec(Expr*) Exprs;

typedef struct {
    bool  is_default;
    Str   type_name;
    Expr* expr;
} GenericAssociation;
typedef Vec(GenericAssociation) GenericAssociations;

struct Expr {
    ExprType type;

    union {
        Str           identifier;
        Constant      constant;
        StringLiteral string_literal;

        Expr *in_parens, *pre_inc, *post_inc, *pre_dec, *post_dec, *pre_plus, *pre_minus, *ref, *deref, *not, *lognot,
            *sizeof_expr;

        // TODO: sizeof_type_name, alignof_type_name

        struct {
            Expr*               expr;
            GenericAssociations generic_associations;
        } generic_selection;

        Exprs exprs;

        CompoundLiteral compound_literal;

        struct {
            // TODO: type-name
            Expr* expr;
        } cast;

        struct {
            Expr* c;
            Expr* t;
            Expr* f;
        } ternary;

        struct {
            Expr* expr;
            Expr* args;
        } call;

        struct {
            Expr* l;
            Expr* r;
        } array_access, dot_access, arrow_access, mul, div, mod, add, sub, lshift, rshift, lt, gt, le, ge, eq, ne, and,
            xor, or, logand, logor, assign, mul_assign, div_assign, add_assign, sub_assign, lshift_assign,
            rshift_assign, and_assign, xor_assign, or_assign;
    };
};

#endif // MISRA_PARSERS_C_H
