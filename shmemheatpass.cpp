/*
 * CSE523
 * SAMPATH KUMAR KILAPARTHI
 * 112079198
 *
 * */

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis/BlockFrequencyInfoImpl.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/LoopInfo.h"
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
		// gets function name from the call instruction

		string cname = dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts())->getName().str();

		// We check fucntions which contains get and put functions. We match the function string cname with selected patterns.

		if (cname.find("put") != std::string::npos || cname.find("get") != std::string::npos) {

			for (auto i = 0; i < ci->getNumArgOperands(); i++)
			{

				ci->getArgOperand(i)->dump();
				if (ci->getArgOperand(i)->getType()->isPointerTy())
				{
					errs() << ci->getArgOperand(i)->stripPointerCasts()->getName().str() << "\n";
				}
				else
				{
					//errs() << ci->getArgOperand(i)->getName().str() << "\t";
					//errs() << ci->getOperand(i);
				}
			}
			//isIntegerTy()
			//case 1
			Type *a3, *a4;
			Value *v3 = ci->getArgOperand(3);
			a3 = ci->getArgOperand(2)->getType();
			a4 = ci->getArgOperand(3)->getType();
			a4->dump();
			if (a4->isIntegerTy())
			{
				// compare the values and see if it out of current PE
				if (ConstantInt* cint = dyn_cast<ConstantInt>(ci->getArgOperand(3))) {
					errs() << "const integer type\n";
					// foo indeed is a ConstantInt, we can use CI here
					errs() << "Const value: " << cint->getSExtValue() <<"\n";
				}
				else {
					// foo was not actually a ConstantInt
					errs() << "Not a const\n";
				}
			}
			else
			{
				// Different types. It must me an integert according to the put and get definitions
				//errs() << "Different types\n";
			}
			//a3->dump();
			//errs() << a3->getSExtValue() << " get value\n";
			//ci->getArgOperand(2)->getSExtValue();
			//ci->getArgOperand(2)->dump();
			errs() << "\nPrinting the actual PE argument: ";
			ci->getArgOperand(3)->dump();
			errs() << "************************************************************************ \n\n";
		}
	}
	
//class shmemheat : public FunctionPass
class shmemheat  : public  BlockFrequencyInfoWrapperPass 
{
public:
	//static char ID;
	map <string,int> functionMap;
	//shmemheat() : FunctionPass(ID){}
	//~shmemheat() {}
	static char ID;
	shmemheat() : BlockFrequencyInfoWrapperPass() {}
	~shmemheat() {}



	void getAnalysisUsage(AnalysisUsage &AU) const
	{
		AU.addRequired<BranchProbabilityInfoWrapperPass>();
		AU.addRequired<LoopInfoWrapperPass>();
		AU.setPreservesAll();
	}


	void printResult()
	{
		// call instructions
		errs() << "\n\t Call instructions:\t" << functionMap["call"];
		errs() << '\n';
	}

	virtual bool runOnBasicBlock(BasicBlock &BB)
	{
		for (BasicBlock::iterator bbs = BB.begin(), bbe = BB.end(); bbs != bbe; ++bbs)
		{
			Instruction* ii = &(*bbs);
			CallSite cs(ii);
			if (!cs.getInstruction()) continue;

			Value* called = cs.getCalledValue()->stripPointerCasts();
			if (Function *fptr = dyn_cast<Function>(called))
			{
				string cname = fptr->getName().str();
				if (cname.find("shmem_init") != std::string::npos) {
					errs() << ii->getOperand(0)->getName();
					errs() << *ii;
				}
				else if (cname.find("shmem") != std::string::npos) {
					CallInst *ci = cast<CallInst>(ii);
					//errs() << "\n\n\nFound  fxn call: " << *ii << "\n";
					errs() << "Function call: " << fptr->getName() << "\n";
					//errs() << "\t\t\t No of arguments: " << fptr->arg_size() << "\n";
					//errs() << "\t this gets arguments properly: " << ci->getNumArgOperands() << "\n";
					PrintFunctionArgs(ci);
				}
			}

		}

	}

	void DisplayCallstatistics(Instruction *ins, uint64_t &count)
	{
		Instruction* ii = ins;
		CallInst *ci = cast<CallInst>(ii);
		string cname = dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts())->getName().str();

		errs() << "Printing function name: " << cname  << " occurs " << count << " times.\n" ;
		// We check fucntions which contains get and put functions. We match the function string cname with selected patterns.

		if (cname.find("put") != std::string::npos || cname.find("get") != std::string::npos) {

			for (auto i = 0; i < ci->getNumArgOperands(); i++)
			{

				ci->getArgOperand(i)->dump();
				if (ci->getArgOperand(i)->getType()->isPointerTy())
				{
					errs() << ci->getArgOperand(i)->stripPointerCasts()->getName().str() << "\n";
				}
				else
				{
					//errs() << ci->getArgOperand(i)->getName().str() << "\t";
					//errs() << ci->getOperand(i);
				}
			}
			//isIntegerTy()
			//case 1
			Type *a3, *a4;
			Value *v3 = ci->getArgOperand(3);
			a3 = ci->getArgOperand(2)->getType();
			a4 = ci->getArgOperand(3)->getType();
			a4->dump();
			if (a4->isIntegerTy())
			{
				// compare the values and see if it out of current PE
				if (ConstantInt* cint = dyn_cast<ConstantInt>(ci->getArgOperand(3))) {
					errs() << "const integer type\n";
					// foo indeed is a ConstantInt, we can use CI here
					errs() << "Const value: " << cint->getSExtValue() << "\n";
				}
				else {
					// foo was not actually a ConstantInt
					errs() << "Not a const\n";
				}
			}
			else
			{
				// Different types. It must me an integert according to the put and get definitions
				//errs() << "Different types\n";
			}
			errs() << "\nPrinting the actual PE argument: ";
			ci->getArgOperand(3)->dump();
			errs() << "\n\n************************************************************************ \n\n";
		}
	}

	virtual bool isBlockOfInterest(BasicBlock &B, vector <Instruction *> &vec)
	{
		bool interest = false;
		for (auto &I : B)
		{
			Instruction* ii = &I;
			CallSite cs(ii);
			if (!cs.getInstruction()) continue;

			Value* called = cs.getCalledValue()->stripPointerCasts();
			if (Function *fptr = dyn_cast<Function>(called))
			{
				string cname = fptr->getName().str();
				if (cname.find("shmem") != std::string::npos  && cname.find("put") != std::string::npos || cname.find("get") != std::string::npos) {
					CallInst *ci = cast<CallInst>(ii);
					errs() << "Function call: " << fptr->getName() << "\n";
					interest = true;
					vec.push_back(ii);
				}
			}
		}
		return interest;
	}
	virtual bool runOnFunction(Function &Func)
	{
		errs().write_escaped(Func.getName());
		errs() << " Function Name: \t"<<Func.getName() <<"\n\t\t Function size " << Func.size();
		for (Function::iterator Its = Func.begin(), Ite = Func.end(); Its!=Ite; ++Its)
			{
				runOnBasicBlock(*Its) ;
			}
			printResult();
			functionMap.clear();


		errs() << "\n\n************************************************************************ \n\n";

		errs() << "Running the Block Frequency Estimation Part \n";

		vector <Instruction *> insv;
		BranchProbabilityInfo &BPI = getAnalysis<BranchProbabilityInfoWrapperPass>().getBPI();
		LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
		BlockFrequencyInfo &locBFI = getBFI();
		locBFI.calculate(Func, BPI, LI);

		// iterate through each block 
		for (auto &B : Func)
		{
			uint64_t BBprofCount = locBFI.getBlockProfileCount(&B).hasValue() ? locBFI.getBlockProfileCount(&B).getValue() : 0;
			uint64_t BBfreqCount = locBFI.getBlockFreq(&B).getFrequency();
			insv.clear();
			if (isBlockOfInterest(B, insv))
			{
				for (auto ins : insv)
				{
					DisplayCallstatistics(ins, BBprofCount);
				}
			}
			
		}
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


//Value* val = arg;
//errs() << val->getName().str() << " -> " << "\n";
//errs() << ci->getArgOperand(i)->getName() << " \t";
//errs() << "\t\t\t\t"<< ci->getArgOperand(i)->getValue() << "\n";


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
		functionMap[string(bbs->getOpcodeName())]++;
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
		functionMap[string(bbs->getOpcodeName())]++;
	}
}

*/
