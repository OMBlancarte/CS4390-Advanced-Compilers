#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include <fstream>
#include <sstream>

using namespace llvm;

struct MemorySSAGraphVisPass : PassInfoMixin<MemorySSAGraphVisPass> {
  
  // Helper function to escape strings for DOT format
  std::string escapeDOT(const std::string &str) {
    std::string result;
    for (char c : str) {
      if (c == '"' || c == '\\') result += '\\';
      if (c == '\n') { result += "\\n"; continue; }
      if (c == '\r') continue;
      result += c;
    }
    return result;
  }
  
  // Get a unique ID for a MemoryAccess
  std::string getAccessID(MemoryAccess *MA) {
    std::stringstream ss;
    ss << "MA_" << (void*)MA;
    return ss.str();
  }
  
  // Get a label for a MemoryAccess
  std::string getAccessLabel(MemoryAccess *MA) {
    std::string result;
    llvm::raw_string_ostream rso(result);
    MA->print(rso);
    rso.flush();
    return escapeDOT(result);
  }
  
  // Get instruction details for label
  std::string getInstructionLabel(Instruction *I) {
    std::string result;
    llvm::raw_string_ostream rso(result);
    I->print(rso);
    rso.flush();
    return escapeDOT(result);
  }

  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM) {
    auto &MSSAResult = AM.getResult<MemorySSAAnalysis>(F);
    auto &MSSA = MSSAResult.getMSSA();

    errs() << "Generating MemorySSA graph for function: " << F.getName() << "\n";

    // Create output filename
    std::string filename = F.getName().str() + "_memssa.dot";
    std::ofstream dotFile(filename);
    
    if (!dotFile.is_open()) {
      errs() << "Error: Could not open file " << filename << "\n";
      return PreservedAnalyses::all();
    }

    // Start DOT graph
    dotFile << "digraph MemorySSA_" << F.getName().str() << " {\n";
    dotFile << "  rankdir=TB;\n";
    dotFile << "  node [shape=box, fontname=\"Courier\"];\n";
    dotFile << "  edge [fontname=\"Courier\", fontsize=10];\n\n";
    
    // Add a title node
    dotFile << "  title [label=\"MemorySSA for " << F.getName().str() 
            << "\", shape=plaintext, fontsize=16, fontname=\"Arial Bold\"];\n\n";

    // Process each basic block
    for (auto &BB : F) {
      std::string bbName = BB.hasName() ? BB.getName().str() : "entry";
      
      // Create a subgraph for each basic block
      dotFile << "  subgraph cluster_" << bbName << " {\n";
      dotFile << "    label=\"BasicBlock: " << bbName << "\";\n";
      dotFile << "    style=filled;\n";
      dotFile << "    color=lightgrey;\n";
      dotFile << "    node [style=filled, color=white];\n\n";

      // Handle MemoryPhi nodes
      if (auto *Phi = MSSA.getMemoryAccess(&BB)) {
        if (auto *MPhi = dyn_cast<MemoryPhi>(Phi)) {
          std::string phiID = getAccessID(MPhi);
          dotFile << "    " << phiID << " [label=\"MemoryPhi\\n" 
                  << bbName << "\", shape=diamond, color=blue, style=filled, fillcolor=lightblue];\n";
          
          // Add edges from incoming values
          for (unsigned i = 0; i < MPhi->getNumIncomingValues(); ++i) {
            auto *IncomingAcc = MPhi->getIncomingValue(i);
            auto *PredBB = MPhi->getIncomingBlock(i);
            std::string predName = PredBB->hasName() ? PredBB->getName().str() : "pred";
            std::string incomingID = getAccessID(IncomingAcc);
            
            dotFile << "    " << incomingID << " -> " << phiID 
                    << " [label=\"from " << predName << "\", color=blue];\n";
          }
        }
      }

      // Handle MemoryDef and MemoryUse
      for (auto &I : BB) {
        if (auto *MA = MSSA.getMemoryAccess(&I)) {
          std::string accessID = getAccessID(MA);
          std::string instLabel = getInstructionLabel(&I);
          
          if (auto *MDef = dyn_cast<MemoryDef>(MA)) {
            // MemoryDef (stores/calls that write)
            dotFile << "    " << accessID << " [label=\"MemoryDef\\n" 
                    << instLabel << "\", color=red, style=filled, fillcolor=lightyellow];\n";
            
            // Add edge to defining access
            auto *DefAccess = MDef->getDefiningAccess();
            if (DefAccess) {
              std::string defID = getAccessID(DefAccess);
              dotFile << "    " << defID << " -> " << accessID 
                      << " [label=\"defines\", color=red, style=bold];\n";
            }
          } else if (auto *MUse = dyn_cast<MemoryUse>(MA)) {
            // MemoryUse (loads)
            dotFile << "    " << accessID << " [label=\"MemoryUse\\n" 
                    << instLabel << "\", color=green, style=filled, fillcolor=lightgreen];\n";
            
            // Add edge to defining access
            auto *DefAccess = MUse->getDefiningAccess();
            if (DefAccess) {
              std::string defID = getAccessID(DefAccess);
              dotFile << "    " << defID << " -> " << accessID 
                      << " [label=\"used by\", color=green, style=dashed];\n";
            }
          }
        }
      }
      
      dotFile << "  }\n\n";
    }

    // Close the graph
    dotFile << "}\n";
    dotFile.close();

    errs() << "MemorySSA graph written to: " << filename << "\n";
    errs() << "To visualize: dot -Tpng " << filename << " -o " 
           << F.getName().str() << "_memssa.png\n";
    errs() << "Or view with: dot -Tx11 " << filename << "\n\n";

    // Also print text output
    errs() << "Text representation:\n";
    errs() << "====================\n";
    for (auto &BB : F) {
      errs() << "BasicBlock: " << BB.getName() << "\n";

      if (auto *Phi = MSSA.getMemoryAccess(&BB)) {
        if (auto *MPhi = dyn_cast<MemoryPhi>(Phi)) {
          errs() << "  MemoryPhi for block " << BB.getName() << ":\n";
          for (unsigned i = 0; i < MPhi->getNumIncomingValues(); ++i) {
            auto *IncomingAcc = MPhi->getIncomingValue(i);
            auto *Pred = MPhi->getIncomingBlock(i);
            errs() << "    from " << Pred->getName() << ": ";
            IncomingAcc->print(errs());
            errs() << "\n";
          }
        }
      }

      for (auto &I : BB) {
        if (auto *MA = MSSA.getMemoryAccess(&I)) {
          errs() << "  ";
          MA->print(errs());
          errs() << " -> ";
          I.print(errs());
          errs() << "\n";
        }
      }
    }

    return PreservedAnalyses::all();
  }
};

extern"C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "MemorySSAGraphVisPass", "v1.0",
          [](PassBuilder &PB) {
            PB.registerAnalysisRegistrationCallback(
                [](FunctionAnalysisManager &FAM) {
                  FAM.registerPass([] { return MemorySSAAnalysis(); });
                });

            PB.registerPipelineParsingCallback(
                [](StringRef Name, FunctionPassManager &FPM,
                   ArrayRef<PassBuilder::PipelineElement>) {
                  if (Name == "memssa-graph-vis") {
                    FPM.addPass(MemorySSAGraphVisPass());
                    return true;
                  }
                  return false;
                });
          }};
}