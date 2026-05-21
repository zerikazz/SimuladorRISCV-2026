# SimuladorRISC-V

Simulador da arquitetura RISC-V matéria de Arquitetura De Computadores.

![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)

---

## Integrantes

- Érika Maria de Sousa Santos
- Giovanna Lara Nassar Santos
- Sophia Verardo de Araújo
---

## Branches

```
versioncpp
└──version.cpp     ← implementação original monociclo em C++
```

```
main
├── monociclo.c     ← reescrita monociclo em C
└── main.c      ← versão pipeline em C
```

```
C++ monociclo  →  C monociclo  →  C pipeline
```

---

## Sobre

Implementa o conjunto de instruções RV32I nas versões monociclo e pipeline de 5 estágios `IF → ID → EX → MEM → WB`.