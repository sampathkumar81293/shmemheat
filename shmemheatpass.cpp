/*
 * CSE523
 * SAMPATH KUMAR KILAPARTHI
 * 112079198
 *
 * */

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
//#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"


#include <map>

using namespace llvm;
using namespace std;

namespace
{

	
	string ParseFunctionName(CallInst *call)
	{
		auto *fptr = call->getCalledFunction();
		if (!fptr) {
			return "received null as fptr";
		}
		else {
			return string(fptr->getName());
		}
	}
	

class shmemheat : public FunctionPass
{
public:
	static char ID;
	map <string,int> opMap;
	shmemheat() : FunctionPass(ID){}
	~shmemheat() {}
	void printResult()
	{
		// call instructions
		errs() << "\n\t Call instructions:\t" << opMap["call"];
		errs() << '\n';
	}


	virtual bool runOnBasicBlock(BasicBlock &BB)
	{
		//errs() << "Basic Block " << '\n';
		
		for (BasicBlock::iterator bbs = BB.begin(), bbe = BB.end(); bbs!=bbe; ++bbs)
			{
			Instruction* ii = &(*bbs);
				//if (isa<CallInst>(ii)) {
			if (string(bbs->getOpcodeName()) == "call") {

				CallInst *ci = cast<CallInst>(ii);
				//errs() << "\t\t "<< cast<CallInst>(ii).getCalledFunction().getName()<< "\n";			
				errs() << "\t\t"<<ParseFunctionName(ci) << "\n";
				errs() << bbs->getOpcodeName() << '\t';
				bbs->printAsOperand(errs(), false);
				errs() << '\n';
				opMap[string(bbs->getOpcodeName())]++;
			}
			}
	}

	virtual bool runOnFunction(Function &Func)
	{
		errs().write_escaped(Func.getName());
		errs() << " \t function size " << Func.size() ;
		for (Function::iterator Its = Func.begin(), Ite = Func.end(); Its!=Ite; ++Its)
			{
				runOnBasicBlock(*Its) ;
			}
			printResult();
			opMap.clear();
		errs() << '\n' ;
		return false;
	}	
};

char shmemheat::ID = 0;
RegisterPass<shmemheat> X("shmemheat" , "Prints shmem heat function  analysis");
}

/*
   switch (OpCode) {
   // Terminators
   case Ret:    return "ret";
   case Br:     return "br";
   case Switch: return "switch";
   case IndirectBr: return "indirectbr";
   case Invoke: return "invoke";
   case Resume: return "resume";
   case Unreachable: return "unreachable";
   case CleanupRet: return "cleanupret";
   case CatchRet: return "catchret";
   case CatchPad: return "catchpad";
   case CatchSwitch: return "catchswitch";
 
   // Standard binary operators...
   case Add: return "add";
   case FAdd: return "fadd";
   case Sub: return "sub";
   case FSub: return "fsub";
   case Mul: return "mul";
   case FMul: return "fmul";
   case UDiv: return "udiv";
   case SDiv: return "sdiv";
   case FDiv: return "fdiv";
   case URem: return "urem";
   case SRem: return "srem";
   case FRem: return "frem";
 
   // Logical operators...
   case And: return "and";
   case Or : return "or";
   case Xor: return "xor";
 
   // Memory instructions...
   case Alloca:        return "alloca";
   case Load:          return "load";
   case Store:         return "store";
   case AtomicCmpXchg: return "cmpxchg";
   case AtomicRMW:     return "atomicrmw";
   case Fence:         return "fence";
   case GetElementPtr: return "getelementptr";
 
   // Convert instructions...
   case Trunc:         return "trunc";
   case ZExt:          return "zext";
   case SExt:          return "sext";
   case FPTrunc:       return "fptrunc";
   case FPExt:         return "fpext";
   case FPToUI:        return "fptoui";
   case FPToSI:        return "fptosi";
   case UIToFP:        return "uitofp";
   case SIToFP:        return "sitofp";
   case IntToPtr:      return "inttoptr";
   case PtrToInt:      return "ptrtoint";
   case BitCast:       return "bitcast";
   case AddrSpaceCast: return "addrspacecast";
 
   // Other instructions...
   case ICmp:           return "icmp";
   case FCmp:           return "fcmp";
   case PHI:            return "phi";
   case Select:         return "select";
   case Call:           return "call";
   case Shl:            return "shl";
   case LShr:           return "lshr";
   case AShr:           return "ashr";
   case VAArg:          return "va_arg";
   case ExtractElement: return "extractelement";
   case InsertElement:  return "insertelement";
   case ShuffleVector:  return "shufflevector";
   case ExtractValue:   return "extractvalue";
   case InsertValue:    return "insertvalue";
   case LandingPad:     return "landingpad";
   case CleanupPad:     return "cleanuppad";
 
   default: return "<Invalid operator> ";
   }  */
