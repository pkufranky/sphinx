/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_IDENT = 258,
     TOK_CONST_INT = 259,
     TOK_CONST_FLOAT = 260,
     TOK_QUOTED_STRING = 261,
     TOK_AS = 262,
     TOK_ASC = 263,
     TOK_AVG = 264,
     TOK_BETWEEN = 265,
     TOK_BY = 266,
     TOK_COUNT = 267,
     TOK_DESC = 268,
     TOK_DELETE = 269,
     TOK_DISTINCT = 270,
     TOK_FROM = 271,
     TOK_GROUP = 272,
     TOK_LIMIT = 273,
     TOK_IN = 274,
     TOK_INSERT = 275,
     TOK_INTO = 276,
     TOK_ID = 277,
     TOK_MATCH = 278,
     TOK_MAX = 279,
     TOK_META = 280,
     TOK_MIN = 281,
     TOK_OPTION = 282,
     TOK_ORDER = 283,
     TOK_SELECT = 284,
     TOK_SHOW = 285,
     TOK_STATUS = 286,
     TOK_SUM = 287,
     TOK_VALUES = 288,
     TOK_WARNINGS = 289,
     TOK_WEIGHT = 290,
     TOK_WITHIN = 291,
     TOK_WHERE = 292,
     TOK_OR = 293,
     TOK_AND = 294,
     TOK_NOT = 295,
     TOK_NE = 296,
     TOK_GTE = 297,
     TOK_LTE = 298,
     TOK_NEG = 299
   };
#endif
#define TOK_IDENT 258
#define TOK_CONST_INT 259
#define TOK_CONST_FLOAT 260
#define TOK_QUOTED_STRING 261
#define TOK_AS 262
#define TOK_ASC 263
#define TOK_AVG 264
#define TOK_BETWEEN 265
#define TOK_BY 266
#define TOK_COUNT 267
#define TOK_DESC 268
#define TOK_DELETE 269
#define TOK_DISTINCT 270
#define TOK_FROM 271
#define TOK_GROUP 272
#define TOK_LIMIT 273
#define TOK_IN 274
#define TOK_INSERT 275
#define TOK_INTO 276
#define TOK_ID 277
#define TOK_MATCH 278
#define TOK_MAX 279
#define TOK_META 280
#define TOK_MIN 281
#define TOK_OPTION 282
#define TOK_ORDER 283
#define TOK_SELECT 284
#define TOK_SHOW 285
#define TOK_STATUS 286
#define TOK_SUM 287
#define TOK_VALUES 288
#define TOK_WARNINGS 289
#define TOK_WEIGHT 290
#define TOK_WITHIN 291
#define TOK_WHERE 292
#define TOK_OR 293
#define TOK_AND 294
#define TOK_NOT 295
#define TOK_NE 296
#define TOK_GTE 297
#define TOK_LTE 298
#define TOK_NEG 299




/* Copy the first part of user declarations.  */


#if USE_WINDOWS
#pragma warning(push,1)
#endif


// some helpers
#include <float.h> // for FLT_MAX

static void AddFloatRangeFilter ( SqlParser_t * pParser, const CSphString & sAttr, float fMin, float fMax )
{
	CSphFilterSettings tFilter;
	tFilter.m_sAttrName = sAttr;
	tFilter.m_eType = SPH_FILTER_FLOATRANGE;
	tFilter.m_fMinValue = fMin;
	tFilter.m_fMaxValue = fMax;
	pParser->m_pQuery->m_dFilters.Add ( tFilter );
}

static void AddUintRangeFilter ( SqlParser_t * pParser, const CSphString & sAttr, DWORD uMin, DWORD uMax )
{
	CSphFilterSettings tFilter;
	tFilter.m_sAttrName = sAttr;
	tFilter.m_eType = SPH_FILTER_RANGE;
	tFilter.m_uMinValue = uMin;
	tFilter.m_uMaxValue = uMax;
	pParser->m_pQuery->m_dFilters.Add ( tFilter );
}


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 1
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */


#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# if YYSTACK_USE_ALLOCA
#  define YYSTACK_ALLOC alloca
# else
#  ifndef YYSTACK_USE_ALLOCA
#   if defined (alloca) || defined (_ALLOCA_H)
#    define YYSTACK_ALLOC alloca
#   else
#    ifdef __GNUC__
#     define YYSTACK_ALLOC __builtin_alloca
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  34
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   376

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  55
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  36
/* YYNRULES -- Number of rules. */
#define YYNRULES  106
/* YYNRULES -- Number of states. */
#define YYNSTATES  231

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   299

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      53,    54,    49,    47,    52,    48,     2,    50,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      43,    41,    44,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    42,    45,    46,    51
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short yyprhs[] =
{
       0,     0,     3,     5,     7,     9,    11,    13,    15,    26,
      28,    32,    34,    38,    45,    52,    59,    66,    68,    74,
      76,    80,    81,    83,    86,    88,    92,    97,   101,   105,
     109,   115,   122,   128,   132,   136,   140,   144,   148,   152,
     156,   160,   166,   170,   174,   176,   180,   181,   183,   187,
     188,   190,   196,   197,   199,   203,   205,   209,   211,   214,
     217,   218,   220,   223,   228,   229,   231,   234,   236,   240,
     244,   248,   250,   252,   254,   257,   260,   264,   268,   272,
     276,   280,   284,   288,   292,   296,   300,   304,   308,   312,
     314,   319,   324,   328,   335,   342,   344,   348,   351,   354,
     357,   368,   369,   373,   375,   379,   381
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      56,     0,    -1,    57,    -1,    83,    -1,    84,    -1,    85,
      -1,    86,    -1,    90,    -1,    29,    58,    16,    60,    61,
      66,    68,    70,    74,    76,    -1,    59,    -1,    58,    52,
      59,    -1,     3,    -1,    80,     7,     3,    -1,     9,    53,
      80,    54,     7,     3,    -1,    24,    53,    80,    54,     7,
       3,    -1,    26,    53,    80,    54,     7,     3,    -1,    32,
      53,    80,    54,     7,     3,    -1,    49,    -1,    12,    53,
      15,     3,    54,    -1,     3,    -1,    60,    52,     3,    -1,
      -1,    62,    -1,    37,    63,    -1,    64,    -1,    63,    39,
      64,    -1,    23,    53,     6,    54,    -1,    22,    41,     4,
      -1,     3,    41,     4,    -1,     3,    42,     4,    -1,     3,
      19,    53,    65,    54,    -1,     3,    40,    19,    53,    65,
      54,    -1,     3,    10,     4,    39,     4,    -1,     3,    44,
       4,    -1,     3,    43,     4,    -1,     3,    45,     4,    -1,
       3,    46,     4,    -1,     3,    41,     5,    -1,     3,    42,
       5,    -1,     3,    44,     5,    -1,     3,    43,     5,    -1,
       3,    10,     5,    39,     5,    -1,     3,    45,     5,    -1,
       3,    46,     5,    -1,     4,    -1,    65,    52,     4,    -1,
      -1,    67,    -1,    17,    11,     3,    -1,    -1,    69,    -1,
      36,    17,    28,    11,    72,    -1,    -1,    71,    -1,    28,
      11,    72,    -1,    73,    -1,    72,    52,    73,    -1,     3,
      -1,     3,     8,    -1,     3,    13,    -1,    -1,    75,    -1,
      18,     4,    -1,    18,     4,    52,     4,    -1,    -1,    77,
      -1,    27,    78,    -1,    79,    -1,    78,    52,    79,    -1,
       3,    41,     3,    -1,     3,    41,     4,    -1,     3,    -1,
       4,    -1,     5,    -1,    48,    80,    -1,    40,    80,    -1,
      80,    47,    80,    -1,    80,    48,    80,    -1,    80,    49,
      80,    -1,    80,    50,    80,    -1,    80,    43,    80,    -1,
      80,    44,    80,    -1,    80,    46,    80,    -1,    80,    45,
      80,    -1,    80,    41,    80,    -1,    80,    42,    80,    -1,
      80,    39,    80,    -1,    80,    38,    80,    -1,    53,    80,
      54,    -1,    81,    -1,     3,    53,    82,    54,    -1,    19,
      53,    82,    54,    -1,     3,    53,    54,    -1,    26,    53,
      80,    52,    80,    54,    -1,    24,    53,    80,    52,    80,
      54,    -1,    80,    -1,    82,    52,    80,    -1,    30,    34,
      -1,    30,    31,    -1,    30,    25,    -1,    20,    21,     3,
      87,    33,    53,     4,    52,    88,    54,    -1,    -1,    53,
      60,    54,    -1,    89,    -1,    88,    52,    89,    -1,     6,
      -1,    14,    16,     3,    37,    22,    41,     4,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,    85,    85,    86,    87,    88,    89,    90,    96,   111,
     112,   116,   117,   118,   119,   120,   121,   122,   123,   138,
     139,   142,   144,   148,   152,   153,   157,   170,   178,   186,
     195,   203,   212,   216,   220,   224,   228,   232,   233,   234,
     235,   240,   244,   248,   255,   256,   259,   261,   265,   272,
     274,   278,   284,   286,   290,   297,   298,   302,   303,   304,
     307,   309,   313,   318,   325,   327,   331,   335,   336,   340,
     341,   347,   348,   349,   350,   351,   352,   353,   354,   355,
     356,   357,   358,   359,   360,   361,   362,   363,   364,   365,
     369,   370,   371,   372,   373,   377,   378,   384,   388,   392,
     398,   406,   408,   412,   413,   417,   423
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "TOK_IDENT", "TOK_CONST_INT", 
  "TOK_CONST_FLOAT", "TOK_QUOTED_STRING", "TOK_AS", "TOK_ASC", "TOK_AVG", 
  "TOK_BETWEEN", "TOK_BY", "TOK_COUNT", "TOK_DESC", "TOK_DELETE", 
  "TOK_DISTINCT", "TOK_FROM", "TOK_GROUP", "TOK_LIMIT", "TOK_IN", 
  "TOK_INSERT", "TOK_INTO", "TOK_ID", "TOK_MATCH", "TOK_MAX", "TOK_META", 
  "TOK_MIN", "TOK_OPTION", "TOK_ORDER", "TOK_SELECT", "TOK_SHOW", 
  "TOK_STATUS", "TOK_SUM", "TOK_VALUES", "TOK_WARNINGS", "TOK_WEIGHT", 
  "TOK_WITHIN", "TOK_WHERE", "TOK_OR", "TOK_AND", "TOK_NOT", "'='", 
  "TOK_NE", "'<'", "'>'", "TOK_GTE", "TOK_LTE", "'+'", "'-'", "'*'", 
  "'/'", "TOK_NEG", "','", "'('", "')'", "$accept", "statement", 
  "select_from", "select_items_list", "select_item", "ident_list", 
  "opt_where_clause", "where_clause", "where_expr", "where_item", 
  "const_list", "opt_group_clause", "group_clause", 
  "opt_group_order_clause", "group_order_clause", "opt_order_clause", 
  "order_clause", "order_items_list", "order_item", "opt_limit_clause", 
  "limit_clause", "opt_option_clause", "option_clause", "option_list", 
  "option_item", "expr", "function", "arglist", "show_warnings", 
  "show_status", "show_meta", "insert_into", "opt_insert_cols_list", 
  "insert_vals_list", "insert_val", "delete_from", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,    61,   296,    60,    62,   297,   298,    43,    45,    42,
      47,   299,    44,    40,    41
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    55,    56,    56,    56,    56,    56,    56,    57,    58,
      58,    59,    59,    59,    59,    59,    59,    59,    59,    60,
      60,    61,    61,    62,    63,    63,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    65,    65,    66,    66,    67,    68,
      68,    69,    70,    70,    71,    72,    72,    73,    73,    73,
      74,    74,    75,    75,    76,    76,    77,    78,    78,    79,
      79,    80,    80,    80,    80,    80,    80,    80,    80,    80,
      80,    80,    80,    80,    80,    80,    80,    80,    80,    80,
      81,    81,    81,    81,    81,    82,    82,    83,    84,    85,
      86,    87,    87,    88,    88,    89,    90
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     1,     1,     1,     1,    10,     1,
       3,     1,     3,     6,     6,     6,     6,     1,     5,     1,
       3,     0,     1,     2,     1,     3,     4,     3,     3,     3,
       5,     6,     5,     3,     3,     3,     3,     3,     3,     3,
       3,     5,     3,     3,     1,     3,     0,     1,     3,     0,
       1,     5,     0,     1,     3,     1,     3,     1,     2,     2,
       0,     1,     2,     4,     0,     1,     2,     1,     3,     3,
       3,     1,     1,     1,     2,     2,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     1,
       4,     4,     3,     6,     6,     1,     3,     2,     2,     2,
      10,     0,     3,     1,     3,     1,     7
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,     0,     0,     0,     0,     0,     2,     3,     4,     5,
       6,     7,     0,     0,    71,    72,    73,     0,     0,     0,
       0,     0,     0,     0,     0,    17,     0,     0,     9,     0,
      89,    99,    98,    97,     1,     0,   101,     0,     0,     0,
       0,     0,     0,     0,    71,     0,     0,    75,    74,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    92,    95,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    88,
      19,    21,    10,    12,    87,    86,    84,    85,    80,    81,
      83,    82,    76,    77,    78,    79,     0,     0,     0,     0,
      90,     0,     0,    91,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    46,    22,     0,   102,     0,    96,     0,
      18,     0,     0,     0,     0,     0,     0,     0,     0,    23,
      24,    20,     0,    49,    47,   106,     0,    13,    94,    14,
      93,    15,    16,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    52,    50,     0,
       0,     0,     0,     0,    28,    37,    29,    38,    34,    40,
      33,    39,    35,    42,    36,    43,    27,     0,    25,    48,
       0,     0,    60,    53,   105,     0,   103,     0,     0,    44,
       0,     0,    26,     0,     0,     0,    64,    61,     0,   100,
      32,    41,     0,    30,     0,     0,    57,    54,    55,    62,
       0,     8,    65,   104,    45,    31,    51,    58,    59,     0,
       0,     0,    66,    67,    56,    63,     0,     0,    69,    70,
      68
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short yydefgoto[] =
{
      -1,     5,     6,    27,    28,    81,   113,   114,   129,   130,
     190,   133,   134,   157,   158,   182,   183,   207,   208,   196,
     197,   211,   212,   222,   223,    29,    30,    70,     7,     8,
       9,    10,    67,   185,   186,    11
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -33
static const short yypact[] =
{
      -4,   -14,   -16,    56,    53,    12,   -33,   -33,   -33,   -33,
     -33,   -33,    59,    74,    -7,   -33,   -33,   -32,    33,    70,
      80,    91,    97,    45,    45,   -33,    45,    -5,   -33,    72,
     -33,   -33,   -33,   -33,   -33,    75,   108,     3,    45,    92,
      45,    45,    45,    45,   114,   125,   139,   237,   -33,   127,
     143,    56,   160,    45,    45,    45,    45,    45,    45,    45,
      45,    45,    45,    45,    45,   184,   143,   187,   -33,   227,
     -10,   141,   231,    18,    93,   110,   155,    45,    45,   -33,
     -33,   -24,   -33,   -33,   237,   237,   245,   245,    42,    42,
      42,    42,    17,    17,   -33,   -33,   207,    43,   199,    45,
     -33,   256,   213,   -33,    45,   301,    45,   303,   304,   197,
     212,     1,   306,   295,   -33,   309,   -33,   310,   227,   312,
     -33,   169,   313,   183,   314,   315,    84,   278,   267,   282,
     -33,   -33,   311,   287,   -33,   -33,   272,   -33,   -33,   -33,
     -33,   -33,   -33,    69,   273,   308,   292,   294,   296,   298,
     300,   302,   321,   322,     1,   326,   316,   307,   -33,   324,
     293,   297,   327,   281,   -33,   -33,   -33,   -33,   -33,   -33,
     -33,   -33,   -33,   -33,   -33,   -33,   -33,   283,   -33,   -33,
     317,   328,   320,   -33,   -33,    47,   -33,   336,   337,   -33,
      48,   327,   -33,   330,   340,   342,   323,   -33,   324,   -33,
     -33,   -33,   343,   -33,    54,   340,    50,   299,   -33,   305,
     341,   -33,   -33,   -33,   -33,   -33,   299,   -33,   -33,   340,
     344,   318,   319,   -33,   -33,   -33,    49,   341,   -33,   -33,
     -33
};

/* YYPGOTO[NTERM-NUM].  */
static const short yypgoto[] =
{
     -33,   -33,   -33,   -33,   325,   286,   -33,   -33,   -33,   195,
     162,   -33,   -33,   -33,   -33,   -33,   -33,   149,   136,   -33,
     -33,   -33,   -33,   -33,   129,   -23,   -33,   329,   -33,   -33,
     -33,   -33,   -33,   -33,   163,   -33
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -12
static const short yytable[] =
{
      47,    48,    12,    49,   126,    13,    44,    15,    16,   -11,
       1,    50,    34,   111,    69,    71,     2,    69,    74,    75,
      76,    38,    19,   127,   128,     3,     4,    45,   112,    46,
      84,    85,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    99,    23,   100,   -11,    37,    51,    44,    15,
      16,    24,   228,   229,   109,   110,    26,    68,   217,    14,
      15,    16,    35,   218,    19,    17,    63,    64,    18,    45,
      99,    46,   103,   160,   161,    19,   118,    36,    31,    52,
      20,   121,    21,   123,    32,    23,    39,    33,    22,    61,
      62,    63,    64,    24,   143,   112,    23,   116,    26,   198,
     202,   199,   203,   144,    24,    25,   202,    72,   215,    26,
      53,    54,    65,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    40,   145,   146,   147,   148,   149,   150,
     151,    53,    54,    41,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,    42,   104,    80,   105,    53,    54,
      43,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    66,   106,    83,   107,    53,    54,    37,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    77,    53,
      54,    79,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    78,    53,    54,   101,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    96,    53,    54,   108,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      98,    53,    54,   138,    55,    56,    57,    58,    59,    60,
      61,    62,    63,    64,   102,    53,    54,   140,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,   115,   104,
      53,    54,   117,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,   119,   106,    53,    54,   120,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    57,    58,
      59,    60,    61,    62,    63,    64,   164,   165,   166,   167,
     168,   169,   170,   171,   172,   173,   174,   175,   122,   131,
     124,   125,   132,   135,   136,   137,   139,   141,   142,   152,
     153,   154,   155,   156,   159,   176,   162,   163,   177,   179,
     184,   189,   187,   180,   191,   181,   188,   192,   195,   194,
     200,   205,   201,   206,   221,   193,   209,   214,   225,   178,
     210,   219,    97,   204,   216,   224,   230,   220,     0,   226,
       0,   213,     0,     0,     0,     0,     0,     0,     0,    73,
       0,   227,     0,     0,     0,     0,    82
};

static const short yycheck[] =
{
      23,    24,    16,    26,     3,    21,     3,     4,     5,    16,
      14,    16,     0,    37,    37,    38,    20,    40,    41,    42,
      43,    53,    19,    22,    23,    29,    30,    24,    52,    26,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    52,    40,    54,    52,    53,    52,     3,     4,
       5,    48,     3,     4,    77,    78,    53,    54,     8,     3,
       4,     5,     3,    13,    19,     9,    49,    50,    12,    24,
      52,    26,    54,     4,     5,    19,    99,     3,    25,     7,
      24,   104,    26,   106,    31,    40,    53,    34,    32,    47,
      48,    49,    50,    48,    10,    52,    40,    54,    53,    52,
      52,    54,    54,    19,    48,    49,    52,    15,    54,    53,
      38,    39,    37,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    53,    40,    41,    42,    43,    44,    45,
      46,    38,    39,    53,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    53,    52,     3,    54,    38,    39,
      53,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    53,    52,     3,    54,    38,    39,    53,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    53,    38,
      39,    54,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    53,    38,    39,    54,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    22,    38,    39,    54,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      33,    38,    39,    54,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,     3,    38,    39,    54,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    41,    52,
      38,    39,    53,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,     7,    52,    38,    39,    54,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    43,    44,
      45,    46,    47,    48,    49,    50,     4,     5,     4,     5,
       4,     5,     4,     5,     4,     5,     4,     5,     7,     3,
       7,     7,    17,     4,     4,     3,     3,     3,     3,    41,
      53,    39,    11,    36,    52,     4,    53,    19,     6,     3,
       6,     4,    39,    17,    53,    28,    39,    54,    18,    11,
       4,    11,     5,     3,     3,    28,     4,     4,     4,   154,
      27,    52,    66,   191,   205,   219,   227,    52,    -1,    41,
      -1,   198,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    40,
      -1,    52,    -1,    -1,    -1,    -1,    51
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    14,    20,    29,    30,    56,    57,    83,    84,    85,
      86,    90,    16,    21,     3,     4,     5,     9,    12,    19,
      24,    26,    32,    40,    48,    49,    53,    58,    59,    80,
      81,    25,    31,    34,     0,     3,     3,    53,    53,    53,
      53,    53,    53,    53,     3,    24,    26,    80,    80,    80,
      16,    52,     7,    38,    39,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    37,    53,    87,    54,    80,
      82,    80,    15,    82,    80,    80,    80,    53,    53,    54,
       3,    60,    59,     3,    80,    80,    80,    80,    80,    80,
      80,    80,    80,    80,    80,    80,    22,    60,    33,    52,
      54,    54,     3,    54,    52,    54,    52,    54,    54,    80,
      80,    37,    52,    61,    62,    41,    54,    53,    80,     7,
      54,    80,     7,    80,     7,     7,     3,    22,    23,    63,
      64,     3,    17,    66,    67,     4,     4,     3,    54,     3,
      54,     3,     3,    10,    19,    40,    41,    42,    43,    44,
      45,    46,    41,    53,    39,    11,    36,    68,    69,    52,
       4,     5,    53,    19,     4,     5,     4,     5,     4,     5,
       4,     5,     4,     5,     4,     5,     4,     6,    64,     3,
      17,    28,    70,    71,     6,    88,    89,    39,    39,     4,
      65,    53,    54,    28,    11,    18,    74,    75,    52,    54,
       4,     5,    52,    54,    65,    11,     3,    72,    73,     4,
      27,    76,    77,    89,     4,    54,    72,     8,    13,    52,
      52,     3,    78,    79,    73,     4,    41,    52,     3,     4,
      79
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrlab1

/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror (pParser, "syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)         \
  Current.first_line   = Rhs[1].first_line;      \
  Current.first_column = Rhs[1].first_column;    \
  Current.last_line    = Rhs[N].last_line;       \
  Current.last_column  = Rhs[N].last_column;
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, pParser)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (cinluded).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylineno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylineno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse ( SqlParser_t * pParser );
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse ( SqlParser_t * pParser )
#else
int
yyparse (pParser)
     SqlParser_t * pParser ;
#endif
#endif
{
  /* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 8:

    {
			pParser->m_pStmt->m_eStmt = STMT_SELECT;
			pParser->m_pQuery->m_sIndexes.SetBinary ( pParser->m_pBuf+yyvsp[-6].m_iStart, yyvsp[-6].m_iEnd-yyvsp[-6].m_iStart );
		;}
    break;

  case 11:

    { pParser->AddItem ( &yyvsp[0], NULL ); ;}
    break;

  case 12:

    { pParser->AddItem ( &yyvsp[-2], &yyvsp[0] ); ;}
    break;

  case 13:

    { pParser->AddItem ( &yyvsp[-3], &yyvsp[0], SPH_AGGR_AVG ); ;}
    break;

  case 14:

    { pParser->AddItem ( &yyvsp[-3], &yyvsp[0], SPH_AGGR_MAX ); ;}
    break;

  case 15:

    { pParser->AddItem ( &yyvsp[-3], &yyvsp[0], SPH_AGGR_MIN ); ;}
    break;

  case 16:

    { pParser->AddItem ( &yyvsp[-3], &yyvsp[0], SPH_AGGR_SUM ); ;}
    break;

  case 17:

    { pParser->AddItem ( &yyvsp[0], NULL ); ;}
    break;

  case 18:

    {
			if ( !pParser->m_pQuery->m_sGroupDistinct.IsEmpty() )
			{
				yyerror ( pParser, "too many COUNT(DISTINCT) clauses" );
				YYERROR;

			} else
			{
				pParser->m_pQuery->m_sGroupDistinct = yyvsp[-1].m_sValue;
			}
		;}
    break;

  case 20:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 26:

    {
			if ( pParser->m_bGotQuery )
			{
				yyerror ( pParser, "too many MATCH() clauses" );
				YYERROR;

			} else
			{
				pParser->m_pQuery->m_sQuery = yyvsp[-1].m_sValue;
				pParser->m_bGotQuery = true;
			}
		;}
    break;

  case 27:

    {
			CSphFilterSettings tFilter;
			tFilter.m_sAttrName = "@id";
			tFilter.m_eType = SPH_FILTER_VALUES;
			tFilter.m_dValues.Add ( yyvsp[0].m_iValue );
			pParser->m_pQuery->m_dFilters.Add ( tFilter );
		;}
    break;

  case 28:

    {
			CSphFilterSettings tFilter;
			tFilter.m_sAttrName = yyvsp[-2].m_sValue;
			tFilter.m_eType = SPH_FILTER_VALUES;
			tFilter.m_dValues.Add ( yyvsp[0].m_iValue );
			pParser->m_pQuery->m_dFilters.Add ( tFilter );
		;}
    break;

  case 29:

    {
			CSphFilterSettings tFilter;
			tFilter.m_sAttrName = yyvsp[-2].m_sValue;
			tFilter.m_eType = SPH_FILTER_VALUES;
			tFilter.m_dValues.Add ( yyvsp[0].m_iValue );
			tFilter.m_bExclude = true;
			pParser->m_pQuery->m_dFilters.Add ( tFilter );
		;}
    break;

  case 30:

    {
			CSphFilterSettings tFilter;
			tFilter.m_sAttrName = yyvsp[-4].m_sValue;
			tFilter.m_eType = SPH_FILTER_VALUES;
			tFilter.m_dValues = yyvsp[-1].m_dValues;
			pParser->m_pQuery->m_dFilters.Add ( tFilter );
		;}
    break;

  case 31:

    {
			CSphFilterSettings tFilter;
			tFilter.m_sAttrName = yyvsp[-5].m_sValue;
			tFilter.m_eType = SPH_FILTER_VALUES;
			tFilter.m_dValues = yyvsp[-2].m_dValues;
			tFilter.m_bExclude = true;
			pParser->m_pQuery->m_dFilters.Add ( tFilter );
		;}
    break;

  case 32:

    {
			AddUintRangeFilter ( pParser, yyvsp[-4].m_sValue, yyvsp[-2].m_iValue, yyvsp[0].m_iValue );
		;}
    break;

  case 33:

    {
			AddUintRangeFilter ( pParser, yyvsp[-2].m_sValue, yyvsp[0].m_iValue+1, UINT_MAX );
		;}
    break;

  case 34:

    {
			AddUintRangeFilter ( pParser, yyvsp[-2].m_sValue, 0, yyvsp[0].m_iValue-1 );
		;}
    break;

  case 35:

    {
			AddUintRangeFilter ( pParser, yyvsp[-2].m_sValue, yyvsp[0].m_iValue, UINT_MAX );
		;}
    break;

  case 36:

    {
			AddUintRangeFilter ( pParser, yyvsp[-2].m_sValue, 0, yyvsp[0].m_iValue );
		;}
    break;

  case 40:

    {
			yyerror ( pParser, "only >=, <=, and BETWEEN floating-point filter types are supported in this version" );
			YYERROR;
		;}
    break;

  case 41:

    {
			AddFloatRangeFilter ( pParser, yyvsp[-4].m_sValue, yyvsp[-2].m_fValue, yyvsp[0].m_fValue );
		;}
    break;

  case 42:

    {
			AddFloatRangeFilter ( pParser, yyvsp[-2].m_sValue, yyvsp[0].m_fValue, FLT_MAX );
		;}
    break;

  case 43:

    {
			AddFloatRangeFilter ( pParser, yyvsp[-2].m_sValue, -FLT_MAX, yyvsp[0].m_fValue );
		;}
    break;

  case 44:

    { yyval.m_dValues.Add ( yyvsp[0].m_iValue ); ;}
    break;

  case 45:

    { yyval.m_dValues.Add ( yyvsp[0].m_iValue ); ;}
    break;

  case 48:

    {
			pParser->m_pQuery->m_eGroupFunc = SPH_GROUPBY_ATTR;
			pParser->m_pQuery->m_sGroupBy = yyvsp[0].m_sValue;
		;}
    break;

  case 51:

    {
			pParser->m_pQuery->m_sSortBy.SetBinary ( pParser->m_pBuf+yyvsp[0].m_iStart, yyvsp[0].m_iEnd-yyvsp[0].m_iStart );
		;}
    break;

  case 54:

    {
			pParser->m_pQuery->m_sOrderBy.SetBinary ( pParser->m_pBuf+yyvsp[0].m_iStart, yyvsp[0].m_iEnd-yyvsp[0].m_iStart );
		;}
    break;

  case 56:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 58:

    { yyval = yyvsp[-1]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 59:

    { yyval = yyvsp[-1]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 62:

    {
			pParser->m_pQuery->m_iOffset = 0;
			pParser->m_pQuery->m_iLimit = yyvsp[0].m_iValue;
		;}
    break;

  case 63:

    {
			pParser->m_pQuery->m_iOffset = yyvsp[-2].m_iValue;
			pParser->m_pQuery->m_iLimit = yyvsp[0].m_iValue;
		;}
    break;

  case 69:

    { if ( !pParser->AddOption ( yyvsp[-2], yyvsp[0] ) ) YYERROR; ;}
    break;

  case 70:

    { if ( !pParser->AddOption ( yyvsp[-2], yyvsp[0] ) ) YYERROR; ;}
    break;

  case 74:

    { yyval = yyvsp[-1]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 75:

    { yyval = yyvsp[-1]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 76:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 77:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 78:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 79:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 80:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 81:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 82:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 83:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 84:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 85:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 86:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 87:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 88:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 90:

    { yyval = yyvsp[-3]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 91:

    { yyval = yyvsp[-3]; yyval.m_iEnd = yyvsp[0].m_iEnd; ;}
    break;

  case 92:

    { yyval = yyvsp[-2]; yyval.m_iEnd = yyvsp[0].m_iEnd ;}
    break;

  case 93:

    { yyval = yyvsp[-5]; yyval.m_iEnd = yyvsp[0].m_iEnd ;}
    break;

  case 94:

    { yyval = yyvsp[-5]; yyval.m_iEnd = yyvsp[0].m_iEnd ;}
    break;

  case 97:

    { pParser->m_pStmt->m_eStmt = STMT_SHOW_WARNINGS; ;}
    break;

  case 98:

    { pParser->m_pStmt->m_eStmt = STMT_SHOW_STATUS; ;}
    break;

  case 99:

    { pParser->m_pStmt->m_eStmt = STMT_SHOW_META; ;}
    break;

  case 100:

    {
			pParser->m_pStmt->m_eStmt = STMT_INSERT_INTO;
			pParser->m_pStmt->m_sInsertIndex = yyvsp[-7].m_sValue;
			pParser->m_pStmt->m_tInsertDocinfo.m_iDocID = yyvsp[-3].m_iValue;
		;}
    break;

  case 103:

    { pParser->m_pStmt->m_dInsertValues.Add ( yyvsp[0].m_sValue ); ;}
    break;

  case 104:

    { pParser->m_pStmt->m_dInsertValues.Add ( yyvsp[0].m_sValue ); ;}
    break;

  case 106:

    {
			pParser->m_pStmt->m_eStmt = STMT_DELETE_FROM;
			pParser->m_pStmt->m_sDeleteIndex = yyvsp[-4].m_sValue;
			pParser->m_pStmt->m_iDeleteID = yyvsp[0].m_iValue;
		;}
    break;


    }

/* Line 991 of yacc.c.  */


  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  char *yymsg;
	  int yyx, yycount;

	  yycount = 0;
	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  for (yyx = yyn < 0 ? -yyn : 0;
	       yyx < (int) (sizeof (yytname) / sizeof (char *)); yyx++)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      yysize += yystrlen (yytname[yyx]) + 15, yycount++;
	  yysize += yystrlen ("syntax error, unexpected ") + 1;
	  yysize += yystrlen (yytname[yytype]);
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yycount = 0;
		  for (yyx = yyn < 0 ? -yyn : 0;
		       yyx < (int) (sizeof (yytname) / sizeof (char *));
		       yyx++)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			const char *yyq = ! yycount ? ", expecting " : " or ";
			yyp = yystpcpy (yyp, yyq);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yycount++;
		      }
		}
	      yyerror (pParser, yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror (pParser, "syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (pParser, "syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      /* Return failure if at end of input.  */
      if (yychar == YYEOF)
        {
	  /* Pop the error token.  */
          YYPOPSTACK;
	  /* Pop the rest of the stack.  */
	  while (yyss < yyssp)
	    {
	      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
	      yydestruct (yystos[*yyssp], yyvsp);
	      YYPOPSTACK;
	    }
	  YYABORT;
        }

      YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
      yydestruct (yytoken, &yylval);
      yychar = YYEMPTY;

    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab2;


/*----------------------------------------------------.
| yyerrlab1 -- error raised explicitly by an action.  |
`----------------------------------------------------*/
yyerrlab1:

  /* Suppress GCC warning that yyerrlab1 is unused when no action
     invokes YYERROR.  */
#if defined (__GNUC_MINOR__) && 2093 <= (__GNUC__ * 1000 + __GNUC_MINOR__)
//  __attribute__ ((__unused__))
#endif


  goto yyerrlab2;


/*---------------------------------------------------------------.
| yyerrlab2 -- pop states until the error token can be shifted.  |
`---------------------------------------------------------------*/
yyerrlab2:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      yyvsp--;
      yystate = *--yyssp;

      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror (pParser, "parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}





#if USE_WINDOWS
#pragma warning(pop)
#endif

