#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "passe_1.h"
#include "miniccutils.h"



void analyse_passe_1(node_t root) {
    push_global_context();

    // Déclarations globales 
    analyse_decl_list(root->opr[0], true);

    reset_env_current_offset();

    // Vérification de la fonction main 
    node_t mainf = root->opr[1];
    
    // Vérification du nom "main"
    if (strcmp(mainf->opr[1]->ident, "main") != 0) {
        fprintf(stderr, "Error line %d: unique function must be named 'main'\n", mainf->lineno);
        exit(1);
    }
    
    // Vérification du type de retour void 
    if (get_type_from_type_node(mainf->opr[0]) != TYPE_VOID) {
        fprintf(stderr, "Error line %d: function 'main' must return void\n", mainf->lineno);
        exit(1);
    }

    push_context();
    analyse_block(mainf->opr[2]);
    mainf->offset = get_env_current_offset();
    pop_context();

    pop_global_context();
}

static void analyse_block(node_t n) {
    push_context();

    analyse_decl_list(n->opr[0], false);

    analyse_stmt_list(n->opr[1]);

    pop_context();

}

static void analyse_stmt_list(node_t n) {
    if (!n) return;
    if (n->nature == NODE_LIST) {
        analyse_stmt_list(n->opr[0]);
        analyse_stmt_list(n->opr[1]);
    } else {
        analyse_stmt(n);
    }
}

static void analyse_print_list(node_t n) {
    if (!n) return;
    if (n->nature == NODE_LIST) {
        analyse_print_list(n->opr[0]);
        analyse_print_list(n->opr[1]);
    } else {
        resolve_idents_expr(n);
        type_expr(n);
    }
}

static void analyse_stmt(node_t n) {
    if (!n) return;

    switch (n->nature) {
        case NODE_BLOCK:
            analyse_block(n);
            break;

        case NODE_IF:
            resolve_idents_expr(n->opr[0]);
            type_expr(n->opr[0]);
            if (n->opr[0]->type != TYPE_BOOL) {
                fprintf(stderr, "Error line %d: if condition must be bool\n", n->opr[0]->lineno);
                exit(1);
            }
            analyse_stmt(n->opr[1]);
            if (n->nops == 3) analyse_stmt(n->opr[2]);
            break;
        

        case NODE_WHILE:
            resolve_idents_expr(n->opr[0]);
            type_expr(n->opr[0]);
            if (n->opr[0]->type != TYPE_BOOL) {
                fprintf(stderr, "Error line %d: while condition must be bool\n", n->opr[0]->lineno);
                exit(1);
            }
            analyse_stmt(n->opr[1]);
            break;
        

        case NODE_DOWHILE:
            analyse_stmt(n->opr[0]);
            resolve_idents_expr(n->opr[1]);
            type_expr(n->opr[1]);
            if (n->opr[1]->type != TYPE_BOOL) {
                fprintf(stderr, "Error line %d: do-while condition must be bool\n", n->opr[1]->lineno);
                exit(1);
            }
            break;
        

        case NODE_FOR:
        // Résolution de tous les identifiants
        for (int i = 0; i < 3; i++) {
            if (n->opr[i]) resolve_idents_expr(n->opr[i]);
        }
    
        // Typage de l'initialisation et de l'incrément
        if (n->opr[0]) type_expr(n->opr[0]);
        if (n->opr[2]) type_expr(n->opr[2]);
    
        // Typage et vérification de la condition (doit être bool) 
        if (n->opr[1]) {
            type_expr(n->opr[1]);
            if (n->opr[1]->type != TYPE_BOOL) {
                fprintf(stderr, "Error line %d: for condition must be bool\n", n->opr[1]->lineno);
                exit(1);
            }
        }
        analyse_stmt(n->opr[3]);
        break;
        
        case NODE_PRINT:
            analyse_print_list(n->opr[0]);
            break;
        
        default:
            resolve_idents_expr(n);
            type_expr(n); 
            break;        
    }
}

static node_type get_type_from_type_node(node_t t) {
    if (!t || t->nature != NODE_TYPE) {
        fprintf(stderr, "Internal error: expected NODE_TYPE\n");
        exit(1);
    }
    return t->type; /* TYPE_INT / TYPE_BOOL / TYPE_VOID */
}

/* n est soit NULL, soit NODE_LIST, soit NODE_DECLS */
static void analyse_decl_list(node_t n, bool global) {
    if (!n) return;
    if (n->nature == NODE_LIST) {
        analyse_decl_list(n->opr[0], global);
        analyse_decl_list(n->opr[1], global);
        return;
    }
    if (n->nature != NODE_DECLS) {
        fprintf(stderr, "Internal error line %d: expected NODE_DECLS\n", n->lineno);
        exit(1);
    }
    analyse_decls(n, global);
}

/* NODE_DECLS(type, listtypedecl) */
static void analyse_decls(node_t n, bool global) {
    node_t type_node = n->opr[0];
    node_t list = n->opr[1];

    // On délègue le parcours de la liste à une fonction dédiée
    analyse_decl_traverse(type_node, list, global);
}

static void analyse_decl_traverse(node_t type_node, node_t list_node, bool global) {
    if (!list_node) return;

    if (list_node->nature == NODE_LIST) {
        // Parcours récursif standard de la liste sans make_node
        analyse_decl_traverse(type_node, list_node->opr[0], global);
        analyse_decl_traverse(type_node, list_node->opr[1], global);
    } else if (list_node->nature == NODE_DECL) {
        // On a atteint une déclaration individuelle
        analyse_decl(type_node, list_node, global);
    }
}
/* NODE_DECL(ident, initExprOrNULL) */
static void analyse_decl(node_t type_node, node_t decl_node, bool global) {
    node_t id = decl_node->opr[0];  
    if (!id || id->nature != NODE_IDENT) {
        fprintf(stderr, "Internal error line %d: declaration without ident\n", decl_node->lineno);
        exit(1);
    }

    /* on remplit le type sur l'occurrence de déclaration */
    id->type = get_type_from_type_node(type_node);
    id->global_decl = global;

    /* on ajoute dans l'environnement courant */
    int32_t off = env_add_element(id->ident, id);
    if (off < 0) {
        fprintf(stderr, "Error line %d: duplicate declaration of '%s'\n", id->lineno, id->ident);
        exit(1);
    }
    id->offset = off;

}

static void resolve_idents_expr(node_t n) {
    if (!n) return;

    switch (n->nature) {
        case NODE_IDENT: {
            node_t decl = (node_t)get_decl_node(n->ident);
            if (!decl) {
                fprintf(stderr, "Error line %d: use of undeclared variable '%s'\n",
                        n->lineno, n->ident);
                exit(1);
            }

            n->decl_node = decl;
            n->type = decl->type;
            n->offset = decl->offset;
            n->global_decl = decl->global_decl;
            return;
        }

        /* Littéraux : rien à résoudre */
        case NODE_INTVAL:
        case NODE_BOOLVAL:
        case NODE_STRINGVAL:
            return;
        

        default:
            /* Parcours générique : résoudre récursivement les enfants */
            for (int i = 0; i < n->nops; i++) {
                resolve_idents_expr(n->opr[i]);
            }
            return;
    }
}

static void type_expr(node_t n) {
    if (!n) return;

    /* D'abord on type les enfants  */
    for (int i = 0; i < n->nops; i++) {
        type_expr(n->opr[i]);
    }

    switch (n->nature) {
        case NODE_INTVAL:
            n->type = TYPE_INT;
            return;

        case NODE_BOOLVAL:
            n->type = TYPE_BOOL;
            return;

        case NODE_IDENT:
            return;

        /* Unaires */
        case NODE_UMINUS:
            if (n->opr[0]->type != TYPE_INT) {
                fprintf(stderr, "Error line %d: unary '-' expects int\n", n->lineno);
                exit(1);
            }
            n->type = TYPE_INT;
            return;

        case NODE_NOT:
            if (n->opr[0]->type != TYPE_BOOL) {
                fprintf(stderr, "Error line %d: '!' expects bool\n", n->lineno);
                exit(1);
            }
            n->type = TYPE_BOOL;
            return;

        /* Arithmétique int,int -> int */
        case NODE_PLUS:
        case NODE_MINUS:
        case NODE_MUL:
        case NODE_DIV:
        case NODE_MOD:
            if (n->opr[0]->type != TYPE_INT || n->opr[1]->type != TYPE_INT) {
                fprintf(stderr, "Error line %d: arithmetic op expects int operands\n", n->lineno);
                exit(1);
            }
            n->type = TYPE_INT;
            return;

        /* Comparaisons int,int -> bool */
        case NODE_LT:
        case NODE_LE:
        case NODE_GT:
        case NODE_GE:
            if (n->opr[0]->type != TYPE_INT || n->opr[1]->type != TYPE_INT) {
                fprintf(stderr, "Error line %d: comparison expects int operands\n", n->lineno);
                exit(1);
            }
            n->type = TYPE_BOOL;
            return;

        /* Logique bool,bool -> bool */
        case NODE_AND:
        case NODE_OR:
            if (n->opr[0]->type != TYPE_BOOL || n->opr[1]->type != TYPE_BOOL) {
                fprintf(stderr, "Error line %d: logical op expects bool operands\n", n->lineno);
                exit(1);
            }
            n->type = TYPE_BOOL;
            return;

        /* Affectation: ident = expr, types identiques */
        case NODE_AFFECT: {
            node_t lhs = n->opr[0];
            node_t rhs = n->opr[1];

            if (!lhs || lhs->nature != NODE_IDENT) {
                fprintf(stderr, "Error line %d: left side of assignment must be an identifier\n", n->lineno);
                exit(1);
            }

            /* lhs déjà typé via résolution */
            if (lhs->type != rhs->type) {
                fprintf(stderr, "Error line %d: type mismatch in assignment\n", n->lineno);
                exit(1);
            }
            n->type = lhs->type;
            return;
        }

        case NODE_EQ:
        case NODE_NE:
            if (n->opr[0]->type != n->opr[1]->type) {
                fprintf(stderr, "Error line %d: '=='/'!=' operands must have same type\n", n->lineno);
                exit(1);
            }
            if (n->opr[0]->type != TYPE_INT && n->opr[0]->type != TYPE_BOOL) {
                fprintf(stderr, "Error line %d: '=='/'!=' only supports int/bool\n", n->lineno);
                exit(1);
            }
            n->type = TYPE_BOOL;
            return;

        case NODE_BAND:
        case NODE_BOR:
        case NODE_BXOR:
            if (n->opr[0]->type != TYPE_INT || n->opr[1]->type != TYPE_INT) {
                fprintf(stderr, "Error line %d: bitwise op expects int operands\n", n->lineno);
                exit(1);
            }
            n->type = TYPE_INT;
            return;
            
        case NODE_SLL:
        case NODE_SRA:
        case NODE_SRL:
            if (n->opr[0]->type != TYPE_INT || n->opr[1]->type != TYPE_INT) {
                fprintf(stderr, "Error line %d: shift op expects int operands\n", n->lineno);
                exit(1);
            }
            n->type = TYPE_INT;
            return;
    
        case NODE_BNOT:
            if (n->opr[0]->type != TYPE_INT) {
                fprintf(stderr, "Error line %d: '~' expects int\n", n->lineno);
                exit(1);
            }
            n->type = TYPE_INT;
            return;
        
            
        default:

        return;
    }
}

