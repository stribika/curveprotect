/* Extended regular expression matching and search library.
   Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Isamu Hasegawa <isamu@yamato.ibm.com>.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with the GNU C Library; if not, see <http://www.gnu.org/licenses/>. 
*/

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined HAVE_WCHAR_H || defined _LIBC
# include <wchar.h>
#endif /* HAVE_WCHAR_H || _LIBC */
#if defined HAVE_WCTYPE_H || defined _LIBC
# include <wctype.h>
#endif /* HAVE_WCTYPE_H || _LIBC */

#ifdef _LIBC
# ifndef _RE_DEFINE_LOCALE_FUNCTIONS
#  define _RE_DEFINE_LOCALE_FUNCTIONS 1
#  include <locale/localeinfo.h>
#  include <locale/elem-hash.h>
#  include <locale/coll-lookup.h>
# endif
#endif

#include "_regex.h" /* gnupg */
#include "regex_internal.h"

static reg_errcode_t match_ctx_init (re_match_context_t *cache, int eflags,
				     re_string_t *input, int n);
static void match_ctx_free (re_match_context_t *cache);
static reg_errcode_t match_ctx_add_entry (re_match_context_t *cache, int node,
                                          int str_idx, int from, int to);
static void match_ctx_clear_flag (re_match_context_t *mctx);
static void sift_ctx_init (re_sift_context_t *sctx, re_dfastate_t **sifted_sts,
                           re_dfastate_t **limited_sts, int last_node,
                           int last_str_idx, int check_subexp);
static reg_errcode_t re_search_internal (const regex_t *preg,
                                         const char *string, int length,
                                         int start, int range, int stop,
                                         size_t nmatch, regmatch_t pmatch[],
                                         int eflags);
static int re_search_2_stub (struct re_pattern_buffer *bufp,
                             const char *string1, int length1,
                             const char *string2, int length2,
                             int start, int range, struct re_registers *regs,
                             int stop, int ret_len);
static int re_search_stub (struct re_pattern_buffer *bufp,
                           const char *string, int length, int start,
                           int range, int stop, struct re_registers *regs,
                           int ret_len);
static unsigned re_copy_regs (struct re_registers *regs, regmatch_t *pmatch,
                              int nregs, int regs_allocated);
static inline re_dfastate_t *acquire_init_state_context (reg_errcode_t *err,
                                                         const regex_t *preg,
                                                         const re_match_context_t *mctx,
                                                         int idx);
static int check_matching (const regex_t *preg, re_match_context_t *mctx,
                           int fl_search, int fl_longest_match);
static int check_halt_node_context (const re_dfa_t *dfa, int node,
                                    unsigned int context);
static int check_halt_state_context (const regex_t *preg,
                                     const re_dfastate_t *state,
                                     const re_match_context_t *mctx, int idx);
static void update_regs (re_dfa_t *dfa, regmatch_t *pmatch, int cur_node,
                         int cur_idx, int nmatch);
static int proceed_next_node (const regex_t *preg, int nregs, regmatch_t *regs,
                              const re_match_context_t *mctx,
                              int *pidx, int node, re_node_set *eps_via_nodes,
                              struct re_fail_stack_t *fs);
static reg_errcode_t push_fail_stack (struct re_fail_stack_t *fs, 
                                      int str_idx, int *dests, int nregs,
                                      regmatch_t *regs,
                                      re_node_set *eps_via_nodes);
static int pop_fail_stack (struct re_fail_stack_t *fs, int *pidx, int nregs,
                           regmatch_t *regs, re_node_set *eps_via_nodes);
static reg_errcode_t set_regs (const regex_t *preg,
                               const re_match_context_t *mctx,
                               size_t nmatch, regmatch_t *pmatch,
                               int fl_backtrack);
#ifdef RE_ENABLE_I18N
static int sift_states_iter_mb (const regex_t *preg,
                                const re_match_context_t *mctx,
                                re_sift_context_t *sctx,
                                int node_idx, int str_idx, int max_str_idx);
#endif /* RE_ENABLE_I18N */
static reg_errcode_t sift_states_backward (const regex_t *preg,
                                           re_match_context_t *mctx,
                                           re_sift_context_t *sctx);
static reg_errcode_t update_cur_sifted_state (const regex_t *preg,
                                              re_match_context_t *mctx,
                                              re_sift_context_t *sctx,
                                              int str_idx,
                                              re_node_set *dest_nodes);
static reg_errcode_t add_epsilon_src_nodes (re_dfa_t *dfa,
                                            re_node_set *dest_nodes,
                                            const re_node_set *candidates);
static reg_errcode_t sub_epsilon_src_nodes (re_dfa_t *dfa, int node,
                                            re_node_set *dest_nodes,
                                            const re_node_set *and_nodes);
static int check_dst_limits (re_dfa_t *dfa, re_node_set *limits,
                             re_match_context_t *mctx, int dst_node,
                             int dst_idx, int src_node, int src_idx);
static int check_dst_limits_calc_pos (re_dfa_t *dfa, re_match_context_t *mctx,
                                      int limit, re_node_set *eclosures,
                                      int subexp_idx, int node, int str_idx);
static reg_errcode_t check_subexp_limits (re_dfa_t *dfa,
                                          re_node_set *dest_nodes,
                                          const re_node_set *candidates,
                                          re_node_set *limits,
                                          struct re_backref_cache_entry *bkref_ents,
                                          int str_idx);
static reg_errcode_t search_subexp (const regex_t *preg,
                                    re_match_context_t *mctx,
                                    re_sift_context_t *sctx, int str_idx,
                                    re_node_set *dest_nodes);
static reg_errcode_t sift_states_bkref (const regex_t *preg,
                                        re_match_context_t *mctx,
                                        re_sift_context_t *sctx,
                                        int str_idx, re_node_set *dest_nodes);
static reg_errcode_t clean_state_log_if_need (re_match_context_t *mctx,
                                              int next_state_log_idx);
static reg_errcode_t merge_state_array (re_dfa_t *dfa, re_dfastate_t **dst,
                                        re_dfastate_t **src, int num);
static re_dfastate_t *transit_state (reg_errcode_t *err, const regex_t *preg,
                                     re_match_context_t *mctx,
                                     re_dfastate_t *state, int fl_search);
static re_dfastate_t *transit_state_sb (reg_errcode_t *err, const regex_t *preg,
                                        re_dfastate_t *pstate,
                                        int fl_search,
                                        re_match_context_t *mctx);
#ifdef RE_ENABLE_I18N
static reg_errcode_t transit_state_mb (const regex_t *preg,
                                       re_dfastate_t *pstate,
                                       re_match_context_t *mctx);
#endif /* RE_ENABLE_I18N */
static reg_errcode_t transit_state_bkref (const regex_t *preg,
                                          re_dfastate_t *pstate,
                                          re_match_context_t *mctx);
static reg_errcode_t transit_state_bkref_loop (const regex_t *preg,
                                               re_node_set *nodes,
                                               re_dfastate_t **work_state_log,
                                               re_match_context_t *mctx);
static re_dfastate_t **build_trtable (const regex_t *dfa,
                                      const re_dfastate_t *state,
                                      int fl_search);
#ifdef RE_ENABLE_I18N
static int check_node_accept_bytes (const regex_t *preg, int node_idx,
                                    const re_string_t *input, int idx);
# ifdef _LIBC
static unsigned int find_collation_sequence_value (const unsigned char *mbs,
                                                   size_t name_len);
# endif /* _LIBC */
#endif /* RE_ENABLE_I18N */
static int group_nodes_into_DFAstates (const regex_t *dfa,
                                       const re_dfastate_t *state,
                                       re_node_set *states_node,
                                       bitset *states_ch);
static int check_node_accept (const regex_t *preg, const re_token_t *node,
                              const re_match_context_t *mctx, int idx);
static reg_errcode_t extend_buffers (re_match_context_t *mctx);

/* Entry point for POSIX code.  */

/* regexec searches for a given pattern, specified by PREG, in the
   string STRING.

   If NMATCH is zero or REG_NOSUB was set in the cflags argument to
   `regcomp', we ignore PMATCH.  Otherwise, we assume PMATCH has at
   least NMATCH elements, and we set them to the offsets of the
   corresponding matched substrings.

   EFLAGS specifies `execution flags' which affect matching: if
   REG_NOTBOL is set, then ^ does not match at the beginning of the
   string; if REG_NOTEOL is set, then $ does not match at the end.

   We return 0 if we find a match and REG_NOMATCH if not.  */

int
regexec (preg, string, nmatch, pmatch, eflags)
    const regex_t *__restrict preg;
    const char *__restrict string;
    size_t nmatch;
    regmatch_t pmatch[];
    int eflags;
{
  reg_errcode_t err;
  int length = strlen (string);
  if (preg->no_sub)
    err = re_search_internal (preg, string, length, 0, length, length, 0,
                              NULL, eflags);
  else
    err = re_search_internal (preg, string, length, 0, length, length, nmatch,
                              pmatch, eflags);
  return err != REG_NOERROR;
}
#ifdef _LIBC
weak_alias (__regexec, regexec)
#endif

/* Entry points for GNU code.  */

/* re_match, re_search, re_match_2, re_search_2

   The former two functions operate on STRING with length LENGTH,
   while the later two operate on concatenation of STRING1 and STRING2
   with lengths LENGTH1 and LENGTH2, respectively.

   re_match() matches the compiled pattern in BUFP against the string,
   starting at index START.

   re_search() first tries matching at index START, then it tries to match
   starting from index START + 1, and so on.  The last start position tried
   is START + RANGE.  (Thus RANGE = 0 forces re_search to operate the same
   way as re_match().)

   The parameter STOP of re_{match,search}_2 specifies that no match exceeding
   the first STOP characters of the concatenation of the strings should be
   concerned.

   If REGS is not NULL, and BUFP->no_sub is not set, the offsets of the match
   and all groups is stroed in REGS.  (For the "_2" variants, the offsets are
   computed relative to the concatenation, not relative to the individual
   strings.)

   On success, re_match* functions return the length of the match, re_search*
   return the position of the start of the match.  Return value -1 means no
   match was found and -2 indicates an internal error.  */

int
re_match (bufp, string, length, start, regs)
    struct re_pattern_buffer *bufp;
    const char *string;
    int length, start;
    struct re_registers *regs;
{
  return re_search_stub (bufp, string, length, start, 0, length, regs, 1);
}
#ifdef _LIBC
weak_alias (__re_match, re_match)
#endif

int
re_search (bufp, string, length, start, range, regs)
    struct re_pattern_buffer *bufp;
    const char *string;
    int length, start, range;
    struct re_registers *regs;
{
  return re_search_stub (bufp, string, length, start, range, length, regs, 0);
}
#ifdef _LIBC
weak_alias (__re_search, re_search)
#endif

int
re_match_2 (bufp, string1, length1, string2, length2, start, regs, stop)
    struct re_pattern_buffer *bufp;
    const char *string1, *string2;
    int length1, length2, start, stop;
    struct re_registers *regs;
{
  return re_search_2_stub (bufp, string1, length1, string2, length2,
                           start, 0, regs, stop, 1);
}
#ifdef _LIBC
weak_alias (__re_match_2, re_match_2)
#endif

int
re_search_2 (bufp, string1, length1, string2, length2, start, range, regs, stop)
    struct re_pattern_buffer *bufp;
    const char *string1, *string2;
    int length1, length2, start, range, stop;
    struct re_registers *regs;
{
  return re_search_2_stub (bufp, string1, length1, string2, length2,
                           start, range, regs, stop, 0);
}
#ifdef _LIBC
weak_alias (__re_search_2, re_search_2)
#endif

static int
re_search_2_stub (bufp, string1, length1, string2, length2, start, range, regs,
                  stop, ret_len)
    struct re_pattern_buffer *bufp;
    const char *string1, *string2;
    int length1, length2, start, range, stop, ret_len;
    struct re_registers *regs;
{
  const char *str;
  int rval;
  int len = length1 + length2;
  int free_str = 0;

  if (BE (length1 < 0 || length2 < 0 || stop < 0, 0))
    return -2;

  /* Concatenate the strings.  */
  if (length2 > 0)
    if (length1 > 0)
      {
        char *s = re_malloc (char, len);

        if (BE (s == NULL, 0))
          return -2;
        memcpy (s, string1, length1);
        memcpy (s + length1, string2, length2);
        str = s;
        free_str = 1;
      }
    else
      str = string2;
  else
    str = string1;

  rval = re_search_stub (bufp, str, len, start, range, stop, regs,
                         ret_len);
  if (free_str)
      re_free ((char *) str);
  return rval;
}

/* The parameters have the same meaning as those of re_search.
   Additional parameters:
   If RET_LEN is nonzero the length of the match is returned (re_match style);
   otherwise the position of the match is returned.  */

static int
re_search_stub (bufp, string, length, start, range, stop, regs, ret_len)
    struct re_pattern_buffer *bufp;
    const char *string;
    int length, start, range, stop, ret_len;
    struct re_registers *regs;
{
  reg_errcode_t result;
  regmatch_t *pmatch;
  int nregs, rval;
  int eflags = 0;

  /* Check for out-of-range.  */
  if (BE (start < 0 || start > length, 0))
    return -1;
  if (BE (start + range > length, 0))
    range = length - start;
  else if (BE (start + range < 0, 0))
    range = -start;

  eflags |= (bufp->not_bol) ? REG_NOTBOL : 0;
  eflags |= (bufp->not_eol) ? REG_NOTEOL : 0;

  /* Compile fastmap if we haven't yet.  */
  if (range > 0 && bufp->fastmap != NULL && !bufp->fastmap_accurate)
    re_compile_fastmap (bufp);

  if (BE (bufp->no_sub, 0))
    regs = NULL;

  /* We need at least 1 register.  */
  if (regs == NULL)
    nregs = 1;
  else if (BE (bufp->regs_allocated == REGS_FIXED &&
               regs->num_regs < bufp->re_nsub + 1, 0))
    {
      nregs = regs->num_regs;
      if (BE (nregs < 1, 0))
        {
          /* Nothing can be copied to regs.  */
          regs = NULL;
          nregs = 1;
        }
    }
  else
    nregs = bufp->re_nsub + 1;
  pmatch = re_malloc (regmatch_t, nregs);
  if (BE (pmatch == NULL, 0))
    return -2;

  result = re_search_internal (bufp, string, length, start, range, stop,
                               nregs, pmatch, eflags);

  rval = 0;

  /* I hope we needn't fill ther regs with -1's when no match was found.  */
  if (result != REG_NOERROR)
    rval = -1;
  else if (regs != NULL)
    {
      /* If caller wants register contents data back, copy them.  */
      bufp->regs_allocated = re_copy_regs (regs, pmatch, nregs,
                                           bufp->regs_allocated);
      if (BE (bufp->regs_allocated == REGS_UNALLOCATED, 0))
        rval = -2;
    }

  if (BE (rval == 0, 1))
    {
      if (ret_len)
        {
          assert (pmatch[0].rm_so == start);
          rval = pmatch[0].rm_eo - start;
        }
      else
        rval = pmatch[0].rm_so;
    }
  re_free (pmatch);
  return rval;
}

static unsigned
re_copy_regs (regs, pmatch, nregs, regs_allocated)
    struct re_registers *regs;
    regmatch_t *pmatch;
    int nregs, regs_allocated;
{
  int rval = REGS_REALLOCATE;
  int i;
  int need_regs = nregs + 1;
  /* We need one extra element beyond `num_regs' for the `-1' marker GNU code
     uses.  */

  /* Have the register data arrays been allocated?  */
  if (regs_allocated == REGS_UNALLOCATED)
    { /* No.  So allocate them with malloc.  */
      regs->start = re_malloc (regoff_t, need_regs);
      if (BE (regs->start == NULL, 0))
        return REGS_UNALLOCATED;
      regs->end = re_malloc (regoff_t, need_regs);
      if (BE (regs->end == NULL, 0))
        {
          re_free (regs->start);
          return REGS_UNALLOCATED;
        }
      regs->num_regs = need_regs;
    }
  else if (regs_allocated == REGS_REALLOCATE)
    { /* Yes.  If we need more elements than were already
         allocated, reallocate them.  If we need fewer, just
         leave it alone.  */
      if (need_regs > regs->num_regs)
        {
          regs->start = re_realloc (regs->start, regoff_t, need_regs);
          if (BE (regs->start == NULL, 0))
            {
              if (regs->end != NULL)
                re_free (regs->end);
              return REGS_UNALLOCATED;
            }
          regs->end = re_realloc (regs->end, regoff_t, need_regs);
          if (BE (regs->end == NULL, 0))
            {
              re_free (regs->start);
              return REGS_UNALLOCATED;
            }
          regs->num_regs = need_regs;
        }
    }
  else
    {
      assert (regs_allocated == REGS_FIXED);
      /* This function may not be called with REGS_FIXED and nregs too big.  */
      assert (regs->num_regs >= nregs);
      rval = REGS_FIXED;
    }

  /* Copy the regs.  */
  for (i = 0; i < nregs; ++i)
    {
      regs->start[i] = pmatch[i].rm_so;
      regs->end[i] = pmatch[i].rm_eo;
    }
  for ( ; i < regs->num_regs; ++i)
    regs->start[i] = regs->end[i] = -1;

  return rval;
}

/* Set REGS to hold NUM_REGS registers, storing them in STARTS and
   ENDS.  Subsequent matches using PATTERN_BUFFER and REGS will use
   this memory for recording register information.  STARTS and ENDS
   must be allocated using the malloc library routine, and must each
   be at least NUM_REGS * sizeof (regoff_t) bytes long.

   If NUM_REGS == 0, then subsequent matches should allocate their own
   register data.

   Unless this function is called, the first search or match using
   PATTERN_BUFFER will allocate its own register data, without
   freeing the old data.  */

void
re_set_registers (bufp, regs, num_regs, starts, ends)
    struct re_pattern_buffer *bufp;
    struct re_registers *regs;
    unsigned num_regs;
    regoff_t *starts, *ends;
{
  if (num_regs)
    {
      bufp->regs_allocated = REGS_REALLOCATE;
      regs->num_regs = num_regs;
      regs->start = starts;
      regs->end = ends;
    }
  else
    {
      bufp->regs_allocated = REGS_UNALLOCATED;
      regs->num_regs = 0;
      regs->start = regs->end = (regoff_t *) 0;
    }
}
#ifdef _LIBC
weak_alias (__re_set_registers, re_set_registers)
#endif

/* Entry points compatible with 4.2 BSD regex library.  We don't define
   them unless specifically requested.  */

#if defined _REGEX_RE_COMP || defined _LIBC
int
# ifdef _LIBC
weak_function
# endif
re_exec (s)
     const char *s;
{
  return 0 == regexec (&re_comp_buf, s, 0, NULL, 0);
}
#endif /* _REGEX_RE_COMP */

static re_node_set empty_set;

/* Internal entry point.  */

/* Searches for a compiled pattern PREG in the string STRING, whose
   length is LENGTH.  NMATCH, PMATCH, and EFLAGS have the same
   mingings with regexec.  START, and RANGE have the same meanings
   with re_search.
   Return REG_NOERROR if we find a match, and REG_NOMATCH if not,
   otherwise return the error code.
   Note: We assume front end functions already check ranges.
   (START + RANGE >= 0 && START + RANGE <= LENGTH)  */

static reg_errcode_t
re_search_internal (preg, string, length, start, range, stop, nmatch, pmatch,
                    eflags)
    const regex_t *preg;
    const char *string;
    int length, start, range, stop, eflags;
    size_t nmatch;
    regmatch_t pmatch[];
{
  reg_errcode_t err;
  re_dfa_t *dfa = (re_dfa_t *)preg->buffer;
  re_string_t input;
  int left_lim, right_lim, incr;
  int fl_longest_match, match_first, match_last = -1;
  re_match_context_t mctx;
  char *fastmap = ((preg->fastmap != NULL && preg->fastmap_accurate)
                   ? preg->fastmap : NULL);

  /* Check if the DFA haven't been compiled.  */
  if (BE (preg->used == 0 || dfa->init_state == NULL
          || dfa->init_state_word == NULL || dfa->init_state_nl == NULL
          || dfa->init_state_begbuf == NULL, 0))
    return REG_NOMATCH;

  re_node_set_init_empty (&empty_set);

  /* We must check the longest matching, if nmatch > 0.  */
  fl_longest_match = (nmatch != 0);

  err = re_string_allocate (&input, string, length, dfa->nodes_len + 1,
                            preg->translate, preg->syntax & RE_ICASE);
  if (BE (err != REG_NOERROR, 0))
    return err;
  input.stop = stop;

  err = match_ctx_init (&mctx, eflags, &input, dfa->nbackref * 2);
  if (BE (err != REG_NOERROR, 0))
    return err;

  /* We will log all the DFA states through which the dfa pass,
     if nmatch > 1, or this dfa has "multibyte node", which is a
     back-reference or a node which can accept multibyte character or
     multi character collating element.  */
  if (nmatch > 1 || dfa->has_mb_node)
    {
      mctx.state_log = re_malloc (re_dfastate_t *, dfa->nodes_len + 1);
      if (BE (mctx.state_log == NULL, 0))
        return REG_ESPACE;
    }
  else
    mctx.state_log = NULL;

#ifdef DEBUG
  /* We assume front-end functions already check them.  */
  assert (start + range >= 0 && start + range <= length);
#endif

  match_first = start;
  input.tip_context = ((eflags & REG_NOTBOL) ? CONTEXT_BEGBUF
                       : CONTEXT_NEWLINE | CONTEXT_BEGBUF);

  /* Check incrementally whether of not the input string match.  */
  incr = (range < 0) ? -1 : 1;
  left_lim = (range < 0) ? start + range : start;
  right_lim = (range < 0) ? start : start + range;

  for (;;)
    {
      /* At first get the current byte from input string.  */
      int ch;
      if (MB_CUR_MAX > 1 && (preg->syntax & RE_ICASE || preg->translate))
        {
          /* In this case, we can't determin easily the current byte,
             since it might be a component byte of a multibyte character.
             Then we use the constructed buffer instead.  */
          /* If MATCH_FIRST is out of the valid range, reconstruct the
             buffers.  */
          if (input.raw_mbs_idx + input.valid_len <= match_first)
            re_string_reconstruct (&input, match_first, eflags,
                                   preg->newline_anchor);
          /* If MATCH_FIRST is out of the buffer, leave it as '\0'.
             Note that MATCH_FIRST must not be smaller than 0.  */
          ch = ((match_first >= length) ? 0
                : re_string_byte_at (&input, match_first - input.raw_mbs_idx));
        }
      else
        {
          /* We apply translate/conversion manually, since it is trivial
             in this case.  */
          /* If MATCH_FIRST is out of the buffer, leave it as '\0'.
             Note that MATCH_FIRST must not be smaller than 0.  */
          ch = (match_first < length) ? (unsigned char)string[match_first] : 0;
          /* Apply translation if we need.  */
          ch = preg->translate ? preg->translate[ch] : ch;
          /* In case of case insensitive mode, convert to upper case.  */
          ch = ((preg->syntax & RE_ICASE) && islower (ch)) ? toupper (ch) : ch;
        }

      /* Eliminate inappropriate one by fastmap.  */
      if (preg->can_be_null || fastmap == NULL || fastmap[ch])
        {
          /* Reconstruct the buffers so that the matcher can assume that
             the matching starts from the begining of the buffer.  */
          re_string_reconstruct (&input, match_first, eflags,
                                 preg->newline_anchor);
#ifdef RE_ENABLE_I18N
          /* Eliminate it when it is a component of a multibyte character
             and isn't the head of a multibyte character.  */
          if (MB_CUR_MAX == 1 || re_string_first_byte (&input, 0))
#endif
            {
              /* It seems to be appropriate one, then use the matcher.  */
              /* We assume that the matching starts from 0.  */
              mctx.state_log_top = mctx.nbkref_ents = mctx.max_mb_elem_len = 0;
              match_last = check_matching (preg, &mctx, 0, fl_longest_match);
              if (match_last != -1)
                {
                  if (BE (match_last == -2, 0))
                    return REG_ESPACE;
                  else
                    break; /* We found a matching.  */
                }
            }
        }
      /* Update counter.  */
      match_first += incr;
      if (match_first < left_lim || right_lim < match_first)
        break;
    }

  /* Set pmatch[] if we need.  */
  if (match_last != -1 && nmatch > 0)
    {
      int reg_idx;

      /* Initialize registers.  */
      for (reg_idx = 0; reg_idx < nmatch; ++reg_idx)
        pmatch[reg_idx].rm_so = pmatch[reg_idx].rm_eo = -1;

      /* Set the points where matching start/end.  */
      pmatch[0].rm_so = 0;
      mctx.match_last = pmatch[0].rm_eo = match_last;

      if (!preg->no_sub && nmatch > 1)
        {
          /* We need the ranges of all the subexpressions.  */
          int halt_node;
          re_dfastate_t **sifted_states;
          re_dfastate_t **lim_states = NULL;
          re_dfastate_t *pstate = mctx.state_log[match_last];
          re_sift_context_t sctx;
#ifdef DEBUG
          assert (mctx.state_log != NULL);
#endif
          halt_node = check_halt_state_context (preg, pstate, &mctx,
                                                match_last);
          if (dfa->has_plural_match)
            {
              match_ctx_clear_flag (&mctx);
              sifted_states = re_malloc (re_dfastate_t *, match_last + 1);
              if (BE (sifted_states == NULL, 0))
                return REG_ESPACE;
              if (dfa->nbackref)
                {
                  lim_states = calloc (sizeof (re_dfastate_t *),
                                       match_last + 1);
                  if (BE (lim_states == NULL, 0))
                    return REG_ESPACE;
                }
              sift_ctx_init (&sctx, sifted_states, lim_states, halt_node,
                             mctx.match_last, 0);
              err = sift_states_backward (preg, &mctx, &sctx);
              if (BE (err != REG_NOERROR, 0))
                return err;
              if (lim_states != NULL)
                {
                  err = merge_state_array (dfa, sifted_states, lim_states,
                                           match_last + 1);
                  if (BE (err != REG_NOERROR, 0))
                    return err;
                  re_free (lim_states);
                }
              re_node_set_free (&sctx.limits);
              re_free (mctx.state_log);
              mctx.state_log = sifted_states;
            }
          mctx.last_node = halt_node;
          err = set_regs (preg, &mctx, nmatch, pmatch,
                          dfa->has_plural_match && dfa->nbackref > 0);
          if (BE (err != REG_NOERROR, 0))
            return err;
        }

      /* At last, add the offset to the each registers, since we slided
         the buffers so that We can assume that the matching starts from 0.  */
      for (reg_idx = 0; reg_idx < nmatch; ++reg_idx)
        if (pmatch[reg_idx].rm_so != -1)
          {
            pmatch[reg_idx].rm_so += match_first;
            pmatch[reg_idx].rm_eo += match_first;
          }
    }

  re_free (mctx.state_log);
  if (dfa->nbackref)
    match_ctx_free (&mctx);
  re_string_destruct (&input);

  return (match_last == -1) ? REG_NOMATCH : REG_NOERROR;
}

/* Acquire an initial state and return it.
   We must select appropriate initial state depending on the context,
   since initial states may have constraints like "\<", "^", etc..  */

static inline re_dfastate_t *
acquire_init_state_context (err, preg, mctx, idx)
     reg_errcode_t *err;
     const regex_t *preg;
     const re_match_context_t *mctx;
     int idx;
{
  re_dfa_t *dfa = (re_dfa_t *) preg->buffer;

  *err = REG_NOERROR;
  if (dfa->init_state->has_constraint)
    {
      unsigned int context;
      context =  re_string_context_at (mctx->input, idx - 1, mctx->eflags,
                                       preg->newline_anchor);
      if (IS_WORD_CONTEXT (context))
        return dfa->init_state_word;
      else if (IS_ORDINARY_CONTEXT (context))
        return dfa->init_state;
      else if (IS_BEGBUF_CONTEXT (context) && IS_NEWLINE_CONTEXT (context))
        return dfa->init_state_begbuf;
      else if (IS_NEWLINE_CONTEXT (context))
        return dfa->init_state_nl;
      else if (IS_BEGBUF_CONTEXT (context))
        {
          /* It is relatively rare case, then calculate on demand.  */
          return  re_acquire_state_context (err, dfa,
                                            dfa->init_state->entrance_nodes,
                                            context);
        }
      else
        /* Must not happen?  */
        return dfa->init_state;
    }
  else
    return dfa->init_state;
}

/* Check whether the regular expression match input string INPUT or not,
   and return the index where the matching end, return -1 if not match,
   or return -2 in case of an error.
   FL_SEARCH means we must search where the matching starts,
   FL_LONGEST_MATCH means we want the POSIX longest matching.
   Note that the matcher assume that the maching starts from the current
   index of the buffer.  */

static int
check_matching (preg, mctx, fl_search, fl_longest_match)
    const regex_t *preg;
    re_match_context_t *mctx;
    int fl_search, fl_longest_match;
{
  reg_errcode_t err;
  int match = 0;
  int match_last = -1;
  int cur_str_idx = re_string_cur_idx (mctx->input);
  re_dfastate_t *cur_state;

  cur_state = acquire_init_state_context (&err, preg, mctx, cur_str_idx);
  /* An initial state must not be NULL(invalid state).  */
  if (BE (cur_state == NULL, 0))
    return -2;
  if (mctx->state_log != NULL)
    mctx->state_log[cur_str_idx] = cur_state;

  if (cur_state->has_backref)
    {
      int i;
      re_dfa_t *dfa = (re_dfa_t *) preg->buffer;
      for (i = 0; i < cur_state->nodes.nelem; ++i)
        {
          re_token_type_t type;
          int node = cur_state->nodes.elems[i];
          int entity = (dfa->nodes[node].type != OP_CONTEXT_NODE ? node
                        : dfa->nodes[node].opr.ctx_info->entity);
          type = dfa->nodes[entity].type;
          if (type == OP_BACK_REF)
            {
              int clexp_idx;
              for (clexp_idx = 0; clexp_idx < cur_state->nodes.nelem;
                   ++clexp_idx)
                {
                  re_token_t *clexp_node;
                  clexp_node = dfa->nodes + cur_state->nodes.elems[clexp_idx];
                  if (clexp_node->type == OP_CLOSE_SUBEXP
                      && clexp_node->opr.idx + 1== dfa->nodes[entity].opr.idx)
                    {
                      err = match_ctx_add_entry (mctx, node, 0, 0, 0);
                      if (BE (err != REG_NOERROR, 0))
                        return -2;
                      break;
                    }
                }
            }
        }
    }

  /* If the RE accepts NULL string.  */
  if (cur_state->halt)
    {
      if (!cur_state->has_constraint
          || check_halt_state_context (preg, cur_state, mctx, cur_str_idx))
        {
          if (!fl_longest_match)
            return cur_str_idx;
          else
            {
              match_last = cur_str_idx;
              match = 1;
            }
        }
    }

  while (!re_string_eoi (mctx->input))
    {
      cur_state = transit_state (&err, preg, mctx, cur_state,
                                 fl_search && !match);
      if (cur_state == NULL) /* Reached at the invalid state or an error.  */
        {
          cur_str_idx = re_string_cur_idx (mctx->input);
          if (BE (err != REG_NOERROR, 0))
            return -2;
          if (fl_search && !match)
            {
              /* Restart from initial state, since we are searching
                 the point from where matching start.  */
#ifdef RE_ENABLE_I18N
              if (MB_CUR_MAX == 1
                  || re_string_first_byte (mctx->input, cur_str_idx))
#endif /* RE_ENABLE_I18N */
                cur_state = acquire_init_state_context (&err, preg, mctx,
                                                        cur_str_idx);
              if (BE (cur_state == NULL && err != REG_NOERROR, 0))
                return -2;
              if (mctx->state_log != NULL)
                mctx->state_log[cur_str_idx] = cur_state;
            }
          else if (!fl_longest_match && match)
            break;
          else /* (fl_longest_match && match) || (!fl_search && !match)  */
            {
              if (mctx->state_log == NULL)
                break;
              else
                {
                  int max = mctx->state_log_top;
                  for (; cur_str_idx <= max; ++cur_str_idx)
                    if (mctx->state_log[cur_str_idx] != NULL)
                      break;
                  if (cur_str_idx > max)
                    break;
                }
            }
        }

      if (cur_state != NULL && cur_state->halt)
        {
          /* Reached at a halt state.
             Check the halt state can satisfy the current context.  */
          if (!cur_state->has_constraint
              || check_halt_state_context (preg, cur_state, mctx,
                                           re_string_cur_idx (mctx->input)))
            {
              /* We found an appropriate halt state.  */
              match_last = re_string_cur_idx (mctx->input);
              match = 1;
              if (!fl_longest_match)
                break;
            }
        }
   }
  return match_last;
}

/* Check NODE match the current context.  */

static int check_halt_node_context (dfa, node, context)
    const re_dfa_t *dfa;
    int node;
    unsigned int context;
{
  int entity;
  re_token_type_t type = dfa->nodes[node].type;
  if (type == END_OF_RE)
    return 1;
  if (type != OP_CONTEXT_NODE)
    return 0;
  entity = dfa->nodes[node].opr.ctx_info->entity;
  if (dfa->nodes[entity].type != END_OF_RE
      || NOT_SATISFY_NEXT_CONSTRAINT (dfa->nodes[node].constraint, context))
    return 0;
  return 1;
}

/* Check the halt state STATE match the current context.
   Return 0 if not match, if the node, STATE has, is a halt node and
   match the context, return the node.  */

static int
check_halt_state_context (preg, state, mctx, idx)
    const regex_t *preg;
    const re_dfastate_t *state;
    const re_match_context_t *mctx;
    int idx;
{
  re_dfa_t *dfa = (re_dfa_t *) preg->buffer;
  int i;
  unsigned int context;
#ifdef DEBUG
  assert (state->halt);
#endif
  context = re_string_context_at (mctx->input, idx, mctx->eflags,
                                  preg->newline_anchor);
  for (i = 0; i < state->nodes.nelem; ++i)
    if (check_halt_node_context (dfa, state->nodes.elems[i], context))
      return state->nodes.elems[i];
  return 0;
}

/* Compute the next node to which "NFA" transit from NODE("NFA" is a NFA
   corresponding to the DFA).
   Return the destination node, and update EPS_VIA_NODES, return -1 in case
   of errors.  */

static int
proceed_next_node (preg, nregs, regs, mctx, pidx, node, eps_via_nodes, fs)
    const regex_t *preg;
    regmatch_t *regs;
    const re_match_context_t *mctx;
    int nregs, *pidx, node;
    re_node_set *eps_via_nodes;
    struct re_fail_stack_t *fs;
{
  re_dfa_t *dfa = (re_dfa_t *)preg->buffer;
  int i, err, dest_node, cur_entity;
  dest_node = -1;
  cur_entity = ((dfa->nodes[node].type == OP_CONTEXT_NODE)
                ? dfa->nodes[node].opr.ctx_info->entity : node);
  if (IS_EPSILON_NODE (dfa->nodes[node].type))
    {
      int ndest, dest_nodes[2], dest_entities[2];
      err = re_node_set_insert (eps_via_nodes, node);
      if (BE (err < 0, 0))
        return -1;
      /* Pick up valid destinations.  */
      for (ndest = 0, i = 0; i < mctx->state_log[*pidx]->nodes.nelem; ++i)
        {
          int candidate = mctx->state_log[*pidx]->nodes.elems[i];
          int entity;
          entity = ((dfa->nodes[candidate].type == OP_CONTEXT_NODE)
                    ? dfa->nodes[candidate].opr.ctx_info->entity : candidate);
          if (!re_node_set_contains (dfa->edests + node, entity))
            continue;
          dest_nodes[0] = (ndest == 0) ? candidate : dest_nodes[0];
          dest_entities[0] = (ndest == 0) ? entity : dest_entities[0];
          dest_nodes[1] = (ndest == 1) ? candidate : dest_nodes[1];
          dest_entities[1] = (ndest == 1) ? entity : dest_entities[1];
          ++ndest;
        }
      if (ndest <= 1)
        return ndest == 0 ? -1 : (ndest == 1 ? dest_nodes[0] : 0);
      if (dest_entities[0] > dest_entities[1])
        {
          int swap_work = dest_nodes[0];
          dest_nodes[0] = dest_nodes[1];
          dest_nodes[1] = swap_work;
        }
      /* In order to avoid infinite loop like "(a*)*".  */
      if (re_node_set_contains (eps_via_nodes, dest_nodes[0]))
        return dest_nodes[1];
      if (fs != NULL)
        push_fail_stack (fs, *pidx, dest_nodes, nregs, regs, eps_via_nodes);
      return dest_nodes[0];
    }
  else
    {
      int naccepted = 0, entity = node;
      re_token_type_t type = dfa->nodes[node].type;
      if (type == OP_CONTEXT_NODE)
        {
          entity = dfa->nodes[node].opr.ctx_info->entity;
          type = dfa->nodes[entity].type;
        }

#ifdef RE_ENABLE_I18N
      if (ACCEPT_MB_NODE (type))
        naccepted = check_node_accept_bytes (preg, entity, mctx->input, *pidx);
      else
#endif /* RE_ENABLE_I18N */
      if (type == OP_BACK_REF)
        {
          int subexp_idx = dfa->nodes[entity].opr.idx;
          naccepted = regs[subexp_idx].rm_eo - regs[subexp_idx].rm_so;
          if (fs != NULL)
            {
              if (regs[subexp_idx].rm_so == -1 || regs[subexp_idx].rm_eo == -1)
                return -1;
              else if (naccepted)
                {
                  char *buf = re_string_get_buffer (mctx->input);
                  if (strncmp (buf + regs[subexp_idx].rm_so, buf + *pidx,
                               naccepted) != 0)
                    return -1;
                }
            }

          if (naccepted == 0)
            {
              err = re_node_set_insert (eps_via_nodes, node);
              if (BE (err < 0, 0))
                return -2;
              dest_node = dfa->nexts[node];
              if (re_node_set_contains (&mctx->state_log[*pidx]->nodes,
                                        dest_node))
                return dest_node;
              for (i = 0; i < mctx->state_log[*pidx]->nodes.nelem; ++i)
                {
                  dest_node = mctx->state_log[*pidx]->nodes.elems[i];
                  if ((dfa->nodes[dest_node].type == OP_CONTEXT_NODE
                       && (dfa->nexts[node]
                           == dfa->nodes[dest_node].opr.ctx_info->entity)))
                    return dest_node;
                }
            }
        }

      if (naccepted != 0
          || check_node_accept (preg, dfa->nodes + node, mctx, *pidx))
        {
          dest_node = dfa->nexts[node];
          *pidx = (naccepted == 0) ? *pidx + 1 : *pidx + naccepted;
          if (fs && (*pidx > mctx->match_last || mctx->state_log[*pidx] == NULL
                     || !re_node_set_contains (&mctx->state_log[*pidx]->nodes,
                                               dest_node)))
            return -1;
          re_node_set_empty (eps_via_nodes);
          return dest_node;
        }
    }
  return -1;
}

static reg_errcode_t
push_fail_stack (fs, str_idx, dests, nregs, regs, eps_via_nodes)
     struct re_fail_stack_t *fs;
     int str_idx, *dests, nregs;
     regmatch_t *regs;
     re_node_set *eps_via_nodes;
{
  reg_errcode_t err;
  int num = fs->num++;
  if (fs->num == fs->alloc)
    {
      fs->alloc *= 2;
      fs->stack = realloc (fs->stack, (sizeof (struct re_fail_stack_ent_t)
                                       * fs->alloc));
      if (fs->stack == NULL)
        return REG_ESPACE;
    }
  fs->stack[num].idx = str_idx;
  fs->stack[num].node = dests[1];
  fs->stack[num].regs = re_malloc (regmatch_t, nregs);
  memcpy (fs->stack[num].regs, regs, sizeof (regmatch_t) * nregs);
  err = re_node_set_init_copy (&fs->stack[num].eps_via_nodes, eps_via_nodes);
  return err;
}
 
static int
pop_fail_stack (fs, pidx, nregs, regs, eps_via_nodes)
     struct re_fail_stack_t *fs;
     int *pidx, nregs;
     regmatch_t *regs;
     re_node_set *eps_via_nodes;
{
  int num = --fs->num;
  assert (num >= 0);
 *pidx = fs->stack[num].idx;
  memcpy (regs, fs->stack[num].regs, sizeof (regmatch_t) * nregs);
  re_node_set_free (eps_via_nodes);
  *eps_via_nodes = fs->stack[num].eps_via_nodes;
  return fs->stack[num].node;
}

/* Set the positions where the subexpressions are starts/ends to registers
   PMATCH.
   Note: We assume that pmatch[0] is already set, and
   pmatch[i].rm_so == pmatch[i].rm_eo == -1 (i > 1).  */

static reg_errcode_t
set_regs (preg, mctx, nmatch, pmatch, fl_backtrack)
     const regex_t *preg;
     const re_match_context_t *mctx;
     size_t nmatch;
     regmatch_t *pmatch;
     int fl_backtrack;
{
  re_dfa_t *dfa = (re_dfa_t *)preg->buffer;
  int idx, cur_node, real_nmatch;
  re_node_set eps_via_nodes;
  struct re_fail_stack_t *fs;
  struct re_fail_stack_t fs_body = {0, 2, NULL};
#ifdef DEBUG
  assert (nmatch > 1);
  assert (mctx->state_log != NULL);
#endif
  if (fl_backtrack)
    {
      fs = &fs_body;
      fs->stack = re_malloc (struct re_fail_stack_ent_t, fs->alloc);
    }
  else
    fs = NULL;
  cur_node = dfa->init_node;
  real_nmatch = (nmatch <= preg->re_nsub) ? nmatch : preg->re_nsub + 1;
  re_node_set_init_empty (&eps_via_nodes);
  for (idx = pmatch[0].rm_so; idx <= pmatch[0].rm_eo ;)
    {
      update_regs (dfa, pmatch, cur_node, idx, real_nmatch);
      if (idx == pmatch[0].rm_eo && cur_node == mctx->last_node)
        {
          int reg_idx;
          if (fs)
            {
              for (reg_idx = 0; reg_idx < nmatch; ++reg_idx)
                if (pmatch[reg_idx].rm_so > -1 && pmatch[reg_idx].rm_eo == -1)
                  break;
              if (reg_idx == nmatch)
                return REG_NOERROR;
              cur_node = pop_fail_stack (fs, &idx, nmatch, pmatch,
                                         &eps_via_nodes);
            }
          else
            return REG_NOERROR;
        }

      /* Proceed to next node.  */
      cur_node = proceed_next_node (preg, nmatch, pmatch, mctx, &idx, cur_node,
                                    &eps_via_nodes, fs);

      if (BE (cur_node < 0, 0))
        {
          if (cur_node == -2)
            return REG_ESPACE;
          if (fs)
            cur_node = pop_fail_stack (fs, &idx, nmatch, pmatch,
                                       &eps_via_nodes);
          else
            return REG_NOMATCH;
        }
    }
  re_node_set_free (&eps_via_nodes);
  return REG_NOERROR;
}

static void
update_regs (dfa, pmatch, cur_node, cur_idx, nmatch)
     re_dfa_t *dfa;
     regmatch_t *pmatch;
     int cur_node, cur_idx, nmatch;
{
  int type = dfa->nodes[cur_node].type;
  int reg_num;
  if (type != OP_OPEN_SUBEXP && type != OP_CLOSE_SUBEXP)
    return;
  reg_num = dfa->nodes[cur_node].opr.idx + 1;
  if (reg_num >= nmatch)
    return;
  if (type == OP_OPEN_SUBEXP)
    {
      /* We are at the first node of this sub expression.  */
      pmatch[reg_num].rm_so = cur_idx;
      pmatch[reg_num].rm_eo = -1;
    }
  else if (type == OP_CLOSE_SUBEXP)
    /* We are at the first node of this sub expression.  */
    pmatch[reg_num].rm_eo = cur_idx;
}

#define NUMBER_OF_STATE 1

/* This function checks the STATE_LOG from the SCTX->last_str_idx to 0
   and sift the nodes in each states according to the following rules.
   Updated state_log will be wrote to STATE_LOG.

   Rules: We throw away the Node `a' in the STATE_LOG[STR_IDX] if...
     1. When STR_IDX == MATCH_LAST(the last index in the state_log):
        If `a' isn't the LAST_NODE and `a' can't epsilon transit to
        the LAST_NODE, we throw away the node `a'.
     2. When 0 <= STR_IDX < MATCH_LAST and `a' accepts
        string `s' and transit to `b':
        i. If 'b' isn't in the STATE_LOG[STR_IDX+strlen('s')], we throw
           away the node `a'.
        ii. If 'b' is in the STATE_LOG[STR_IDX+strlen('s')] but 'b' is
            throwed away, we throw away the node `a'.
     3. When 0 <= STR_IDX < n and 'a' epsilon transit to 'b':
        i. If 'b' isn't in the STATE_LOG[STR_IDX], we throw away the
           node `a'.
        ii. If 'b' is in the STATE_LOG[STR_IDX] but 'b' is throwed away,
            we throw away the node `a'.  */

#define STATE_NODE_CONTAINS(state,node) \
  ((state) != NULL && re_node_set_contains (&(state)->nodes, node))

static reg_errcode_t
sift_states_backward (preg, mctx, sctx)
     const regex_t *preg;
     re_match_context_t *mctx;
     re_sift_context_t *sctx;
{
  reg_errcode_t err;
  re_dfa_t *dfa = (re_dfa_t *)preg->buffer;
  int null_cnt = 0;
  int str_idx = sctx->last_str_idx;
  re_node_set cur_dest;
  re_node_set *cur_src; /* Points the state_log[str_idx]->nodes  */

#ifdef DEBUG
  assert (mctx->state_log != NULL && mctx->state_log[str_idx] != NULL);
#endif
  cur_src = &mctx->state_log[str_idx]->nodes;

  /* Build sifted state_log[str_idx].  It has the nodes which can epsilon
     transit to the last_node and the last_node itself.  */
  err = re_node_set_init_1 (&cur_dest, sctx->last_node);
  if (BE (err != REG_NOERROR, 0))
    return err;
  err = update_cur_sifted_state (preg, mctx, sctx, str_idx, &cur_dest);
  if (BE (err != REG_NOERROR, 0))
    return err;

  /* Then check each states in the state_log.  */
  while (str_idx > 0)
    {
      int i, ret;
      /* Update counters.  */
      null_cnt = (sctx->sifted_states[str_idx] == NULL) ? null_cnt + 1 : 0;
      if (null_cnt > mctx->max_mb_elem_len)
        {
          memset (sctx->sifted_states, '\0',
                  sizeof (re_dfastate_t *) * str_idx);
          return REG_NOERROR;
        }
      re_node_set_empty (&cur_dest);
      --str_idx;
      cur_src = ((mctx->state_log[str_idx] == NULL) ? &empty_set
                 : &mctx->state_log[str_idx]->nodes);

      /* Then build the next sifted state.
         We build the next sifted state on `cur_dest', and update
         `sifted_states[str_idx]' with `cur_dest'.
         Note:
         `cur_dest' is the sifted state from `state_log[str_idx + 1]'.
         `cur_src' points the node_set of the old `state_log[str_idx]'.  */
      for (i = 0; i < cur_src->nelem; i++)
        {
          int prev_node = cur_src->elems[i];
          int entity = prev_node;
          int naccepted = 0;
          re_token_type_t type = dfa->nodes[prev_node].type;

          if (IS_EPSILON_NODE(type))
            continue;
          if (type == OP_CONTEXT_NODE)
            {
              entity = dfa->nodes[prev_node].opr.ctx_info->entity;
              type = dfa->nodes[entity].type;
            }
#ifdef RE_ENABLE_I18N
          /* If the node may accept `multi byte'.  */
          if (ACCEPT_MB_NODE (type))
            naccepted = sift_states_iter_mb (preg, mctx, sctx, entity, str_idx,
                                             sctx->last_str_idx);

#endif /* RE_ENABLE_I18N */
          /* We don't check backreferences here.
             See update_cur_sifted_state().  */

          if (!naccepted
              && check_node_accept (preg, dfa->nodes + prev_node, mctx,
                                    str_idx)
              && STATE_NODE_CONTAINS (sctx->sifted_states[str_idx + 1],
                                      dfa->nexts[prev_node]))
            naccepted = 1;

          if (naccepted == 0)
            continue;

          if (sctx->limits.nelem)
            {
              int to_idx = str_idx + naccepted;
              if (check_dst_limits (dfa, &sctx->limits, mctx,
                                    dfa->nexts[prev_node], to_idx,
                                    prev_node, str_idx))
                continue;
            }
          ret = re_node_set_insert (&cur_dest, prev_node);
          if (BE (ret == -1, 0))
            return err;
        }

      /* Add all the nodes which satisfy the following conditions:
         - It can epsilon transit to a node in CUR_DEST.
         - It is in CUR_SRC.
         And update state_log.  */
      err = update_cur_sifted_state (preg, mctx, sctx, str_idx, &cur_dest);
      if (BE (err != REG_NOERROR, 0))
        return err;
    }

  re_node_set_free (&cur_dest);
  return REG_NOERROR;
}

/* Helper functions.  */

static inline reg_errcode_t
clean_state_log_if_need (mctx, next_state_log_idx)
    re_match_context_t *mctx;
    int next_state_log_idx;
{
  int top = mctx->state_log_top;

  if (next_state_log_idx >= mctx->input->bufs_len
      || (next_state_log_idx >= mctx->input->valid_len
          && mctx->input->valid_len < mctx->input->len))
    {
      reg_errcode_t err;
      err = extend_buffers (mctx);
      if (BE (err != REG_NOERROR, 0))
        return err;
    }

  if (top < next_state_log_idx)
    {
      memset (mctx->state_log + top + 1, '\0',
              sizeof (re_dfastate_t *) * (next_state_log_idx - top));
      mctx->state_log_top = next_state_log_idx;
    }
  return REG_NOERROR;
}

static reg_errcode_t merge_state_array (dfa, dst, src, num)
     re_dfa_t *dfa;
     re_dfastate_t **dst;
     re_dfastate_t **src;
     int num;
{
  int st_idx;
  reg_errcode_t err;
  for (st_idx = 0; st_idx < num; ++st_idx)
    {
      if (dst[st_idx] == NULL)
        dst[st_idx] = src[st_idx];
      else if (src[st_idx] != NULL)
        {
          re_node_set merged_set;
          err = re_node_set_init_union (&merged_set, &dst[st_idx]->nodes,
                                        &src[st_idx]->nodes);
          if (BE (err != REG_NOERROR, 0))
            return err;
          dst[st_idx] = re_acquire_state (&err, dfa, &merged_set);
          if (BE (err != REG_NOERROR, 0))
            return err;
          re_node_set_free (&merged_set);
        }
    }
  return REG_NOERROR;
}

static reg_errcode_t
update_cur_sifted_state (preg, mctx, sctx, str_idx, dest_nodes)
     const regex_t *preg;
     re_match_context_t *mctx;
     re_sift_context_t *sctx;
     int str_idx;
     re_node_set *dest_nodes;
{
  reg_errcode_t err;
  re_dfa_t *dfa = (re_dfa_t *)preg->buffer;
  const re_node_set *candidates;
  candidates = ((mctx->state_log[str_idx] == NULL) ? &empty_set
                : &mctx->state_log[str_idx]->nodes);

  /* At first, add the nodes which can epsilon transit to a node in
     DEST_NODE.  */
  err = add_epsilon_src_nodes (dfa, dest_nodes, candidates);
  if (BE (err != REG_NOERROR, 0))
    return err;

  /* Then, check the limitations in the current sift_context.  */
  if (sctx->limits.nelem)
    {
      err = check_subexp_limits (dfa, dest_nodes, candidates, &sctx->limits,
                                 mctx->bkref_ents, str_idx);
      if (BE (err != REG_NOERROR, 0))
        return err;
    }

  /* Update state_log.  */
  sctx->sifted_states[str_idx] = re_acquire_state (&err, dfa, dest_nodes);
  if (BE (sctx->sifted_states[str_idx] == NULL && err != REG_NOERROR, 0))
    return err;

  /* If we are searching for the subexpression candidates.
     Note that we were from transit_state_bkref_loop() in this case.  */
  if (sctx->check_subexp)
    {
      err = search_subexp (preg, mctx, sctx, str_idx, dest_nodes);
      if (BE (err != REG_NOERROR, 0))
        return err;
    }

  if ((mctx->state_log[str_idx] != NULL
       && mctx->state_log[str_idx]->has_backref))
    {
      err = sift_states_bkref (preg, mctx, sctx, str_idx, dest_nodes);
      if (BE (err != REG_NOERROR, 0))
        return err;
    }
  return REG_NOERROR;
}

static reg_errcode_t
add_epsilon_src_nodes (dfa, dest_nodes, candidates)
     re_dfa_t *dfa;
     re_node_set *dest_nodes;
     const re_node_set *candidates;
{
  reg_errcode_t err;
  int src_idx;
  re_node_set src_copy;

  err = re_node_set_init_copy (&src_copy, dest_nodes);
  if (BE (err != REG_NOERROR, 0))
    return err;
  for (src_idx = 0; src_idx < src_copy.nelem; ++src_idx)
    {
      err = re_node_set_add_intersect (dest_nodes, candidates,
                                       dfa->inveclosures
                                       + src_copy.elems[src_idx]);
      if (BE (err != REG_NOERROR, 0))
        return err;
    }
  re_node_set_free (&src_copy);
  return REG_NOERROR;
}

static reg_errcode_t
sub_epsilon_src_nodes (dfa, node, dest_nodes, candidates)
     re_dfa_t *dfa;
     int node;
     re_node_set *dest_nodes;
     const re_node_set *candidates;
{
    int ecl_idx;
    reg_errcode_t err;
    re_node_set *inv_eclosure = dfa->inveclosures + node;
    re_node_set except_nodes;
    re_node_set_init_empty (&except_nodes);
    for (ecl_idx = 0; ecl_idx < inv_eclosure->nelem; ++ecl_idx)
      {
        int cur_node = inv_eclosure->elems[ecl_idx];
        if (cur_node == node)
          continue;
        if (dfa->edests[cur_node].nelem)
          {
            int edst1 = dfa->edests[cur_node].elems[0];
            int edst2 = ((dfa->edests[cur_node].nelem > 1)
                         ? dfa->edests[cur_node].elems[1] : -1);
            if ((!re_node_set_contains (inv_eclosure, edst1)
                 && re_node_set_contains (dest_nodes, edst1))
                || (edst2 > 0
                    && !re_node_set_contains (inv_eclosure, edst2)
                    && re_node_set_contains (dest_nodes, edst2)))
              {
                err = re_node_set_add_intersect (&except_nodes, candidates,
                                                 dfa->inveclosures + cur_node);
                if (BE (err != REG_NOERROR, 0))
                  return err;
              }
          }
      }
    for (ecl_idx = 0; ecl_idx < inv_eclosure->nelem; ++ecl_idx)
      {
        int cur_node = inv_eclosure->elems[ecl_idx];
        if (!re_node_set_contains (&except_nodes, cur_node))
          {
            int idx = re_node_set_contains (dest_nodes, cur_node) - 1;
            re_node_set_remove_at (dest_nodes, idx);
          }
      }
    re_node_set_free (&except_nodes);
    return REG_NOERROR;
}

static int
check_dst_limits (dfa, limits, mctx, dst_node, dst_idx, src_node, src_idx)
     re_dfa_t *dfa;
     re_node_set *limits;
     re_match_context_t *mctx;
     int dst_node, dst_idx, src_node, src_idx;
{
  int lim_idx, src_pos, dst_pos;

  for (lim_idx = 0; lim_idx < limits->nelem; ++lim_idx)
    {
      int bkref, subexp_idx/*, node_idx, cls_node*/;
      struct re_backref_cache_entry *ent;
      ent = mctx->bkref_ents + limits->elems[lim_idx];
      bkref = (dfa->nodes[ent->node].type == OP_CONTEXT_NODE
               ? dfa->nodes[ent->node].opr.ctx_info->entity : ent->node);
      subexp_idx = dfa->nodes[bkref].opr.idx - 1;

      dst_pos = check_dst_limits_calc_pos (dfa, mctx, limits->elems[lim_idx],
                                           dfa->eclosures + dst_node,
                                           subexp_idx, dst_node, dst_idx);
      src_pos = check_dst_limits_calc_pos (dfa, mctx, limits->elems[lim_idx],
                                           dfa->eclosures + src_node,
                                           subexp_idx, src_node, src_idx);

      /* In case of:
         <src> <dst> ( <subexp> )
         ( <subexp> ) <src> <dst>
         ( <subexp1> <src> <subexp2> <dst> <subexp3> )  */
      if (src_pos == dst_pos)
        continue; /* This is unrelated limitation.  */
      else
        return 1;
    }
  return 0;
}

static int
check_dst_limits_calc_pos (dfa, mctx, limit, eclosures, subexp_idx, node,
                           str_idx)
     re_dfa_t *dfa;
     re_match_context_t *mctx;
     re_node_set *eclosures;
     int limit, subexp_idx, node, str_idx;
{
  struct re_backref_cache_entry *lim = mctx->bkref_ents + limit;
  int pos = (str_idx < lim->subexp_from ? -1
             : (lim->subexp_to < str_idx ? 1 : 0));
  if (pos == 0
      && (str_idx == lim->subexp_from || str_idx == lim->subexp_to))
    {
      int node_idx;
      for (node_idx = 0; node_idx < eclosures->nelem; ++node_idx)
        {
          int node = eclosures->elems[node_idx];
          int entity = node;
          re_token_type_t type= dfa->nodes[node].type;
          if (type == OP_CONTEXT_NODE)
            {
              entity = dfa->nodes[node].opr.ctx_info->entity;
              type = dfa->nodes[entity].type;
            }
          if (type == OP_BACK_REF)
            {
              int bi;
              for (bi = 0; bi < mctx->nbkref_ents; ++bi)
                {
                  struct re_backref_cache_entry *ent = mctx->bkref_ents + bi;
                  if (ent->node == node && ent->subexp_from == ent->subexp_to
                      && ent->str_idx == str_idx)
                    {
                      int cpos, dst;
                      dst = dfa->nexts[node];
                      cpos = check_dst_limits_calc_pos (dfa, mctx, limit,
                                                        dfa->eclosures + dst,
                                                        subexp_idx, dst,
                                                        str_idx);
                      if ((str_idx == lim->subexp_from && cpos == -1)
                          || (str_idx == lim->subexp_to && cpos == 0))
                        return cpos;
                    }
                }
            }
          if (type == OP_OPEN_SUBEXP && subexp_idx == dfa->nodes[node].opr.idx
              && str_idx == lim->subexp_from)
            {
              pos = -1;
              break;
            }
          if (type == OP_CLOSE_SUBEXP && subexp_idx == dfa->nodes[node].opr.idx
              && str_idx == lim->subexp_to)
            break;
        }
      if (node_idx == eclosures->nelem && str_idx == lim->subexp_to)
        pos = 1;
    }
  return pos;
}

/* Check the limitations of sub expressions LIMITS, and remove the nodes
   which are against limitations from DEST_NODES. */

static reg_errcode_t
check_subexp_limits (dfa, dest_nodes, candidates, limits, bkref_ents, str_idx)
     re_dfa_t *dfa;
     re_node_set *dest_nodes;
     const re_node_set *candidates;
     re_node_set *limits;
     struct re_backref_cache_entry *bkref_ents;
     int str_idx;
{
  reg_errcode_t err;
  int node_idx, lim_idx;

  for (lim_idx = 0; lim_idx < limits->nelem; ++lim_idx)
    {
      int bkref, subexp_idx;
      struct re_backref_cache_entry *ent;
      ent = bkref_ents + limits->elems[lim_idx];

      if (str_idx <= ent->subexp_from || ent->str_idx < str_idx)
        continue; /* This is unrelated limitation.  */

      bkref = (dfa->nodes[ent->node].type == OP_CONTEXT_NODE
               ? dfa->nodes[ent->node].opr.ctx_info->entity : ent->node);
      subexp_idx = dfa->nodes[bkref].opr.idx - 1;

      if (ent->subexp_to == str_idx)
        {
          int ops_node = -1;
          int cls_node = -1;
          for (node_idx = 0; node_idx < dest_nodes->nelem; ++node_idx)
            {
              int node = dest_nodes->elems[node_idx];
              re_token_type_t type= dfa->nodes[node].type;
              if (type == OP_OPEN_SUBEXP
                  && subexp_idx == dfa->nodes[node].opr.idx)
                ops_node = node;
              else if (type == OP_CLOSE_SUBEXP
                       && subexp_idx == dfa->nodes[node].opr.idx)
                cls_node = node;
            }

          /* Check the limitation of the open subexpression.  */
          /* Note that (ent->subexp_to = str_idx != ent->subexp_from).  */
          if (ops_node >= 0)
            {
              err = sub_epsilon_src_nodes(dfa, ops_node, dest_nodes,
                                          candidates);
              if (BE (err != REG_NOERROR, 0))
                return err;
            }
          /* Check the limitation of the close subexpression.  */
          for (node_idx = 0; node_idx < dest_nodes->nelem; ++node_idx)
            {
              int node = dest_nodes->elems[node_idx];
              if (!re_node_set_contains (dfa->inveclosures + node, cls_node)
                  && !re_node_set_contains (dfa->eclosures + node, cls_node))
                {
                  /* It is against this limitation.
                     Remove it form the current sifted state.  */
                  err = sub_epsilon_src_nodes(dfa, node, dest_nodes,
                                              candidates);
                  if (BE (err != REG_NOERROR, 0))
                    return err;
                  --node_idx;
                }
            }
        }
      else /* (ent->subexp_to != str_idx)  */
        {
          for (node_idx = 0; node_idx < dest_nodes->nelem; ++node_idx)
            {
              int node = dest_nodes->elems[node_idx];
              re_token_type_t type= dfa->nodes[node].type;
              if (type == OP_CLOSE_SUBEXP || type == OP_OPEN_SUBEXP)
                {
                  if (subexp_idx != dfa->nodes[node].opr.idx)
                    continue;
                  if ((type == OP_CLOSE_SUBEXP && ent->subexp_to != str_idx)
                      || (type == OP_OPEN_SUBEXP))
                    {
                      /* It is against this limitation.
                         Remove it form the current sifted state.  */
                      err = sub_epsilon_src_nodes(dfa, node, dest_nodes,
                                                  candidates);
                      if (BE (err != REG_NOERROR, 0))
                        return err;
                    }
                }
            }
        }
    }
  return REG_NOERROR;
}

/* Search for the top (in case of sctx->check_subexp < 0) or the
   bottom (in case of sctx->check_subexp > 0) of the subexpressions
   which the backreference sctx->cur_bkref can match.  */

static reg_errcode_t
search_subexp (preg, mctx, sctx, str_idx, dest_nodes)
     const regex_t *preg;
     re_match_context_t *mctx;
     re_sift_context_t *sctx;
     int str_idx;
     re_node_set *dest_nodes;
{
  reg_errcode_t err;
  re_dfa_t *dfa = (re_dfa_t *)preg->buffer;
  re_sift_context_t local_sctx;
  int node_idx, node=0; /* gnupg */
  const re_node_set *candidates;
  re_dfastate_t **lim_states = NULL;
  candidates = ((mctx->state_log[str_idx] == NULL) ? &empty_set
                : &mctx->state_log[str_idx]->nodes);
  local_sctx.sifted_states = NULL; /* Mark that it hasn't been initialized.  */

  for (node_idx = 0; node_idx < dest_nodes->nelem; ++node_idx)
    {
      re_token_type_t type;
      int entity;
      node = dest_nodes->elems[node_idx];
      type = dfa->nodes[node].type;
      entity = (type != OP_CONTEXT_NODE ? node
                : dfa->nodes[node].opr.ctx_info->entity);
      type = (type != OP_CONTEXT_NODE ? type : dfa->nodes[entity].type);

      if (type == OP_CLOSE_SUBEXP
          && sctx->check_subexp == dfa->nodes[node].opr.idx + 1)
        {
          re_dfastate_t *cur_state;
          /* Found the bottom of the subexpression, then search for the
             top of it.  */
          if (local_sctx.sifted_states == NULL)
            {
              /* It hasn't been initialized yet, initialize it now.  */
              local_sctx = *sctx;
              err = re_node_set_init_copy (&local_sctx.limits, &sctx->limits);
              if (BE (err != REG_NOERROR, 0))
                return err;
            }
          local_sctx.check_subexp = -sctx->check_subexp;
          local_sctx.limited_states = sctx->limited_states;
          local_sctx.last_node = node;
          local_sctx.last_str_idx = local_sctx.cls_subexp_idx = str_idx;
          cur_state = local_sctx.sifted_states[str_idx];
          err = sift_states_backward (preg, mctx, &local_sctx);
          local_sctx.sifted_states[str_idx] = cur_state;
          if (BE (err != REG_NOERROR, 0))
            return err;
          /* There must not 2 same node in a node set.  */
          break;
        }
      else if (type == OP_OPEN_SUBEXP
               && -sctx->check_subexp == dfa->nodes[node].opr.idx + 1)
        {
          /* Found the top of the subexpression, check that the
             backreference can match the input string.  */
          char *buf;
          int dest_str_idx;
          int bkref_str_idx = re_string_cur_idx (mctx->input);
          int subexp_len = sctx->cls_subexp_idx - str_idx;
          if (subexp_len < 0 || bkref_str_idx + subexp_len > mctx->input->len)
            break;

          if (bkref_str_idx + subexp_len > mctx->input->valid_len
              && mctx->input->valid_len < mctx->input->len)
            {
              reg_errcode_t err;
              err = extend_buffers (mctx);
              if (BE (err != REG_NOERROR, 0))
                return err;
            }
          buf = (char *) re_string_get_buffer (mctx->input);
          if (strncmp (buf + str_idx, buf + bkref_str_idx, subexp_len) != 0)
            break;

          if (sctx->limits.nelem && str_idx > 0)
            {
              re_dfastate_t *cur_state = sctx->sifted_states[str_idx];
              if (lim_states == NULL)
                {
                  lim_states = re_malloc (re_dfastate_t *, str_idx + 1);
                }
              if (local_sctx.sifted_states == NULL)
                {
                  /* It hasn't been initialized yet, initialize it now.  */
                  local_sctx = *sctx;
                  if (BE (lim_states == NULL, 0))
                    return REG_ESPACE;
                  err = re_node_set_init_copy (&local_sctx.limits,
                                               &sctx->limits);
                  if (BE (err != REG_NOERROR, 0))
                    return err;
                }
              local_sctx.check_subexp = 0;
              local_sctx.last_node = node;
              local_sctx.last_str_idx = str_idx;
              local_sctx.limited_states = lim_states;
              memset (lim_states, '\0',
                      sizeof (re_dfastate_t*) * (str_idx + 1));
              err = sift_states_backward (preg, mctx, &local_sctx);
              if (BE (err != REG_NOERROR, 0))
                return err;
              if (local_sctx.sifted_states[0] == NULL
                  && local_sctx.limited_states[0] == NULL)
                {
                  sctx->sifted_states[str_idx] = cur_state;
                  break;
                }
              sctx->sifted_states[str_idx] = cur_state;
            }
          /* Successfully matched, add a new cache entry.  */
          dest_str_idx = bkref_str_idx + subexp_len;
          err = match_ctx_add_entry (mctx, sctx->cur_bkref, bkref_str_idx,
                                     str_idx, sctx->cls_subexp_idx);
          if (BE (err != REG_NOERROR, 0))
            return err;
          err = clean_state_log_if_need (mctx, dest_str_idx);
          if (BE (err != REG_NOERROR, 0))
            return err;
          break;
        }
    }

  /* Remove the top/bottom of the sub expression we processed.  */
  if (node_idx < dest_nodes->nelem)
    {
      err = sub_epsilon_src_nodes(dfa, node, dest_nodes, candidates);
      if (BE (err != REG_NOERROR, 0))
        return err;
      /* Update state_log.  */
      sctx->sifted_states[str_idx] = re_acquire_state (&err, dfa, dest_nodes);
      if (BE (err != REG_NOERROR, 0))
        return err;
    }

  if (local_sctx.sifted_states != NULL)
    re_node_set_free (&local_sctx.limits);
  if (lim_states != NULL)
    re_free (lim_states);
  return REG_NOERROR;
}

static reg_errcode_t
sift_states_bkref (preg, mctx, sctx, str_idx, dest_nodes)
     const regex_t *preg;
     re_match_context_t *mctx;
     re_sift_context_t *sctx;
     int str_idx;
     re_node_set *dest_nodes;
{
  reg_errcode_t err;
  re_dfa_t *dfa = (re_dfa_t *)preg->buffer;
  int node_idx, node;
  re_sift_context_t local_sctx;
  const re_node_set *candidates;
  candidates = ((mctx->state_log[str_idx] == NULL) ? &empty_set
                : &mctx->state_log[str_idx]->nodes);
  local_sctx.sifted_states = NULL; /* Mark that it hasn't been initialized.  */

  for (node_idx = 0; node_idx < candidates->nelem; ++node_idx)
    {
      int entity;
      int cur_bkref_idx = re_string_cur_idx (mctx->input);
      re_token_type_t type;
      node = candidates->elems[node_idx];
      type = dfa->nodes[node].type;
      entity = (type != OP_CONTEXT_NODE ? node
                : dfa->nodes[node].opr.ctx_info->entity);
      type = (type != OP_CONTEXT_NODE ? type : dfa->nodes[entity].type);
      if (node == sctx->cur_bkref && str_idx == cur_bkref_idx)
        continue;
      /* Avoid infinite loop for the REs like "()\1+".  */
      if (node == sctx->last_node && str_idx == sctx->last_str_idx)
        continue;
      if (type == OP_BACK_REF)
        {
          int enabled_idx;
          for (enabled_idx = 0; enabled_idx < mctx->nbkref_ents; ++enabled_idx)
            {
              int disabled_idx, subexp_len, to_idx;
              struct re_backref_cache_entry *entry;
              entry = mctx->bkref_ents + enabled_idx;
              subexp_len = entry->subexp_to - entry->subexp_from;
              to_idx = str_idx + subexp_len;

              if (entry->node != node || entry->str_idx != str_idx
                  || to_idx > sctx->last_str_idx
                  || sctx->sifted_states[to_idx] == NULL)
                continue;
              if (!STATE_NODE_CONTAINS (sctx->sifted_states[to_idx],
                                        dfa->nexts[node]))
                {
                  int dst_idx;
                  re_node_set *dsts = &sctx->sifted_states[to_idx]->nodes;
                  for (dst_idx = 0; dst_idx < dsts->nelem; ++dst_idx)
                    {
                      int dst_node = dsts->elems[dst_idx];
                      if (dfa->nodes[dst_node].type == OP_CONTEXT_NODE
                          && (dfa->nodes[dst_node].opr.ctx_info->entity
                              == dfa->nexts[node]))
                        break;
                    }
                  if (dst_idx == dsts->nelem)
                    continue;
                }

              if (check_dst_limits (dfa, &sctx->limits, mctx, node,
                                    str_idx, dfa->nexts[node], to_idx))
                continue;
              if (sctx->check_subexp == dfa->nodes[entity].opr.idx)
                {
                  char *buf;
                  buf = (char *) re_string_get_buffer (mctx->input);
                  if (strncmp (buf + entry->subexp_from,
                               buf + cur_bkref_idx, subexp_len) != 0)
                    continue;
                  err = match_ctx_add_entry (mctx, sctx->cur_bkref,
                                             cur_bkref_idx, entry->subexp_from,
                                             entry->subexp_to);
                  if (BE (err != REG_NOERROR, 0))
                    return err;
                  err = clean_state_log_if_need (mctx, cur_bkref_idx
                                                 + subexp_len);
                  if (BE (err != REG_NOERROR, 0))
                    return err;
                }
              else
                {
                  re_dfastate_t *cur_state;
                  entry->flag = 0;
                  for (disabled_idx = enabled_idx + 1;
                       disabled_idx < mctx->nbkref_ents; ++disabled_idx)
                    {
                      struct re_backref_cache_entry *entry2;
                      entry2 = mctx->bkref_ents + disabled_idx;
                      if (entry2->node != node || entry2->str_idx != str_idx)
                        continue;
                      entry2->flag = 1;
                    }

                  if (local_sctx.sifted_states == NULL)
                    {
                      local_sctx = *sctx;
                      err = re_node_set_init_copy (&local_sctx.limits,
                                                   &sctx->limits);
                      if (BE (err != REG_NOERROR, 0))
                        return err;
                    }
                  local_sctx.last_node = node;
                  local_sctx.last_str_idx = str_idx;
                  err = re_node_set_insert (&local_sctx.limits, enabled_idx);
                  if (BE (err < 0, 0))
                    return REG_ESPACE;
                  cur_state = local_sctx.sifted_states[str_idx];
                  err = sift_states_backward (preg, mctx, &local_sctx);
                  if (BE (err != REG_NOERROR, 0))
                    return err;
                  if (sctx->limited_states != NULL)
                    {
                      err = merge_state_array (dfa, sctx->limited_states,
                                               local_sctx.sifted_states,
                                               str_idx + 1);
                      if (BE (err != REG_NOERROR, 0))
                        return err;
                    }
                  local_sctx.sifted_states[str_idx] = cur_state;
                  re_node_set_remove_at (&local_sctx.limits,
                                         local_sctx.limits.nelem - 1);
                  entry->flag = 1;
                }
            }
          for (enabled_idx = 0; enabled_idx < mctx->nbkref_ents; ++enabled_idx)
            {
              struct re_backref_cache_entry *entry;
              entry = mctx->bkref_ents + enabled_idx;
              if (entry->node == node && entry->str_idx == str_idx)
                entry->flag = 0;
            }
        }
    }
  if (local_sctx.sifted_states != NULL)
    {
      re_node_set_free (&local_sctx.limits);
    }

  return REG_NOERROR;
}


#ifdef RE_ENABLE_I18N
static int
sift_states_iter_mb (preg, mctx, sctx, node_idx, str_idx, max_str_idx)
    const regex_t *preg;
    const re_match_context_t *mctx;
    re_sift_context_t *sctx;
    int node_idx, str_idx, max_str_idx;
{
  re_dfa_t *dfa = (re_dfa_t *) preg->buffer;
  int naccepted;
  /* Check the node can accept `multi byte'.  */
  naccepted = check_node_accept_bytes (preg, node_idx, mctx->input, str_idx);
  if (naccepted > 0 && str_idx + naccepted <= max_str_idx &&
      !STATE_NODE_CONTAINS (sctx->sifted_states[str_idx + naccepted],
                            dfa->nexts[node_idx]))
    /* The node can't accept the `multi byte', or the
       destination was already throwed away, then the node
       could't accept the current input `multi byte'.   */
    naccepted = 0;
  /* Otherwise, it is sure that the node could accept
     `naccepted' bytes input.  */
  return naccepted;
}
#endif /* RE_ENABLE_I18N */


/* Functions for state transition.  */

/* Return the next state to which the current state STATE will transit by
   accepting the current input byte, and update STATE_LOG if necessary.
   If STATE can accept a multibyte char/collating element/back reference
   update the destination of STATE_LOG.  */

static re_dfastate_t *
transit_state (err, preg, mctx, state, fl_search)
     reg_errcode_t *err;
     const regex_t *preg;
     re_match_context_t *mctx;
     re_dfastate_t *state;
     int fl_search;
{
  re_dfa_t *dfa = (re_dfa_t *) preg->buffer;
  re_dfastate_t **trtable, *next_state;
  unsigned char ch;

  if (re_string_cur_idx (mctx->input) + 1 >= mctx->input->bufs_len
      || (re_string_cur_idx (mctx->input) + 1 >= mctx->input->valid_len
          && mctx->input->valid_len < mctx->input->len))
    {
      *err = extend_buffers (mctx);
      if (BE (*err != REG_NOERROR, 0))
        return NULL;
    }

  *err = REG_NOERROR;
  if (state == NULL)
    {
      next_state = state;
      re_string_skip_bytes (mctx->input, 1);
    }
  else
    {
#ifdef RE_ENABLE_I18N
      /* If the current state can accept multibyte.  */
      if (state->accept_mb)
        {
          *err = transit_state_mb (preg, state, mctx);
          if (BE (*err != REG_NOERROR, 0))
            return NULL;
        }
#endif /* RE_ENABLE_I18N */

      /* Then decide the next state with the single byte.  */
      if (1)
        {
          /* Use transition table  */
          ch = re_string_fetch_byte (mctx->input);
          trtable = fl_search ? state->trtable_search : state->trtable;
          if (trtable == NULL)
            {
              trtable = build_trtable (preg, state, fl_search);
              if (fl_search)
                state->trtable_search = trtable;
              else
                state->trtable = trtable;
            }
          next_state = trtable[ch];
        }
      else
        {
          /* don't use transition table  */
          next_state = transit_state_sb (err, preg, state, fl_search, mctx);
          if (BE (next_state == NULL && err != REG_NOERROR, 0))
            return NULL;
        }
    }

  /* Update the state_log if we need.  */
  if (mctx->state_log != NULL)
    {
      int cur_idx = re_string_cur_idx (mctx->input);
      if (cur_idx > mctx->state_log_top)
        {
          mctx->state_log[cur_idx] = next_state;
          mctx->state_log_top = cur_idx;
        }
      else if (mctx->state_log[cur_idx] == 0)
        {
          mctx->state_log[cur_idx] = next_state;
        }
      else
        {
          re_dfastate_t *pstate;
          unsigned int context;
          re_node_set next_nodes, *log_nodes, *table_nodes = NULL;
          /* If (state_log[cur_idx] != 0), it implies that cur_idx is
             the destination of a multibyte char/collating element/
             back reference.  Then the next state is the union set of
             these destinations and the results of the transition table.  */
          pstate = mctx->state_log[cur_idx];
          log_nodes = pstate->entrance_nodes;
          if (next_state != NULL)
            {
              table_nodes = next_state->entrance_nodes;
              *err = re_node_set_init_union (&next_nodes, table_nodes,
                                             log_nodes);
              if (BE (*err != REG_NOERROR, 0))
                return NULL;
            }
          else
            next_nodes = *log_nodes;
          /* Note: We already add the nodes of the initial state,
                   then we don't need to add them here.  */

          context = re_string_context_at (mctx->input,
                                          re_string_cur_idx (mctx->input) - 1,
                                          mctx->eflags, preg->newline_anchor);
          next_state = mctx->state_log[cur_idx]
            = re_acquire_state_context (err, dfa, &next_nodes, context);
          /* We don't need to check errors here, since the return value of
             this function is next_state and ERR is already set.  */

          if (table_nodes != NULL)
            re_node_set_free (&next_nodes);
        }
      /* If the next state has back references.  */
      if (next_state != NULL && next_state->has_backref)
        {
          *err = transit_state_bkref (preg, next_state, mctx);
          if (BE (*err != REG_NOERROR, 0))
            return NULL;
          next_state = mctx->state_log[cur_idx];
        }
    }
  return next_state;
}

/* Helper functions for transit_state.  */

/* Return the next state to which the current state STATE will transit by
   accepting the current input byte.  */

static re_dfastate_t *
transit_state_sb (err, preg, state, fl_search, mctx)
     reg_errcode_t *err;
     const regex_t *preg;
     re_dfastate_t *state;
     int fl_search;
     re_match_context_t *mctx;
{
  re_dfa_t *dfa = (re_dfa_t *) preg->buffer;
  re_node_set next_nodes;
  re_dfastate_t *next_state;
  int node_cnt, cur_str_idx = re_string_cur_idx (mctx->input);
  unsigned int context;

  *err = re_node_set_alloc (&next_nodes, state->nodes.nelem + 1);
  if (BE (*err != REG_NOERROR, 0))
    return NULL;
  for (node_cnt = 0; node_cnt < state->nodes.nelem; ++node_cnt)
    {
      int cur_node = state->nodes.elems[node_cnt];
      if (check_node_accept (preg, dfa->nodes + cur_node, mctx, cur_str_idx))
        {
          *err = re_node_set_merge (&next_nodes,
                                    dfa->eclosures + dfa->nexts[cur_node]);
          if (BE (*err != REG_NOERROR, 0))
            return NULL;
        }
    }
  if (fl_search)
    {
#ifdef RE_ENABLE_I18N
      int not_initial = 0;
      if (MB_CUR_MAX > 1)
        for (node_cnt = 0; node_cnt < next_nodes.nelem; ++node_cnt)
          if (dfa->nodes[next_nodes.elems[node_cnt]].type == CHARACTER)
            {
              not_initial = dfa->nodes[next_nodes.elems[node_cnt]].mb_partial;
              break;
            }
      if (!not_initial)
#endif
        {
          *err = re_node_set_merge (&next_nodes,
                                    dfa->init_state->entrance_nodes);
          if (BE (*err != REG_NOERROR, 0))
            return NULL;
        }
    }
  context = re_string_context_at (mctx->input, cur_str_idx, mctx->eflags,
                                  preg->newline_anchor);
  next_state = re_acquire_state_context (err, dfa, &next_nodes, context);
  /* We don't need to check errors here, since the return value of
     this function is next_state and ERR is already set.  */

  re_node_set_free (&next_nodes);
  re_string_skip_bytes (mctx->input, 1);
  return next_state;
}

#ifdef RE_ENABLE_I18N
static reg_errcode_t
transit_state_mb (preg, pstate, mctx)
    const regex_t *preg;
    re_dfastate_t *pstate;
    re_match_context_t *mctx;
{
  reg_errcode_t err;
  re_dfa_t *dfa = (re_dfa_t *) preg->buffer;
  int i;

  for (i = 0; i < pstate->nodes.nelem; ++i)
    {
      re_node_set dest_nodes, *new_nodes;
      int cur_node_idx = pstate->nodes.elems[i];
      int naccepted = 0, dest_idx;
      unsigned int context;
      re_dfastate_t *dest_state;

      if (dfa->nodes[cur_node_idx].type == OP_CONTEXT_NODE)
        {
          context = re_string_context_at (mctx->input,
                                          re_string_cur_idx (mctx->input),
                                          mctx->eflags, preg->newline_anchor);
          if (NOT_SATISFY_NEXT_CONSTRAINT (dfa->nodes[cur_node_idx].constraint,
                                        context))
            continue;
          cur_node_idx = dfa->nodes[cur_node_idx].opr.ctx_info->entity;
        }

      /* How many bytes the node can accepts?  */
      if (ACCEPT_MB_NODE (dfa->nodes[cur_node_idx].type))
        naccepted = check_node_accept_bytes (preg, cur_node_idx, mctx->input,
                                             re_string_cur_idx (mctx->input));
      if (naccepted == 0)
        continue;

      /* The node can accepts `naccepted' bytes.  */
      dest_idx = re_string_cur_idx (mctx->input) + naccepted;
      mctx->max_mb_elem_len = ((mctx->max_mb_elem_len < naccepted) ? naccepted
                               : mctx->max_mb_elem_len);
      err = clean_state_log_if_need (mctx, dest_idx);
      if (BE (err != REG_NOERROR, 0))
        return err;
#ifdef DEBUG
      assert (dfa->nexts[cur_node_idx] != -1);
#endif
      /* `cur_node_idx' may point the entity of the OP_CONTEXT_NODE,
         then we use pstate->nodes.elems[i] instead.  */
      new_nodes = dfa->eclosures + dfa->nexts[pstate->nodes.elems[i]];

      dest_state = mctx->state_log[dest_idx];
      if (dest_state == NULL)
        dest_nodes = *new_nodes;
      else
        {
          err = re_node_set_init_union (&dest_nodes,
                                        dest_state->entrance_nodes, new_nodes);
          if (BE (err != REG_NOERROR, 0))
            return err;
        }
      context = re_string_context_at (mctx->input, dest_idx - 1, mctx->eflags,
                                      preg->newline_anchor);
      mctx->state_log[dest_idx]
        = re_acquire_state_context (&err, dfa, &dest_nodes, context);
      if (BE (mctx->state_log[dest_idx] == NULL && err != REG_NOERROR, 0))
        return err;
      if (dest_state != NULL)
        re_node_set_free (&dest_nodes);
    }
  return REG_NOERROR;
}
#endif /* RE_ENABLE_I18N */

static reg_errcode_t
transit_state_bkref (preg, pstate, mctx)
    const regex_t *preg;
    re_dfastate_t *pstate;
    re_match_context_t *mctx;
{
  reg_errcode_t err;
  re_dfastate_t **work_state_log;

  work_state_log = re_malloc (re_dfastate_t *,
                              re_string_cur_idx (mctx->input) + 1);
  if (BE (work_state_log == NULL, 0))
    return REG_ESPACE;

  err = transit_state_bkref_loop (preg, &pstate->nodes, work_state_log, mctx);
  re_free (work_state_log);
  return err;
}

/* Caller must allocate `work_state_log'.  */

static reg_errcode_t
transit_state_bkref_loop (preg, nodes, work_state_log, mctx)
    const regex_t *preg;
    re_node_set *nodes;
    re_dfastate_t **work_state_log;
    re_match_context_t *mctx;
{
  reg_errcode_t err;
  re_dfa_t *dfa = (re_dfa_t *) preg->buffer;
  int i;
  regmatch_t *cur_regs = re_malloc (regmatch_t, preg->re_nsub + 1);
  int cur_str_idx = re_string_cur_idx (mctx->input);
  if (BE (cur_regs == NULL, 0))
    return REG_ESPACE;

  for (i = 0; i < nodes->nelem; ++i)
    {
      int dest_str_idx, subexp_idx, prev_nelem, bkc_idx;
      int node_idx = nodes->elems[i];
      unsigned int context;
      re_token_t *node = dfa->nodes + node_idx;
      re_node_set *new_dest_nodes;
      re_sift_context_t sctx;

      /* Check whether `node' is a backreference or not.  */
      if (node->type == OP_BACK_REF)
        subexp_idx = node->opr.idx;
      else if (node->type == OP_CONTEXT_NODE &&
               dfa->nodes[node->opr.ctx_info->entity].type == OP_BACK_REF)
        {
          context = re_string_context_at (mctx->input, cur_str_idx,
                                          mctx->eflags, preg->newline_anchor);
          if (NOT_SATISFY_NEXT_CONSTRAINT (node->constraint, context))
            continue;
          subexp_idx = dfa->nodes[node->opr.ctx_info->entity].opr.idx;
        }
      else
        continue;

      /* `node' is a backreference.
         Check the substring which the substring matched.  */
      sift_ctx_init (&sctx, work_state_log, NULL, node_idx, cur_str_idx,
                     subexp_idx);
      sctx.cur_bkref = node_idx;
      match_ctx_clear_flag (mctx);
      err = sift_states_backward (preg, mctx, &sctx);
      if (BE (err != REG_NOERROR, 0))
        return err;

      /* And add the epsilon closures (which is `new_dest_nodes') of
         the backreference to appropriate state_log.  */
#ifdef DEBUG
      assert (dfa->nexts[node_idx] != -1);
#endif
      for (bkc_idx = 0; bkc_idx < mctx->nbkref_ents; ++bkc_idx)
        {
          int subexp_len;
          re_dfastate_t *dest_state;
          struct re_backref_cache_entry *bkref_ent;
          bkref_ent = mctx->bkref_ents + bkc_idx;
          if (bkref_ent->node != node_idx || bkref_ent->str_idx != cur_str_idx)
            continue;
          subexp_len = bkref_ent->subexp_to - bkref_ent->subexp_from;
          new_dest_nodes = ((node->type == OP_CONTEXT_NODE && subexp_len == 0)
                            ? dfa->nodes[node_idx].opr.ctx_info->bkref_eclosure
                            : dfa->eclosures + dfa->nexts[node_idx]);
          dest_str_idx = (cur_str_idx + bkref_ent->subexp_to
                          - bkref_ent->subexp_from);
          context = (IS_WORD_CHAR (re_string_byte_at (mctx->input,
                                                      dest_str_idx - 1))
                     ? CONTEXT_WORD : 0);
          dest_state = mctx->state_log[dest_str_idx];
          prev_nelem = ((mctx->state_log[cur_str_idx] == NULL) ? 0
                        : mctx->state_log[cur_str_idx]->nodes.nelem);
          /* Add `new_dest_node' to state_log.  */
          if (dest_state == NULL)
            {
              mctx->state_log[dest_str_idx]
                = re_acquire_state_context (&err, dfa, new_dest_nodes,
                                            context);
              if (BE (mctx->state_log[dest_str_idx] == NULL
                      && err != REG_NOERROR, 0))
                return err;
            }
          else
            {
              re_node_set dest_nodes;
              err = re_node_set_init_union (&dest_nodes,
                                            dest_state->entrance_nodes,
                                            new_dest_nodes);
              if (BE (err != REG_NOERROR, 0))
                return err;
              mctx->state_log[dest_str_idx]
                = re_acquire_state_context (&err, dfa, &dest_nodes, context);
              if (BE (mctx->state_log[dest_str_idx] == NULL
                      && err != REG_NOERROR, 0))
                return err;
              re_node_set_free (&dest_nodes);
            }
          /* We need to check recursively if the backreference can epsilon
             transit.  */
          if (subexp_len == 0
              && mctx->state_log[cur_str_idx]->nodes.nelem > prev_nelem)
            {
              err = transit_state_bkref_loop (preg, new_dest_nodes,
                                              work_state_log, mctx);
              if (BE (err != REG_NOERROR, 0))
                return err;
            }
        }
    }
  re_free (cur_regs);
  return REG_NOERROR;
}

/* Build transition table for the state.
   Return the new table if succeeded, otherwise return NULL.  */

static re_dfastate_t **
build_trtable (preg, state, fl_search)
    const regex_t *preg;
    const re_dfastate_t *state;
    int fl_search;
{
  reg_errcode_t err;
  re_dfa_t *dfa = (re_dfa_t *) preg->buffer;
  int i, j, k, ch;
  int ndests; /* Number of the destination states from `state'.  */
  re_dfastate_t **trtable, **dest_states, **dest_states_word, **dest_states_nl;
  re_node_set follows, *dests_node;
  bitset *dests_ch;
  bitset acceptable;

  /* We build DFA states which corresponds to the destination nodes
     from `state'.  `dests_node[i]' represents the nodes which i-th
     destination state contains, and `dests_ch[i]' represents the
     characters which i-th destination state accepts.  */
  dests_node = re_malloc (re_node_set, SBC_MAX);
  dests_ch = re_malloc (bitset, SBC_MAX);

  /* Initialize transiton table.  */
  trtable = (re_dfastate_t **) calloc (sizeof (re_dfastate_t *), SBC_MAX);
  if (BE (dests_node == NULL || dests_ch == NULL || trtable == NULL, 0))
    return NULL;

  /* At first, group all nodes belonging to `state' into several
     destinations.  */
  ndests = group_nodes_into_DFAstates (preg, state, dests_node, dests_ch);
  if (BE (ndests <= 0, 0))
    {
      re_free (dests_node);
      re_free (dests_ch);
      /* Return NULL in case of an error, trtable otherwise.  */
      return (ndests < 0) ? NULL : trtable;
    }

  dest_states = re_malloc (re_dfastate_t *, ndests);
  dest_states_word = re_malloc (re_dfastate_t *, ndests);
  dest_states_nl = re_malloc (re_dfastate_t *, ndests);
  bitset_empty (acceptable);

  err = re_node_set_alloc (&follows, ndests + 1);
  if (BE (dest_states == NULL || dest_states_word == NULL
          || dest_states_nl == NULL || err != REG_NOERROR, 0))
    return NULL;

  /* Then build the states for all destinations.  */
  for (i = 0; i < ndests; ++i)
    {
      int next_node;
      re_node_set_empty (&follows);
      /* Merge the follows of this destination states.  */
      for (j = 0; j < dests_node[i].nelem; ++j)
        {
          next_node = dfa->nexts[dests_node[i].elems[j]];
          if (next_node != -1)
            {
              err = re_node_set_merge (&follows, dfa->eclosures + next_node);
              if (BE (err != REG_NOERROR, 0))
                return NULL;
            }
        }
      /* If search flag is set, merge the initial state.  */
      if (fl_search)
        {
#ifdef RE_ENABLE_I18N
          int not_initial = 0;
          for (j = 0; j < follows.nelem; ++j)
            if (dfa->nodes[follows.elems[j]].type == CHARACTER)
              {
                not_initial = dfa->nodes[follows.elems[j]].mb_partial;
                break;
              }
          if (!not_initial)
#endif
            {
              err = re_node_set_merge (&follows,
                                       dfa->init_state->entrance_nodes);
              if (BE (err != REG_NOERROR, 0))
                return NULL;
            }
        }
      dest_states[i] = re_acquire_state_context (&err, dfa, &follows, 0);
      if (BE (dest_states[i] == NULL && err != REG_NOERROR, 0))
        return NULL;
      /* If the new state has context constraint,
         build appropriate states for these contexts.  */
      if (dest_states[i]->has_constraint)
        {
          dest_states_word[i] = re_acquire_state_context (&err, dfa, &follows,
                                                          CONTEXT_WORD);
          if (BE (dest_states_word[i] == NULL && err != REG_NOERROR, 0))
            return NULL;
          dest_states_nl[i] = re_acquire_state_context (&err, dfa, &follows,
                                                        CONTEXT_NEWLINE);
          if (BE (dest_states_nl[i] == NULL && err != REG_NOERROR, 0))
            return NULL;
        }
      else
        {
          dest_states_word[i] = dest_states[i];
          dest_states_nl[i] = dest_states[i];
        }
      bitset_merge (acceptable, dests_ch[i]);
    }

  /* Update the transition table.  */
  /* For all characters ch...:  */
  for (i = 0, ch = 0; i < BITSET_UINTS; ++i)
    for (j = 0; j < UINT_BITS; ++j, ++ch)
      if ((acceptable[i] >> j) & 1)
        {
          /* The current state accepts the character ch.  */
          if (IS_WORD_CHAR (ch))
            {
              for (k = 0; k < ndests; ++k)
                if ((dests_ch[k][i] >> j) & 1)
                  {
                    /* k-th destination accepts the word character ch.  */
                    trtable[ch] = dest_states_word[k];
                    /* There must be only one destination which accepts
                       character ch.  See group_nodes_into_DFAstates.  */
                    break;
                  }
            }
          else /* not WORD_CHAR */
            {
              for (k = 0; k < ndests; ++k)
                if ((dests_ch[k][i] >> j) & 1)
                  {
                    /* k-th destination accepts the non-word character ch.  */
                    trtable[ch] = dest_states[k];
                    /* There must be only one destination which accepts
                       character ch.  See group_nodes_into_DFAstates.  */
                    break;
                  }
            }
        }
  /* new line */
  if (bitset_contain (acceptable, NEWLINE_CHAR))
    {
      /* The current state accepts newline character.  */
      for (k = 0; k < ndests; ++k)
        if (bitset_contain (dests_ch[k], NEWLINE_CHAR))
          {
            /* k-th destination accepts newline character.  */
            trtable[NEWLINE_CHAR] = dest_states_nl[k];
            /* There must be only one destination which accepts
               newline.  See group_nodes_into_DFAstates.  */
            break;
          }
    }

  re_free (dest_states_nl);
  re_free (dest_states_word);
  re_free (dest_states);

  re_node_set_free (&follows);
  for (i = 0; i < ndests; ++i)
    re_node_set_free (dests_node + i);

  re_free (dests_ch);
  re_free (dests_node);

  return trtable;
}

/* Group all nodes belonging to STATE into several destinations.
   Then for all destinations, set the nodes belonging to the destination
   to DESTS_NODE[i] and set the characters accepted by the destination
   to DEST_CH[i].  This function return the number of destinations.  */

static int
group_nodes_into_DFAstates (preg, state, dests_node, dests_ch)
    const regex_t *preg;
    const re_dfastate_t *state;
    re_node_set *dests_node;
    bitset *dests_ch;
{
  reg_errcode_t err;
  const re_dfa_t *dfa = (re_dfa_t *) preg->buffer;
  int i, j, k;
  int ndests; /* Number of the destinations from `state'.  */
  bitset accepts; /* Characters a node can accept.  */
  const re_node_set *cur_nodes = &state->nodes;
  bitset_empty (accepts);
  ndests = 0;

  /* For all the nodes belonging to `state',  */
  for (i = 0; i < cur_nodes->nelem; ++i)
    {
      unsigned int constraint = 0;
      re_token_t *node = &dfa->nodes[cur_nodes->elems[i]];
      re_token_type_t type = node->type;

      if (type == OP_CONTEXT_NODE)
        {
          constraint = node->constraint;
          node = dfa->nodes + node->opr.ctx_info->entity;
          type = node->type;
        }

      /* Enumerate all single byte character this node can accept.  */
      if (type == CHARACTER)
        bitset_set (accepts, node->opr.c);
      else if (type == SIMPLE_BRACKET)
        {
          bitset_merge (accepts, node->opr.sbcset);
        }
      else if (type == OP_PERIOD)
        {
          bitset_set_all (accepts);
          if (!(preg->syntax & RE_DOT_NEWLINE))
            bitset_clear (accepts, '\n');
          if (preg->syntax & RE_DOT_NOT_NULL)
            bitset_clear (accepts, '\0');
        }
      else
        continue;

      /* Check the `accepts' and sift the characters which are not
         match it the context.  */
      if (constraint)
        {
          if (constraint & NEXT_WORD_CONSTRAINT)
            for (j = 0; j < BITSET_UINTS; ++j)
              accepts[j] &= dfa->word_char[j];
          else if (constraint & NEXT_NOTWORD_CONSTRAINT)
            for (j = 0; j < BITSET_UINTS; ++j)
              accepts[j] &= ~dfa->word_char[j];
          else if (constraint & NEXT_NEWLINE_CONSTRAINT)
            {
              int accepts_newline = bitset_contain (accepts, NEWLINE_CHAR);
              bitset_empty (accepts);
              if (accepts_newline)
                bitset_set (accepts, NEWLINE_CHAR);
              else
                continue;
            }
        }

      /* Then divide `accepts' into DFA states, or create a new
         state.  */
      for (j = 0; j < ndests; ++j)
        {
          bitset intersec; /* Intersection sets, see below.  */
          bitset remains;
          /* Flags, see below.  */
          int has_intersec, not_subset, not_consumed;

          /* Optimization, skip if this state doesn't accept the character.  */
          if (type == CHARACTER && !bitset_contain (dests_ch[j], node->opr.c))
            continue;

          /* Enumerate the intersection set of this state and `accepts'.  */
          has_intersec = 0;
          for (k = 0; k < BITSET_UINTS; ++k)
            has_intersec |= intersec[k] = accepts[k] & dests_ch[j][k];
          /* And skip if the intersection set is empty.  */
          if (!has_intersec)
            continue;

          /* Then check if this state is a subset of `accepts'.  */
          not_subset = not_consumed = 0;
          for (k = 0; k < BITSET_UINTS; ++k)
            {
              not_subset |= remains[k] = ~accepts[k] & dests_ch[j][k];
              not_consumed |= accepts[k] = accepts[k] & ~dests_ch[j][k];
            }

          /* If this state isn't a subset of `accepts', create a
             new group state, which has the `remains'. */
          if (not_subset)
            {
              bitset_copy (dests_ch[ndests], remains);
              bitset_copy (dests_ch[j], intersec);
              err = re_node_set_init_copy (dests_node + ndests, &dests_node[j]);
              if (BE (err != REG_NOERROR, 0))
                return -1;
              ++ndests;
            }

          /* Put the position in the current group. */
          err = re_node_set_insert (&dests_node[j], cur_nodes->elems[i]);
          if (BE (err < 0, 0))
            return -1;

          /* If all characters are consumed, go to next node. */
          if (!not_consumed)
            break;
        }
      /* Some characters remain, create a new group. */
      if (j == ndests)
        {
          bitset_copy (dests_ch[ndests], accepts);
          err = re_node_set_init_1 (dests_node + ndests, cur_nodes->elems[i]);
          if (BE (err != REG_NOERROR, 0))
            return -1;
          ++ndests;
          bitset_empty (accepts);
        }
    }
  return ndests;
}

#ifdef RE_ENABLE_I18N
/* Check how many bytes the node `dfa->nodes[node_idx]' accepts.
   Return the number of the bytes the node accepts.
   STR_IDX is the current index of the input string.

   This function handles the nodes which can accept one character, or
   one collating element like '.', '[a-z]', opposite to the other nodes
   can only accept one byte.  */

static int
check_node_accept_bytes (preg, node_idx, input, str_idx)
    const regex_t *preg;
    int node_idx, str_idx;
    const re_string_t *input;
{
  const re_dfa_t *dfa = (re_dfa_t *) preg->buffer;
  const re_token_t *node = dfa->nodes + node_idx;
  int elem_len = re_string_elem_size_at (input, str_idx);
  int char_len = re_string_char_size_at (input, str_idx);
  int i;
# ifdef _LIBC
  int j;
  uint32_t nrules = _NL_CURRENT_WORD (LC_COLLATE, _NL_COLLATE_NRULES);
# endif /* _LIBC */
  if (elem_len <= 1 && char_len <= 1)
    return 0;
  if (node->type == OP_PERIOD)
    {
      /* '.' accepts any one character except the following two cases.  */
      if ((!(preg->syntax & RE_DOT_NEWLINE) &&
           re_string_byte_at (input, str_idx) == '\n') ||
          ((preg->syntax & RE_DOT_NOT_NULL) &&
           re_string_byte_at (input, str_idx) == '\0'))
        return 0;
      return char_len;
    }
  else if (node->type == COMPLEX_BRACKET)
    {
      const re_charset_t *cset = node->opr.mbcset;
# ifdef _LIBC
      const unsigned char *pin = re_string_get_buffer (input) + str_idx;
# endif /* _LIBC */
      int match_len = 0;
      wchar_t wc = ((cset->nranges || cset->nchar_classes || cset->nmbchars)
                    ? re_string_wchar_at (input, str_idx) : 0);

      /* match with multibyte character?  */
      for (i = 0; i < cset->nmbchars; ++i)
        if (wc == cset->mbchars[i])
          {
            match_len = char_len;
            goto check_node_accept_bytes_match;
          }
      /* match with character_class?  */
      for (i = 0; i < cset->nchar_classes; ++i)
        {
          wctype_t wt = cset->char_classes[i];
          if (__iswctype (wc, wt))
            {
              match_len = char_len;
              goto check_node_accept_bytes_match;
            }
        }

# ifdef _LIBC
      if (nrules != 0)
        {
          unsigned int in_collseq = 0;
          const int32_t *table, *indirect;
          const unsigned char *weights, *extra;
          const char *collseqwc;
          int32_t idx;
          /* This #include defines a local function!  */
#  include <locale/weight.h>

          /* match with collating_symbol?  */
          if (cset->ncoll_syms)
            extra = (const unsigned char *)
              _NL_CURRENT (LC_COLLATE, _NL_COLLATE_SYMB_EXTRAMB);
          for (i = 0; i < cset->ncoll_syms; ++i)
            {
              const unsigned char *coll_sym = extra + cset->coll_syms[i];
              /* Compare the length of input collating element and
                 the length of current collating element.  */
              if (*coll_sym != elem_len)
                continue;
              /* Compare each bytes.  */
              for (j = 0; j < *coll_sym; j++)
                if (pin[j] != coll_sym[1 + j])
                  break;
              if (j == *coll_sym)
                {
                  /* Match if every bytes is equal.  */
                  match_len = j;
                  goto check_node_accept_bytes_match;
                }
            }

          if (cset->nranges)
            {
              if (elem_len <= char_len)
                {
                  collseqwc = _NL_CURRENT (LC_COLLATE, _NL_COLLATE_COLLSEQWC);
                  in_collseq = collseq_table_lookup (collseqwc, wc);
                }
              else
                in_collseq = find_collation_sequence_value (pin, elem_len);
            }
          /* match with range expression?  */
          for (i = 0; i < cset->nranges; ++i)
            if (cset->range_starts[i] <= in_collseq
                && in_collseq <= cset->range_ends[i])
              {
                match_len = elem_len;
                goto check_node_accept_bytes_match;
              }

          /* match with equivalence_class?  */
          if (cset->nequiv_classes)
            {
              const unsigned char *cp = pin;
              table = (const int32_t *)
                _NL_CURRENT (LC_COLLATE, _NL_COLLATE_TABLEMB);
              weights = (const unsigned char *)
                _NL_CURRENT (LC_COLLATE, _NL_COLLATE_WEIGHTMB);
              extra = (const unsigned char *)
                _NL_CURRENT (LC_COLLATE, _NL_COLLATE_EXTRAMB);
              indirect = (const int32_t *)
                _NL_CURRENT (LC_COLLATE, _NL_COLLATE_INDIRECTMB);
              idx = findidx (&cp);
              if (idx > 0)
                for (i = 0; i < cset->nequiv_classes; ++i)
                  {
                    int32_t equiv_class_idx = cset->equiv_classes[i];
                    size_t weight_len = weights[idx];
                    if (weight_len == weights[equiv_class_idx])
                      {
                        int cnt = 0;
                        while (cnt <= weight_len
                               && (weights[equiv_class_idx + 1 + cnt]
                                   == weights[idx + 1 + cnt]))
                          ++cnt;
                        if (cnt > weight_len)
                          {
                            match_len = elem_len;
                            goto check_node_accept_bytes_match;
                          }
                      }
                  }
            }
        }
      else
# endif /* _LIBC */
        {
          /* match with range expression?  */
#if __GNUC__ >= 2
          wchar_t cmp_buf[] = {L'\0', L'\0', wc, L'\0', L'\0', L'\0'};
#else
          wchar_t cmp_buf[] = {L'\0', L'\0', L'\0', L'\0', L'\0', L'\0'};
          cmp_buf[2] = wc;
#endif
          for (i = 0; i < cset->nranges; ++i)
            {
              cmp_buf[0] = cset->range_starts[i];
              cmp_buf[4] = cset->range_ends[i];
              if (wcscoll (cmp_buf, cmp_buf + 2) <= 0
                  && wcscoll (cmp_buf + 2, cmp_buf + 4) <= 0)
                {
                  match_len = char_len;
                  goto check_node_accept_bytes_match;
                }
            }
        }
    check_node_accept_bytes_match:
      if (!cset->non_match)
        return match_len;
      else
        {
          if (match_len > 0)
            return 0;
          else
            return (elem_len > char_len) ? elem_len : char_len;
        }
    }
  return 0;
}

# ifdef _LIBC
static unsigned int
find_collation_sequence_value (mbs, mbs_len)
    const unsigned char *mbs;
    size_t mbs_len;
{
  uint32_t nrules = _NL_CURRENT_WORD (LC_COLLATE, _NL_COLLATE_NRULES);
  if (nrules == 0)
    {
      if (mbs_len == 1)
        {
          /* No valid character.  Match it as a single byte character.  */
          const unsigned char *collseq = (const unsigned char *)
            _NL_CURRENT (LC_COLLATE, _NL_COLLATE_COLLSEQMB);
          return collseq[mbs[0]];
        }
      return UINT_MAX;
    }
  else
    {
      int32_t idx;
      const unsigned char *extra = (const unsigned char *)
        _NL_CURRENT (LC_COLLATE, _NL_COLLATE_SYMB_EXTRAMB);

      for (idx = 0; ;)
        {
          int mbs_cnt, found = 0;
          int32_t elem_mbs_len;
          /* Skip the name of collating element name.  */
          idx = idx + extra[idx] + 1;
          elem_mbs_len = extra[idx++];
          if (mbs_len == elem_mbs_len)
            {
              for (mbs_cnt = 0; mbs_cnt < elem_mbs_len; ++mbs_cnt)
                if (extra[idx + mbs_cnt] != mbs[mbs_cnt])
                  break;
              if (mbs_cnt == elem_mbs_len)
                /* Found the entry.  */
                found = 1;
            }
          /* Skip the byte sequence of the collating element.  */
          idx += elem_mbs_len;
          /* Adjust for the alignment.  */
          idx = (idx + 3) & ~3;
          /* Skip the collation sequence value.  */
          idx += sizeof (uint32_t);
          /* Skip the wide char sequence of the collating element.  */
          idx = idx + sizeof (uint32_t) * (extra[idx] + 1);
          /* If we found the entry, return the sequence value.  */
          if (found)
            return *(uint32_t *) (extra + idx);
          /* Skip the collation sequence value.  */
          idx += sizeof (uint32_t);
        }
    }
}
# endif /* _LIBC */
#endif /* RE_ENABLE_I18N */

/* Check whether the node accepts the byte which is IDX-th
   byte of the INPUT.  */

static int
check_node_accept (preg, node, mctx, idx)
    const regex_t *preg;
    const re_token_t *node;
    const re_match_context_t *mctx;
    int idx;
{
  const re_dfa_t *dfa = (re_dfa_t *) preg->buffer;
  const re_token_t *cur_node;
  unsigned char ch;
  if (node->type == OP_CONTEXT_NODE)
    {
      /* The node has constraints.  Check whether the current context
         satisfies the constraints.  */
      unsigned int context = re_string_context_at (mctx->input, idx,
                                                   mctx->eflags,
                                                   preg->newline_anchor);
      if (NOT_SATISFY_NEXT_CONSTRAINT (node->constraint, context))
        return 0;
      cur_node = dfa->nodes + node->opr.ctx_info->entity;
    }
  else
    cur_node = node;

  ch = re_string_byte_at (mctx->input, idx);
  if (cur_node->type == CHARACTER)
    return cur_node->opr.c == ch;
  else if (cur_node->type == SIMPLE_BRACKET)
    return bitset_contain (cur_node->opr.sbcset, ch);
  else if (cur_node->type == OP_PERIOD)
    return !((ch == '\n' && !(preg->syntax & RE_DOT_NEWLINE))
             || (ch == '\0' && (preg->syntax & RE_DOT_NOT_NULL)));
  else
    return 0;
}

/* Extend the buffers, if the buffers have run out.  */

static reg_errcode_t
extend_buffers (mctx)
     re_match_context_t *mctx;
{
  reg_errcode_t ret;
  re_string_t *pstr = mctx->input;

  /* Double the lengthes of the buffers.  */
  ret = re_string_realloc_buffers (pstr, pstr->bufs_len * 2);
  if (BE (ret != REG_NOERROR, 0))
    return ret;

  if (mctx->state_log != NULL)
    {
      /* And double the length of state_log.  */
      mctx->state_log = re_realloc (mctx->state_log, re_dfastate_t *,
                                    pstr->bufs_len * 2);
      if (BE (mctx->state_log == NULL, 0))
        return REG_ESPACE;
    }

  /* Then reconstruct the buffers.  */
  if (pstr->icase)
    {
#ifdef RE_ENABLE_I18N
      if (MB_CUR_MAX > 1)
        build_wcs_upper_buffer (pstr);
      else
#endif /* RE_ENABLE_I18N  */
        build_upper_buffer (pstr);
    }
  else
    {
#ifdef RE_ENABLE_I18N
      if (MB_CUR_MAX > 1)
        build_wcs_buffer (pstr);
      else
#endif /* RE_ENABLE_I18N  */
        {
          if (pstr->trans != NULL)
            re_string_translate_buffer (pstr);
          else
            pstr->valid_len = pstr->bufs_len;
        }
    }
  return REG_NOERROR;
}


/* Functions for matching context.  */

static reg_errcode_t
match_ctx_init (mctx, eflags, input, n)
    re_match_context_t *mctx;
    int eflags, n;
    re_string_t *input;
{
  mctx->eflags = eflags;
  mctx->input = input;
  mctx->match_last = -1;
  if (n > 0)
    {
      mctx->bkref_ents = re_malloc (struct re_backref_cache_entry, n);
      if (BE (mctx->bkref_ents == NULL, 0))
        return REG_ESPACE;
    }
  else
    mctx->bkref_ents = NULL;
  mctx->nbkref_ents = 0;
  mctx->abkref_ents = n;
  mctx->max_mb_elem_len = 0;
  return REG_NOERROR;
}

static void
match_ctx_free (mctx)
    re_match_context_t *mctx;
{
  re_free (mctx->bkref_ents);
}

/* Add a new backreference entry to the cache.  */

static reg_errcode_t
match_ctx_add_entry (mctx, node, str_idx, from, to)
    re_match_context_t *mctx;
    int node, str_idx, from, to;
{
  if (mctx->nbkref_ents >= mctx->abkref_ents)
    {
      mctx->bkref_ents = re_realloc (mctx->bkref_ents,
                                     struct re_backref_cache_entry,
                                     mctx->abkref_ents * 2);
      if (BE (mctx->bkref_ents == NULL, 0))
        return REG_ESPACE;
      memset (mctx->bkref_ents + mctx->nbkref_ents, '\0',
             sizeof (struct re_backref_cache_entry) * mctx->abkref_ents);
      mctx->abkref_ents *= 2;
    }
  mctx->bkref_ents[mctx->nbkref_ents].node = node;
  mctx->bkref_ents[mctx->nbkref_ents].str_idx = str_idx;
  mctx->bkref_ents[mctx->nbkref_ents].subexp_from = from;
  mctx->bkref_ents[mctx->nbkref_ents].subexp_to = to;
  mctx->bkref_ents[mctx->nbkref_ents++].flag = 0;
  if (mctx->max_mb_elem_len < to - from)
    mctx->max_mb_elem_len = to - from;
  return REG_NOERROR;
}

static void
match_ctx_clear_flag (mctx)
     re_match_context_t *mctx;
{
  int i;
  for (i = 0; i < mctx->nbkref_ents; ++i)
    {
      mctx->bkref_ents[i].flag = 0;
    }
}

static void
sift_ctx_init (sctx, sifted_sts, limited_sts, last_node, last_str_idx,
               check_subexp)
    re_sift_context_t *sctx;
    re_dfastate_t **sifted_sts, **limited_sts;
    int last_node, last_str_idx, check_subexp;
{
  sctx->sifted_states = sifted_sts;
  sctx->limited_states = limited_sts;
  sctx->last_node = last_node;
  sctx->last_str_idx = last_str_idx;
  sctx->check_subexp = check_subexp;
  re_node_set_init_empty (&sctx->limits);
}
