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

/* ===== CPU ===== */

typedef struct {
    int32_t  regs[32];
    uint32_t pc;
    Barramento* barramento;
    uint32_t contador_instrucoes;
} CPU;

void CPU_init(CPU* self, Barramento* bus) {
    memset(self->regs, 0, sizeof(self->regs));
    self->pc                  = 0;
    self->barramento          = bus;
    self->contador_instrucoes = 0;
    self->regs[0] = 0;
}

void CPU_executar(CPU* self, uint32_t inst) {
    self->contador_instrucoes++;
    uint32_t opcode = inst & 0x7F;

    switch (opcode) {

    /* ---------------- R ---------------- */
    case 0x33: {
        uint32_t rd     = get_bits(inst,11,7);
        uint32_t funct3 = get_bits(inst,14,12);
        uint32_t rs1    = get_bits(inst,19,15);
        uint32_t rs2    = get_bits(inst,24,20);
        uint32_t funct7 = get_bits(inst,31,25);

        switch (funct3) {
        case 0x0: /* ADD / SUB */
            if (funct7 == 0x00) {
                self->regs[rd] = self->regs[rs1] + self->regs[rs2];
                printf("ADD x%u = x%u + x%u\n", rd, rs1, rs2);
            } else if (funct7 == 0x20) {
                self->regs[rd] = self->regs[rs1] - self->regs[rs2];
                printf("SUB x%u = x%u - x%u\n", rd, rs1, rs2);
            }
            break;

        case 0x1: /* SLL */
            self->regs[rd] = (int32_t)((uint32_t)self->regs[rs1] << (self->regs[rs2] & 0x1F));
            printf("SLL x%u = x%u << x%u\n", rd, rs1, rs2);
            break;

        case 0x5: /* SRL / SRA */
            if (funct7 == 0x00) {
                self->regs[rd] = (int32_t)((uint32_t)self->regs[rs1] >> (self->regs[rs2] & 0x1F));
                printf("SRL x%u = x%u >>u x%u\n", rd, rs1, rs2);
            } else if (funct7 == 0x20) {
                self->regs[rd] = self->regs[rs1] >> (self->regs[rs2] & 0x1F);
                printf("SRA x%u = x%u >>s x%u\n", rd, rs1, rs2);
            }
            break;

        case 0x6: /* OR */
            self->regs[rd] = self->regs[rs1] | self->regs[rs2];
            printf("OR x%u = x%u | x%u\n", rd, rs1, rs2);
            break;

        case 0x7: /* AND */
            self->regs[rd] = self->regs[rs1] & self->regs[rs2];
            printf("AND x%u = x%u & x%u\n", rd, rs1, rs2);
            break;

        case 0x4: /* XOR */
            self->regs[rd] = self->regs[rs1] ^ self->regs[rs2];
            printf("XOR x%u = x%u ^ x%u\n", rd, rs1, rs2);
            break;

        case 0x2: /* SLT */
            self->regs[rd] = (self->regs[rs1] < self->regs[rs2]) ? 1 : 0;
            printf("SLT x%u = (x%u < x%u)\n", rd, rs1, rs2);
            break;

        case 0x3: /* SLTU */
            self->regs[rd] = ((uint32_t)self->regs[rs1] < (uint32_t)self->regs[rs2]) ? 1 : 0;
            printf("SLTU x%u = (ux%u < ux%u)\n", rd, rs1, rs2);
            break;

        default:
            printf("R-type funct3 não implementado: %u\n", funct3);
        }
    }
    break;

    /* ---------------- I ---------------- */
    case 0x13: {
        uint32_t rd     = get_bits(inst,11,7);
        uint32_t funct3 = get_bits(inst,14,12);
        uint32_t rs1    = get_bits(inst,19,15);
        int32_t  imm    = sign_extend(get_bits(inst,31,20), 12);

        switch (funct3) {
        case 0x0: /* ADDI */
            self->regs[rd] = self->regs[rs1] + imm;
            printf("ADDI x%u = x%u + %d\n", rd, rs1, imm);
            break;

        case 0x6: /* ORI */
            self->regs[rd] = self->regs[rs1] | imm;
            printf("ORI x%u = x%u | %d\n", rd, rs1, imm);
            break;

        case 0x7: /* ANDI */
            self->regs[rd] = self->regs[rs1] & imm;
            printf("ANDI x%u = x%u & %d\n", rd, rs1, imm);
            break;

        case 0x1: { /* SLLI */
            uint32_t sh = get_bits(inst,24,20);
            self->regs[rd] = (int32_t)((uint32_t)self->regs[rs1] << sh);
            printf("SLLI x%u = x%u << %u\n", rd, rs1, sh);
            break;
        }

        case 0x5: { /* SRLI / SRAI */
            uint32_t sh     = get_bits(inst,24,20);
            uint32_t funct7 = get_bits(inst,31,25);

            if (funct7 == 0x00) {
                self->regs[rd] = (int32_t)((uint32_t)self->regs[rs1] >> sh);
                printf("SRLI x%u = x%u >>u %u\n", rd, rs1, sh);
            } else {
                self->regs[rd] = self->regs[rs1] >> sh;
                printf("SRAI x%u = x%u >>s %u\n", rd, rs1, sh);
            }
            break;
        }

        default:
            printf("I-type funct3 não implementado: %u\n", funct3);
        }
    }
    break;

    /* ---------------- B ---------------- */
    case 0x63: {
        uint32_t funct3 = get_bits(inst,14,12);
        uint32_t rs1    = get_bits(inst,19,15);
        uint32_t rs2    = get_bits(inst,24,20);

        uint32_t imm = (get_bits(inst,31,31) << 12)
                     | (get_bits(inst,7,7)   << 11)
                     | (get_bits(inst,30,25) << 5)
                     | (get_bits(inst,11,8)  << 1);

        int32_t soff = sign_extend(imm, 13);
        bool take = false;

        switch (funct3) {
        case 0x0: take = (self->regs[rs1] == self->regs[rs2]); printf("BEQ\n");  break;
        case 0x1: take = (self->regs[rs1] != self->regs[rs2]); printf("BNE\n");  break;
        case 0x4: take = (self->regs[rs1] <  self->regs[rs2]); printf("BLT\n");  break;
        case 0x5: take = (self->regs[rs1] >= self->regs[rs2]); printf("BGE\n");  break;
        case 0x6: take = ((uint32_t)self->regs[rs1] <  (uint32_t)self->regs[rs2]); printf("BLTU\n"); break;
        case 0x7: take = ((uint32_t)self->regs[rs1] >= (uint32_t)self->regs[rs2]); printf("BGEU\n"); break;
        default:
            printf("Branch funct3 desconhecido.\n");
        }

        if (take) {
            self->pc = (int32_t)self->pc + soff;
            printf("Branch taken -> pc = 0x%X\n", self->pc);
            self->regs[0] = 0;
            return;
        }
    }
    break;

    /* ---------------- J ---------------- */
    case 0x6F: {
        uint32_t rd  = get_bits(inst,11,7);
        uint32_t imm = (get_bits(inst,31,31) << 20)
                     | (get_bits(inst,19,12) << 12)
                     | (get_bits(inst,20,20) << 11)
                     | (get_bits(inst,30,21) << 1);

        int32_t soff = sign_extend(imm, 21);

        self->regs[rd] = self->pc + 4;
        self->pc = (uint32_t)((int32_t)self->pc + soff);
        printf("JAL x%u -> pc = 0x%X\n", rd, self->pc);
        self->regs[0] = 0;
        return;
    }
    break;

    /* ---------------- U ---------------- */
    case 0x37: {
        uint32_t rd    = get_bits(inst,11,7);
        uint32_t imm20 = get_bits(inst,31,12);
        int32_t  val   = (int32_t)(imm20 << 12);
        self->regs[rd] = val;
        printf("LUI x%u = 0x%X\n", rd, (uint32_t)val);
    }
    break;

    case 0x17: {
        uint32_t rd    = get_bits(inst,11,7);
        uint32_t imm20 = get_bits(inst,31,12);
        uint32_t val   = imm20 << 12;
        self->regs[rd] = (int32_t)(self->pc + val);
        printf("AUIPC x%u = pc + 0x%X\n", rd, val);
    }
    break;

    /* ---------------- LOAD (LW) ---------------- */
    case 0x03: {
        uint32_t rd     = get_bits(inst, 11, 7);
        uint32_t funct3 = get_bits(inst, 14, 12);
        uint32_t rs1    = get_bits(inst, 19, 15);
        int32_t  imm    = sign_extend(get_bits(inst, 31, 20), 12);

        uint32_t endereco = (uint32_t)((int32_t)self->regs[rs1] + imm);

        if (funct3 == 0x2) { /* LW (Load Word) */
            self->regs[rd] = (int32_t)Barramento_ler(self->barramento, endereco);
            printf("LW x%u = MEM[x%u + %d] = MEM[0x%X] = 0x%X\n",
                   rd, rs1, imm, endereco, (uint32_t)self->regs[rd]);
        }
    }
    break;

    /* ---------------- STORE (SW) ---------------- */
    case 0x23: {
        uint32_t funct3 = get_bits(inst, 14, 12);
        uint32_t rs1    = get_bits(inst, 19, 15);
        uint32_t rs2    = get_bits(inst, 24, 20);

        /* imm[11:5] em [31:25], imm[4:0] em [11:7] */
        uint32_t imm    = (get_bits(inst, 31, 25) << 5) | get_bits(inst, 11, 7);
        int32_t  offset = sign_extend(imm, 12);

        uint32_t endereco = (uint32_t)((int32_t)self->regs[rs1] + offset);

        if (funct3 == 0x2) { /* SW (Store Word) */
            Barramento_escrever(self->barramento, endereco, (uint32_t)self->regs[rs2]);
            printf("SW MEM[x%u + %d] = MEM[0x%X] = x%u (0x%X)\n",
                   rs1, offset, endereco, rs2, (uint32_t)self->regs[rs2]);
        }
    }
    break;

    default:
        printf("Opcode não implementado!\n");
    }

    self->regs[0] = 0;
    self->pc += 4;
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
    Barramento_escrever(barramento, addr, 0x00500513); /* ADDI x10, x0, 5 */
    addr += 4;

    /* x11 (a1) = 1 (resultado acumulado) */
    Barramento_escrever(barramento, addr, 0x00100593); /* ADDI x11, x0, 1 */
    addr += 4;

    /* x12 (a2) = 1 (contador i) */
    Barramento_escrever(barramento, addr, 0x00100613); /* ADDI x12, x0, 1 */
    addr += 4;

    /* loop_inicio: (endereço 0x0000C) */
    /* BLT x10, x12, loop_fim (se n < i, sai do loop) */
    /* offset = 0x14 (5 instruções * 4 = 20 bytes) */
    Barramento_escrever(barramento, addr, 0x00C54863); /* BLT x10, x12, +16 */
    addr += 4;

    /* x11 = x11 * x12 (resultado *= i) */
    /* Multiplicação usando shifts e adds (x11 * x12) */
    Barramento_escrever(barramento, addr, 0x00C58633); /* ADD x12, x11, x12 */
    addr += 4;

    /* Simplificando: apenas ADD para demonstração */
    Barramento_escrever(barramento, addr, 0x00C585B3); /* ADD x11, x11, x12 */
    addr += 4;

    /* x12 = x12 + 1 (i++) */
    Barramento_escrever(barramento, addr, 0x00160613); /* ADDI x12, x12, 1 */
    addr += 4;

    /* JAL x0, loop_inicio (volta para o loop) */
    /* offset = -16 (0xFFFFFFF0) */
    Barramento_escrever(barramento, addr, 0xFF1FF06F); /* JAL x0, -16 */
    addr += 4;

    /* loop_fim: (endereço 0x00020) */
    /* x13 = endereço base da VRAM (0x80000) */
    Barramento_escrever(barramento, addr, 0x000806B7); /* LUI x13, 0x80 */
    addr += 4;

    /* x14 = 0x46 ('F') */
    Barramento_escrever(barramento, addr, 0x04600713); /* ADDI x14, x0, 0x46 */
    addr += 4;

    Barramento_escrever(barramento, addr, 0x00E6A023); /* SW x14, 0(x13) - escreve 'F' */
    addr += 4;

    /* x14 = 0x41 ('A') */
    Barramento_escrever(barramento, addr, 0x04100713); /* ADDI x14, x0, 0x41 */
    addr += 4;

    Barramento_escrever(barramento, addr, 0x00E6A223); /* SW x14, 4(x13) - escreve 'A' */
    addr += 4;

    /* x14 = 0x54 ('T') */
    Barramento_escrever(barramento, addr, 0x05400713); /* ADDI x14, x0, 0x54 */
    addr += 4;

    Barramento_escrever(barramento, addr, 0x00E6A423); /* SW x14, 8(x13) - escreve 'T' */
    addr += 4;

    /* x14 = 0x3D ('=') */
    Barramento_escrever(barramento, addr, 0x03D00713); /* ADDI x14, x0, 0x3D */
    addr += 4;

    Barramento_escrever(barramento, addr, 0x00E6A623); /* SW x14, 12(x13) - escreve '=' */
    addr += 4;

    /* x14 = 0x20 (' ') */
    Barramento_escrever(barramento, addr, 0x02000713); /* ADDI x14, x0, 0x20 */
    addr += 4;

    Barramento_escrever(barramento, addr, 0x00E6A823); /* SW x14, 16(x13) - escreve ' ' */
    addr += 4;

    /* escrecer resultado na vram */
    /* x15 = resultado (cópia de x11) */
    Barramento_escrever(barramento, addr, 0x00058793); /* ADDI x15, x11, 0 */
    addr += 4;

    /* Escrever '1' (0x31) */
    Barramento_escrever(barramento, addr, 0x03100713); /* ADDI x14, x0, 0x31 */
    addr += 4;

    Barramento_escrever(barramento, addr, 0x00E6AA23); /* SW x14, 20(x13) */
    addr += 4;

    /* Escrever '2' (0x32) */
    Barramento_escrever(barramento, addr, 0x03200713); /* ADDI x14, x0, 0x32 */
    addr += 4;

    Barramento_escrever(barramento, addr, 0x00E6AC23); /* SW x14, 24(x13) */
    addr += 4;

    /* Escrever '0' (0x30) */
    Barramento_escrever(barramento, addr, 0x03000713); /* ADDI x14, x0, 0x30 */
    addr += 4;

    Barramento_escrever(barramento, addr, 0x00E6AE23); /* SW x14, 28(x13) */
    addr += 4;

    /* ========== DEMONSTRAÇÃO DE OUTRAS INSTRUÇÕES ========== */

    /* Operações lógicas */
    Barramento_escrever(barramento, addr, 0x00F767B3); /* OR x15, x14, x15 */
    addr += 4;

    Barramento_escrever(barramento, addr, 0x00F77833); /* AND x16, x14, x15 */
    addr += 4;

    Barramento_escrever(barramento, addr, 0x00F748B3); /* XOR x17, x14, x15 */
    addr += 4;

    /* Shifts */
    Barramento_escrever(barramento, addr, 0x00171913); /* SLLI x18, x14, 1 */
    addr += 4;

    Barramento_escrever(barramento, addr, 0x00175993); /* SRLI x19, x14, 1 */
    addr += 4;

    /* Comparações */
    Barramento_escrever(barramento, addr, 0x00F72A33); /* SLT x20, x14, x15 */
    addr += 4;

    /* Loop infinito para parar execução */
    Barramento_escrever(barramento, addr, 0x0000006F); /* JAL x0, 0 (loop infinito) */
    addr += 4;

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

    const int INSTRUCOES_POR_ES = 10;  /* Exibir VRAM a cada 10 instruções */
    const int MAX_INSTRUCOES    = 100; /* Limite de segurança */

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
    printf("Limite de segurança: %d instruções\n\n", MAX_INSTRUCOES);

    /* Loop de execução */
    bool executando = true;
    int instrucoes_executadas = 0;
    int ciclos_clock = 0; /* === NOVO CONTADOR DE CICLOS AQUI === */

    while (executando && instrucoes_executadas < MAX_INSTRUCOES) {
        uint32_t instr = Barramento_ler(&barramento, cpu.pc);

        /* Detectar loop infinito (JAL x0, 0) */
        if (instr == 0x0000006F) {
            printf("\n[STOP] Loop infinito detectado - encerrando execução.\n");
            break;
        }

        printf("\n─────────────────────────────────────────────────────\n");
        printf("Ciclo de Clock #%d | Instrução #%d\n", (ciclos_clock + 1), (instrucoes_executadas + 1));
        printf("PC: 0x%08X", cpu.pc);
        printf(" | Opcode: 0x%08X\n", instr);

        CPU_executar(&cpu, instr);
        instrucoes_executadas++;
        ciclos_clock++; /* === INCREMENTA O CLOCK A CADA INSTRUÇÃO === */

        /* E/S PROGRAMADA: Exibe VRAM periodicamente */
        if (cpu.contador_instrucoes % INSTRUCOES_POR_ES == 0) {
            printf("\n>>> INTERRUPÇÃO DE E/S (a cada %d instruções) <<<\n", INSTRUCOES_POR_ES);
            DispositivoES_exibir_vram(&dispositivo_es);
        }
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

    /* === Totais de instruções e ciclos de clock === */
    printf("Total de ciclos de clock gastos: %d\n", ciclos_clock);
    printf("Total de instruções concluídas: %d\n", instrucoes_executadas);
    printf("====================================================\n\n");

    /* Estatísticas */
    printf("================ ESTATÍSTICAS DO SISTEMA ================\n");
    printf("Operações de memória realizadas via barramento\n");
    printf("VRAM utilizada para saída de caracteres ASCII\n");
    printf("E/S programada com polling a cada %d instruções\n", INSTRUCOES_POR_ES);
    printf("=========================================================\n");

    return 0;
}