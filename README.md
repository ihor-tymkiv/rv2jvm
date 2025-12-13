# rv2jvm (WIP)
RISC-V to JVM bytecode compiler

## Planned Instruction Support for RV32I
From Chapter 2 of ratified ISA specification [_The RISC-V Instruction Set Manual Volume I: Unprivileged ISA_](https://drive.google.com/file/d/1uviu1nH-tScFfgrovvFCrj7Omv8tFtkp/view?usp=drive_link "https://drive.google.com/file/d/1uviu1nH-tScFfgrovvFCrj7Omv8tFtkp/view?usp=drive_link") (ver: 20250508, May 2025)
| Instruction           |
| --------------------- |
| ADDI rd, rs1, imm     |
| MV rd, rs1            |
| SLTI rd, rs1, imm     |
| SLTIU rd, rs1, imm    |
| SEQZ rd, rs           |
| ANDI rd, rs1, imm     |
| ORI rd, rs1, imm      |
| XORI rd, rs1, imm     |
| NOT rd, rs1           |
| SLLI rd, rs1, imm     |
| SRLI rd, rs1, imm     |
| SRAI rd, rs1, imm     |
| LUI rd, imm           |
| AUIPC rd, imm         |
| ADD rd, rs1, rs2      |
| SUB rd, rs1, rs2      |
| SLT rd, rs1, rs2      |
| SLTU rd, rs1, rs2     |
| SNEZ rd, rs1          |
| AND rd, rs1, rs2      |
| OR rd, rs1, rs2       |
| XOR rd, rs1, rs2      |
| SLL rd, rs1, rs2      |
| SRL rd, rs1, rs2      |
| SRA rd, rs1, rs2      |
| NOP                   |
| JAL rd, offset        |
| J offset              |
| JALR rd, rs1, offset  |
| JR rs1                |
| RET                   |
| BEQ rs1, rs2, offset  |
| BNE rs1, rs2, offset  |
| BLT rs1, rs2, offset  |
| BLTU rs1, rs2, offset |
| BGE rs1, rs2, offset  |
| BGEU rs1, rs2, offset |
| LW rd, offset(rs1)    |
| LH rd, offset(rs1)    |
| LHU rd, offset(rs1)   |
| LB rd, offset(rs1)    |
| LBU rd, offset(rs1)   |
| SW rs2, offset(rs1)   |
| SH rs2, offset(rs1)   |
| SB rs2, offset(rs1)   |
| FENCE                 |
| ECALL (prev: SCALL)   |
| EBREAK (prev: SBREAK) |

## References
- [Java SE8 JVM Spec](https://docs.oracle.com/javase/specs/jvms/se8/html/index.html)
- [_The RISC-V Instruction Set Manual Volume I: Unprivileged ISA_](https://drive.google.com/file/d/1uviu1nH-tScFfgrovvFCrj7Omv8tFtkp/view?usp=drive_link "https://drive.google.com/file/d/1uviu1nH-tScFfgrovvFCrj7Omv8tFtkp/view?usp=drive_link") (ver: 20250508, May 2025)
