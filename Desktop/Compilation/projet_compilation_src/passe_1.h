
#ifndef _PASSE_1_
#define _PASSE_1_

#include "defs.h"


void analyse_passe_1(node_t root);
static void analyse_stmt(node_t n);
static void analyse_block(node_t n);
static void analyse_stmt_list(node_t n);
static void analyse_decl_list(node_t n, bool global);
static void analyse_decl_traverse(node_t type_node, node_t list_node, bool global);
static void analyse_decls(node_t n, bool global);
static void analyse_decl(node_t type_node, node_t decl_node, bool global);
static node_type get_type_from_type_node(node_t t);
static void resolve_idents_expr(node_t n);
static void type_expr(node_t n);


#endif

