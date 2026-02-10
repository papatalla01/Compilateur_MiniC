#ifndef _PASSE_2_H_
#define _PASSE_2_H_

#include "defs.h"

static void gen_global_decls(node_t n);
static void gen_one_decls(node_t decls);
static void gen_decl_list(node_t n);
static int32_t eval_const_init(node_t e);
static void gen_func(node_t func);
static void gen_block(node_t block);
static void gen_stmt_list(node_t n);
static void gen_stmt(node_t n);

static int32_t gen_expr(node_t e);                 
static void gen_expr_discard(node_t e);           
static void gen_print(node_t printnode);

static void store_ident(node_t ident_use_or_decl, int32_t reg_val);
static int32_t load_ident(node_t ident_use_or_decl);

static void load_data_base_into(int32_t reg);
static void load_data_addr_into(int32_t reg, int32_t offset);
void gen_code_passe_2(node_t root);

#endif

