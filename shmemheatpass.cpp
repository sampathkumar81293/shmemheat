/*
 * CSE523
 * SAMPATH KUMAR KILAPARTHI
 * 112079198
 *
 * */

#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/InstIterator.h"
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
	typedef  BasicBlock* bbt;
	

	 struct heatNode {
		int ID;
		bbt bb;
		int profcount;
		int freqcount;
		int noofcallins;
		bool imp;
		heatNode(int id, bbt bb ) : ID(id), bb(bb) , profcount(0) , freqcount(0), noofcallins(0), imp(false){}
		void setID(int ID) { this->ID = ID; }
		int getID() { return ID; }
		void setnoofcallins(int ID) { this->noofcallins = ID; }
		int getnoofcallins() { return noofcallins; }
		void setfreqcount(int fc) { this->freqcount = fc; }
		int getfreqcount() { return freqcount; }
		void setprofcount(int pc) { this->profcount = pc; }
		int getprofcount() { return profcount; }
	};

	 struct VariableMetaInfo {
		 AllocaInst *alloca;
		 bool is_static_alloca;
		 bool is_array_alloca;
		 uint64_t arraysize;
		 vector <Value *> defstack;
		 SmallPtrSet<BasicBlock *, 32> defblocks;
		 VariableMetaInfo(AllocaInst *ai) {
			 alloca = ai;
			 is_static_alloca = false;
			 is_array_alloca = false;
		 };

	 };

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
			//a3->dump();
			//errs() << a3->getSExtValue() << " get value\n";
			//ci->getArgOperand(2)->getSExtValue();
			//ci->getArgOperand(2)->dump();
			errs() << "\nPrinting the actual PE argument: ";
			ci->getArgOperand(3)->dump();
			errs() << "************************************************************************ \n\n";
		}
	}
	class shmemheat : public  BlockFrequencyInfoWrapperPass
	{
	public:

		vector <VariableMetaInfo*> Variableinfos;
		map < Instruction *, VariableMetaInfo* > Inst2VarInfo_map;
		map <string, int> functionMap;
		map < bbt, heatNode *> heatmp;
		map < int , heatNode *> heatIDmp;
		static char ID;
		int id = 1;
		shmemheat() : BlockFrequencyInfoWrapperPass() {}
		~shmemheat() {}

		int getNoOfNodes()
		{
			return id - 1;
		}
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

		void printheadnodeinfo()
		{
			int nodesize = getNoOfNodes();
			heatNode *tmp = NULL;
			for (int i = 1; i <= nodesize; i++)
			{
				tmp = heatIDmp[i];
				errs() << "\nID: " << tmp->getID();
				errs() << "\nfreqcount: " << tmp->getfreqcount();
				errs() << "\nprofcount: " << tmp->getprofcount();
				errs() << "\nNo of call instructions " << tmp->getnoofcallins() << "\n";
			}
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
			errs() << "\t\tPrinting function name: " << cname << " occurs " << count << " times.\n";
			// We check fucntions which contains get and put functions. We match the function string cname with selected patterns.
			if (cname.find("put") != std::string::npos || cname.find("get") != std::string::npos) 
			{
				for (auto i = 0; i < ci->getNumArgOperands(); i++)
				{
					//ci->getArgOperand(i)->dump();
					if (ci->getArgOperand(i)->getType()->isPointerTy())
					{
						errs() << "\t\t"<<ci->getArgOperand(i)->stripPointerCasts()->getName().str() << "\n";
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
				//a4->dump();
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
				errs() << "\t\tPrinting the actual PE argument: ";
				ci->getArgOperand(3)->dump();
				errs() << "\t\t************************************************************************ \n\n";
			}
		}

		virtual bool isCallOfInterest(string &cname)
		{
			return (cname.find("shmem") != std::string::npos  && cname.find("put") != std::string::npos || cname.find("get") != std::string::npos);
		}

		virtual bool isShmemCall(string &cname)
		{
			return (cname.find("shmem") != std::string::npos);
		}

		virtual bool isBlockOfInterest(BasicBlock &B, vector <Instruction *> &vec, vector <Instruction *> &callinst)
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
					if (isShmemCall(cname))
					{
						callinst.push_back(ii);
					}
					if (isCallOfInterest(cname)) {
						CallInst *ci = cast<CallInst>(ii);
						//errs() << "Function call: " << fptr->getName() << "\n";
						interest = true;
						vec.push_back(ii);
					}
				}
			}
			return interest;
		}

		bool Is_var_defed_and_used(VariableMetaInfo *varinfo) {
			for (auto *use : varinfo->alloca->users()) {
				Instruction *useinst;
				if ((useinst = dyn_cast<LoadInst>(use))) {
					Inst2VarInfo_map[useinst] = varinfo;
				}
				else if ((useinst = dyn_cast<StoreInst>(use)) ) {
					if (useinst->getOperand(1) == varinfo->alloca) {
						Inst2VarInfo_map[useinst] = varinfo;
						varinfo->defblocks.insert(useinst->getParent() );
					}
					else {
						return false;
					}
				}
				else {
					return false;
				}
			}
			return true;
		}

		virtual bool runOnFunction(Function &Func)
		{
			errs().write_escaped(Func.getName());
			errs() << " Function Name: " << Func.getName();
			errs() << "\n Function size " << Func.size();
		/*	for (Function::iterator Its = Func.begin(), Ite = Func.end(); Its != Ite; ++Its)
			{
				runOnBasicBlock(*Its);
			}
			*/
			//printResult();

			for (auto &insref: Func.getEntryBlock()) {
				AllocaInst *alloca;
				if ((alloca = dyn_cast<AllocaInst>(&insref))) {
					errs() << " \n Identified a alloca instruction";

					bool is_interesting = (alloca->getAllocatedType()->isSized());
					errs() << " \n issized (): " << is_interesting << "\nisstaticalloca: " << alloca->isStaticAlloca();
					//errs() << "\n getallocasizeinbytes(): " << getAllocaSizeInBytes(alloca);


					VariableMetaInfo  *varinfo = new VariableMetaInfo(alloca);
					if (alloca->isStaticAlloca()) {
						varinfo->is_static_alloca = true;
					}

					if (alloca->isArrayAllocation()) {
						const ConstantInt *CI = dyn_cast<ConstantInt>(alloca->getArraySize());
						
						varinfo->is_array_alloca = true;
						varinfo->arraysize = CI->getZExtValue();
					}

					if (Is_var_defed_and_used(varinfo)) {
						// variableinfos
						Variableinfos.push_back(varinfo);
					}
					else {
						delete varinfo;
					}

				}
			}

			functionMap.clear();

			errs() << "\n\n************************************************************************ \n\n";
			errs() << "Running the Block Frequency Estimation Part \n";

			vector <Instruction *> insv, callinst;
			bool loop = false;

			BranchProbabilityInfo &BPI = getAnalysis<BranchProbabilityInfoWrapperPass>().getBPI();
			LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
			BlockFrequencyInfo &locBFI = getBFI();
			locBFI.calculate(Func, BPI, LI);

			dbgs() << "&&&&&&&&&&&&&&&&&&&&&&\n";
			locBFI.print(llvm::dbgs());
			dbgs() << "%%%%%%%%%%%%%%%%%%%%%\n";
			BPI.print(llvm::dbgs());


			// iterate through each block 
			for (auto &B : Func)
			{
				uint64_t BBprofCount = locBFI.getBlockProfileCount(&B).hasValue() ? locBFI.getBlockProfileCount(&B).getValue() : 0;
				uint64_t BBfreqCount = locBFI.getBlockFreq(&B).getFrequency();
				insv.clear();
				if (isBlockOfInterest(B, insv, callinst))
				{
					heatNode *hnode = new heatNode(id, &B);
					hnode->setfreqcount(BBfreqCount);
					hnode->setprofcount(BBprofCount);
					errs() << "**********************************\nprof count: " << BBprofCount << "\t freq count: " << BBfreqCount;

						errs() << " This block  : \t" << B.getName() << " has\t " << B.size() << " Instructions.\n";
						errs() << " Found " << callinst.size() << " shmem related call instructions\n";
						errs() << " Display Call statistics: \n";
						hnode->setnoofcallins(callinst.size());
						for (auto ins : insv)
						{
							DisplayCallstatistics(ins, BBprofCount == 0? BBfreqCount : BBprofCount);
						}
					loop = LI.getLoopFor(&B);
					if (loop == false){
						errs() << "Not an affine loop. Not of some interest\n";
					}
					else {
						// handle cases here
						errs() << "Affine loop found here\n";
						//errs() << loop->getCanonicalInductionVariable()->getName() << "\n";
					}
						heatmp[&B] = hnode;
						heatIDmp[id] = hnode;
						id++;
				}

			}
			errs() << '\n';
			printheadnodeinfo();

			return false;
		}
	};

	char shmemheat::ID = 0;
	RegisterPass<shmemheat> X("shmemheat", "Prints shmem heat function  analysis");
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
