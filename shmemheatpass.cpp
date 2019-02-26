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
#include "llvm/IR/CallSite.h"
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

	void PrintFunctionArgs(CallInst *ci)
	{
		string cname = dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts())->getName().str();
		if (cname.find("put") != std::string::npos || cname.find("get") != std::string::npos) {

			/*
			Value *val0 = ci->getArgOperand(0);
			Value *val1 = ci->getArgOperand(1);
			Value *val2 = ci->getArgOperand(2);
			Value *val3 = ci->getArgOperand(3);
			errs() << val0->getName().str()  << "\n";
			errs() << val1->getName().str() << "\n";
			errs() << val2->getName().str() << "\n";
			errs() << val3->getName().str() << "\n";
			*/
			for (auto i = 0; i < ci->getNumArgOperands(); i++)
			{
				//Value* val = arg;
				//errs() << val->getName().str() << " -> " << "\n";
				//errs() << ci->getArgOperand(i)->getName() << " \t";
				//errs() << "\t\t\t\t"<< ci->getArgOperand(i)->getValue() << "\n";
				ci->getArgOperand(i)->dump();
			}
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

		
		for (BasicBlock::iterator bbs = BB.begin(), bbe = BB.end(); bbs != bbe; ++bbs)
		{
			Instruction* ii = &(*bbs);
			CallSite cs(ii);
			if (!cs.getInstruction()) continue;

			Value* called = cs.getCalledValue()->stripPointerCasts();
			if (Function *fptr = dyn_cast<Function>(called))
			{
				string cname = fptr->getName().str();
				if (cname.find("sh") != std::string::npos) {
					CallInst *ci = cast<CallInst>(ii);
					errs() << "\n\n\nFound  fxn call: " << *ii << "\n";
					errs() << "Function call: " << fptr->getName() << "\n";
					//errs() << "\t\t\t No of arguments: " << fptr->arg_size() << "\n";
					errs() << "\t this gets arguments properly: " << ci->getNumArgOperands() << "\n";
					PrintFunctionArgs(ci);
				}
				//}
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





/*virtual bool runOnModule(Module &M)
{
	for (auto F = M.begin(), E = M.end(); F != E; ++F)
	{
		runOnFunction(*F);
	}
	return false;
}*/


//if (isa<CallInst>(ii)) {
/*
if (string(bbs->getOpcodeName()) == "call") {

		CallInst *ci = cast<CallInst>(ii);
		//errs() << "\t\t "<< cast<CallInst>(ii).getCalledFunction().getName()<< "\n"; 
		errs() << "\t\t "<< dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts()).getName()<< "\n";  
		errs() << "\t\t"<<ParseFunctionName(ci) << "\n";
		PrintFunctionArgs(ci);
		errs() << bbs->getOpcodeName() << '\t';
		bbs->printAsOperand(errs(), false);
		errs() << '\n';
		opMap[string(bbs->getOpcodeName())]++;
	}
	*/



/*

if (string(bbs->getOpcodeName()) == "call") {

	CallInst *ci = cast<CallInst>(ii);
	//errs() << "\t\t "<< cast<CallInst>(ii).getCalledFunction().getName()<< "\n"; 
	//errs() << "\t\t BokkaBokka" << dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts())->getName().str() << "\n";
	//errs() << "\t\t" << ParseFunctionName(ci) << "\n";
	//PrintFunctionArgs(ci);
	string cname = dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts())->getName().str();
	if (cname.find("sh_") != std::string::npos) {

		errs() << bbs->getOpcodeName() << '\t';
		bbs->printAsOperand(errs(), false);
		errs() << '\n';
		opMap[string(bbs->getOpcodeName())]++;
	}
}

*/
