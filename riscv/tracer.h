// See LICENSE for license details.

#ifndef _RISCV_TRACER_H
#define _RISCV_TRACER_H

#include "processor.h"

static inline void trace_opcode_pc(processor_t* p, insn_bits_t opc, insn_t insn, reg_t pc) {
  uint32_t bits = insn.bits();
  int kind = (bits & 0x7c);
  if (!p->trfd)
    p->trfd = fopen("trace.log", "w");
  if (p->cnt < 1000)
    {
      switch(opc)
          {
            case MATCH_AMOADD_D:
            case MATCH_AMOADD_W:
            case MATCH_AMOAND_D:
            case MATCH_AMOAND_W:
            case MATCH_AMOMAX_D:
            case MATCH_AMOMAXU_D:
            case MATCH_AMOMAXU_W:
            case MATCH_AMOMAX_W:
            case MATCH_AMOMIN_D:
            case MATCH_AMOMINU_D:
            case MATCH_AMOMINU_W:
            case MATCH_AMOMIN_W:
            case MATCH_AMOOR_D:
            case MATCH_AMOOR_W:
            case MATCH_AMOSWAP_D:
            case MATCH_AMOSWAP_W:
            case MATCH_AMOXOR_D:
            case MATCH_AMOXOR_W:
              fprintf(p->trfd,   "atomic_insn: 0x%.016lX: DASM(0x%X): %d\n", pc, bits, p->cnt++);
              break;
          default:
            break;
          }      
      fflush(p->trfd);
    }
}

#define trace_opcode(p,o,i)  trace_opcode_pc(p,o,i,pc)

#endif
