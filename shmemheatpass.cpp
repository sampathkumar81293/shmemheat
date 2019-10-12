/*
 * CSE 523 / 524
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
#include <vector>
#include <stack>
#include "../LivenessAnalysis/LivenessAnalysis.cpp"


using namespace llvm;
using namespace std;


	typedef  BasicBlock* bbt;
	/* This a custom structur to preserve information that is relevant to this pass*/

	const int DEFAULT = 5;

	struct heatNode {

		int ID;/* ID for future use*/
		bbt bb; /* pointer to basic block*/
		int profcount; /* count of profile analysis*/
		int freqcount; /* estimated frequecy count of the BB */
		int noofcallins; /* contains the number of call instructions. This is handy for further analysis*/
		bool imp; /* marks the importance of the given  BB */

		/* constructor*/
		heatNode(int id, bbt bb) : ID(id), bb(bb), profcount(0), freqcount(0), noofcallins(0), imp(false) {}
	
		/* setter*/
		void setID(int ID) { this->ID = ID; }
		/* getter*/
		int getID() { return ID; }
		/* setter*/
		void setnoofcallins(int ID) { this->noofcallins = ID; }
		/* getter*/
		int getnoofcallins() { return noofcallins; }
		/* setter*/
		void setfreqcount(int fc) { this->freqcount = fc; }
		/* getter*/
		int getfreqcount() { return freqcount; }
		/* setter*/
		void setprofcount(int pc) { this->profcount = pc; }
		/* getter*/
		int getprofcount() { return profcount; }
	};
	/*  This struct stores the  metainfo of the variables that are declared at  the start.
		This are generally alloca instructions. LLVM generates alloca instructions for heap and stack allocated variables.
		These variables are used later by the IR. This is information is cructial in finding out whether the given variable is
		being used by the Basic Block of interest.
	*/
	struct VariableMetaInfo {
	
		/* pointer to alloca instruciton*/
		AllocaInst *alloca;
		/* Marks if the given alloca instrucion has static size allcoation parameter*/
		bool is_static_alloca;
		/* marks if the intruction pertains to an array*/
		bool is_array_alloca;
		/* Info of whether this is a pointer or not */
		bool isPointer;

		/* Holds the array size if is_array_alloca is true */
		uint64_t arraysize;
		/* places where the variable is defined. Otherwise DEF's indicate that the variable is being updated*/
		vector <Value *> defstack;
		SmallPtrSet<BasicBlock *, 32> defblocks;
		VariableMetaInfo(AllocaInst *ai) {
			alloca = ai;
			is_static_alloca = false;
			is_array_alloca = false;
		}
	};



	struct CallMetaInfo {

		/* pointer to Call instruction*/
		CallInst *ci;
		vector< vector<Instruction *>> vva;
		CallMetaInfo(CallInst *cinst) {
			ci = cinst;
			vva.clear();
		}

	};

	string ParseFunctionName(CallInst *call) {
		auto *fptr = call->getCalledFunction();
		if (!fptr) {
			return "received null as fptr";
		} else {
			return string(fptr->getName());
		}
	}

	namespace {

	class shmemheat : public  BlockFrequencyInfoWrapperPass {
	public:

		/* Vector of all variable metainformation */
		vector <VariableMetaInfo*> Variableinfos;
		/* map of instruction and it's variable meta information*/
		DenseMap < Instruction *, VariableMetaInfo* > Inst2VarInfo_map;

		// call intruction and it's operands involved for alloca
		vector < CallMetaInfo *>  callinstvec;
		map < CallInst *, CallMetaInfo* > Callinst2AllocaMap;
		map <string, int> functionMap;
		map < bbt, heatNode *> heatmp;
		map < int, heatNode *> heatIDmp;
		map< string, int> functionToCodeMap;
		static char ID;
		int id = 1;
		shmemheat() : BlockFrequencyInfoWrapperPass() {}
		~shmemheat() {}

		/*
		 * This function peeks into the alloca instructions
		 *
		 */

		bool find_alloca_from_vec(AllocaInst *AI, int *index) {
			bool present = false;
			for (int i = 0; i < Variableinfos.size(); i++) {
				if (Variableinfos[i]->alloca == AI) {
					*index = i;
					present = true;
					break;
				}
			}
			return present;
		}
		void peek_into_alloca_mappings(Value *v1) {
			errs() << "Print argument type: " << *(v1->getType()) << "\n";
			v1->dump();
			if (v1->getType()->isPointerTy()) {
				//errs() << "1. pointer type\n";
				if (isa<LoadInst>(v1)) {
					errs() << "Load type\n";

					Instruction *useinst;
					if ((useinst = cast<LoadInst>(v1))) {
						
						//errs() << "\nPrinting the alloca: \t";
						//errs() << vinfo->alloca->dump() << "\n";
						if (Inst2VarInfo_map.find(useinst) != Inst2VarInfo_map.end()) {
							VariableMetaInfo *vinfo = Inst2VarInfo_map[useinst];
							errs() << "Found alloca mapped instruction\n";
						} else {
							errs() << "Couldn't find it";
						}

					}
				} else if (isa<AllocaInst>(v1)) {
					errs() << "Alloca type\n";
					//AllocaInst *allocinst;
					int index = -1;
					if (AllocaInst *allocinst = cast<AllocaInst>(v1)) {
						for (auto vinfo : Variableinfos) {

							errs() << "\nCalculated flag: " << (vinfo->alloca == allocinst);

							if (vinfo->alloca == allocinst) {
								//errs() << "\nPrinting the alloca: \t";
								//errs() << vinfo->alloca->dump() << "\n";

								if (find_alloca_from_vec(allocinst, &index)) {
									errs() << "Found alloca direct instruction\n";
								} else {
									errs() << "Lookup failed\n";
								}
								break;
							}
						}
					}
				} else if (isa<StoreInst>(v1)) {

					errs() << "Found Store instruction\n";

				} else if (isa<GetElementPtrInst>(v1)) {
					errs() << "Found GEPI instruction\n";
					//AllocaInst *allocinst;
					if (GetElementPtrInst *GEPI = cast<GetElementPtrInst>(v1)) {

						errs() << GEPI->getNumOperands() << "\n";

						for (int i = 0; i < GEPI->getNumOperands(); i++) {
							errs() << "\t" << *(GEPI->getOperand(i)) << "\n";
						}
					}
				} else {
					errs() << "Went to else case\n";
				}
			} else {
				errs() << "Not a pointer type argument\n";
			}

		}

		/*
		 * This function prints the function arguments
		 */


		void get_allocainst_for_every_operand(Instruction *ci, vector <Instruction *> &va) {
			stack <Instruction *> s;
			//vector <Instruction *> v;
			Instruction *ii;
			//ii = cast<Intruction>(*ci);
			ii = ci;
			s.push(ii);
			bool flag = true;
			while (!s.empty()) {
				ii = s.top(); s.pop();
				for (Use &U : ii->operands()) {
					Value *v = U.get();
					errs() << "\n\t\t\t\t \"  starting on : " << *v << "\n";
					if (dyn_cast<Instruction>(v)) {
						errs() << "\n\"  processing " << *dyn_cast<Instruction>(v) << "\n\t" << "\"" << " -> " << "\"" << *ii << "\"" << ";\n";
						//flag = true;
						ii = dyn_cast<Instruction>(v);
						s.push(ii);
						if (AllocaInst *alloca = dyn_cast_or_null<AllocaInst>(ii)) {
							va.push_back(ii);
						}
					}
				}
			}
		}

		void PrintFunctionArgs(CallInst *ci, CallMetaInfo *cmi) {
			// gets function name from the call instruction
			string cname = dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts())->getName().str();
			// We check fucntions which contains get and put functions. We match the function string cname with selected patterns.
			Value *v1, *v2, *v3, *v4;
			LoadInst *li1, *li2;

			errs() << "Iterating over the operands on the call instruction\n";

			for (Use &U : ci->operands()) {
				vector <Instruction *> va;
				va.clear();
				Value *v = U.get();

				if (dyn_cast<Instruction>(v)) {
					//errs() << "\n\"" << *dyn_cast<Instruction>(v) << "\n\t" << "\"" << " -> " << "\"" << *ci << "\"" << ";\n";

					get_allocainst_for_every_operand(dyn_cast<Instruction>(v), va);
					for (int i = 0; i < va.size(); i++) {
						errs() <<  i << " 'th alloca map: " << *(va[i]) << "\n";
					}
					cmi->vva.push_back(va);
				}
			}


			errs() << "Alloca instructions ended\n";


			if (cname.find("put") != std::string::npos || cname.find("get") != std::string::npos) {

				errs() << "\n\nfunction args trace start\n";
				Function* fn = ci->getCalledFunction();
				//for (auto arg = fn->arg_begin(); arg != fn->arg_end(); ++arg) {
					//errs() << *(arg) << "\n";
				//}
				errs() << "function args trace end\n\n";

				v1 = ci->getArgOperand(0);
				v2 = ci->getArgOperand(1);
				v3 = ci->getArgOperand(2);
				v4 = ci->getArgOperand(3);

				peek_into_alloca_mappings(v1);
				peek_into_alloca_mappings(v2);

				/*

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
				*/
			}
		}

		int getNoOfNodes() {
			return id - 1;
		}

		void getAnalysisUsage(AnalysisUsage &AU) const {
			AU.addRequired<BranchProbabilityInfoWrapperPass>();
			AU.addRequired<LoopInfoWrapperPass>();
			AU.addRequired<LivenessAnalysisPass>();
			AU.setPreservesAll();
		}

		void printResult() {
			// call instructions
			errs() << "\n\t Call instructions:\t" << functionMap["call"];
			errs() << '\n';
		}

		void printheadnodeinfo() {

			int nodesize = getNoOfNodes();
			heatNode *tmp = NULL;
			for (int i = 1; i <= nodesize; i++) {
				tmp = heatIDmp[i];
				errs() << "\nID: " << tmp->getID();
				errs() << "\nfreqcount: " << tmp->getfreqcount();
				errs() << "\nprofcount: " << tmp->getprofcount();
				errs() << "\nNo of call instructions " << tmp->getnoofcallins() << "\n";
			}
		}
		int GetFunctionID(string &cname) {
			/*
			shmem_init 1
			shmem_put  2
			shmem_get  3
			default    4
			*/
			int value = DEFAULT;
			if (cname.find("shmem_init") != std::string::npos) {
				value = 1;
			}
			else if (cname.find("shmem") != std::string::npos && cname.find("put") != std::string::npos ) {
				value = 2;
			}
			else if (cname.find("shmem") != std::string::npos && cname.find("get") != std::string::npos) {
				value = 3;
			} else {
				value = DEFAULT;
			}
			return value;
		}
		virtual bool ProcessBasicBlock(BasicBlock &BB) {

			for (BasicBlock::iterator bbs = BB.begin(), bbe = BB.end(); bbs != bbe; ++bbs) {

				Instruction* ii = &(*bbs);
				CallSite cs(ii);
				if (!cs.getInstruction()) continue;
				Value* called = cs.getCalledValue()->stripPointerCasts();
				if (Function *fptr = dyn_cast<Function>(called)) {
					string cname = fptr->getName().str();

					// Add provision to look for different shmem functions
								/*
								shmem_init 1
								shmem_put  2
								shmem_get  3
								default    4
								*/
					switch (GetFunctionID(cname)) {
					case 1: {
						errs() << ii->getOperand(0)->getName();
						errs() << *ii;
						break;
					}
					case 2:
					case 3: {
						CallInst *ci = cast<CallInst>(ii);
						//errs() << "\n\n\nFound  fxn call: " << *ii << "\n";
						errs() << "Function call: " << fptr->getName() << "\n";
						//errs() << "\t\t\t No of arguments: " << fptr->arg_size() << "\n";
						//errs() << "\t this gets arguments properly: " << ci->getNumArgOperands() << "\n";
						CallMetaInfo *cmi = new CallMetaInfo(ci);
						PrintFunctionArgs(ci, cmi);
						callinstvec.push_back(cmi);
						Callinst2AllocaMap[ci] = cmi;
						break;
					}
					default:
						errs() << "Default case invoked: " << cname << "\n";
						break;

					}
				}
			}
		}


		void DisplayAllocaforCallInstruction(CallInst *ci) {
			if (Callinst2AllocaMap.find(ci) != Callinst2AllocaMap.end()) {
				CallMetaInfo *cmi = Callinst2AllocaMap[ci];
					//cmi->vva.push_back(va);   sampath

				for (auto &va : cmi->vva) {
					for (auto al : va) {
					errs() << *al;
					}
				}
			}

		}
		void DisplayCallstatistics(Instruction *ins, uint64_t &count) {
			Instruction* ii = ins;
			CallInst *ci = cast<CallInst>(ii);
			string cname = dyn_cast<Function>(ci->getCalledValue()->stripPointerCasts())->getName().str();
			errs() << "\t\tPrinting function name: " << cname << " occurs " << count << " times.\n";
			// We check fucntions which contains get and put functions. We match the function string cname with selected patterns.

			int functionID = GetFunctionID(cname);
			if ( functionID >=2 && functionID <= 3 ) {

				for (auto i = 0; i < ci->getNumArgOperands(); i++) {

					//ci->getArgOperand(i)->dump();
					if (ci->getArgOperand(i)->getType()->isPointerTy()) {

						errs() << "\t\t" << ci->getArgOperand(i)->stripPointerCasts()->getName().str() << "\n";
					} else {
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
				if (a4->isIntegerTy()) {

					// compare the values and see if it out of current PE
					if (ConstantInt* cint = dyn_cast<ConstantInt>(ci->getArgOperand(3))) {
						errs() << "const integer type\n";
						// foo indeed is a ConstantInt, we can use CI here
						errs() << "Const value: " << cint->getSExtValue() << "\n";
					} else {
						// foo was not actually a ConstantInt
						errs() << "Not a const\n";
					}
				} else {
					// Different types. It must me an integert according to the put and get definitions
					//errs() << "Different types\n";
				}
				errs() << "\t\tPrinting the actual PE argument: ";
				ci->getArgOperand(3)->dump();
				errs() << "\t\t************************************************************************ \n\n";
			}


			DisplayAllocaforCallInstruction(ci);
		}

		virtual bool isCallOfInterest(string &cname) {

			int value = GetFunctionID(cname);
			return DEFAULT != value;

		}

		virtual bool isShmemCall(string &cname) {
			return (cname.find("shmem") != std::string::npos);
		}

		virtual bool isBlockOfInterest(BasicBlock &B, vector <Instruction *> &vec, vector <Instruction *> &callinst) {
			bool interest = false;
			for (auto &I : B) {
				Instruction* ii = &I;
				CallSite cs(ii);
				if (!cs.getInstruction()) continue;

				Value* called = cs.getCalledValue()->stripPointerCasts();
				if (Function *fptr = dyn_cast<Function>(called)) {
					string cname = fptr->getName().str();
					if (isShmemCall(cname)) {
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


			int i = 1;
			/*for (auto *use : varinfo->alloca->users()) {
				Instruction *useinst;
				errs() << i++ << " \t" << *(dyn_cast<Instruction>(use)) << "\n";

				if (useinst = dyn_cast<GetElementPtrInst>(use)) {
					errs() << "*******************GEPI found\n";
				}
			}*/

			for (auto *use : varinfo->alloca->users()) {
				Instruction *useinst;
				errs() << i++ << " \t" << *(dyn_cast<Instruction>(use)) << "\n";

				if (useinst = dyn_cast<GetElementPtrInst>(use)) {
					errs() << "*******************GEPI found\n";
				}


				if ((useinst = dyn_cast<LoadInst>(use))) {
					//useinst->print(errs()); errs() << "\n";
					if (Inst2VarInfo_map.find(useinst) == Inst2VarInfo_map.end()) {
						Inst2VarInfo_map[useinst] = varinfo;
						//errs() << "\nLoad dump:\n";
						//useinst->dump();
					} else {
						errs() << "Replacing an existing entry\n";
					}
				} else if ((useinst = dyn_cast<StoreInst>(use))) {
					//useinst->print(errs()); errs() << "\n";
					if (useinst->getOperand(1) == varinfo->alloca) {
						Inst2VarInfo_map[useinst] = varinfo;
						varinfo->defblocks.insert(useinst->getParent());
					} else {
						return false;
					}
				} else {
					errs() << "|||||||||||Looping out|||||||||||||||||||";
					//useinst->print(errs()); errs() << "\n";
					return false;
				}
			}
			return true;
		}

		void printEveryInstruction(Function &Func) {

			for (auto block = Func.getBasicBlockList().begin(); block != Func.getBasicBlockList().end(); block++) {
				for (auto inst = block->begin(); inst != block->end(); inst++) {
					for (Use &U : inst->operands()) {
						Value *v = U.get();
						if (dyn_cast<Instruction>(v)) {
							errs() << "\n\"" << *dyn_cast<Instruction>(v) << "\n\t" << "\"" << " -> " << "\"" << *inst << "\"" << ";\n";
						}
					}
					errs() << "used\n";
				}
			}
		}

		void processAllocaInstructions(Function &Func) {

			for (auto &insref : Func.getEntryBlock()) {
				Instruction *I = &insref;
				// We check if the given instruction can be casted to a Alloca instruction.
				if (AllocaInst *alloca = dyn_cast_or_null<AllocaInst>(&insref)) {
					//errs() << " \n Identified a alloca instruction : " << (I)->getNumOperands();

					/* This sets if the alloca instruction is of specific size or not.*/
					bool is_interesting = (alloca->getAllocatedType()->isSized());
					//errs() << " \n issized (): " << is_interesting << "\nisstaticalloca: " << alloca->isStaticAlloca();
					//errs() << " is array allocation: " << alloca->isArrayAllocation();
					//errs() << "\n getallocasizeinbytes(): " << getAllocaSizeInBytes(alloca);
					bool isArray = alloca->isArrayAllocation() || alloca->getType()->getElementType()->isArrayTy();

					errs() << "\nPointer type allocation: " << alloca->getAllocatedType()->isPointerTy();
					errs() << "\n Array type allocation: " << alloca->getAllocatedType()->isArrayTy();
					//if (isArray) errs() << " array[" << *(alloca->getArraySize()) << "]"  << *(alloca->getOperand(0)) <<"\n";

					VariableMetaInfo  *varinfo = new VariableMetaInfo(alloca);

					/* tells if it is sized*/
					if (alloca->isStaticAlloca()) {
						varinfo->is_static_alloca = true;
					}
					/* Tells if the alloca is a pointer allocation*/
					if (alloca->getAllocatedType()->isPointerTy()) {
						varinfo->isPointer = true;
					}
					/* check if an allocation is array and retrieve it's size*/
					if (isArray || alloca->getAllocatedType()->isArrayTy()) {

						/*The AllocaInst instruction allocates stack memory.The value that it
							returns is always a pointer to memory.

							You should run an experiment to double - check this, but I believe
							AllocaInst::getType() returns the type of the value that is the result
							of the alloca while AllocaInst::getAllocatedType() returns the type of
							the value that is allocated.For example, if the alloca allocates a
							struct { int; int }, then getAllocatedType() returns a struct type and
							getType() return a "pointer to struct" type.*/

							//errs() << "size : " << cast<ArrayType>(alloca->getAllocatedType())->getNumElements() << "\n";
						errs() << "Allocated type" << *(alloca->getAllocatedType()) << " \n";

						Value* arraysize = alloca->getArraySize();
						/*Value* totalsize = ConstantInt::get(arraysize->getType(), CurrentDL->getTypeAllocSize(II->getAllocatedType()));
						totalsize = Builder->CreateMul(totalsize, arraysize);
						totalsize = Builder->CreateIntCast(totalsize, MySizeType, false);
						TheState.SetSizeForPointerVariable(II, totalsize);*/
						const ConstantInt *CI = dyn_cast<ConstantInt>(alloca->getArraySize());
						varinfo->is_array_alloca = true;
						varinfo->arraysize = cast<ArrayType>(alloca->getAllocatedType())->getNumElements();
						//varinfo->arraysize = CI->getZExtValue();
						errs() << "\nAlloca instruction is an array inst of size : " << *(CI) << " sz  " << varinfo->arraysize;
					}

					if (Is_var_defed_and_used(varinfo)) {
						// variableinfos
						Variableinfos.push_back(varinfo);
					} else {
						delete varinfo;
					}
				}
			}
		}

		// maintains mapping between call instruction and it's operands alloca mappings
		void ProcessAllBasicBlocks(Function &Func) {
			for (Function::iterator Its = Func.begin(), Ite = Func.end(); Its != Ite; ++Its) {
				ProcessBasicBlock(*Its);
			}
		}
		virtual bool runOnFunction(Function &Func) {

			errs().write_escaped(Func.getName());
			errs() << "\n\n************************************************************************ \n\n";
			errs() << "\nFunction Name: " << Func.getName();

			vector<Instruction *> worklist;
			/*
			 * printEveryInstruction(Func);
			 */
			errs() << "\n\tFunction size " << Func.size();
			//printResult();
			/*
			 * Get hold of alloca instructions. Since, these intructions are Alloca we can use getEntryBlock() to
			 * iterate over the first few ones.
			 */
			processAllocaInstructions(Func);
			// Run the alloca identification in every call instruction
			errs() << "\n\n************************************************************************ \n\n";
			errs() << "Run the alloca identification in every call instruction \n";

			ProcessAllBasicBlocks(Func);

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
			for (auto &B : Func) {
				uint64_t BBprofCount = locBFI.getBlockProfileCount(&B).hasValue() ? locBFI.getBlockProfileCount(&B).getValue() : 0;
				uint64_t BBfreqCount = locBFI.getBlockFreq(&B).getFrequency();
				insv.clear();
				if (isBlockOfInterest(B, insv, callinst)) {
					heatNode *hnode = new heatNode(id, &B);
					hnode->setfreqcount(BBfreqCount);
					hnode->setprofcount(BBprofCount);
					errs() << "**********************************\nprof count: " << BBprofCount << "\t freq count: " << BBfreqCount;

					errs() << " This block  : \t" << B.getName() << " has\t " << B.size() << " Instructions.\n";
					errs() << " Found " << callinst.size() << " shmem related call instructions\n";
					errs() << " Display Call statistics: \n";
					hnode->setnoofcallins(callinst.size());
					for (auto ins : insv) {
						DisplayCallstatistics(ins, BBprofCount == 0 ? BBfreqCount : BBprofCount);
					}
					loop = LI.getLoopFor(&B);
					if (loop == false) {
						errs() << "Not an affine loop. Not of some interest\n";
					} else {
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



/*bool UsingArray = false;
//errs() << "Number of opeands: " << I->getNumOperands() << "\n";
for (unsigned num = 0; num < I->getNumOperands(); ++num)
	if (isa<ArrayType>(I->getOperand(num)->getType())) {
		errs() << "\nAlloca instruction is an array inst";
		UsingArray = true;
	}*/

	//I->print(errs());
	//errs() << "************\n";
	//errs() << "number of operands : " << insref.getNumOperands();
	//insref.dump();
	//errs() << "\n";



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

		/*

			explain what  needs to be done
			Problmes: eeverywhere
			ex: which poart to use
				differentia; chckpoint system
					use later


					explain:
						example: wih examples


						Have them side by side and exaplaint



						Title:
						Compiler support for Fault Tolerance


						Many of today’s compute clusters are large systems consisting of hundreds to thousands
						of compute nodes. Long-running parallel applications typically use parallel programming
						libraries to move data between the nodes. Many of them are time sensitive real time applications
						that are bounded to run within specified time frames. Since nodes may fail while a job is still active,
						programs must be designed so that they are able to continue despite the faults/ failures. A fault tolerant system would reliably
						provide an intended service even in the presence of critical failures. The most common
						way involves saving interim data at regular intervals (so-called checkpoints) in such a way that
						the code can be restarted with the most recent checkpoint after a critical failure. Since writing out data
						is expensive, help to optimize the use of checkpoints is valuable.


						Simulators which are based on Weather , Petroleum Reserve Calculations, Verilog VHDL etc typically run for
						days. These are generally time sensitive, memory intensive applications. This criticality in time is what
						distinguishes these Real time systems from non- critical ones. While Our systems should be tolerant towards
						any failures, they should also have suitable mechanisms to recover from any potential hardware or software faults.

						LLVM is fast becoming the vehicle of choice for a wide variety of compiler research and development activities
						and is also increasingly being adopted by hardware vendors to provide compiler support for parallel programming
						interfaces. In this project, LLVM is extended to support efficient checkpointing.



						Openshmem Checker:

						OpenSHMEM is a programming interface for computer clusters that enables
						message passing, just like MPI. It contains library routines that enable remote
						data to be fetched or written directly and so they do not interfere with
						code running on the location storing the data object of interest, thereby leading to
						higher performance. But openSHMEM compiler doesn't have the inbuilt support for symantical
						checks that are related to synchronisation. As an example, while it syntactically to have
						a get statement followed by a put, the results may not be consistent. This is because of
						the overhead involved with a put request.
						Additionally, it is convenient to identify potential points for checkpointing in the front End.
						However, these might not be optimal. We should revert to IR optimizations to decide upon the strategy
						to place these checkpointer placeholders.


						Design:



						Def Use Chains:

						int z  ==>   %1 = alloca i32, align 4				==> Alloca Instruction
						int y        %2 = alloca i32, align 4
						z = 1		 store i32 1, i32* %1, align 4			==> Def Instruction
						y = z+1      %5 = add nsw i32 %4, 1					==> Use Instruction
									 store i32 %5, i32* %2, align 4

						z = 4        store i32 4, i32* %1, align 4			==> Def Instruction

						x = y+1      %6 = load i32, i32* %2, align 4		==> pseudo Use Instruction
									 %7 = add nsw i32 %6, 1						We should account for this as well. This should be mapped to z
									 store i32 %7, i32* %3, align 4

						Array Instruction:

									%6 = alloca [9 x i64], align 16

						shmem_alloc pointer:

									%19 = getelementptr inbounds [9 x i64], [9 x i64]* %6, i64 0, i64 %18


						Objects to be checkpointed:

						We have extended Extend LLVM to analyze its use of data in order to decide on what data objects need
						to be most frequently checkpointed. We generated IR code and wrote a pass to decide on relevant checkpoint metrics.
						In this pass, we have cached information on the variables at the start of every Basic Block. These are the
						variables that are both symmetric and non-symmetric. At this point, we can't differentiate between this two because LLVM
						converts both the allocations into Alloca Instrucrions. So we can't differentiate between Instruction that are Stack / Heap
						alocated. Since the Basic Block entry contains all the relevant Alloca Instructions we iterated through it and cached it
						in VariableMetaInfo for later use. The metainfo includes other information like isArray, reference to alloca instruction, size of
						array if it is an array allocation. Additionally, we also maintain DEF-USE maps to a given alloca instruction. This comes
						handy in mapping a given ArgumentOperand to its Alloca variable. This information is crutial to identify which variables are
						 being acted upon.


							:VariableMetaInfo:
									We cache meta info of variables into this structure.

						Identification of Call Instructions:

						To identify the Call instructions, we have iterated on the Instructions of interest(i.e usually shmem_get and shmem_put call
						instructions ). These instructions are cached into our custom DS for later use. Additionally, we maintain a map of
						arguments and it's corresponding alloca instructions. We use this info to decide which variables are being updated.
						Since these arrays can be quite large sometimes, we are also trying to find the stripe of the array segment that is
						being updated. It helps to further narrow down our checkpointing memory footprint without having to store everything i nthe symmetric
						memory of a PE. The idea to analyse the heat entropy of data exchange between PE's.


						Strategy for Profitability:

						To decide on the strategy for profitability we developed and implemented in LLVM a strategy for deciding whether
						it is profitable to perform checkpointing.









		*/
