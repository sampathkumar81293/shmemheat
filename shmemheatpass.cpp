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


