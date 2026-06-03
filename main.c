#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

static inline int32_t sign_extend(uint32_t val, int bits){
    uint32_t m = 1u << (bits - 1);
    return (int32_t)((val ^ m) - m);
}

static inline uint32_t get_bits(uint32_t v, int hi, int lo){
    return (v >> lo) & ((1u << (hi - lo + 1)) - 1);
}

/* ===== Memoria ===== */

#define TAMANHO_TOTAL 0xA0000u /* 640 KB */

typedef struct {
    uint32_t memoria_dados[TAMANHO_TOTAL];
} Memoria;

void Memoria_init(Memoria* self) {
    for (uint32_t i = 0; i < TAMANHO_TOTAL; i++)
        self->memoria_dados[i] = 0;
}

void Memoria_escrever32(Memoria* self, uint32_t endereco, uint32_t valor){
    self->memoria_dados[endereco / 4] = valor;
}

uint32_t Memoria_ler32(Memoria* self, uint32_t endereco){
    return self->memoria_dados[endereco / 4];
}

void Memoria_mostrar_memoria_info(Memoria* self){
    (void)self;
    printf("\n==================== MEMÓRIA ====================\n");
    printf("Tamanho total: 640 KB\n");
    printf("Faixas de endereços:\n");
    printf(" - RAM:   0x00000  até 0x7FFFF\n");
    printf(" - VRAM:  0x80000  até 0x8FFFF\n");
    printf(" - I/O:   0x9FC00  até 0x9FFFF\n");
    printf("=================================================\n\n");
}

/* ===== Barramento ===== */

/* Sinais de controle */
enum Controle {
    IDLE  = 0x00,
    READ  = 0x01,   /* bit 0: leitura */
    WRITE = 0x02,   /* bit 1: escrita */
    IO    = 0x04    /* bit 2: operação de I/O */
};

typedef struct {
    /* Três barramentos separados */
    uint32_t barramento_dados;      /* 32 bits - transporta dados */
    uint32_t barramento_enderecos;  /* 32 bits - transporta endereços */
    uint8_t  barramento_controle;   /* bits de controle (READ, WRITE, etc) */

    Memoria* memoria;
} Barramento;

void Barramento_init(Barramento* self, Memoria* mem) {
    self->memoria              = mem;
    self->barramento_dados     = 0;
    self->barramento_enderecos = 0;
    self->barramento_controle  = IDLE;
    printf("Barramento inicializado:\n");
    printf(" - Barramento de Dados: 32 bits\n");
    printf(" - Barramento de Endereços: 32 bits\n");
    printf(" - Barramento de Controle: READ, WRITE, IO\n");
}

/* Operação de leitura através do barramento */
uint32_t Barramento_ler(Barramento* self, uint32_t endereco) {
    /* 1. Coloca endereço no barramento de endereços */
    self->barramento_enderecos = endereco;
    /* 2. Ativa sinal de controle para leitura */
    self->barramento_controle = READ;
    /* 3. Memória coloca dados no barramento de dados */
    self->barramento_dados = Memoria_ler32(self->memoria, endereco);
    /* 4. Retorna o dado lido */
    uint32_t dado = self->barramento_dados;
    /* 5. Limpa barramento de controle */
    self->barramento_controle = IDLE;
    return dado;
}

/* Operação de escrita através do barramento */
void Barramento_escrever(Barramento* self, uint32_t endereco, uint32_t valor) {
    /* 1. Coloca endereço no barramento de endereços */
    self->barramento_enderecos = endereco;
    /* 2. Coloca dado no barramento de dados */
    self->barramento_dados = valor;
    /* 3. Ativa sinal de controle para escrita */
    self->barramento_controle = WRITE;
    /* 4. Memória recebe o dado do barramento */
    Memoria_escrever32(self->memoria, endereco, valor);
    /* 5. Limpa barramento de controle */
    self->barramento_controle = IDLE;
}

/* Métodos para debug e visualização */
void Barramento_mostrar_estado(Barramento* self) {
    printf("\n========== ESTADO DO BARRAMENTO ==========\n");
    printf("Barramento de Endereços: 0x%08X\n", self->barramento_enderecos);
    printf("Barramento de Dados:     0x%08X\n", self->barramento_dados);
    printf("Barramento de Controle:  ");

    if (self->barramento_controle == IDLE) printf("IDLE");
    else {
        if (self->barramento_controle & READ)  printf("READ ");
        if (self->barramento_controle & WRITE) printf("WRITE ");
        if (self->barramento_controle & IO)    printf("IO ");
    }
    printf("\n==========================================\n\n");
}

/* Getters para inspeção */
uint32_t Barramento_get_dados(const Barramento* self)    { return self->barramento_dados; }
uint32_t Barramento_get_endereco(const Barramento* self) { return self->barramento_enderecos; }
uint8_t  Barramento_get_controle(const Barramento* self) { return self->barramento_controle; }

/* ===== DispositivoES ===== */

#define VRAM_INICIO  0x80000u
#define VRAM_FIM     0x8FFFFu
#define VRAM_TAMANHO ((VRAM_FIM - VRAM_INICIO + 1) / 4) /* em words */

typedef struct {
    Memoria* memoria;
} DispositivoES;

void DispositivoES_init(DispositivoES* self, Memoria* mem) {
    self->memoria = mem;
}

/* Exibe o conteúdo da VRAM como caracteres ASCII */
void DispositivoES_exibir_vram(DispositivoES* self) {
    printf("\n╔════════════════════════════════════════════════════════╗\n");
    printf("║             SAÍDA DE VÍDEO (VRAM - E/S)                ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("Endereço 0x80000 - 0x8FFFF:\n");
    printf("┌────────────────────────────────────────────────────────┐\n│ ");

    int caracteres_linha = 0;
    bool tem_conteudo = false;

    for (uint32_t addr = VRAM_INICIO; addr <= VRAM_FIM; addr += 4) {
        uint32_t word = Memoria_ler32(self->memoria, addr);

        /* Cada word contém 4 bytes (caracteres) */
        for (int i = 0; i < 4; i++) {
            uint8_t byte = (word >> (i * 8)) & 0xFF;

            if (byte != 0) {
                tem_conteudo = true;

                /* Imprime caractere ASCII ou '.' se não imprimível */
                if (byte >= 32 && byte <= 126) {
                    printf("%c", (char)byte);
                } else if (byte == 10) { /* newline */
                    printf("\n│ ");
                    caracteres_linha = 0;
                    continue;
                } else {
                    printf(".");
                }

                caracteres_linha++;

                /* Quebra linha a cada 54 caracteres */
                if (caracteres_linha >= 54) {
                    printf("\n│ ");
                    caracteres_linha = 0;
                }
            }
        }
    }

    if (!tem_conteudo) {
        printf("[VRAM vazia - sem conteúdo para exibir]");
    }

    printf("\n└────────────────────────────────────────────────────────┘\n\n");
}

bool DispositivoES_eh_endereco_vram(DispositivoES* self, uint32_t endereco) {
    (void)self;
    return (endereco >= VRAM_INICIO && endereco <= VRAM_FIM);
}

/* ===== Temporização ===== */
typedef struct {
    uint32_t pc_clk_q;
    uint32_t mem_instrucao;
    uint32_t banco_regs;
    uint32_t mux;
    uint32_t ula;
    uint32_t mem_dados;
    uint32_t mux_final;
    uint32_t setup_wb;
} LatenciasReferencia;

static const LatenciasReferencia LATENCIAS_PADRAO = {
    .pc_clk_q      = 30,
    .mem_instrucao = 250,
    .banco_regs    = 150,
    .mux           = 25,
    .ula           = 200,
    .mem_dados     = 250,
    .mux_final     = 25,
    .setup_wb      = 20
};

/* ===== REGISTRADORES DE PIPELINE ===== */

typedef struct {
    uint32_t pc;
    uint32_t instrucao;
    bool valido;
} Reg_Busca_Decodificacao;

typedef struct {
    uint32_t pc;
    uint32_t instrucao;
    uint32_t opcode;
    uint32_t funct3;
    uint32_t funct7;
    uint32_t rd;
    uint32_t rs1_id;
    uint32_t rs2_id;
    int32_t  val_rs1;
    int32_t  val_rs2;
    int32_t  imm;
    /* sinais de controle */
    bool escrever_reg;
    bool ler_mem;
    bool escrever_mem;
    bool eh_branch;
    bool eh_jal;
    bool valido;
} Reg_Decodificacao_Execucao;

typedef struct {
    uint32_t rd;
    uint32_t instrucao;
    int32_t  resultado_ula;
    int32_t  dado_escrita;   /* dado de rs2 para SW */
    uint32_t endereco;
    /* sinais de controle repassados */
    bool escrever_reg;
    bool ler_mem;
    bool escrever_mem;
    bool valido;
} Reg_Execucao_Memoria;

typedef struct {
    uint32_t rd;
    uint32_t instrucao;
    int32_t  dado_final;
    bool escrever_reg;
    bool valido;
} Reg_Memoria_Retorno;

/* ===== CPU COM PIPELINE ===== */

typedef struct {
    int32_t  regs[32];
    uint32_t pc;
    Barramento* barramento;
    uint32_t contador_instrucoes;
    bool parada_detectada;

    Reg_Busca_Decodificacao    reg_bd;
    Reg_Decodificacao_Execucao reg_de;
    Reg_Execucao_Memoria       reg_em;
    Reg_Memoria_Retorno        reg_mr;

    double tempo_total_ps; /* Variável de contabilização do trabalho físico */
} CPU;

void CPU_init(CPU* self, Barramento* bus) {
    memset(self->regs, 0, sizeof(self->regs));
    self->pc                  = 0;
    self->barramento          = bus;
    self->contador_instrucoes = 0;
    self->parada_detectada    = false;
    memset(&self->reg_bd, 0, sizeof(self->reg_bd));
    memset(&self->reg_de, 0, sizeof(self->reg_de));
    memset(&self->reg_em, 0, sizeof(self->reg_em));
    memset(&self->reg_mr, 0, sizeof(self->reg_mr));
    self->regs[0]        = 0;
    self->tempo_total_ps = 0.0;
}

void CPU_buscar(CPU* self) {
    uint32_t instrucao = Barramento_ler(self->barramento, self->pc);
    self->reg_bd.pc        = self->pc;
    self->reg_bd.instrucao = instrucao;
    self->reg_bd.valido    = true;
    self->pc += 4;
}

void CPU_decodificar(CPU* self) {
    if (!self->reg_bd.valido) {
        self->reg_de.valido = false;
        return;
    }

    uint32_t inst = self->reg_bd.instrucao;

    /* Detectar loop infinito (JAL x0, 0) */
    if (inst == 0x0000006F) {
        self->parada_detectada = true;
    }

    self->reg_de.pc        = self->reg_bd.pc;
    self->reg_de.instrucao = inst;
    self->reg_de.opcode = inst & 0x7F;
    self->reg_de.funct3 = get_bits(inst, 14, 12);
    self->reg_de.funct7 = get_bits(inst, 31, 25);
    self->reg_de.rd     = get_bits(inst, 11, 7);
    uint32_t rs1        = get_bits(inst, 19, 15);
    uint32_t rs2        = get_bits(inst, 24, 20);
    self->reg_de.rs1_id  = rs1;
    self->reg_de.rs2_id  = rs2;
    self->reg_de.val_rs1 = self->regs[rs1];
    self->reg_de.val_rs2 = self->regs[rs2];

    /* sinais de controle zerados */
    self->reg_de.escrever_reg = false;
    self->reg_de.ler_mem      = false;
    self->reg_de.escrever_mem = false;
    self->reg_de.eh_branch    = false;
    self->reg_de.eh_jal       = false;

    switch (self->reg_de.opcode) {
    /* ---------------- R ---------------- */
    case 0x33: /* R-type */
        self->reg_de.imm = 0;
        self->reg_de.escrever_reg = true;
        break;
    /* ---------------- I ---------------- */
    case 0x13: /* I-type ALU */
        self->reg_de.imm = sign_extend(get_bits(inst, 31, 20), 12);
        self->reg_de.escrever_reg = true;
        break;
    /* ---------------- LOAD (LW) ---------------- */
    case 0x03: /* LOAD */
        self->reg_de.imm = sign_extend(get_bits(inst, 31, 20), 12);
        self->reg_de.escrever_reg = true;
        self->reg_de.ler_mem = true;
        break;
    /* ---------------- STORE (SW) ---------------- */
    case 0x23: { /* STORE */
        /* imm[11:5] em [31:25], imm[4:0] em [11:7] */
        uint32_t imm_s = (get_bits(inst, 31, 25) << 5) | get_bits(inst, 11, 7);
        self->reg_de.imm = sign_extend(imm_s, 12);
        self->reg_de.escrever_mem = true;
        break;
    }
    /* ---------------- B ---------------- */
    case 0x63: { /* BRANCH */
        uint32_t imm_b = (get_bits(inst,31,31) << 12)
                       | (get_bits(inst,7,7)   << 11)
                       | (get_bits(inst,30,25) << 5)
                       | (get_bits(inst,11,8)  << 1);
        self->reg_de.imm = sign_extend(imm_b, 13);
        self->reg_de.eh_branch = true;
        break;
    }
    /* ---------------- J ---------------- */
    case 0x6F: { /* JAL */
        uint32_t imm_j = (get_bits(inst,31,31) << 20)
                       | (get_bits(inst,19,12) << 12)
                       | (get_bits(inst,20,20) << 11)
                       | (get_bits(inst,30,21) << 1);
        self->reg_de.imm = sign_extend(imm_j, 21);
        self->reg_de.escrever_reg = true;
        self->reg_de.eh_jal = true;
        break;
    }
    /* ---------------- U ---------------- */
    case 0x37: /* LUI */
        self->reg_de.imm = (int32_t)(get_bits(inst, 31, 12) << 12);
        self->reg_de.escrever_reg = true;
        break;
    case 0x17: /* AUIPC */
        self->reg_de.imm = (int32_t)(get_bits(inst, 31, 12) << 12);
        self->reg_de.escrever_reg = true;
        break;
    default:
        self->reg_de.imm = 0;
        break;
    }

    self->reg_de.valido = true;
}

void CPU_executar_ula(CPU* self) {
    if (!self->reg_de.valido) {
        self->reg_em.valido = false;
        return;
    }

    self->reg_em.rd           = self->reg_de.rd;
    self->reg_em.dado_escrita = self->reg_de.val_rs2;
    self->reg_em.escrever_reg = self->reg_de.escrever_reg;
    self->reg_em.ler_mem      = self->reg_de.ler_mem;
    self->reg_em.escrever_mem = self->reg_de.escrever_mem;
    self->reg_em.resultado_ula = 0;
    self->reg_em.instrucao    = self->reg_de.instrucao;
    self->reg_em.valido = true;

    uint32_t opcode = self->reg_de.opcode;
    uint32_t funct3 = self->reg_de.funct3;
    uint32_t funct7 = self->reg_de.funct7;
    int32_t  v1     = self->reg_de.val_rs1;
    int32_t  v2     = self->reg_de.val_rs2;
    int32_t  imm    = self->reg_de.imm;

    /* Forwarding do estágio MEM */
    if (self->reg_em.escrever_reg && self->reg_em.rd != 0) {
        if (self->reg_em.rd == self->reg_de.rs1_id) v1 = self->reg_em.resultado_ula;
        if (self->reg_em.rd == self->reg_de.rs2_id) v2 = self->reg_em.resultado_ula;
    }

    /* Forwarding do estágio WB */
    if (self->reg_mr.escrever_reg && self->reg_mr.rd != 0) {
        if (self->reg_mr.rd == self->reg_de.rs1_id &&
            !(self->reg_em.escrever_reg && self->reg_em.rd == self->reg_de.rs1_id)) {
            v1 = self->reg_mr.dado_final;
        }
        if (self->reg_mr.rd == self->reg_de.rs2_id &&
            !(self->reg_em.escrever_reg && self->reg_em.rd == self->reg_de.rs2_id)) {
            v2 = self->reg_mr.dado_final;
        }
    }

    switch (opcode) {
    /* ---------------- R ---------------- */
    case 0x33: /* R-type */
        switch (funct3) {
        case 0x0: /* ADD / SUB */
            if (funct7 == 0x00) { self->reg_em.resultado_ula = v1 + v2; printf("ADD x%u = x? + x?\n", self->reg_de.rd); }
            else if (funct7 == 0x20) { self->reg_em.resultado_ula = v1 - v2; printf("SUB x%u = x? - x?\n", self->reg_de.rd); }
            break;
        case 0x1: /* SLL */
            self->reg_em.resultado_ula = (int32_t)((uint32_t)v1 << (v2 & 0x1F)); printf("SLL x%u\n", self->reg_de.rd);
            break;
        case 0x5: /* SRL / SRA */
            if (funct7 == 0x00) { self->reg_em.resultado_ula = (int32_t)((uint32_t)v1 >> (v2 & 0x1F)); printf("SRL x%u\n", self->reg_de.rd); }
            else if (funct7 == 0x20) { self->reg_em.resultado_ula = v1 >> (v2 & 0x1F); printf("SRA x%u\n", self->reg_de.rd); }
            break;
        case 0x6: /* OR */
            self->reg_em.resultado_ula = v1 | v2; printf("OR x%u\n", self->reg_de.rd);
            break;
        case 0x7: /* AND */
            self->reg_em.resultado_ula = v1 & v2; printf("AND x%u\n", self->reg_de.rd);
            break;
        case 0x4: /* XOR */
            self->reg_em.resultado_ula = v1 ^ v2; printf("XOR x%u\n", self->reg_de.rd);
            break;
        case 0x2: /* SLT */
            self->reg_em.resultado_ula = (v1 < v2) ? 1 : 0; printf("SLT x%u\n", self->reg_de.rd);
            break;
        case 0x3: /* SLTU */
            self->reg_em.resultado_ula = ((uint32_t)v1 < (uint32_t)v2) ? 1 : 0; printf("SLTU x%u\n", self->reg_de.rd);
            break;
        default:
            printf("R-type funct3 não implementado: %u\n", funct3);
            break;
        }
        break;

    /* ---------------- I ---------------- */
    case 0x13: /* I-type ALU */
        switch (funct3) {
        case 0x0: /* ADDI */
            self->reg_em.resultado_ula = v1 + imm; printf("ADDI x%u = x? + %d\n", self->reg_de.rd, imm);
            break;
        case 0x6: /* ORI */
            self->reg_em.resultado_ula = v1 | imm; printf("ORI x%u\n", self->reg_de.rd);
            break;
        case 0x7: /* ANDI */
            self->reg_em.resultado_ula = v1 & imm; printf("ANDI x%u\n", self->reg_de.rd);
            break;
        case 0x1: { /* SLLI */
            uint32_t sh = (uint32_t)imm & 0x1F; self->reg_em.resultado_ula = (int32_t)((uint32_t)v1 << sh); printf("SLLI x%u\n", self->reg_de.rd);
            break;
        }
        case 0x5: { /* SRLI / SRAI */
            uint32_t sh = (uint32_t)imm & 0x1F;
            if (funct7 == 0x00) { self->reg_em.resultado_ula = (int32_t)((uint32_t)v1 >> sh); printf("SRLI x%u\n", self->reg_de.rd); }
            else { self->reg_em.resultado_ula = v1 >> sh; printf("SRAI x%u\n", self->reg_de.rd); }
            break;
        }
        default: printf("I-type funct3 não implementado: %u\n", funct3); break;
        }
        break;

    /* ---------------- LOAD (LW) ---------------- */
    case 0x03: /* LOAD — calcula endereço */
        self->reg_em.endereco = (uint32_t)(v1 + imm);
        printf("LW x%u MEM[0x%X]\n", self->reg_de.rd, self->reg_em.endereco);
        break;

    /* ---------------- STORE (SW) ---------------- */
    case 0x23: /* STORE — calcula endereço */
        self->reg_em.endereco = (uint32_t)(v1 + imm);
        printf("SW MEM[0x%X] = x?\n", self->reg_em.endereco);
        break;

    /* ---------------- B ---------------- */
    case 0x63: { /* BRANCH */
        bool take = false;
        switch (funct3) {
        case 0x0: take = (v1 == v2); printf("BEQ\n"); break;
        case 0x1: take = (v1 != v2); printf("BNE\n"); break;
        case 0x4: take = (v1 <  v2); printf("BLT\n"); break;
        case 0x5: take = (v1 >= v2); printf("BGE\n"); break;
        case 0x6: take = ((uint32_t)v1 <  (uint32_t)v2); printf("BLTU\n"); break;
        case 0x7: take = ((uint32_t)v1 >= (uint32_t)v2); printf("BGEU\n"); break;
        default: printf("Branch funct3 desconhecido.\n"); break;
        }
        if (take) {
            self->pc = (uint32_t)((int32_t)self->reg_de.pc + imm);
            printf("Branch taken -> pc = 0x%X\n", self->pc);
            self->reg_em.escrever_reg = false;

            /* FLUSH DO PIPELINE: Invalida as instruções lidas indevidamente */
            self->reg_bd.valido = false;
            self->reg_de.valido = false;
        }
        break;
    }

    /* ---------------- J ---------------- */
    case 0x6F: { /* JAL */
        self->reg_em.resultado_ula = (int32_t)(self->reg_de.pc + 4);
        self->pc = (uint32_t)((int32_t)self->reg_de.pc + imm);
        printf("JAL x%u -> pc = 0x%X\n", self->reg_de.rd, self->pc);

        /* FLUSH DO PIPELINE: Invalida as instruções lidas indevidamente */
        self->reg_bd.valido = false;
        self->reg_de.valido = false;
        break;
    }

    /* ---------------- U ---------------- */
    case 0x37: /* LUI */
        self->reg_em.resultado_ula = imm;
        printf("LUI x%u = 0x%X\n", self->reg_de.rd, (uint32_t)imm);
        break;

    case 0x17: /* AUIPC */
        self->reg_em.resultado_ula = (int32_t)(self->reg_de.pc + (uint32_t)imm);
        printf("AUIPC x%u = pc + 0x%X\n", self->reg_de.rd, (uint32_t)imm);
        break;

    default:
        printf("Opcode não implementado!\n");
        break;
    }
}

void CPU_acessar_memoria(CPU* self) {
    if (!self->reg_em.valido) {
        self->reg_mr.valido = false;
        return;
    }

    self->reg_mr.rd           = self->reg_em.rd;
    self->reg_mr.escrever_reg = self->reg_em.escrever_reg;
    self->reg_mr.instrucao    = self->reg_em.instrucao;
    self->reg_mr.valido       = true;

    if (self->reg_em.ler_mem) {
        /* LW */
        int32_t dado = (int32_t)Barramento_ler(self->barramento, self->reg_em.endereco);
        self->reg_mr.dado_final = dado;
        printf("LW leu MEM[0x%X] = 0x%X\n", self->reg_em.endereco, (uint32_t)dado);
    } else if (self->reg_em.escrever_mem) {
        /* SW */
        Barramento_escrever(self->barramento, self->reg_em.endereco, (uint32_t)self->reg_em.dado_escrita);
        printf("SW escreveu MEM[0x%X] = 0x%X\n", self->reg_em.endereco, (uint32_t)self->reg_em.dado_escrita);
        self->reg_mr.dado_final = self->reg_em.resultado_ula;
    } else {
        self->reg_mr.dado_final = self->reg_em.resultado_ula;
    }
}

void CPU_escrever_retorno(CPU* self) {
    if (!self->reg_mr.valido) return;

    if (self->reg_mr.escrever_reg && self->reg_mr.rd != 0) {
        self->regs[self->reg_mr.rd] = self->reg_mr.dado_final;
    }
    self->regs[0] = 0;
    self->contador_instrucoes++; /* Apenas incrementa quando a instrução termina o pipeline */
}

void CPU_contabilizar_tempo(CPU* self, uint32_t inst) {
    uint32_t opcode = inst & 0x7F;

    /* Tempo base para todas as instruções (Fetch + Decode) */
    uint32_t t = LATENCIAS_PADRAO.pc_clk_q +
                 LATENCIAS_PADRAO.mem_instrucao +
                 LATENCIAS_PADRAO.banco_regs;

    switch (opcode) {
        case 0x33: case 0x13: /* ALU R-type e I-type */
            t += LATENCIAS_PADRAO.mux + LATENCIAS_PADRAO.ula +
                 LATENCIAS_PADRAO.mux_final + LATENCIAS_PADRAO.setup_wb;
            break;
        case 0x03: /* LOAD */
            t += LATENCIAS_PADRAO.mux + LATENCIAS_PADRAO.ula +
                 LATENCIAS_PADRAO.mem_dados + LATENCIAS_PADRAO.mux_final +
                 LATENCIAS_PADRAO.setup_wb;
            break;
        case 0x23: /* STORE */
            t += LATENCIAS_PADRAO.mux + LATENCIAS_PADRAO.ula +
                 LATENCIAS_PADRAO.mem_dados;
            break;
        case 0x63: /* BRANCH */
            t += LATENCIAS_PADRAO.mux + LATENCIAS_PADRAO.ula;
            break;
        case 0x6F: case 0x37: case 0x17: /* JUMPS, LUI, AUIPC */
            t += LATENCIAS_PADRAO.ula + LATENCIAS_PADRAO.mux_final +
                 LATENCIAS_PADRAO.setup_wb;
            break;
    }

    self->tempo_total_ps += (double)t;
}

void carregar_programa_completo(Barramento* barramento) {
    printf("\n========== CARREGANDO PROGRAMA DE TESTE COMPLETO ==========\n");
    printf("Programa: Cálculo de Fatorial e operações diversas\n");
    printf("Funcionalidades demonstradas:\n");
    printf(" - Instruções aritméticas (ADD, SUB, ADDI)\n");
    printf(" - Instruções lógicas (AND, OR, XOR)\n");
    printf(" - Instruções de shift (SLL, SRL)\n");
    printf(" - Branches (BEQ, BNE, BLT)\n");
    printf(" - Jumps (JAL)\n");
    printf(" - Load/Store (LW, SW)\n");
    printf(" - Acesso à VRAM para E/S\n");
    printf("===========================================================\n\n");

    uint32_t addr = 0x00000;

    /* x10 (a0) = 5 (número para calcular fatorial) */
    Barramento_escrever(barramento, addr, 0x00500513); addr += 4; /* ADDI x10, x0, 5 */

    /* x11 (a1) = 1 (resultado acumulado) */
    Barramento_escrever(barramento, addr, 0x00100593); addr += 4; /* ADDI x11, x0, 1 */

    /* x12 (a2) = 1 (contador i) */
    Barramento_escrever(barramento, addr, 0x00100613); addr += 4; /* ADDI x12, x0, 1 */

    /* loop_inicio: (endereço 0x0000C) */
    /* BLT x10, x12, loop_fim (se n < i, sai do loop) */
    /* offset = 0x14 (5 instruções * 4 = 20 bytes) */
    Barramento_escrever(barramento, addr, 0x00C54863); addr += 4; /* BLT x10, x12, +16 */

    /* x11 = x11 * x12 (resultado *= i) */
    /* Multiplicação usando shifts e adds (x11 * x12) */
    Barramento_escrever(barramento, addr, 0x00C58633); addr += 4; /* ADD x12, x11, x12 */

    /* Simplificando: apenas ADD para demonstração */
    Barramento_escrever(barramento, addr, 0x00C585B3); addr += 4; /* ADD x11, x11, x12 */

    /* x12 = x12 + 1 (i++) */
    Barramento_escrever(barramento, addr, 0x00160613); addr += 4; /* ADDI x12, x12, 1 */

    /* JAL x0, loop_inicio (volta para o loop) */
    /* offset = -16 (0xFFFFFFF0) */
    Barramento_escrever(barramento, addr, 0xFF1FF06F); addr += 4; /* JAL x0, -16 */

    /* loop_fim: (endereço 0x00020) */
    /* x13 = endereço base da VRAM (0x80000) */
    Barramento_escrever(barramento, addr, 0x000806B7); addr += 4; /* LUI x13, 0x80 */

    /* x14 = 0x46 ('F') */
    Barramento_escrever(barramento, addr, 0x04600713); addr += 4; /* ADDI x14, x0, 0x46 */
    Barramento_escrever(barramento, addr, 0x00E6A023); addr += 4; /* SW x14, 0(x13) - escreve 'F' */

    /* x14 = 0x41 ('A') */
    Barramento_escrever(barramento, addr, 0x04100713); addr += 4; /* ADDI x14, x0, 0x41 */
    Barramento_escrever(barramento, addr, 0x00E6A223); addr += 4; /* SW x14, 4(x13) - escreve 'A' */

    /* x14 = 0x54 ('T') */
    Barramento_escrever(barramento, addr, 0x05400713); addr += 4; /* ADDI x14, x0, 0x54 */
    Barramento_escrever(barramento, addr, 0x00E6A423); addr += 4; /* SW x14, 8(x13) - escreve 'T' */

    /* x14 = 0x3D ('=') */
    Barramento_escrever(barramento, addr, 0x03D00713); addr += 4; /* ADDI x14, x0, 0x3D */
    Barramento_escrever(barramento, addr, 0x00E6A623); addr += 4; /* SW x14, 12(x13) - escreve '=' */

    /* x14 = 0x20 (' ') */
    Barramento_escrever(barramento, addr, 0x02000713); addr += 4; /* ADDI x14, x0, 0x20 */
    Barramento_escrever(barramento, addr, 0x00E6A823); addr += 4; /* SW x14, 16(x13) - escreve ' ' */

    /* escrever resultado na vram */
    /* x15 = resultado */
    Barramento_escrever(barramento, addr, 0x00058793); addr += 4; /* ADDI x15, x11, 0 */

    /* Escrever '1' (0x31) */
    Barramento_escrever(barramento, addr, 0x03100713); addr += 4; /* ADDI x14, x0, 0x31 */
    Barramento_escrever(barramento, addr, 0x00E6AA23); addr += 4; /* SW x14, 20(x13) */

    /* Escrever '2' (0x32) */
    Barramento_escrever(barramento, addr, 0x03200713); addr += 4; /* ADDI x14, x0, 0x32 */
    Barramento_escrever(barramento, addr, 0x00E6AC23); addr += 4; /* SW x14, 24(x13) */

    /* Escrever '0' (0x30) */
    Barramento_escrever(barramento, addr, 0x03000713); addr += 4; /* ADDI x14, x0, 0x30 */
    Barramento_escrever(barramento, addr, 0x00E6AE23); addr += 4; /* SW x14, 28(x13) */

    /* ========== DEMONSTRAÇÃO DE OUTRAS INSTRUÇÕES ========== */
    /* Operações lógicas */
    Barramento_escrever(barramento, addr, 0x00F767B3); addr += 4; /* OR x15, x14, x15 */
    Barramento_escrever(barramento, addr, 0x00F77833); addr += 4; /* AND x16, x14, x15 */
    Barramento_escrever(barramento, addr, 0x00F748B3); addr += 4; /* XOR x17, x14, x15 */

    /* Shifts */
    Barramento_escrever(barramento, addr, 0x00171913); addr += 4; /* SLLI x18, x14, 1 */
    Barramento_escrever(barramento, addr, 0x00175993); addr += 4; /* SRLI x19, x14, 1 */

    /* Comparações */
    Barramento_escrever(barramento, addr, 0x00F72A33); addr += 4; /* SLT x20, x14, x15 */

    /* Loop infinito para parar execução */
    Barramento_escrever(barramento, addr, 0x0000006F); addr += 4; /* JAL x0, 0 (loop infinito) */

    printf("Programa carregado: %u instruções\n", (addr/4));
    printf("Tamanho: %u bytes\n\n", addr);
}

/* ======================================================= */
/* MAIN — Memória + CPU original                           */
/* ======================================================= */
int main(){
    Memoria memoria;
    Barramento barramento;
    CPU cpu;
    DispositivoES dispositivo_es;

    Memoria_init(&memoria);
    Barramento_init(&barramento, &memoria);
    CPU_init(&cpu, &barramento);
    DispositivoES_init(&dispositivo_es, &memoria);

    const int INSTRUCOES_POR_ES = 10;
    const int MAX_INSTRUCOES = 100;

    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║        SIMULADOR DE COMPUTADOR RISC-V 32-bit              ║\n");
    printf("║                  Arquitetura RV32I                        ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");

    Memoria_mostrar_memoria_info(&memoria);

    printf("======================= CPU ============================\n");
    printf("Registradores: 32 x 32-bit (x0-x31)\n");
    printf("PC inicial: 0x00000000\n");
    printf("Instruções implementadas:\n");
    printf(" • Tipo R: ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU\n");
    printf(" • Tipo I: ADDI, ANDI, ORI, SLLI, SRLI, SRAI, LW\n");
    printf(" • Tipo S: SW\n");
    printf(" • Tipo B: BEQ, BNE, BLT, BGE, BLTU, BGEU\n");
    printf(" • Tipo U: LUI, AUIPC\n");
    printf(" • Tipo J: JAL\n");
    printf("========================================================\n\n");

    /* Carregar programa de teste */
    carregar_programa_completo(&barramento);

    printf("=============== INICIANDO EXECUÇÃO ===============\n");
    printf("Configuração de E/S: Exibir VRAM a cada %d instruções\n", INSTRUCOES_POR_ES);
    printf("Limite de segurança: %d ciclos de clock\n\n", MAX_INSTRUCOES);

    int instrucoes_executadas = 0; 

    /* Loop de execução */
    while (!cpu.parada_detectada && cpu.contador_instrucoes < MAX_INSTRUCOES) {

        printf("\n─────────────────────────────────────────────────────\n");
        printf("Ciclo de Clock #%d\n", (instrucoes_executadas + 1));

        /* Chamada das fases na ordem inversa para simular paralelismo */
        CPU_escrever_retorno(&cpu);

        if (cpu.reg_mr.valido && cpu.reg_mr.instrucao != 0x00000000) {
            CPU_contabilizar_tempo(&cpu, cpu.reg_mr.instrucao);
        }

        CPU_acessar_memoria(&cpu);
        CPU_executar_ula(&cpu);
        CPU_decodificar(&cpu);
        CPU_buscar(&cpu);

        instrucoes_executadas++;

        /* E/S PROGRAMADA: Exibe VRAM periodicamente */
        /* Exibe a VRAM baseado na quantidade de instruções que completaram o pipeline */
        if (cpu.contador_instrucoes > 0 && (cpu.contador_instrucoes % INSTRUCOES_POR_ES == 0)) {
            printf("\n>>> INTERRUPÇÃO DE E/S (a cada %d instruções retiradas) <<<\n", INSTRUCOES_POR_ES);
            DispositivoES_exibir_vram(&dispositivo_es);
        }
    }

    if(cpu.parada_detectada) {
        printf("\n[STOP] Fim da execução detectado.\n");
    }

    /* Exibição final */
    printf("\n\n");
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║                   EXECUÇÃO FINALIZADA                     ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");

    printf("============= ESTADO FINAL DA VRAM =============\n");
    DispositivoES_exibir_vram(&dispositivo_es);

    printf("================ ESTADO FINAL DA CPU ================\n");
    printf("Registradores (apenas não-zero):\n");
    for (int i = 0; i < 32; i++) {
        if (cpu.regs[i] != 0) {
            printf("  x%2d (", i);

            /* Nome ABI */
            if      (i == 10) printf("a0");
            else if (i == 11) printf("a1");
            else if (i == 12) printf("a2");
            else if (i == 13) printf("a3");
            else if (i == 14) printf("a4");
            else if (i == 15) printf("a5");
            else              printf("  ");

            printf("): %10d (0x%08X)\n", cpu.regs[i], (uint32_t)cpu.regs[i]);
        }
    }

    printf("\nPC final: 0x%08X\n", cpu.pc);

    /* O relógio do Pipeline é ditado pelo gargalo (Estágio de Memória) + Overhead */
    double periodo_clock_pipeline = (double)(LATENCIAS_PADRAO.pc_clk_q +
                                             LATENCIAS_PADRAO.mem_instrucao +
                                             LATENCIAS_PADRAO.setup_wb);

    double tempo_clock_pipeline = (double)instrucoes_executadas * periodo_clock_pipeline;

    /* Agora exibe os totais corretos separados */
    printf("Total de ciclos de clock gastos: %d\n", instrucoes_executadas);
    printf("Total de instruções concluídas: %u\n", cpu.contador_instrucoes);
    printf("====================================================\n\n");

    /* Estatísticas */
    printf("================ ESTATÍSTICAS DO SISTEMA ================\n");
    printf("Tempo por ciclo de clock (Tc):   %.0f ps (Gargalo Memória)\n", periodo_clock_pipeline);
    printf("Trabalho Físico (Serial):        %.0f ps (%.3f ns)\n", cpu.tempo_total_ps, cpu.tempo_total_ps / 1000.0);
    printf("Tempo Real Simulado (Pipeline):  %.0f ps (%.3f ns)\n", tempo_clock_pipeline, tempo_clock_pipeline / 1000.0);
    printf("---------------------------------------------------------\n");
    printf("Operações de memória realizadas via barramento\n");
    printf("VRAM utilizada para saída de caracteres ASCII\n");
    printf("E/S programada com polling a cada %d instruções\n", INSTRUCOES_POR_ES);
    printf("=========================================================\n");

    return 0;
}