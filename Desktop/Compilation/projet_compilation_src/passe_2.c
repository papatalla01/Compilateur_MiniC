#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "defs.h"
#include "passe_2.h"
#include "miniccutils.h"
#include "arch.h"

extern int trace_level;


void gen_code_passe_2(node_t root) {
    if (!root || root->nature != NODE_PROGRAM) return;

    create_data_sec_inst();

    gen_global_decls(root->opr[0]);

    int32_t nstr = get_global_strings_number();
    for (int32_t i = 0; i < nstr; i++) {
        char lab[32];
        snprintf(lab, sizeof(lab), "_S%d", i);
        create_asciiz_inst(lab, get_global_string(i));
    }

    create_text_sec_inst();
    gen_func(root->opr[1]);
}

static void gen_global_decls(node_t n) {
    if (!n) return;
    if (n->nature == NODE_LIST) {
        gen_global_decls(n->opr[0]);
        gen_global_decls(n->opr[1]);
        return;
    }
    gen_one_decls(n);
}

static void gen_one_decls(node_t decls) {
    if (!decls || decls->nature != NODE_DECLS) return;
    gen_decl_list(decls->opr[1]);
}

static void gen_decl_list(node_t n) {
    if (!n) return;
    if (n->nature == NODE_LIST) {
        gen_decl_list(n->opr[0]);
        gen_decl_list(n->opr[1]);
        return;
    }
    if (n->nature != NODE_DECL) return;

    node_t id = n->opr[0]; // Identifiant de la déclaration
    node_t init = n->opr[1];   
    int32_t init_val = 0;
    if (init) init_val = eval_const_init(init);

    // Utilisation de l'offset pour l'adressage
    create_word_inst(id ? id->ident : NULL, init_val);
}

// Évalue une initialisation constante pour les variables globales
static int32_t eval_const_init(node_t e) {
    if (!e) return 0;
    switch (e->nature) {
        case NODE_INTVAL:  return e->value;
        case NODE_BOOLVAL: return e->value ? 1 : 0;
        default:
            return 0;
    }
}

static void gen_func(node_t func) {
    if (!func || func->nature != NODE_FUNC) return;

    create_label_str_inst("main");
    create_stack_allocation_inst();

    set_temporary_start_offset(func->offset);
    reset_temporary_max_offset();

    gen_block(func->opr[2]);

    int32_t total = func->offset + get_temporary_max_offset();
    create_stack_deallocation_inst(total);

    create_addiu_inst(2, get_r0(), 10); // $v0 = 10
    create_syscall_inst();
}

static void gen_block(node_t block) {
    if (!block || block->nature != NODE_BLOCK) return;
    gen_stmt_list(block->opr[1]);
}

static void gen_stmt_list(node_t n) {
    if (!n) return;
    if (n->nature == NODE_LIST) {
        gen_stmt_list(n->opr[0]);
        gen_stmt_list(n->opr[1]);
    } else {
        gen_stmt(n);
    }
}

static void gen_stmt(node_t n) {
    if (!n) return;

    switch (n->nature) {
        case NODE_BLOCK:
            gen_block(n);
            return;

        case NODE_PRINT:
            gen_print(n);
            return;

        case NODE_IF: {
            int32_t l_else = get_new_label();
            int32_t l_end  = get_new_label();

            int32_t r = gen_expr(n->opr[0]); // Condition
            create_beq_inst(r, get_r0(), (n->nops == 3) ? l_else : l_end);
            release_reg();

            gen_stmt(n->opr[1]);
            if (n->nops == 3) {
                create_j_inst(l_end);
                create_label_inst(l_else);
                gen_stmt(n->opr[2]); // Sinon
            }
            create_label_inst(l_end);
            return;
        }

        case NODE_WHILE: {
            int32_t l_begin = get_new_label();
            int32_t l_end   = get_new_label();

            create_label_inst(l_begin);
            int32_t r = gen_expr(n->opr[0]);
            create_beq_inst(r, get_r0(), l_end);
            release_reg();

            gen_stmt(n->opr[1]);
            create_j_inst(l_begin);
            create_label_inst(l_end);
            return;
        }

        case NODE_DOWHILE: {
            int32_t l_begin = get_new_label();
            create_label_inst(l_begin);

            gen_stmt(n->opr[0]); // Corps

            int32_t r = gen_expr(n->opr[1]); // Condition
            create_bne_inst(r, get_r0(), l_begin);
            release_reg();
            return;
        }

        case NODE_FOR: {
            int32_t l_begin = get_new_label();
            int32_t l_end   = get_new_label();

            if (n->opr[0]) gen_expr_discard(n->opr[0]); // Initialisation

            create_label_inst(l_begin);

            if (n->opr[1]) {
                int32_t r = gen_expr(n->opr[1]); // Condition
                create_beq_inst(r, get_r0(), l_end);
                release_reg();
            }

            gen_stmt(n->opr[3]); // Corps

            if (n->opr[2]) gen_expr_discard(n->opr[2]); // Étape

            create_j_inst(l_begin);
            create_label_inst(l_end);
            return;
        }

        default:
            gen_expr_discard(n);
            return;
    }
}

static void gen_param(node_t x) {
    if (!x) return;

    if (x->nature == NODE_LIST) {
        gen_param(x->opr[0]);
        gen_param(x->opr[1]);
        return;
    }

    if (x->nature == NODE_STRINGVAL) {
        /* syscall print_string : v0=4, a0=adresse */
        load_data_addr_into(4, x->offset);     /* $a0 = addr */
        create_addiu_inst(2, get_r0(), 4);      /* $v0 = 4 */
        create_syscall_inst();
        return;
    }

    /* sinon : expression (ident/int/bool/...) => syscall print_int */
    int32_t r = gen_expr(x);
    create_addu_inst(4, r, get_r0());          /* $a0 = r */
    release_reg();
    create_addiu_inst(2, get_r0(), 1);         /* $v0 = 1 */
    create_syscall_inst();
}

static void gen_print(node_t printnode) {
    if (!printnode) return;
    node_t p = printnode->opr[0];
    if (!p) return;

    gen_param(p);
}


static void gen_expr_discard(node_t e) {
    if (!e) return;
    int32_t r = gen_expr(e);
    (void)r;
    release_reg();
}

static int32_t gen_expr(node_t e) {
    if (!e) return get_r0();

    switch (e->nature) {
        case NODE_INTVAL: {
            allocate_reg();
            int32_t r = get_current_reg();
            create_addiu_inst(r, get_r0(), e->value);
            return r;
        }

        case NODE_BOOLVAL: {
            allocate_reg();
            int32_t r = get_current_reg();
            create_addiu_inst(r, get_r0(), e->value ? 1 : 0);
            return r;
        }

        case NODE_IDENT:
            return load_ident(e);

        case NODE_AFFECT: {
            int32_t rval = gen_expr(e->opr[1]);
            store_ident(e->opr[0], rval);
            return rval;
        }

        case NODE_UMINUS: {
            int32_t r = gen_expr(e->opr[0]);
            create_subu_inst(r, get_r0(), r);
            return r;
        }

        case NODE_NOT: {
            int32_t r = gen_expr(e->opr[0]);
            create_xori_inst(r, r, 1);
            return r;
        }

        case NODE_BNOT: {
            int32_t r = gen_expr(e->opr[0]);
            create_nor_inst(r, r, get_r0());
            return r;
        }

        default:
            break;
    }

    node_t a = e->opr[0];
    node_t b = e->opr[1];

    int32_t ra = gen_expr(a);

    bool spilled = false;
    if (!reg_available()) {
        push_temporary(ra);
        release_reg();
        spilled = true;
    }

    int32_t rb = gen_expr(b);

    if (spilled) {
        allocate_reg();
        int32_t rrest = get_current_reg();
        pop_temporary(rrest);
        ra = rrest;
    }

    switch (e->nature) {
        case NODE_PLUS:
            create_addu_inst(ra, ra, rb);
            break;
        case NODE_MINUS:
            create_subu_inst(ra, ra, rb);
            break;
        case NODE_MUL:
            create_mult_inst(ra, rb);
            create_mflo_inst(ra);
            break;
        case NODE_DIV:
            create_teq_inst(rb, get_r0()); // Génère une exception si rb == 0
            create_div_inst(ra, rb);
            create_mflo_inst(ra);
            break;
        case NODE_MOD:
            create_teq_inst(rb, get_r0()); // Génère une exception si rb == 0
            create_div_inst(ra, rb);
            create_mfhi_inst(ra);
            break;

        case NODE_LT:
            create_slt_inst(ra, ra, rb);
            break;
        case NODE_GT:
            create_slt_inst(ra, rb, ra);
            break;
        case NODE_LE:
            create_slt_inst(ra, rb, ra);
            create_xori_inst(ra, ra, 1);
            break;
        case NODE_GE:
            create_slt_inst(ra, ra, rb);
            create_xori_inst(ra, ra, 1);
            break;

        case NODE_EQ: {
            create_xor_inst(ra, ra, rb);
            create_sltiu_inst(ra, ra, 1);
            break;
        }
        case NODE_NE: {
            create_xor_inst(ra, ra, rb);
            create_sltu_inst(ra, get_r0(), ra);
            break;
        }

        case NODE_AND:
            create_and_inst(ra, ra, rb);
            break;
        case NODE_OR:
            create_or_inst(ra, ra, rb);
            break;

        case NODE_BAND:
            create_and_inst(ra, ra, rb);
            break;
        case NODE_BOR:
            create_or_inst(ra, ra, rb);
            break;
        case NODE_BXOR:
            create_xor_inst(ra, ra, rb);
            break;

        case NODE_SLL:
            create_sllv_inst(ra, ra, rb);
            break;
        case NODE_SRA:
            create_srav_inst(ra, ra, rb);
            break;
        case NODE_SRL:
            create_srlv_inst(ra, ra, rb);
            break;

        default:
            break;
    }

    release_reg();
    return ra;
}

static int32_t load_ident(node_t ident_use_or_decl) {
    allocate_reg();
    int32_t r = get_current_reg();

    if (ident_use_or_decl->global_decl) {
        load_data_base_into(r);
        create_lw_inst(r, ident_use_or_decl->offset, r);
    } else {
        create_lw_inst(r, ident_use_or_decl->offset, get_stack_reg());
    }
    return r;
}

static void store_ident(node_t ident_use_or_decl, int32_t reg_val) {
    if (!ident_use_or_decl) return;

    if (!ident_use_or_decl->global_decl) {
        create_sw_inst(reg_val, ident_use_or_decl->offset, get_stack_reg());
        return;
    }

    if (reg_available()) {
        allocate_reg();
        int32_t raddr = get_current_reg();
        load_data_addr_into(raddr, ident_use_or_decl->offset);
        create_sw_inst(reg_val, 0, raddr);
        release_reg();
    } else {
        push_temporary(reg_val);
        release_reg();

        allocate_reg();
        int32_t raddr = get_current_reg();
        load_data_addr_into(raddr, ident_use_or_decl->offset);

        allocate_reg();
        int32_t rval2 = get_current_reg();
        pop_temporary(rval2);

        create_sw_inst(rval2, 0, raddr);

        release_reg();
        release_reg();
    }
}

static void load_data_base_into(int32_t reg) {
    int32_t base = get_data_sec_start_addr();
    int32_t hi = (base >> 16) & 0xFFFF;
    int32_t lo = base & 0xFFFF;
    create_lui_inst(reg, hi);
    create_ori_inst(reg, reg, lo);
}

static void load_data_addr_into(int32_t reg, int32_t offset) {
    load_data_base_into(reg);
    create_addiu_inst(reg, reg, offset);
}
