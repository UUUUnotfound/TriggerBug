#ifndef _TRIGGERBUG_
#define _TRIGGERBUG_

#include "engine.h"
#include "engine/variable.h"
#include "engine/register.h"
#include "engine/memory.h"
#include "engine/state_class.h"
#include "engine/state_analyzer.h"
#include "engine/guest_helper_defs.h"


namespace SP {
    template <typename ADDR, class TC>
    class StatePrinter;
};

namespace TR {

    class StateX86 : public State<Addr32> {
        friend class SP::StatePrinter<Addr32, StateX86>;
        friend class win32;
        friend class linux32;
        StateX86(StateX86* father_state, Addr32 gse) :State(father_state, gse) {};
    public:
        StateX86(vex_context<Addr32>& vex_info, Addr32 gse, Bool _need_record) :State(vex_info, gse, _need_record) 
        {  };

        virtual Kernel* ForkState(Addr32 ges) override { return new StateX86(this, ges); };
        virtual bool  StateCompression(State<Addr32> const& next) override { return true; }
        virtual void  StateCompressMkSymbol(State<Addr32> const& newState) override {  };
        //Thread Environment Block
        rsval<Addr32> TEB() override;
    };





    class StateAMD64 : public State<Addr64> {
        friend class SP::StatePrinter<Addr64, StateAMD64>;
        friend class win64;
        friend class linux64;
        StateAMD64(StateAMD64* father_state, Addr64 gse) :
            State(father_state, gse)
        {
        };
    public:
        StateAMD64(vex_context<Addr64>& vex_info, Addr64 gse, Bool _need_record) :
            State(vex_info, gse, _need_record)
        {};


        virtual Kernel* ForkState(Addr64 ges) override { return new StateAMD64(this, ges); };
        virtual bool  StateCompression(State<Addr64> const& next) override { return true; }
        virtual void  StateCompressMkSymbol(State<Addr64> const& newState) override {  };
        //Thread Environment Block
        rsval<Addr64> TEB() override { return regs.get<Ity_I64>(AMD64_IR_OFFSET::FS_CONST); }
    };


};

namespace SP {

    template <typename ADDR, class TC>
    class StatePrinter : public TC {
        TRControlFlags m_trtraceflags;
    public:
        StatePrinter(StatePrinter* father_state, ADDR gse) : TC(father_state, gse), m_trtraceflags(father_state->m_trtraceflags) {};

        inline TRControlFlags setFlag(TRControlFlags t) { return (TRControlFlags)setFlag((ULong)t); }
        inline ULong setFlag(ULong f) { return *(ULong*)&m_trtraceflags |= f; }

        inline TRControlFlags delFlag(TRControlFlags f) { return (TRControlFlags)delFlag((ULong)f); }
        inline ULong delFlag(ULong f) { return *(ULong*)&m_trtraceflags &= ~f; }

        inline bool getFlag(TRControlFlags t) const { return m_trtraceflags & t; }
        inline ULong getFlags() const { return m_trtraceflags; }
        inline ULong& getFlagRef() { return m_trtraceflags; }

        void pp_call_space(){
            UInt size = m_InvokStack.size();
            printf("[%-2d:%2d]", size, mem.get_user());
            for (UInt i = 0; i < size; i++) {
                vex_printf("  ");
            }
        }
        void pp_call_space(ADDR addr) {
            UInt size = m_InvokStack.size();
            printf("[%-2d:%2d] 0x%-16x", size, mem.get_user(), addr);
            for (UInt i = 0; i < size; i++) {
                vex_printf("  ");
            }
        }
    public:
        StatePrinter(vex_context<ADDR>& vex_info, ADDR gse, Bool _need_record) :
            TC(vex_info, gse, _need_record),
            m_trtraceflags(CF_None) {};


        void   traceStart() override {
            if (getFlag(CF_traceState)) {
                pp_call_space();
                std::cout << "\n+++++++++++++++ Thread ID: " << GetCurrentThreadId() << "  address: " << std::hex << get_cpu_ip() << "  Started +++++++++++++++\n" << std::endl;
            };
        }

        void   traceFinish() override {
            if (getFlag(CF_traceState)) {
                if (status() == Fork) {
                    vex_printf("Fork from: %p to:{ ", get_cpu_ip());
                    for (auto bc : branch) {
                        vex_printf(" %p", bc->get_state_ep());
                    }
                    vex_printf(" };", get_cpu_ip());
                }
                std::cout << "\n+++++++++++++++ Thread ID: " << GetCurrentThreadId() << "  address: " << std::hex << get_cpu_ip() << "  OVER +++++++++++++++\n" << std::endl;
            }
        }

        void   traceIRStmtEnd(IRStmt* s) override {
            if (getFlag(CF_ppStmts)) {
                pp_call_space();
                if (s->tag == Ist_WrTmp) {
                    UInt tmp = s->Ist.WrTmp.tmp;
                    vex_printf("t%u = ", tmp);
                    std::cout << m_ir_temp[tmp];
                    vex_printf(" = ");
                    ppIRExpr(s->Ist.WrTmp.data);
                }
                else {
                    ppIRStmt(s);
                }
                vex_printf("\n");
            }
        };

        void   traceIRSB(IRSB* bb) override {
            if (getFlag(CF_traceJmp)) {
                pp_call_space();
                vex_printf("Jmp: %llx \n", get_cpu_ip());
            }
        };
        void traceInvoke(ADDR call, ADDR bp) {
            if (getFlag(CF_traceInvoke)) {
                pp_call_space(get_cpu_ip());
                vex_printf("Call: %llx bp: %llx\n", call, bp);
            }
        }

        virtual Kernel* ForkState(ADDR ges) override { return new StatePrinter<ADDR, TC>(this, ges); };
        virtual State_Tag call_back_hook(Hook_struct const& hs) override { setFlag(hs.cflag); return (hs.cb) ? (hs.cb)(this) : Running; }
        virtual bool  StateCompression(State<ADDR> const& next) override { return true; }
        virtual void  StateCompressMkSymbol(State<ADDR> const& newState) override {  };
    };

    using win64 = StatePrinter<Addr64, TR::win64>;
    using win32 = StatePrinter<Addr32, TR::win32>;
    using linux64 = StatePrinter<Addr64, TR::linux64>;
    using linux32 = StatePrinter<Addr32, TR::linux32>;
};



#endif