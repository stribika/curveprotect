/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison implementation for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0

/* Substitute the variable and function names.  */
#define yyparse         cf_parse
#define yylex           cf_lex
#define yyerror         cf_error
#define yylval          cf_lval
#define yychar          cf_char
#define yydebug         cf_debug
#define yynerrs         cf_nerrs


/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 23 "cf-parse.y"

/* Headers */
#include <config.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include "common/sockaddr.h"
#include "libknot/dname.h"
#include "libknot/binary.h"
#include "libknot/edns.h"
#include "knot/conf/conf.h"
#include "libknotd_la-cf-parse.h" /* Automake generated header. */

extern int cf_lex (YYSTYPE *lvalp, void *scanner);
extern void cf_error(void *scanner, const char *format, ...);
extern conf_t *new_config;
static conf_iface_t *this_iface = 0;
static conf_iface_t *this_remote = 0;
static conf_zone_t *this_zone = 0;
static conf_group_t *this_group = 0;
static list *this_list = 0;
static conf_log_t *this_log = 0;
static conf_log_map_t *this_logmap = 0;
//#define YYERROR_VERBOSE 1

static void conf_init_iface(void *scanner, char* ifname, int port)
{
   this_iface = malloc(sizeof(conf_iface_t));
   if (this_iface == NULL) {
      cf_error(scanner, "not enough memory when allocating interface");
      return;
   }
   memset(this_iface, 0, sizeof(conf_iface_t));
   this_iface->name = ifname;
   this_iface->port = port;
}

static void conf_start_iface(void *scanner, char* ifname)
{
   conf_init_iface(scanner, ifname, -1);
   add_tail(&new_config->ifaces, &this_iface->n);
   ++new_config->ifaces_count;
}

static conf_iface_t *conf_get_remote(const char *name)
{
	conf_iface_t *remote;
	WALK_LIST (remote, new_config->remotes) {
		if (strcmp(remote->name, name) == 0) {
			return remote;
		}
	}

	return NULL;
}

static void conf_start_remote(void *scanner, char *remote)
{
   if (conf_get_remote(remote) != NULL) {
      cf_error(scanner, "remote '%s' already defined", remote);
      return;
   }

   this_remote = malloc(sizeof(conf_iface_t));
   if (this_remote == NULL) {
      cf_error(scanner, "not enough memory when allocating remote");
      return;
   }

   memset(this_remote, 0, sizeof(conf_iface_t));
   this_remote->name = remote;
   add_tail(&new_config->remotes, &this_remote->n);
   sockaddr_init(&this_remote->via, -1);
   ++new_config->remotes_count;
}

static void conf_remote_set_via(void *scanner, char *item) {
   /* Find existing node in interfaces. */
   node* r = 0; conf_iface_t* found = 0;
   WALK_LIST (r, new_config->ifaces) {
      if (strcmp(((conf_iface_t*)r)->name, item) == 0) {
         found = (conf_iface_t*)r;
         break;
      }
   }

   /* Check */
   if (!found) {
      cf_error(scanner, "interface '%s' is not defined", item);
   } else {
      sockaddr_set(&this_remote->via, found->family, found->address, 0);
   }
}

static conf_group_t *conf_get_group(const char *name)
{
	conf_group_t *group;
	WALK_LIST (group, new_config->groups) {
		if (strcmp(group->name, name) == 0) {
			return group;
		}
	}

	return NULL;
}

static void conf_start_group(void *scanner, char *name)
{
	conf_group_t *group = conf_get_group(name);
	if (group) {
		cf_error(scanner, "group '%s' already defined", name);
		return;
	}

	if (conf_get_remote(name) != NULL) {
		cf_error(scanner, "group name '%s' conflicts with remote name",
		         name);
		free(name);
		return;
	}

	/* Add new group. */

	group = calloc(1, sizeof(conf_group_t));
	if (!group) {
		cf_error(scanner, "out of memory");
		free(name);
		return;
	}

	group->name = name;
	init_list(&group->remotes);

	add_tail(&new_config->groups, &group->n);
	this_group = group;
}

static void conf_add_member_into_group(void *scanner, char *name)
{
	if (!this_group) {
		cf_error(scanner, "parser error, variable 'this_group' null");
		free(name);
		return;
	}

	if (conf_get_remote(name) == NULL) {
		cf_error(scanner, "remote '%s' is not defined", name);
		free(name);
		return;
	}

	// add the remote into the group while silently ignoring duplicates

	conf_group_remote_t *remote;
	node *n;
	WALK_LIST (n, this_group->remotes) {
		remote = (conf_group_remote_t *)n;
		if (strcmp(remote->name, name) == 0) {
			free(name);
			return;
		}
	}

	remote = calloc(1, sizeof(conf_group_remote_t));
	remote->name = name;
	add_tail(&this_group->remotes, &remote->n);
}

static bool set_remote_or_group(void *scanner, char *name,
				void (*install)(void *, conf_iface_t *))
{
	// search remotes

	conf_iface_t *remote = conf_get_remote(name);
	if (remote) {
		install(scanner, remote);
		return true;
	}

	// search groups

	conf_group_t *group = conf_get_group(name);
	if (group) {
		conf_group_remote_t *group_remote;
		WALK_LIST (group_remote, group->remotes) {
			remote = conf_get_remote(group_remote->name);
			if (!remote)
				continue;
			install(scanner, remote);
		}

		return true;
	}

	return false;
}

static void conf_acl_item_install(void *scanner, conf_iface_t *found)
{
	// silently skip duplicates

	conf_remote_t *remote;
	WALK_LIST (remote, *this_list) {
		if (remote->remote == found) {
			return;
		}
	}

	// additional check for transfers

	if ((this_list == &this_zone->acl.xfr_in ||
	    this_list == &this_zone->acl.notify_out) && found->port == 0)
	{
		cf_error(scanner, "remote specified for XFR/IN or "
		"NOTIFY/OUT needs to have valid port!");
		return;
	}

	// add into the list

	remote = malloc(sizeof(conf_remote_t));
	if (!remote) {
		cf_error(scanner, "out of memory");
		return;
	}

	remote->remote = found;
	add_tail(this_list, &remote->n);
}

static void conf_acl_item(void *scanner, char *item)
{
	if (!set_remote_or_group(scanner, item, conf_acl_item_install)) {
		cf_error(scanner, "remote or group '%s' not defined", item);
	}

	free(item);
}

static int conf_key_exists(void *scanner, char *item)
{
    /* Find existing node in keys. */
    knot_dname_t *sample = knot_dname_new_from_str(item, strlen(item), 0);
    conf_key_t* r = 0;
    WALK_LIST (r, new_config->keys) {
        if (knot_dname_compare(r->k.name, sample) == 0) {
           cf_error(scanner, "key '%s' is already defined", item);
	   knot_dname_free(&sample);
           return 1;
        }
    }

    knot_dname_free(&sample);
    return 0;
}

static int conf_key_add(void *scanner, knot_tsig_key_t **key, char *item)
{
    /* Reset */
    *key = 0;

    /* Find in keys */
    knot_dname_t *sample = knot_dname_new_from_str(item, strlen(item), 0);

    conf_key_t* r = 0;
    WALK_LIST (r, new_config->keys) {
        if (knot_dname_compare(r->k.name, sample) == 0) {
           *key = &r->k;
           knot_dname_free(&sample);
           return 0;
        }
    }

    cf_error(scanner, "key '%s' is not defined", item);
    knot_dname_free(&sample);
    return 1;
}

static void conf_zone_start(void *scanner, char *name) {
   this_zone = malloc(sizeof(conf_zone_t));
   if (this_zone == NULL || name == NULL) {
      cf_error(scanner, "out of memory while allocating zone config");
      return;
   }
   memset(this_zone, 0, sizeof(conf_zone_t));
   this_zone->enable_checks = -1; // Default policy applies
   this_zone->notify_timeout = -1; // Default policy applies
   this_zone->notify_retries = 0; // Default policy applies
   this_zone->ixfr_fslimit = -1; // Default policy applies
   this_zone->dbsync_timeout = -1; // Default policy applies
   this_zone->disable_any = -1; // Default policy applies
   this_zone->build_diffs = -1; // Default policy applies

   // Append mising dot to ensure FQDN
   size_t nlen = strlen(name);
   if (name[nlen - 1] != '.') {
      this_zone->name = malloc(nlen + 2);
      if (this_zone->name != NULL) {
	memcpy(this_zone->name, name, nlen);
	this_zone->name[nlen] = '.';
	this_zone->name[++nlen] = '\0';
     }
     free(name);
   } else {
      this_zone->name = name; /* Already FQDN */
   }

   /* Initialize ACL lists. */
   init_list(&this_zone->acl.xfr_in);
   init_list(&this_zone->acl.xfr_out);
   init_list(&this_zone->acl.notify_in);
   init_list(&this_zone->acl.notify_out);
   init_list(&this_zone->acl.update_in);

   /* Check domain name. */
   knot_dname_t *dn = NULL;
   if (this_zone->name != NULL) {
      dn = knot_dname_new_from_str(this_zone->name, nlen, 0);
   }
   if (dn == NULL) {
     free(this_zone->name);
     free(this_zone);
     this_zone = NULL;
     cf_error(scanner, "invalid zone origin");
   } else {
     /* Check for duplicates. */
     if (hattrie_tryget(new_config->names, (const char*)dn->name, dn->size) != NULL) {
           cf_error(scanner, "zone '%s' is already present, refusing to "
			     "duplicate", this_zone->name);
           knot_dname_free(&dn);
           free(this_zone->name);
           this_zone->name = NULL;
           /* Must not free, some versions of flex might continue after error and segfault.
            * free(this_zone); this_zone = NULL;
            */
           return;
     }

     /* Directly discard dname, won't be needed. */
     add_tail(&new_config->zones, &this_zone->n);
     *hattrie_get(new_config->names, (const char*)dn->name, dn->size) = (void *)1;
     ++new_config->zones_count;
     knot_dname_free(&dn);
   }
}

static int conf_mask(void* scanner, int nval, int prefixlen) {
    if (nval < 0 || nval > prefixlen) {
        cf_error(scanner, "IPv%c subnet prefix '%d' is out of range <0,%d>",
                 prefixlen == IPV4_PREFIXLEN ? '4' : '6', nval, prefixlen);
        return prefixlen; /* single host */
    }
    return nval;
}

/*! \brief Replace string value. */
static void opt_replace(char **opt, char *new_opt, bool val)
{
	/* Clear old value. */
	free(*opt);
	*opt = NULL;
	/* Replace if val is True. */
	if (val) {
		*opt = new_opt;
	} else {
		free(new_opt);
	}
}

/*! \brief Generate automatic defaults for server identity, version and NSID. */
static void ident_auto(int tok, conf_t *conf, bool val)
{
	switch(tok) {
	case SVERSION:
		opt_replace(&conf->version, strdup("Knot DNS " PACKAGE_VERSION), val);
		break;
	case IDENTITY:
		opt_replace(&conf->identity, sockaddr_hostname(), val);
		break;
	case NSID:
		opt_replace(&conf->nsid, sockaddr_hostname(), val);
		if (conf->nsid) {
			conf->nsid_len = strlen(conf->nsid);
		}
		break;
	default:
		break;
	}
}



/* Line 268 of yacc.c  */
#line 474 "knot/conf/libknotd_la-cf-parse.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     END = 258,
     INVALID_TOKEN = 259,
     TEXT = 260,
     HEXSTR = 261,
     NUM = 262,
     INTERVAL = 263,
     SIZE = 264,
     BOOL = 265,
     SYSTEM = 266,
     IDENTITY = 267,
     HOSTNAME = 268,
     SVERSION = 269,
     NSID = 270,
     STORAGE = 271,
     KEY = 272,
     KEYS = 273,
     MAX_UDP_PAYLOAD = 274,
     TSIG_ALGO_NAME = 275,
     WORKERS = 276,
     USER = 277,
     RUNDIR = 278,
     PIDFILE = 279,
     REMOTES = 280,
     GROUPS = 281,
     ZONES = 282,
     FILENAME = 283,
     DISABLE_ANY = 284,
     SEMANTIC_CHECKS = 285,
     NOTIFY_RETRIES = 286,
     NOTIFY_TIMEOUT = 287,
     DBSYNC_TIMEOUT = 288,
     IXFR_FSLIMIT = 289,
     XFR_IN = 290,
     XFR_OUT = 291,
     UPDATE_IN = 292,
     NOTIFY_IN = 293,
     NOTIFY_OUT = 294,
     BUILD_DIFFS = 295,
     MAX_CONN_IDLE = 296,
     MAX_CONN_HS = 297,
     MAX_CONN_REPLY = 298,
     RATE_LIMIT = 299,
     RATE_LIMIT_SIZE = 300,
     RATE_LIMIT_SLIP = 301,
     TRANSFERS = 302,
     INTERFACES = 303,
     ADDRESS = 304,
     PORT = 305,
     IPA = 306,
     IPA6 = 307,
     VIA = 308,
     CONTROL = 309,
     ALLOW = 310,
     LISTEN_ON = 311,
     LOG = 312,
     LOG_DEST = 313,
     LOG_SRC = 314,
     LOG_LEVEL = 315
   };
#endif
/* Tokens.  */
#define END 258
#define INVALID_TOKEN 259
#define TEXT 260
#define HEXSTR 261
#define NUM 262
#define INTERVAL 263
#define SIZE 264
#define BOOL 265
#define SYSTEM 266
#define IDENTITY 267
#define HOSTNAME 268
#define SVERSION 269
#define NSID 270
#define STORAGE 271
#define KEY 272
#define KEYS 273
#define MAX_UDP_PAYLOAD 274
#define TSIG_ALGO_NAME 275
#define WORKERS 276
#define USER 277
#define RUNDIR 278
#define PIDFILE 279
#define REMOTES 280
#define GROUPS 281
#define ZONES 282
#define FILENAME 283
#define DISABLE_ANY 284
#define SEMANTIC_CHECKS 285
#define NOTIFY_RETRIES 286
#define NOTIFY_TIMEOUT 287
#define DBSYNC_TIMEOUT 288
#define IXFR_FSLIMIT 289
#define XFR_IN 290
#define XFR_OUT 291
#define UPDATE_IN 292
#define NOTIFY_IN 293
#define NOTIFY_OUT 294
#define BUILD_DIFFS 295
#define MAX_CONN_IDLE 296
#define MAX_CONN_HS 297
#define MAX_CONN_REPLY 298
#define RATE_LIMIT 299
#define RATE_LIMIT_SIZE 300
#define RATE_LIMIT_SLIP 301
#define TRANSFERS 302
#define INTERFACES 303
#define ADDRESS 304
#define PORT 305
#define IPA 306
#define IPA6 307
#define VIA 308
#define CONTROL 309
#define ALLOW 310
#define LISTEN_ON 311
#define LOG 312
#define LOG_DEST 313
#define LOG_SRC 314
#define LOG_LEVEL 315




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 293 of yacc.c  */
#line 422 "cf-parse.y"

    struct {
       char *t;
       long i;
       size_t l;
       knot_tsig_algorithm_t alg;
    } tok;



/* Line 293 of yacc.c  */
#line 641 "knot/conf/libknotd_la-cf-parse.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 653 "knot/conf/libknotd_la-cf-parse.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  3
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   271

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  67
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  35
/* YYNRULES -- Number of rules.  */
#define YYNRULES  151
/* YYNRULES -- Number of states.  */
#define YYNSTATES  300

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   315

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    66,     2,     2,    65,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    61,
       2,     2,     2,     2,    62,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    63,     2,    64,     2,     2,     2,     2,
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
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     6,     7,    10,    11,    13,    15,    17,
      19,    21,    23,    24,    29,    34,    41,    46,    53,    56,
      62,    65,    70,    75,    80,    85,    90,    95,   100,   105,
     110,   115,   120,   125,   131,   136,   141,   146,   151,   156,
     161,   166,   171,   176,   181,   184,   190,   191,   193,   195,
     197,   199,   201,   202,   207,   212,   219,   226,   231,   238,
     245,   250,   255,   260,   265,   268,   274,   276,   277,   279,
     283,   285,   288,   294,   296,   298,   300,   302,   304,   305,
     307,   309,   311,   313,   315,   316,   320,   324,   325,   329,
     330,   332,   334,   336,   338,   340,   342,   346,   348,   351,
     357,   361,   366,   371,   376,   381,   386,   391,   396,   401,
     406,   411,   414,   418,   423,   428,   433,   438,   443,   448,
     453,   458,   463,   464,   466,   470,   474,   475,   479,   481,
     484,   485,   486,   492,   498,   499,   505,   507,   509,   512,
     518,   523,   529,   533,   535,   538,   541,   544,   547,   550,
     553,   556
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      68,     0,    -1,    69,     3,    -1,    -1,    69,   101,    -1,
      -1,     5,    -1,    25,    -1,    59,    -1,    57,    -1,    60,
      -1,    54,    -1,    -1,    71,    50,     7,    61,    -1,    71,
      49,    51,    61,    -1,    71,    49,    51,    62,     7,    61,
      -1,    71,    49,    52,    61,    -1,    71,    49,    52,    62,
       7,    61,    -1,    48,    63,    -1,    72,    70,    63,    71,
      64,    -1,    11,    63,    -1,    73,    14,     5,    61,    -1,
      73,    14,    10,    61,    -1,    73,    12,     5,    61,    -1,
      73,    12,    10,    61,    -1,    73,    13,     5,    61,    -1,
      73,    15,     6,    61,    -1,    73,    15,     5,    61,    -1,
      73,    15,    10,    61,    -1,    73,    19,     7,    61,    -1,
      73,    16,     5,    61,    -1,    73,    23,     5,    61,    -1,
      73,    24,     5,    61,    -1,    73,    17,    20,     5,    61,
      -1,    73,    21,     7,    61,    -1,    73,    22,     5,    61,
      -1,    73,    41,     8,    61,    -1,    73,    42,     8,    61,
      -1,    73,    43,     8,    61,    -1,    73,    44,     7,    61,
      -1,    73,    45,     9,    61,    -1,    73,    45,     7,    61,
      -1,    73,    46,     7,    61,    -1,    73,    47,     7,    61,
      -1,    18,    63,    -1,    74,     5,    20,     5,    61,    -1,
      -1,     5,    -1,    59,    -1,    57,    -1,    60,    -1,    54,
      -1,    -1,    76,    50,     7,    61,    -1,    76,    49,    51,
      61,    -1,    76,    49,    51,    65,     7,    61,    -1,    76,
      49,    51,    62,     7,    61,    -1,    76,    49,    52,    61,
      -1,    76,    49,    52,    65,     7,    61,    -1,    76,    49,
      52,    62,     7,    61,    -1,    76,    17,     5,    61,    -1,
      76,    53,    51,    61,    -1,    76,    53,    52,    61,    -1,
      76,    53,     5,    61,    -1,    25,    63,    -1,    77,    75,
      63,    76,    64,    -1,     5,    -1,    -1,    78,    -1,    79,
      66,    78,    -1,     5,    -1,    26,    63,    -1,    81,    80,
      63,    79,    64,    -1,    35,    -1,    36,    -1,    38,    -1,
      39,    -1,    37,    -1,    -1,     5,    -1,    59,    -1,    57,
      -1,    60,    -1,    54,    -1,    -1,    84,    83,    66,    -1,
      84,    83,    61,    -1,    -1,    85,     5,    61,    -1,    -1,
      22,    -1,    25,    -1,    59,    -1,    57,    -1,    60,    -1,
      54,    -1,     7,    65,     5,    -1,     5,    -1,    86,    63,
      -1,    87,    82,    63,    85,    64,    -1,    87,    82,    84,
      -1,    87,    28,     5,    61,    -1,    87,    40,    10,    61,
      -1,    87,    30,    10,    61,    -1,    87,    29,    10,    61,
      -1,    87,    33,     7,    61,    -1,    87,    33,     8,    61,
      -1,    87,    34,     9,    61,    -1,    87,    34,     7,    61,
      -1,    87,    31,     7,    61,    -1,    87,    32,     7,    61,
      -1,    27,    63,    -1,    88,    87,    64,    -1,    88,    29,
      10,    61,    -1,    88,    40,    10,    61,    -1,    88,    30,
      10,    61,    -1,    88,    34,     9,    61,    -1,    88,    34,
       7,    61,    -1,    88,    31,     7,    61,    -1,    88,    32,
       7,    61,    -1,    88,    33,     7,    61,    -1,    88,    33,
       8,    61,    -1,    -1,    89,    -1,    90,    60,    66,    -1,
      90,    60,    61,    -1,    -1,    91,    59,    90,    -1,    58,
      -1,    28,     5,    -1,    -1,    -1,    95,    92,    63,    91,
      64,    -1,    95,    93,    63,    91,    64,    -1,    -1,    57,
      97,    63,    95,    94,    -1,    56,    -1,    55,    -1,    54,
      63,    -1,   100,    98,    63,    71,    64,    -1,   100,    98,
       5,    61,    -1,   100,    99,    63,    85,    64,    -1,   100,
      99,    84,    -1,    61,    -1,    73,    64,    -1,    72,    64,
      -1,    74,    64,    -1,    77,    64,    -1,    81,    64,    -1,
      88,    64,    -1,    96,    64,    -1,   100,    64,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   485,   485,   487,   489,   492,   493,   494,   495,   496,
     497,   498,   501,   502,   509,   517,   530,   538,   554,   555,
     563,   564,   565,   566,   567,   568,   573,   574,   575,   576,
     584,   585,   586,   587,   592,   599,   620,   621,   622,   623,
     624,   625,   626,   627,   631,   632,   684,   685,   686,   687,
     688,   689,   692,   693,   700,   709,   718,   732,   741,   750,
     764,   772,   776,   780,   787,   788,   796,   799,   801,   802,
     806,   810,   811,   815,   818,   821,   824,   827,   832,   833,
     834,   835,   836,   837,   840,   841,   842,   845,   846,   874,
     875,   876,   877,   878,   879,   880,   881,   898,   902,   903,
     904,   905,   906,   907,   908,   909,   910,   911,   912,   913,
     920,   930,   931,   932,   933,   934,   935,   936,   937,   944,
     951,   958,   961,   970,   971,   972,   975,   976,   982,  1005,
    1032,  1036,  1037,  1038,  1041,  1041,  1045,  1049,  1055,  1056,
    1063,  1069,  1070,  1073,  1073,  1073,  1073,  1073,  1073,  1073,
    1073,  1073
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "END", "INVALID_TOKEN", "TEXT", "HEXSTR",
  "NUM", "INTERVAL", "SIZE", "BOOL", "SYSTEM", "IDENTITY", "HOSTNAME",
  "SVERSION", "NSID", "STORAGE", "KEY", "KEYS", "MAX_UDP_PAYLOAD",
  "TSIG_ALGO_NAME", "WORKERS", "USER", "RUNDIR", "PIDFILE", "REMOTES",
  "GROUPS", "ZONES", "FILENAME", "DISABLE_ANY", "SEMANTIC_CHECKS",
  "NOTIFY_RETRIES", "NOTIFY_TIMEOUT", "DBSYNC_TIMEOUT", "IXFR_FSLIMIT",
  "XFR_IN", "XFR_OUT", "UPDATE_IN", "NOTIFY_IN", "NOTIFY_OUT",
  "BUILD_DIFFS", "MAX_CONN_IDLE", "MAX_CONN_HS", "MAX_CONN_REPLY",
  "RATE_LIMIT", "RATE_LIMIT_SIZE", "RATE_LIMIT_SLIP", "TRANSFERS",
  "INTERFACES", "ADDRESS", "PORT", "IPA", "IPA6", "VIA", "CONTROL",
  "ALLOW", "LISTEN_ON", "LOG", "LOG_DEST", "LOG_SRC", "LOG_LEVEL", "';'",
  "'@'", "'{'", "'}'", "'/'", "','", "$accept", "config", "conf_entries",
  "interface_start", "interface", "interfaces", "system", "keys",
  "remote_start", "remote", "remotes", "group_member", "group",
  "group_start", "groups", "zone_acl_start", "zone_acl_item",
  "zone_acl_list", "zone_acl", "zone_start", "zone", "zones",
  "log_prios_start", "log_prios", "log_src", "log_dest", "log_file",
  "log_end", "log_start", "log", "$@1", "ctl_listen_start",
  "ctl_allow_start", "control", "conf", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,   302,   303,   304,
     305,   306,   307,   308,   309,   310,   311,   312,   313,   314,
     315,    59,    64,   123,   125,    47,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    67,    68,    69,    69,    70,    70,    70,    70,    70,
      70,    70,    71,    71,    71,    71,    71,    71,    72,    72,
      73,    73,    73,    73,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    73,    73,    73,
      73,    73,    73,    73,    74,    74,    75,    75,    75,    75,
      75,    75,    76,    76,    76,    76,    76,    76,    76,    76,
      76,    76,    76,    76,    77,    77,    78,    79,    79,    79,
      80,    81,    81,    82,    82,    82,    82,    82,    83,    83,
      83,    83,    83,    83,    84,    84,    84,    85,    85,    86,
      86,    86,    86,    86,    86,    86,    86,    86,    87,    87,
      87,    87,    87,    87,    87,    87,    87,    87,    87,    87,
      87,    88,    88,    88,    88,    88,    88,    88,    88,    88,
      88,    88,    89,    90,    90,    90,    91,    91,    92,    93,
      94,    95,    95,    95,    97,    96,    98,    99,   100,   100,
     100,   100,   100,   101,   101,   101,   101,   101,   101,   101,
     101,   101
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     0,     2,     0,     1,     1,     1,     1,
       1,     1,     0,     4,     4,     6,     4,     6,     2,     5,
       2,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     5,     4,     4,     4,     4,     4,     4,
       4,     4,     4,     4,     2,     5,     0,     1,     1,     1,
       1,     1,     0,     4,     4,     6,     6,     4,     6,     6,
       4,     4,     4,     4,     2,     5,     1,     0,     1,     3,
       1,     2,     5,     1,     1,     1,     1,     1,     0,     1,
       1,     1,     1,     1,     0,     3,     3,     0,     3,     0,
       1,     1,     1,     1,     1,     1,     3,     1,     2,     5,
       3,     4,     4,     4,     4,     4,     4,     4,     4,     4,
       4,     2,     3,     4,     4,     4,     4,     4,     4,     4,
       4,     4,     0,     1,     3,     3,     0,     3,     1,     2,
       0,     0,     5,     5,     0,     5,     1,     1,     2,     5,
       4,     5,     3,     1,     2,     2,     2,     2,     2,     2,
       2,     2
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       3,     0,     0,     1,     2,     0,     0,     0,     0,     0,
       0,     0,   134,   143,     5,     0,     0,    46,     0,    89,
       0,     0,     4,    20,    44,    64,    71,   111,    18,   138,
       0,     6,     7,    11,     9,     8,    10,   145,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   144,     0,   146,
      47,    51,    49,    48,    50,   147,     0,    70,   148,     0,
      97,     0,    90,    91,     0,     0,     0,     0,     0,     0,
       0,    95,    93,    92,    94,   149,     0,     0,   150,   137,
     136,   151,     0,    84,   131,    12,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      52,    67,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    98,     0,     0,     0,     0,     0,     0,     0,
      73,    74,    77,    75,    76,     0,   112,    84,     0,    12,
      87,   142,   130,     0,    23,    24,    25,    21,    22,    27,
      26,    28,    30,     0,    29,    34,    35,    31,    32,    36,
      37,    38,    39,    41,    40,    42,    43,     0,     0,    66,
      68,     0,    96,   113,   115,   118,   119,   120,   121,   117,
     116,   114,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    87,   100,   140,     0,     0,    79,    83,    81,
      80,    82,     0,     0,   128,     0,     0,   135,     0,     0,
      19,    33,    45,     0,     0,     0,     0,    65,    72,     0,
     101,   104,   103,   109,   110,   105,   106,   108,   107,   102,
       0,   139,     0,   141,    86,    85,   129,   126,   126,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    69,
      99,    88,     0,     0,    14,     0,    16,     0,    13,    60,
      54,     0,     0,    57,     0,     0,    53,    63,    61,    62,
     122,   132,   133,     0,     0,     0,     0,     0,     0,   123,
     127,    15,    17,    56,    55,    59,    58,     0,   125,   124
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,     2,    38,   153,    14,    15,    16,    66,   178,
      17,   180,   181,    69,    18,   147,   212,   151,   206,    86,
      87,    19,   289,   290,   262,   215,   216,   217,   152,    20,
      30,    92,    93,    21,    22
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -49
static const yytype_int16 yypact[] =
{
     -49,     9,    66,   -49,   -49,   -43,   -25,    31,    34,    52,
      56,    61,   -49,   -49,    14,    87,     0,    22,     1,     3,
     -48,   -34,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,
      63,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   101,     8,
      42,    21,    35,    53,   115,    65,   143,   158,   160,   161,
     159,   162,   163,   165,    37,   167,   168,   -49,   148,   -49,
     -49,   -49,   -49,   -49,   -49,   -49,   113,   -49,   -49,   114,
     -49,   104,   -49,   -49,   169,   170,   171,   174,   110,    78,
     172,   -49,   -49,   -49,   -49,   -49,   120,   109,   -49,   -49,
     -49,   -49,    -4,   121,   -49,   -49,   124,   125,   126,   127,
     128,   129,   130,   131,   132,   189,   134,   135,   136,   137,
     138,   139,   140,   141,   142,   144,   145,   146,   149,   199,
     -49,   203,   204,   150,   151,   152,   153,   154,   155,   156,
     157,   164,   -49,   214,   210,   211,   215,   216,   147,    89,
     -49,   -49,   -49,   -49,   -49,   217,   -49,   166,   173,   -49,
     -49,    -5,   -16,   -47,   -49,   -49,   -49,   -49,   -49,   -49,
     -49,   -49,   -49,   175,   -49,   -49,   -49,   -49,   -49,   -49,
     -49,   -49,   -49,   -49,   -49,   -49,   -49,   176,    72,   -49,
     -49,    41,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,
     -49,   -49,   177,   178,   179,   180,   181,   182,   183,   184,
     185,   186,   -49,    -5,   -49,   -35,     2,   -49,   -49,   -49,
     -49,   -49,   -13,   219,   -49,   187,   188,   -49,   -28,   221,
     -49,   -49,   -49,   225,   106,   224,    -1,   -49,   -49,   203,
     -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,   -49,
       6,   -49,   191,   -49,   -49,   -49,   -49,   -49,   -49,    98,
     100,   192,   193,    51,    91,   194,   195,   196,   197,   -49,
     -49,   -49,    16,    24,   -49,   226,   -49,   228,   -49,   -49,
     -49,   241,   242,   -49,   252,   253,   -49,   -49,   -49,   -49,
     -49,   -49,   -49,   200,   201,   202,   205,   206,   207,   -49,
     209,   -49,   -49,   -49,   -49,   -49,   -49,    29,   -49,   -49
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
     -49,   -49,   -49,   -49,    77,   -49,   -49,   -49,   -49,   -49,
     -49,    36,   -49,   -49,   -49,   -49,   -49,    85,    62,   -49,
     -49,   -49,   -49,   -49,    23,   -49,   -49,   -49,   -49,   -49,
     -49,   -49,   -49,   -49,   -49
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -79
static const yytype_int16 yytable[] =
{
     207,   148,   218,   219,   256,    58,    67,   242,    70,     3,
      71,   242,   213,    96,   218,   219,    88,   220,    97,    31,
      23,    89,    90,   249,   250,    72,    99,    60,    73,   241,
      91,   100,    74,    75,    76,    77,    78,    79,    24,    32,
     101,   102,   214,    80,   115,   103,   116,    98,   244,   208,
     257,   258,   209,   245,   210,   211,   -78,    81,   104,   149,
      82,   -78,    83,    84,    59,    68,   243,    85,    33,     4,
     260,    34,   106,    35,    36,   280,    61,     5,    37,    62,
     281,    63,    64,   280,     6,   129,    65,   130,   282,   223,
     298,     7,     8,     9,    25,   299,   199,    26,   200,    39,
      40,    41,    42,    43,    44,   228,    45,   229,    46,    47,
      48,    49,   270,   271,    10,    27,   272,   127,   128,    28,
      11,   224,   225,    12,    29,   226,    94,    13,    50,    51,
      52,    53,    54,    55,    56,   105,   227,   133,   134,   135,
     136,   137,   138,   139,   140,   141,   142,   143,   144,   145,
     107,    57,   273,   274,   197,   198,   275,   253,   254,   264,
     265,   266,   267,   108,    95,   109,   110,   111,   119,   122,
     112,   113,   114,   146,   117,   118,   120,   121,   125,   123,
     124,   126,   131,   132,   150,   154,   155,   156,   157,   158,
     159,   160,   161,   162,   163,   164,   165,   166,   167,   168,
     169,   170,   171,   172,   177,   173,   174,   175,   179,   182,
     176,   183,   184,   185,   186,   187,   188,   189,   190,   192,
     193,   194,   195,   196,   246,   191,   205,   201,   251,   202,
     252,   255,   203,   283,   204,   284,   221,   222,   230,   231,
     232,   233,   234,   235,   236,   237,   238,   239,   285,   286,
     247,   248,   261,   268,   269,   276,   277,   278,   279,   287,
     288,   291,   292,   293,   240,   259,   294,   295,   296,   297,
       0,   263
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-49))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
       5,     5,    49,    50,     5,     5,     5,     5,     5,     0,
       7,     5,    28,     5,    49,    50,    64,    64,    10,     5,
      63,    55,    56,    51,    52,    22,     5,     5,    25,    64,
      64,    10,    29,    30,    31,    32,    33,    34,    63,    25,
       5,     6,    58,    40,     7,    10,     9,     5,    61,    54,
      51,    52,    57,    66,    59,    60,    61,    54,     5,    63,
      57,    66,    59,    60,    64,    64,    64,    64,    54,     3,
      64,    57,     7,    59,    60,    59,    54,    11,    64,    57,
      64,    59,    60,    59,    18,     7,    64,     9,    64,    17,
      61,    25,    26,    27,    63,    66,     7,    63,     9,    12,
      13,    14,    15,    16,    17,    64,    19,    66,    21,    22,
      23,    24,    61,    62,    48,    63,    65,     7,     8,    63,
      54,    49,    50,    57,    63,    53,    63,    61,    41,    42,
      43,    44,    45,    46,    47,    20,    64,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
       7,    64,    61,    62,     7,     8,    65,    51,    52,    61,
      62,    61,    62,     5,    63,     5,     5,     8,    20,    65,
       8,     8,     7,    64,     7,     7,    63,    63,     7,    10,
      10,     7,    10,    63,    63,    61,    61,    61,    61,    61,
      61,    61,    61,    61,     5,    61,    61,    61,    61,    61,
      61,    61,    61,    61,     5,    61,    61,    61,     5,     5,
      61,    61,    61,    61,    61,    61,    61,    61,    61,     5,
      10,    10,     7,     7,     5,    61,   149,    10,     7,    63,
       5,     7,   147,     7,    61,     7,    61,    61,    61,    61,
      61,    61,    61,    61,    61,    61,    61,    61,     7,     7,
      63,    63,    61,    61,    61,    61,    61,    61,    61,     7,
       7,    61,    61,    61,   202,   229,    61,    61,    61,    60,
      -1,   248
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    68,    69,     0,     3,    11,    18,    25,    26,    27,
      48,    54,    57,    61,    72,    73,    74,    77,    81,    88,
      96,   100,   101,    63,    63,    63,    63,    63,    63,    63,
      97,     5,    25,    54,    57,    59,    60,    64,    70,    12,
      13,    14,    15,    16,    17,    19,    21,    22,    23,    24,
      41,    42,    43,    44,    45,    46,    47,    64,     5,    64,
       5,    54,    57,    59,    60,    64,    75,     5,    64,    80,
       5,     7,    22,    25,    29,    30,    31,    32,    33,    34,
      40,    54,    57,    59,    60,    64,    86,    87,    64,    55,
      56,    64,    98,    99,    63,    63,     5,    10,     5,     5,
      10,     5,     6,    10,     5,    20,     7,     7,     5,     5,
       5,     8,     8,     8,     7,     7,     9,     7,     7,    20,
      63,    63,    65,    10,    10,     7,     7,     7,     8,     7,
       9,    10,    63,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    64,    82,     5,    63,
      63,    84,    95,    71,    61,    61,    61,    61,    61,    61,
      61,    61,    61,     5,    61,    61,    61,    61,    61,    61,
      61,    61,    61,    61,    61,    61,    61,     5,    76,     5,
      78,    79,     5,    61,    61,    61,    61,    61,    61,    61,
      61,    61,     5,    10,    10,     7,     7,     7,     8,     7,
       9,    10,    63,    84,    61,    71,    85,     5,    54,    57,
      59,    60,    83,    28,    58,    92,    93,    94,    49,    50,
      64,    61,    61,    17,    49,    50,    53,    64,    64,    66,
      61,    61,    61,    61,    61,    61,    61,    61,    61,    61,
      85,    64,     5,    64,    61,    66,     5,    63,    63,    51,
      52,     7,     5,    51,    52,     7,     5,    51,    52,    78,
      64,    61,    91,    91,    61,    62,    61,    62,    61,    61,
      61,    62,    65,    61,    62,    65,    61,    61,    61,    61,
      59,    64,    64,     7,     7,     7,     7,     7,     7,    89,
      90,    61,    61,    61,    61,    61,    61,    60,    61,    66
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (scanner, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, scanner)
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
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value, scanner); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void *scanner)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep, scanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    void *scanner;
#endif
{
  if (!yyvaluep)
    return;
  YYUSE (scanner);
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep, void *scanner)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep, scanner)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
    void *scanner;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep, scanner);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule, void *scanner)
#else
static void
yy_reduce_print (yyvsp, yyrule, scanner)
    YYSTYPE *yyvsp;
    int yyrule;
    void *scanner;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       , scanner);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule, scanner); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
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
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep, void *scanner)
#else
static void
yydestruct (yymsg, yytype, yyvaluep, scanner)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
    void *scanner;
#endif
{
  YYUSE (yyvaluep);
  YYUSE (scanner);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void *scanner);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *scanner)
#else
int
yyparse (scanner)
    void *scanner;
#endif
#endif
{
/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

    /* Number of syntax errors so far.  */
    int yynerrs;

    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

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
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
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

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
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
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

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
        case 2:

/* Line 1806 of yacc.c  */
#line 485 "cf-parse.y"
    { return 0; }
    break;

  case 6:

/* Line 1806 of yacc.c  */
#line 493 "cf-parse.y"
    { conf_start_iface(scanner, (yyvsp[(1) - (1)].tok).t); }
    break;

  case 7:

/* Line 1806 of yacc.c  */
#line 494 "cf-parse.y"
    { conf_start_iface(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 8:

/* Line 1806 of yacc.c  */
#line 495 "cf-parse.y"
    { conf_start_iface(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 496 "cf-parse.y"
    { conf_start_iface(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 10:

/* Line 1806 of yacc.c  */
#line 497 "cf-parse.y"
    { conf_start_iface(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 11:

/* Line 1806 of yacc.c  */
#line 498 "cf-parse.y"
    { conf_start_iface(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 13:

/* Line 1806 of yacc.c  */
#line 502 "cf-parse.y"
    {
     if (this_iface->port > 0) {
       cf_error(scanner, "only one port definition is allowed in interface section\n");
     } else {
       this_iface->port = (yyvsp[(3) - (4)].tok).i;
     }
   }
    break;

  case 14:

/* Line 1806 of yacc.c  */
#line 509 "cf-parse.y"
    {
     if (this_iface->address != 0) {
       cf_error(scanner, "only one address is allowed in interface section\n");
     } else {
       this_iface->address = (yyvsp[(3) - (4)].tok).t;
       this_iface->family = AF_INET;
     }
   }
    break;

  case 15:

/* Line 1806 of yacc.c  */
#line 517 "cf-parse.y"
    {
     if (this_iface->address != 0) {
       cf_error(scanner, "only one address is allowed in interface section\n");
     } else {
       this_iface->address = (yyvsp[(3) - (6)].tok).t;
       this_iface->family = AF_INET;
       if (this_iface->port > 0) {
	 cf_error(scanner, "only one port definition is allowed in interface section\n");
       } else {
	 this_iface->port = (yyvsp[(5) - (6)].tok).i;
       }
     }
   }
    break;

  case 16:

/* Line 1806 of yacc.c  */
#line 530 "cf-parse.y"
    {
     if (this_iface->address != 0) {
       cf_error(scanner, "only one address is allowed in interface section\n");
     } else {
       this_iface->address = (yyvsp[(3) - (4)].tok).t;
       this_iface->family = AF_INET6;
     }
   }
    break;

  case 17:

/* Line 1806 of yacc.c  */
#line 538 "cf-parse.y"
    {
     if (this_iface->address != 0) {
       cf_error(scanner, "only one address is allowed in interface section\n");
     } else {
       this_iface->address = (yyvsp[(3) - (6)].tok).t;
       this_iface->family = AF_INET6;
       if (this_iface->port > 0) {
          cf_error(scanner, "only one port definition is allowed in interface section\n");
       } else {
          this_iface->port = (yyvsp[(5) - (6)].tok).i;
       }
     }
   }
    break;

  case 19:

/* Line 1806 of yacc.c  */
#line 555 "cf-parse.y"
    {
   if (this_iface->address == 0) {
     cf_error(scanner, "interface '%s' has no defined address", this_iface->name);
   }
 }
    break;

  case 21:

/* Line 1806 of yacc.c  */
#line 564 "cf-parse.y"
    { new_config->version = (yyvsp[(3) - (4)].tok).t; }
    break;

  case 22:

/* Line 1806 of yacc.c  */
#line 565 "cf-parse.y"
    { ident_auto(SVERSION, new_config, (yyvsp[(3) - (4)].tok).i); }
    break;

  case 23:

/* Line 1806 of yacc.c  */
#line 566 "cf-parse.y"
    { new_config->identity = (yyvsp[(3) - (4)].tok).t; }
    break;

  case 24:

/* Line 1806 of yacc.c  */
#line 567 "cf-parse.y"
    { ident_auto(IDENTITY, new_config, (yyvsp[(3) - (4)].tok).i); }
    break;

  case 25:

/* Line 1806 of yacc.c  */
#line 568 "cf-parse.y"
    {
     fprintf(stderr, "warning: Config option 'system.hostname' is deprecated. "
                     "Use 'system.identity' instead.\n");
     free((yyvsp[(3) - (4)].tok).t);
 }
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 573 "cf-parse.y"
    { new_config->nsid = (yyvsp[(3) - (4)].tok).t; new_config->nsid_len = (yyvsp[(3) - (4)].tok).l; }
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 574 "cf-parse.y"
    { new_config->nsid = (yyvsp[(3) - (4)].tok).t; new_config->nsid_len = strlen(new_config->nsid); }
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 575 "cf-parse.y"
    { ident_auto(NSID, new_config, (yyvsp[(3) - (4)].tok).i); }
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 576 "cf-parse.y"
    {
     if ((yyvsp[(3) - (4)].tok).i < EDNS_MIN_UDP_PAYLOAD || (yyvsp[(3) - (4)].tok).i > EDNS_MAX_UDP_PAYLOAD) {
        cf_error(scanner, "maximal UDP payload size is out of range (%u-%u)",
                 EDNS_MIN_UDP_PAYLOAD, EDNS_MAX_UDP_PAYLOAD);
     } else {
        new_config->max_udp_payload = (yyvsp[(3) - (4)].tok).i;
     }
 }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 584 "cf-parse.y"
    { new_config->storage = (yyvsp[(3) - (4)].tok).t; }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 585 "cf-parse.y"
    { new_config->rundir = (yyvsp[(3) - (4)].tok).t; }
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 586 "cf-parse.y"
    { new_config->pidfile = (yyvsp[(3) - (4)].tok).t; }
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 587 "cf-parse.y"
    {
     fprintf(stderr, "warning: Config option 'system.key' is deprecated "
                     "and has no effect.\n");
     free((yyvsp[(4) - (5)].tok).t);
 }
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 592 "cf-parse.y"
    {
     if ((yyvsp[(3) - (4)].tok).i <= 0) {
        cf_error(scanner, "worker count must be greater than 0\n");
     } else {
        new_config->workers = (yyvsp[(3) - (4)].tok).i;
     }
 }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 599 "cf-parse.y"
    {
     new_config->uid = new_config->gid = -1; // Invalidate
     char* dpos = strchr((yyvsp[(3) - (4)].tok).t, '.'); // Find uid.gid format
     if (dpos != NULL) {
        struct group *grp = getgrnam(dpos + 1); // Skip dot
        if (grp != NULL) {
          new_config->gid = grp->gr_gid;
        } else {
          cf_error(scanner, "invalid group name '%s'", dpos + 1);
        }
        *dpos = '\0'; // Cut off
     }
     struct passwd* pwd = getpwnam((yyvsp[(3) - (4)].tok).t);
     if (pwd != NULL) {
       new_config->uid = pwd->pw_uid;
     } else {
       cf_error(scanner, "invalid user name '%s'", (yyvsp[(3) - (4)].tok).t);
     }

     free((yyvsp[(3) - (4)].tok).t);
 }
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 620 "cf-parse.y"
    { new_config->max_conn_idle = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 621 "cf-parse.y"
    { new_config->max_conn_hs = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 622 "cf-parse.y"
    { new_config->max_conn_reply = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 623 "cf-parse.y"
    { new_config->rrl = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 624 "cf-parse.y"
    { new_config->rrl_size = (yyvsp[(3) - (4)].tok).l; }
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 625 "cf-parse.y"
    { new_config->rrl_size = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 626 "cf-parse.y"
    { new_config->rrl_slip = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 627 "cf-parse.y"
    { new_config->xfers = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 632 "cf-parse.y"
    {
     /* Check algorithm length. */
     if (knot_tsig_digest_length((yyvsp[(3) - (5)].tok).alg) == 0) {
        cf_error(scanner, "unsupported digest algorithm");
     }

     /* Normalize to FQDN */
     char *fqdn = (yyvsp[(2) - (5)].tok).t;
     size_t fqdnl = strlen(fqdn);
     if (fqdn[fqdnl - 1] != '.') {
        fqdnl = ((fqdnl + 2)/4+1)*4; /* '.', '\0' */
        char* tmpdn = malloc(fqdnl);
	if (!tmpdn) {
	   cf_error(scanner, "out of memory when allocating string");
	   free(fqdn);
	   fqdn = NULL;
	   fqdnl = 0;
	} else {
	   strncpy(tmpdn, fqdn, fqdnl);
	   strncat(tmpdn, ".", 1);
	   free(fqdn);
	   fqdn = tmpdn;
	   fqdnl = strlen(fqdn);
	}
     }

     if (fqdn != NULL && !conf_key_exists(scanner, fqdn)) {
         knot_dname_t *dname = knot_dname_new_from_str(fqdn, fqdnl, 0);
         if (!dname) {
             cf_error(scanner, "key name '%s' not in valid domain name format",
                      fqdn);
         } else {
             knot_dname_to_lower(dname);
             conf_key_t *k = malloc(sizeof(conf_key_t));
             memset(k, 0, sizeof(conf_key_t));
             k->k.name = dname;
             k->k.algorithm = (yyvsp[(3) - (5)].tok).alg;
             if (knot_binary_from_base64((yyvsp[(4) - (5)].tok).t, &(k->k.secret)) != 0) {
                 cf_error(scanner, "invalid key secret '%s'", (yyvsp[(4) - (5)].tok).t);
                 knot_dname_free(&dname);
                 free(k);
             } else {
                 add_tail(&new_config->keys, &k->n);
                 ++new_config->key_count;
             }
         }
     }

     free((yyvsp[(4) - (5)].tok).t);
     free(fqdn);
}
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 685 "cf-parse.y"
    { conf_start_remote(scanner, (yyvsp[(1) - (1)].tok).t); }
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 686 "cf-parse.y"
    { conf_start_remote(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 49:

/* Line 1806 of yacc.c  */
#line 687 "cf-parse.y"
    { conf_start_remote(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 688 "cf-parse.y"
    { conf_start_remote(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 689 "cf-parse.y"
    { conf_start_remote(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 693 "cf-parse.y"
    {
     if (this_remote->port != 0) {
       cf_error(scanner, "only one port definition is allowed in remote section\n");
     } else {
       this_remote->port = (yyvsp[(3) - (4)].tok).i;
     }
   }
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 700 "cf-parse.y"
    {
     if (this_remote->address != 0) {
       cf_error(scanner, "only one address is allowed in remote section\n");
     } else {
       this_remote->address = (yyvsp[(3) - (4)].tok).t;
       this_remote->prefix = IPV4_PREFIXLEN;
       this_remote->family = AF_INET;
     }
   }
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 709 "cf-parse.y"
    {
       if (this_remote->address != 0) {
         cf_error(scanner, "only one address is allowed in remote section\n");
       } else {
         this_remote->address = (yyvsp[(3) - (6)].tok).t;
         this_remote->family = AF_INET;
         this_remote->prefix = conf_mask(scanner, (yyvsp[(5) - (6)].tok).i, IPV4_PREFIXLEN);
       }
     }
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 718 "cf-parse.y"
    {
     if (this_remote->address != 0) {
       cf_error(scanner, "only one address is allowed in remote section\n");
     } else {
       this_remote->address = (yyvsp[(3) - (6)].tok).t;
       this_remote->family = AF_INET;
       this_remote->prefix = IPV4_PREFIXLEN;
       if (this_remote->port != 0) {
	 cf_error(scanner, "only one port definition is allowed in remote section\n");
       } else {
	 this_remote->port = (yyvsp[(5) - (6)].tok).i;
       }
     }
   }
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 732 "cf-parse.y"
    {
     if (this_remote->address != 0) {
       cf_error(scanner, "only one address is allowed in remote section\n");
     } else {
       this_remote->address = (yyvsp[(3) - (4)].tok).t;
       this_remote->family = AF_INET6;
       this_remote->prefix = IPV6_PREFIXLEN;
     }
   }
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 741 "cf-parse.y"
    {
       if (this_remote->address != 0) {
         cf_error(scanner, "only one address is allowed in remote section\n");
       } else {
         this_remote->address = (yyvsp[(3) - (6)].tok).t;
         this_remote->family = AF_INET6;
         this_remote->prefix = conf_mask(scanner, (yyvsp[(5) - (6)].tok).i, IPV6_PREFIXLEN);
       }
     }
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 750 "cf-parse.y"
    {
     if (this_remote->address != 0) {
       cf_error(scanner, "only one address is allowed in remote section\n");
     } else {
       this_remote->address = (yyvsp[(3) - (6)].tok).t;
       this_remote->family = AF_INET6;
       this_remote->prefix = IPV6_PREFIXLEN;
       if (this_remote->port != 0) {
	 cf_error(scanner, "only one port definition is allowed in remote section\n");
       } else {
	 this_remote->port = (yyvsp[(5) - (6)].tok).i;
       }
     }
   }
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 764 "cf-parse.y"
    {
     if (this_remote->key != 0) {
       cf_error(scanner, "only one TSIG key definition is allowed in remote section\n");
     } else {
        conf_key_add(scanner, &this_remote->key, (yyvsp[(3) - (4)].tok).t);
     }
     free((yyvsp[(3) - (4)].tok).t);
   }
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 772 "cf-parse.y"
    {
     sockaddr_set(&this_remote->via, AF_INET, (yyvsp[(3) - (4)].tok).t, 0);
     free((yyvsp[(3) - (4)].tok).t);
   }
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 776 "cf-parse.y"
    {
     sockaddr_set(&this_remote->via, AF_INET6, (yyvsp[(3) - (4)].tok).t, 0);
     free((yyvsp[(3) - (4)].tok).t);
   }
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 780 "cf-parse.y"
    {
     conf_remote_set_via(scanner, (yyvsp[(3) - (4)].tok).t);
     free((yyvsp[(3) - (4)].tok).t);
   }
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 788 "cf-parse.y"
    {
     if (this_remote->address == 0) {
       cf_error(scanner, "remote '%s' has no defined address", this_remote->name);
     }
   }
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 796 "cf-parse.y"
    { conf_add_member_into_group(scanner, (yyvsp[(1) - (1)].tok).t); }
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 806 "cf-parse.y"
    { conf_start_group(scanner, (yyvsp[(1) - (1)].tok).t); }
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 815 "cf-parse.y"
    {
      this_list = &this_zone->acl.xfr_in;
   }
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 818 "cf-parse.y"
    {
      this_list = &this_zone->acl.xfr_out;
   }
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 821 "cf-parse.y"
    {
      this_list = &this_zone->acl.notify_in;
   }
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 824 "cf-parse.y"
    {
      this_list = &this_zone->acl.notify_out;
   }
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 827 "cf-parse.y"
    {
      this_list = &this_zone->acl.update_in;
 }
    break;

  case 79:

/* Line 1806 of yacc.c  */
#line 833 "cf-parse.y"
    { conf_acl_item(scanner, (yyvsp[(1) - (1)].tok).t); }
    break;

  case 80:

/* Line 1806 of yacc.c  */
#line 834 "cf-parse.y"
    { conf_acl_item(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 81:

/* Line 1806 of yacc.c  */
#line 835 "cf-parse.y"
    { conf_acl_item(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 82:

/* Line 1806 of yacc.c  */
#line 836 "cf-parse.y"
    { conf_acl_item(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 83:

/* Line 1806 of yacc.c  */
#line 837 "cf-parse.y"
    { conf_acl_item(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 88:

/* Line 1806 of yacc.c  */
#line 846 "cf-parse.y"
    {
      /* Find existing node in remotes. */
      node* r = 0; conf_iface_t* found = 0;
      WALK_LIST (r, new_config->remotes) {
	 if (strcmp(((conf_iface_t*)r)->name, (yyvsp[(2) - (3)].tok).t) == 0) {
	    found = (conf_iface_t*)r;
	    break;
	 }
      }

      /* Append to list if found. */
      if (!found) {
	 cf_error(scanner, "remote '%s' is not defined", (yyvsp[(2) - (3)].tok).t);
      } else {
	 conf_remote_t *remote = malloc(sizeof(conf_remote_t));
	 if (!remote) {
	    cf_error(scanner, "out of memory");
	 } else {
	    remote->remote = found;
	    add_tail(this_list, &remote->n);
	 }
      }

      /* Free text token. */
      free((yyvsp[(2) - (3)].tok).t);
   }
    break;

  case 90:

/* Line 1806 of yacc.c  */
#line 875 "cf-parse.y"
    { conf_zone_start(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 91:

/* Line 1806 of yacc.c  */
#line 876 "cf-parse.y"
    { conf_zone_start(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 92:

/* Line 1806 of yacc.c  */
#line 877 "cf-parse.y"
    { conf_zone_start(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 93:

/* Line 1806 of yacc.c  */
#line 878 "cf-parse.y"
    { conf_zone_start(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 94:

/* Line 1806 of yacc.c  */
#line 879 "cf-parse.y"
    { conf_zone_start(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 95:

/* Line 1806 of yacc.c  */
#line 880 "cf-parse.y"
    { conf_zone_start(scanner, strdup((yyvsp[(1) - (1)].tok).t)); }
    break;

  case 96:

/* Line 1806 of yacc.c  */
#line 881 "cf-parse.y"
    {
    if ((yyvsp[(1) - (3)].tok).i < 0 || (yyvsp[(1) - (3)].tok).i > 255) {
        cf_error(scanner, "rfc2317 origin prefix '%ld' out of bounds", (yyvsp[(1) - (3)].tok).i);
    }
    size_t len = 3 + 1 + strlen((yyvsp[(3) - (3)].tok).t) + 1; /* <0,255> '/' rest */
    char *name = malloc(len * sizeof(char));
    if (name == NULL) {
        cf_error(scanner, "out of memory");
    } else {
        name[0] = '\0';
        if (snprintf(name, len, "%ld/%s", (yyvsp[(1) - (3)].tok).i, (yyvsp[(3) - (3)].tok).t) < 0) {
            cf_error(scanner,"failed to convert rfc2317 origin to string");
        }
    }
    free((yyvsp[(3) - (3)].tok).t);
    conf_zone_start(scanner, name);
 }
    break;

  case 97:

/* Line 1806 of yacc.c  */
#line 898 "cf-parse.y"
    { conf_zone_start(scanner, (yyvsp[(1) - (1)].tok).t); }
    break;

  case 101:

/* Line 1806 of yacc.c  */
#line 905 "cf-parse.y"
    { this_zone->file = (yyvsp[(3) - (4)].tok).t; }
    break;

  case 102:

/* Line 1806 of yacc.c  */
#line 906 "cf-parse.y"
    { this_zone->build_diffs = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 103:

/* Line 1806 of yacc.c  */
#line 907 "cf-parse.y"
    { this_zone->enable_checks = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 104:

/* Line 1806 of yacc.c  */
#line 908 "cf-parse.y"
    { this_zone->disable_any = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 105:

/* Line 1806 of yacc.c  */
#line 909 "cf-parse.y"
    { this_zone->dbsync_timeout = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 106:

/* Line 1806 of yacc.c  */
#line 910 "cf-parse.y"
    { this_zone->dbsync_timeout = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 107:

/* Line 1806 of yacc.c  */
#line 911 "cf-parse.y"
    { new_config->ixfr_fslimit = (yyvsp[(3) - (4)].tok).l; }
    break;

  case 108:

/* Line 1806 of yacc.c  */
#line 912 "cf-parse.y"
    { this_zone->ixfr_fslimit = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 109:

/* Line 1806 of yacc.c  */
#line 913 "cf-parse.y"
    {
       if ((yyvsp[(3) - (4)].tok).i < 1) {
	   cf_error(scanner, "notify retries must be positive integer");
       } else {
	   this_zone->notify_retries = (yyvsp[(3) - (4)].tok).i;
       }
   }
    break;

  case 110:

/* Line 1806 of yacc.c  */
#line 920 "cf-parse.y"
    {
	if ((yyvsp[(3) - (4)].tok).i < 1) {
	   cf_error(scanner, "notify timeout must be positive integer");
       } else {
	   this_zone->notify_timeout = (yyvsp[(3) - (4)].tok).i;
       }
   }
    break;

  case 113:

/* Line 1806 of yacc.c  */
#line 932 "cf-parse.y"
    { new_config->disable_any = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 114:

/* Line 1806 of yacc.c  */
#line 933 "cf-parse.y"
    { new_config->build_diffs = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 115:

/* Line 1806 of yacc.c  */
#line 934 "cf-parse.y"
    { new_config->zone_checks = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 116:

/* Line 1806 of yacc.c  */
#line 935 "cf-parse.y"
    { new_config->ixfr_fslimit = (yyvsp[(3) - (4)].tok).l; }
    break;

  case 117:

/* Line 1806 of yacc.c  */
#line 936 "cf-parse.y"
    { new_config->ixfr_fslimit = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 118:

/* Line 1806 of yacc.c  */
#line 937 "cf-parse.y"
    {
       if ((yyvsp[(3) - (4)].tok).i < 1) {
	   cf_error(scanner, "notify retries must be positive integer");
       } else {
	   new_config->notify_retries = (yyvsp[(3) - (4)].tok).i;
       }
   }
    break;

  case 119:

/* Line 1806 of yacc.c  */
#line 944 "cf-parse.y"
    {
	if ((yyvsp[(3) - (4)].tok).i < 1) {
	   cf_error(scanner, "notify timeout must be positive integer");
       } else {
	   new_config->notify_timeout = (yyvsp[(3) - (4)].tok).i;
       }
   }
    break;

  case 120:

/* Line 1806 of yacc.c  */
#line 951 "cf-parse.y"
    {
	if ((yyvsp[(3) - (4)].tok).i < 1) {
	   cf_error(scanner, "zonefile sync timeout must be positive integer");
       } else {
	   new_config->dbsync_timeout = (yyvsp[(3) - (4)].tok).i;
       }
 }
    break;

  case 121:

/* Line 1806 of yacc.c  */
#line 958 "cf-parse.y"
    { new_config->dbsync_timeout = (yyvsp[(3) - (4)].tok).i; }
    break;

  case 122:

/* Line 1806 of yacc.c  */
#line 961 "cf-parse.y"
    {
  this_logmap = malloc(sizeof(conf_log_map_t));
  this_logmap->source = 0;
  this_logmap->prios = 0;
  add_tail(&this_log->map, &this_logmap->n);
}
    break;

  case 124:

/* Line 1806 of yacc.c  */
#line 971 "cf-parse.y"
    { this_logmap->prios |= (yyvsp[(2) - (3)].tok).i; }
    break;

  case 125:

/* Line 1806 of yacc.c  */
#line 972 "cf-parse.y"
    { this_logmap->prios |= (yyvsp[(2) - (3)].tok).i; }
    break;

  case 127:

/* Line 1806 of yacc.c  */
#line 976 "cf-parse.y"
    {
     this_logmap->source = (yyvsp[(2) - (3)].tok).i;
     this_logmap = 0;
   }
    break;

  case 128:

/* Line 1806 of yacc.c  */
#line 982 "cf-parse.y"
    {
  /* Find already existing rule. */
  this_log = 0;
  node *n = 0;
  WALK_LIST(n, new_config->logs) {
    conf_log_t* log = (conf_log_t*)n;
    if (log->type == (yyvsp[(1) - (1)].tok).i) {
      this_log = log;
      break;
    }
  }

  if (!this_log) {
    this_log = malloc(sizeof(conf_log_t));
    this_log->type = (yyvsp[(1) - (1)].tok).i;
    this_log->file = 0;
    init_list(&this_log->map);
    add_tail(&new_config->logs, &this_log->n);
    ++new_config->logs_count;
  }
}
    break;

  case 129:

/* Line 1806 of yacc.c  */
#line 1005 "cf-parse.y"
    {
  /* Find already existing rule. */
  this_log = 0;
  node *n = 0;
  WALK_LIST(n, new_config->logs) {
    conf_log_t* log = (conf_log_t*)n;
    if (log->type == LOGT_FILE) {
      if (strcmp((yyvsp[(2) - (2)].tok).t, log->file) == 0) {
        this_log = log;
	free((yyvsp[(2) - (2)].tok).t);
        break;
      }
    }
  }

  /* Create new rule. */
  if (!this_log) {
    this_log = malloc(sizeof(conf_log_t));
    this_log->type = LOGT_FILE;
    this_log->file = strcpath((yyvsp[(2) - (2)].tok).t);
    init_list(&this_log->map);
    add_tail(&new_config->logs, &this_log->n);
    ++new_config->logs_count;
  }
}
    break;

  case 130:

/* Line 1806 of yacc.c  */
#line 1032 "cf-parse.y"
    {
}
    break;

  case 134:

/* Line 1806 of yacc.c  */
#line 1041 "cf-parse.y"
    { new_config->logs_count = 0; }
    break;

  case 136:

/* Line 1806 of yacc.c  */
#line 1045 "cf-parse.y"
    { conf_init_iface(scanner, NULL, -1); }
    break;

  case 137:

/* Line 1806 of yacc.c  */
#line 1049 "cf-parse.y"
    {
    this_list = &new_config->ctl.allow;
  }
    break;

  case 138:

/* Line 1806 of yacc.c  */
#line 1055 "cf-parse.y"
    { new_config->ctl.have = true; }
    break;

  case 139:

/* Line 1806 of yacc.c  */
#line 1056 "cf-parse.y"
    {
     if (this_iface->address == 0) {
       cf_error(scanner, "control interface has no defined address");
     } else {
       new_config->ctl.iface = this_iface;
     }
 }
    break;

  case 140:

/* Line 1806 of yacc.c  */
#line 1063 "cf-parse.y"
    {
     this_iface->address = (yyvsp[(3) - (4)].tok).t;
     this_iface->family = AF_UNIX;
     this_iface->port = 0;
     new_config->ctl.iface = this_iface;
 }
    break;



/* Line 1806 of yacc.c  */
#line 3277 "knot/conf/libknotd_la-cf-parse.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
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
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (scanner, YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (scanner, yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval, scanner);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
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


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp, scanner);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

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

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (scanner, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, scanner);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp, scanner);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 2067 of yacc.c  */
#line 1075 "cf-parse.y"


