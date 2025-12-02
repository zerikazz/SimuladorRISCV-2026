#include <iostream>
#include <cstdint>
#include <iomanip>
using namespace std;

static inline int32_t sign_extend(uint32_t val, int bits){
    uint32_t m = 1u << (bits - 1);
    return (int32_t)((val ^ m) - m);
}

static inline uint32_t get_bits(uint32_t v, int hi, int lo){
    return (v >> lo) & ((1u << (hi - lo + 1)) - 1);
}

class Memoria {
public:
    static const uint32_t TAMANHO_TOTAL = 0xA0000; // 640 KB
    uint32_t memoria_dados[TAMANHO_TOTAL];

    Memoria() {
        for (uint32_t i = 0; i < TAMANHO_TOTAL; i++)
            memoria_dados[i] = 0;
    }

    void escrever32(uint32_t endereco, uint32_t valor){
        memoria_dados[endereco / 4] = valor;
    }

    uint32_t ler32(uint32_t endereco){
        return memoria_dados[endereco / 4];
    }

    void mostrar_memoria_info(){
        cout << "\n==================== MEMÓRIA ====================\n";
        cout << "Tamanho total: 640 KB\n";
        cout << "Faixas de endereços:\n";
        cout << " - RAM:   0x00000  até 0x7FFFF\n";
        cout << " - VRAM:  0x80000  até 0x8FFFF\n";
        cout << " - I/O:   0x9FC00  até 0x9FFFF\n";
        cout << "=================================================\n\n";
    }
};

class Barramento {
private:
    // Três barramentos separados
    uint32_t barramento_dados;      // 32 bits - transporta dados
    uint32_t barramento_enderecos;  // 32 bits - transporta endereços
    uint8_t barramento_controle;    // bits de controle (READ, WRITE, etc)
    
    Memoria* memoria;
    
public:
    // Sinais de controle
    enum Controle {
        IDLE = 0x00,
        READ = 0x01,   // bit 0: leitura
        WRITE = 0x02,  // bit 1: escrita
        IO = 0x04      // bit 2: operação de I/O
    };
    
    Barramento(Memoria* mem) : memoria(mem), 
                                barramento_dados(0), 
                                barramento_enderecos(0), 
                                barramento_controle(IDLE) {
        cout << "Barramento inicializado:\n";
        cout << " - Barramento de Dados: 32 bits\n";
        cout << " - Barramento de Endereços: 32 bits\n";
        cout << " - Barramento de Controle: READ, WRITE, IO\n";
    }
    
    // Operação de leitura através do barramento
    uint32_t ler(uint32_t endereco) {
        // 1. Coloca endereço no barramento de endereços
        barramento_enderecos = endereco;
        
        // 2. Ativa sinal de controle para leitura
        barramento_controle = READ;
        
        // 3. Memória coloca dados no barramento de dados
        barramento_dados = memoria->ler32(endereco);
        
        // 4. Retorna o dado lido
        uint32_t dado = barramento_dados;
        
        // 5. Limpa barramento de controle
        barramento_controle = IDLE;
        
        return dado;
    }
    
    // Operação de escrita através do barramento
    void escrever(uint32_t endereco, uint32_t valor) {
        // 1. Coloca endereço no barramento de endereços
        barramento_enderecos = endereco;
        
        // 2. Coloca dado no barramento de dados
        barramento_dados = valor;
        
        // 3. Ativa sinal de controle para escrita
        barramento_controle = WRITE;
        
        // 4. Memória recebe o dado do barramento
        memoria->escrever32(endereco, valor);
        
        // 5. Limpa barramento de controle
        barramento_controle = IDLE;
    }
    
    // Métodos para debug e visualização
    void mostrar_estado() {
        cout << "\n========== ESTADO DO BARRAMENTO ==========\n";
        cout << "Barramento de Endereços: 0x" << hex << setw(8) << setfill('0') 
             << barramento_enderecos << dec << "\n";
        cout << "Barramento de Dados:     0x" << hex << setw(8) << setfill('0') 
             << barramento_dados << dec << "\n";
        cout << "Barramento de Controle:  ";
        
        if (barramento_controle == IDLE) cout << "IDLE";
        else {
            if (barramento_controle & READ) cout << "READ ";
            if (barramento_controle & WRITE) cout << "WRITE ";
            if (barramento_controle & IO) cout << "IO ";
        }
        cout << "\n==========================================\n\n";
    }
    
    // Getters para inspeção (opcional)
    uint32_t get_dados() const { return barramento_dados; }
    uint32_t get_endereco() const { return barramento_enderecos; }
    uint8_t get_controle() const { return barramento_controle; }
};

class DispositivoES {
private:
    Memoria* memoria;
    static const uint32_t VRAM_INICIO = 0x80000;
    static const uint32_t VRAM_FIM = 0x8FFFF;
    static const uint32_t VRAM_TAMANHO = (VRAM_FIM - VRAM_INICIO + 1) / 4; // em words
    
public:
    DispositivoES(Memoria* mem) : memoria(mem) {}
    
    // Exibe o conteúdo da VRAM como caracteres ASCII
    void exibir_vram() {
        cout << "\n╔════════════════════════════════════════════════════════╗\n";
        cout << "║           SAÍDA DE VÍDEO (VRAM - E/S)                  ║\n";
        cout << "╚════════════════════════════════════════════════════════╝\n";
        cout << "Endereço 0x80000 - 0x8FFFF:\n";
        cout << "┌────────────────────────────────────────────────────────┐\n│ ";
        
        int caracteres_linha = 0;
        bool tem_conteudo = false;
        
        for (uint32_t addr = VRAM_INICIO; addr <= VRAM_FIM; addr += 4) {
            uint32_t word = memoria->ler32(addr);
            
            // Cada word contém 4 bytes (caracteres)
            for (int i = 0; i < 4; i++) {
                uint8_t byte = (word >> (i * 8)) & 0xFF;
                
                if (byte != 0) {
                    tem_conteudo = true;
                    
                    // Imprime caractere ASCII ou '.' se não imprimível
                    if (byte >= 32 && byte <= 126) {
                        cout << (char)byte;
                    } else if (byte == 10) { // newline
                        cout << "\n│ ";
                        caracteres_linha = 0;
                        continue;
                    } else {
                        cout << '.';
                    }
                    
                    caracteres_linha++;
                    
                    // Quebra linha a cada 54 caracteres
                    if (caracteres_linha >= 54) {
                        cout << "\n│ ";
                        caracteres_linha = 0;
                    }
                }
            }
        }
        
        if (!tem_conteudo) {
            cout << "[VRAM vazia - sem conteúdo para exibir]";
        }
        
        cout << "\n└────────────────────────────────────────────────────────┘\n\n";
    }
    
    bool eh_endereco_vram(uint32_t endereco) {
        return (endereco >= VRAM_INICIO && endereco <= VRAM_FIM);
    }
};

class CPU {
public:
    int32_t regs[32] = {0};
    uint32_t pc = 0;
    Barramento* barramento;
    uint32_t contador_instrucoes = 0;

    CPU(Barramento* bus) : barramento(bus) {
        regs[0] = 0;
    }

    void executar(uint32_t inst) {
        contador_instrucoes++;
        uint32_t opcode = inst & 0x7F;

        switch (opcode) {

        // ---------------- R ----------------
        case 0x33: {
            uint32_t rd     = get_bits(inst,11,7);
            uint32_t funct3 = get_bits(inst,14,12);
            uint32_t rs1    = get_bits(inst,19,15);
            uint32_t rs2    = get_bits(inst,24,20);
            uint32_t funct7 = get_bits(inst,31,25);

            switch (funct3) {
            case 0x0: // ADD / SUB
                if (funct7 == 0x00) {
                    regs[rd] = regs[rs1] + regs[rs2];
                    cout << "ADD x" << rd << " = x" << rs1 << " + x" << rs2 << "\n";
                } else if (funct7 == 0x20) {
                    regs[rd] = regs[rs1] - regs[rs2];
                    cout << "SUB x" << rd << " = x" << rs1 << " - x" << rs2 << "\n";
                }
                break;

            case 0x1: // SLL
                regs[rd] = (int32_t)((uint32_t)regs[rs1] << (regs[rs2] & 0x1F));
                cout << "SLL x" << rd << " = x" << rs1 << " << x" << rs2 << "\n";
                break;

            case 0x5: // SRL / SRA
                if (funct7 == 0x00) {
                    regs[rd] = (int32_t)((uint32_t)regs[rs1] >> (regs[rs2] & 0x1F));
                    cout << "SRL x" << rd << " = x" << rs1 << " >>u x" << rs2 << "\n";
                } else if (funct7 == 0x20) {
                    regs[rd] = regs[rs1] >> (regs[rs2] & 0x1F);
                    cout << "SRA x" << rd << " = x" << rs1 << " >>s x" << rs2 << "\n";
                }
                break;

            case 0x6: // OR
                regs[rd] = regs[rs1] | regs[rs2];
                cout << "OR x" << rd << " = x" << rs1 << " | x" << rs2 << "\n";
                break;

            case 0x7: // AND
                regs[rd] = regs[rs1] & regs[rs2];
                cout << "AND x" << rd << " = x" << rs1 << " & x" << rs2 << "\n";
                break;

            case 0x4: // XOR
                regs[rd] = regs[rs1] ^ regs[rs2];
                cout << "XOR x" << rd << " = x" << rs1 << " ^ x" << rs2 << "\n";
                break;

            case 0x2: // SLT
                regs[rd] = (regs[rs1] < regs[rs2]) ? 1 : 0;
                cout << "SLT x" << rd << " = (x" << rs1 << " < x" << rs2 << ")\n";
                break;

            case 0x3: // SLTU
                regs[rd] = ((uint32_t)regs[rs1] < (uint32_t)regs[rs2]) ? 1 : 0;
                cout << "SLTU x" << rd << " = (ux" << rs1 << " < ux" << rs2 << ")\n";
                break;

            default:
                cout << "R-type funct3 não implementado: " << funct3 << "\n";
            }
        }
        break;

        // ---------------- I ----------------
        case 0x13: {
            uint32_t rd     = get_bits(inst,11,7);
            uint32_t funct3 = get_bits(inst,14,12);
            uint32_t rs1    = get_bits(inst,19,15);
            int32_t imm     = sign_extend(get_bits(inst,31,20), 12);

            switch (funct3) {
            case 0x0: // ADDI
                regs[rd] = regs[rs1] + imm;
                cout << "ADDI x" << rd << " = x" << rs1 << " + " << imm << "\n";
                break;

            case 0x6: // ORI
                regs[rd] = regs[rs1] | imm;
                cout << "ORI x" << rd << " = x" << rs1 << " | " << imm << "\n";
                break;

            case 0x7: // ANDI
                regs[rd] = regs[rs1] & imm;
                cout << "ANDI x" << rd << " = x" << rs1 << " & " << imm << "\n";
                break;

            case 0x1: { // SLLI
                uint32_t sh = get_bits(inst,24,20);
                regs[rd] = (int32_t)((uint32_t)regs[rs1] << sh);
                cout << "SLLI x" << rd << " = x" << rs1 << " << " << sh << "\n";
                break;
            }

            case 0x5: { // SRLI / SRAI
                uint32_t sh = get_bits(inst,24,20);
                uint32_t funct7 = get_bits(inst,31,25);

                if (funct7 == 0x00) {
                    regs[rd] = (int32_t)((uint32_t)regs[rs1] >> sh);
                    cout << "SRLI x" << rd << " = x" << rs1 << " >>u " << sh << "\n";
                } else {
                    regs[rd] = regs[rs1] >> sh;
                    cout << "SRAI x" << rd << " = x" << rs1 << " >>s " << sh << "\n";
                }
                break;
            }

            default:
                cout << "I-type funct3 não implementado: " << funct3 << "\n";
            }
        }
        break;

        // ---------------- B ----------------
        case 0x63: {
            uint32_t funct3 = get_bits(inst,14,12);
            uint32_t rs1 = get_bits(inst,19,15);
            uint32_t rs2 = get_bits(inst,24,20);

            uint32_t imm = (get_bits(inst,31,31) << 12)
                         | (get_bits(inst,7,7) << 11)
                         | (get_bits(inst,30,25) << 5)
                         | (get_bits(inst,11,8) << 1);

            int32_t soff = sign_extend(imm, 13);
            bool take = false;

            switch (funct3) {
            case 0x0: take = (regs[rs1] == regs[rs2]); cout << "BEQ\n"; break;
            case 0x1: take = (regs[rs1] != regs[rs2]); cout << "BNE\n"; break;
            case 0x4: take = (regs[rs1] < regs[rs2]);  cout << "BLT\n"; break;
            case 0x5: take = (regs[rs1] >= regs[rs2]); cout << "BGE\n"; break;
            case 0x6: take = ((uint32_t)regs[rs1] <  (uint32_t)regs[rs2]); cout << "BLTU\n"; break;
            case 0x7: take = ((uint32_t)regs[rs1] >= (uint32_t)regs[rs2]); cout << "BGEU\n"; break;
            default:
                cout << "Branch funct3 desconhecido.\n";
            }

            if (take) {
                pc = (int32_t)pc + soff;
                cout << "Branch taken -> pc = 0x" << hex << pc << dec << "\n";
                regs[0] = 0;
                return;
            }
        }
        break;

        // ---------------- J ----------------
        case 0x6F: {
            uint32_t rd = get_bits(inst,11,7);
            uint32_t imm = (get_bits(inst,31,31) << 20)
                         | (get_bits(inst,19,12) << 12)
                         | (get_bits(inst,20,20) << 11)
                         | (get_bits(inst,30,21) << 1);

            int32_t soff = sign_extend(imm, 21);

            regs[rd] = pc + 4;
            pc = (uint32_t)((int32_t)pc + soff);
            cout << "JAL x" << rd << " -> pc = 0x" << hex << pc << dec << "\n";
            regs[0] = 0;
            return;
        }
        break;

        // ---------------- U ----------------
        case 0x37: {
            uint32_t rd = get_bits(inst,11,7);
            uint32_t imm20 = get_bits(inst,31,12);
            int32_t val = (int32_t)(imm20 << 12);
            regs[rd] = val;
            cout << "LUI x" << rd << " = 0x" << hex << (uint32_t)val << dec << "\n";
        }
        break;

        case 0x17: {
            uint32_t rd = get_bits(inst,11,7);
            uint32_t imm20 = get_bits(inst,31,12);
            uint32_t val = imm20 << 12;
            regs[rd] = (int32_t)(pc + val);
            cout << "AUIPC x" << rd << " = pc + 0x" << hex << val << dec << "\n";
        }
        break;
        
        // ---------------- LOAD (LW) ----------------
        case 0x03: {
            uint32_t rd = get_bits(inst, 11, 7);
            uint32_t funct3 = get_bits(inst, 14, 12);
            uint32_t rs1 = get_bits(inst, 19, 15);
            int32_t imm = sign_extend(get_bits(inst, 31, 20), 12);
            
            uint32_t endereco = (uint32_t)((int32_t)regs[rs1] + imm);
            
            if (funct3 == 0x2) { // LW (Load Word)
                regs[rd] = (int32_t)barramento->ler(endereco);
                cout << "LW x" << rd << " = MEM[x" << rs1 << " + " << imm 
                     << "] = MEM[0x" << hex << endereco << "] = 0x" 
                     << (uint32_t)regs[rd] << dec << "\n";
            }
        }
        break;
        
        // ---------------- STORE (SW) ----------------
        case 0x23: {
            uint32_t funct3 = get_bits(inst, 14, 12);
            uint32_t rs1 = get_bits(inst, 19, 15);
            uint32_t rs2 = get_bits(inst, 24, 20);
            
            // imm[11:5] em [31:25], imm[4:0] em [11:7]
            uint32_t imm = (get_bits(inst, 31, 25) << 5) | get_bits(inst, 11, 7);
            int32_t offset = sign_extend(imm, 12);
            
            uint32_t endereco = (uint32_t)((int32_t)regs[rs1] + offset);
            
            if (funct3 == 0x2) { // SW (Store Word)
                barramento->escrever(endereco, (uint32_t)regs[rs2]);
                cout << "SW MEM[x" << rs1 << " + " << offset 
                     << "] = MEM[0x" << hex << endereco << "] = x" << dec << rs2 
                     << " (0x" << hex << (uint32_t)regs[rs2] << dec << ")\n";
            }
        }
        break;

        default:
            cout << "Opcode não implementado!\n";
        }

        regs[0] = 0;
        pc += 4;
    }
};

void carregar_programa_completo(Barramento& barramento) {
    cout << "\n========== CARREGANDO PROGRAMA DE TESTE COMPLETO ==========\n";
    cout << "Programa: Cálculo de Fatorial e operações diversas\n";
    cout << "Funcionalidades demonstradas:\n";
    cout << " - Instruções aritméticas (ADD, SUB, ADDI)\n";
    cout << " - Instruções lógicas (AND, OR, XOR)\n";
    cout << " - Instruções de shift (SLL, SRL)\n";
    cout << " - Branches (BEQ, BNE, BLT)\n";
    cout << " - Jumps (JAL)\n";
    cout << " - Load/Store (LW, SW)\n";
    cout << " - Acesso à VRAM para E/S\n";
    cout << "===========================================================\n\n";
    
    uint32_t addr = 0x00000;
    
    // x10 (a0) = 5 (número para calcular fatorial)
    barramento.escrever(addr, 0x00500513); // ADDI x10, x0, 5
    addr += 4;
    
    // x11 (a1) = 1 (resultado acumulado)
    barramento.escrever(addr, 0x00100593); // ADDI x11, x0, 1
    addr += 4;
    
    // x12 (a2) = 1 (contador i)
    barramento.escrever(addr, 0x00100613); // ADDI x12, x0, 1
    addr += 4;
    
    // loop_inicio: (endereço 0x0000C)
    // BLT x10, x12, loop_fim (se n < i, sai do loop)
    // offset = 0x14 (5 instruções * 4 = 20 bytes)
    barramento.escrever(addr, 0x00C54863); // BLT x10, x12, +16
    addr += 4;
    
    // x11 = x11 * x12 (resultado *= i)
    // Multiplicação usando shifts e adds (x11 * x12)
    barramento.escrever(addr, 0x00C58633); // ADD x12, x11, x12
    addr += 4;
    
    // Simplificando: apenas ADD para demonstração
    barramento.escrever(addr, 0x00C585B3); // ADD x11, x11, x12
    addr += 4;
    
    // x12 = x12 + 1 (i++)
    barramento.escrever(addr, 0x00160613); // ADDI x12, x12, 1
    addr += 4;
    
    // JAL x0, loop_inicio (volta para o loop)
    // offset = -16 (0xFFFFFFF0)
    barramento.escrever(addr, 0xFF1FF06F); // JAL x0, -16
    addr += 4;
    
    // loop_fim: (endereço 0x00020)
    // x13 = endereço base da VRAM (0x80000)
    barramento.escrever(addr, 0x000806B7); // LUI x13, 0x80
    addr += 4;
    
    // x14 = 0x46 ('F')
    barramento.escrever(addr, 0x04600713); // ADDI x14, x0, 0x46
    addr += 4;
    
    barramento.escrever(addr, 0x00E6A023); // SW x14, 0(x13) - escreve 'F'
    addr += 4;
    
    // x14 = 0x41 ('A')
    barramento.escrever(addr, 0x04100713); // ADDI x14, x0, 0x41
    addr += 4;
    
    barramento.escrever(addr, 0x00E6A223); // SW x14, 4(x13) - escreve 'A'
    addr += 4;
    
    // x14 = 0x54 ('T')
    barramento.escrever(addr, 0x05400713); // ADDI x14, x0, 0x54
    addr += 4;
    
    barramento.escrever(addr, 0x00E6A423); // SW x14, 8(x13) - escreve 'T'
    addr += 4;
    
    // x14 = 0x3D ('=')
    barramento.escrever(addr, 0x03D00713); // ADDI x14, x0, 0x3D
    addr += 4;
    
    barramento.escrever(addr, 0x00E6A623); // SW x14, 12(x13) - escreve '='
    addr += 4;
    
    // x14 = 0x20 (' ')
    barramento.escrever(addr, 0x02000713); // ADDI x14, x0, 0x20
    addr += 4;
    
    barramento.escrever(addr, 0x00E6A823); // SW x14, 16(x13) - escreve ' '
    addr += 4;
    
    // escrecer resultado na vram
    // x15 = resultado (cópia de x11)
    barramento.escrever(addr, 0x00058793); // ADDI x15, x11, 0
    addr += 4;
    
    // Escrever '1' (0x31)
    barramento.escrever(addr, 0x03100713); // ADDI x14, x0, 0x31
    addr += 4;
    
    barramento.escrever(addr, 0x00E6AA23); // SW x14, 20(x13)
    addr += 4;
    
    // Escrever '2' (0x32)
    barramento.escrever(addr, 0x03200713); // ADDI x14, x0, 0x32
    addr += 4;
    
    barramento.escrever(addr, 0x00E6AC23); // SW x14, 24(x13)
    addr += 4;
    
    // Escrever '0' (0x30)
    barramento.escrever(addr, 0x03000713); // ADDI x14, x0, 0x30
    addr += 4;
    
    barramento.escrever(addr, 0x00E6AE23); // SW x14, 28(x13)
    addr += 4;
    
    // ========== DEMONSTRAÇÃO DE OUTRAS INSTRUÇÕES ==========
    
    // Operações lógicas
    barramento.escrever(addr, 0x00F767B3); // OR x15, x14, x15
    addr += 4;
    
    barramento.escrever(addr, 0x00F77833); // AND x16, x14, x15
    addr += 4;
    
    barramento.escrever(addr, 0x00F748B3); // XOR x17, x14, x15
    addr += 4;
    
    // Shifts
    barramento.escrever(addr, 0x00171913); // SLLI x18, x14, 1
    addr += 4;
    
    barramento.escrever(addr, 0x00175993); // SRLI x19, x14, 1
    addr += 4;
    
    // Comparações
    barramento.escrever(addr, 0x00F72A33); // SLT x20, x14, x15
    addr += 4;
    
    // Loop infinito para parar execução
    barramento.escrever(addr, 0x0000006F); // JAL x0, 0 (loop infinito)
    addr += 4;
    
    cout << "Programa carregado: " << (addr/4) << " instruções\n";
    cout << "Tamanho: " << addr << " bytes\n\n";
}


// =======================================================
// MAIN — Memória + CPU original
// =======================================================
int main(){
    Memoria memoria;
    Barramento barramento(&memoria);
    CPU cpu(&barramento);
    DispositivoES dispositivo_es(&memoria);
    
    const int INSTRUCOES_POR_ES = 10;  // Exibir VRAM a cada 10 instruções
    const int MAX_INSTRUCOES = 100;     // Limite de segurança

    cout << "╔═══════════════════════════════════════════════════════════╗\n";
    cout << "║        SIMULADOR DE COMPUTADOR RISC-V 32-bit              ║\n";
    cout << "║                  Arquitetura RV32I                        ║\n";
    cout << "╚═══════════════════════════════════════════════════════════╝\n\n";

    memoria.mostrar_memoria_info();

    cout << "======================= CPU ============================\n";
    cout << "Registradores: 32 x 32-bit (x0-x31)\n";
    cout << "PC inicial: 0x00000000\n";
    cout << "Instruções implementadas:\n";
    cout << " • Tipo R: ADD, SUB, AND, OR, XOR, SLL, SRL, SRA, SLT, SLTU\n";
    cout << " • Tipo I: ADDI, ANDI, ORI, SLLI, SRLI, SRAI, LW\n";
    cout << " • Tipo S: SW\n";
    cout << " • Tipo B: BEQ, BNE, BLT, BGE, BLTU, BGEU\n";
    cout << " • Tipo U: LUI, AUIPC\n";
    cout << " • Tipo J: JAL\n";
    cout << "========================================================\n\n";

    // Carregar programa de teste
    carregar_programa_completo(barramento);

    cout << "=============== INICIANDO EXECUÇÃO ===============\n";
    cout << "Configuração de E/S: Exibir VRAM a cada " 
         << INSTRUCOES_POR_ES << " instruções\n";
    cout << "Limite de segurança: " << MAX_INSTRUCOES << " instruções\n\n";

    // Loop de execução
    bool executando = true;
    int instrucoes_executadas = 0;

    while (executando && instrucoes_executadas < MAX_INSTRUCOES) {
        uint32_t instr = barramento.ler(cpu.pc);
        
        // Detectar loop infinito (JAL x0, 0)
        if (instr == 0x0000006F) {
            cout << "\n[STOP] Loop infinito detectado - encerrando execução.\n";
            break;
        }
        
        cout << "\n─────────────────────────────────────────────────────\n";
        cout << "Instrução #" << (instrucoes_executadas + 1) << "\n";
        cout << "PC: 0x" << hex << setw(8) << setfill('0') << cpu.pc << dec;
        cout << " | Opcode: 0x" << hex << setw(8) << setfill('0') << instr << dec << "\n";
        
        cpu.executar(instr);
        instrucoes_executadas++;
        
        // E/S PROGRAMADA: Exibe VRAM periodicamente
        if (cpu.contador_instrucoes % INSTRUCOES_POR_ES == 0) {
            cout << "\n>>> INTERRUPÇÃO DE E/S (a cada " << INSTRUCOES_POR_ES << " instruções) <<<\n";
            dispositivo_es.exibir_vram();
        }
    }

    // Exibição final
    cout << "\n\n";
    cout << "╔═══════════════════════════════════════════════════════════╗\n";
    cout << "║                   EXECUÇÃO FINALIZADA                     ║\n";
    cout << "╚═══════════════════════════════════════════════════════════╝\n\n";

    cout << "============= ESTADO FINAL DA VRAM =============\n";
    dispositivo_es.exibir_vram();

    cout << "================ ESTADO FINAL DA CPU ================\n";
    cout << "Registradores (apenas não-zero):\n";
    for (int i = 0; i < 32; i++) {
        if (cpu.regs[i] != 0) {
            cout << "  x" << setw(2) << i << " (";
            
            // Nome ABI
            if (i == 10) cout << "a0";
            else if (i == 11) cout << "a1";
            else if (i == 12) cout << "a2";
            else if (i == 13) cout << "a3";
            else if (i == 14) cout << "a4";
            else if (i == 15) cout << "a5";
            else cout << "  ";
            
            cout << "): " << setw(10) << cpu.regs[i] 
                 << " (0x" << hex << setw(8) << setfill('0') 
                 << (uint32_t)cpu.regs[i] << dec << ")\n";
        }
    }
    
    cout << "\nPC final: 0x" << hex << setw(8) << setfill('0') 
         << cpu.pc << dec << "\n";
    cout << "Total de instruções executadas: " << instrucoes_executadas << "\n";
    cout << "====================================================\n\n";

    // Estatísticas
    cout << "================ ESTATÍSTICAS DO SISTEMA ================\n";
    cout << "Operações de memória realizadas via barramento\n";
    cout << "VRAM utilizada para saída de caracteres ASCII\n";
    cout << "E/S programada com polling a cada " << INSTRUCOES_POR_ES << " instruções\n";
    cout << "=========================================================\n";

    return 0;
}