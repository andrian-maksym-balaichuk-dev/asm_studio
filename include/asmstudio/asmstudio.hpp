#ifndef ASMSTUDIO_ASMSTUDIO_HPP
#define ASMSTUDIO_ASMSTUDIO_HPP


#include <asmstudio/support/Compat.hpp>

// Core
#include <asmstudio/support/Diagnostic.hpp>
#include <asmstudio/support/SourceLocation.hpp>
#include <asmstudio/support/Types.hpp>

// OOP DSL API
#include <asmstudio/dsl/Condition.hpp>
#include <asmstudio/dsl/Expr.hpp>
#include <asmstudio/dsl/Function.hpp>
#include <asmstudio/dsl/Program.hpp>
#include <asmstudio/dsl/Stmt.hpp>
#include <asmstudio/dsl/Variable.hpp>

// IR
#include <asmstudio/ir/IRTypes.hpp>

// Assembly parser
#include <asmstudio/parse/AsmParse.hpp>

// Studio tools
#include <asmstudio/codegen/Arm64Emitter.hpp>
#include <asmstudio/codegen/PseudoEmitter.hpp>
#include <asmstudio/analysis/Explain.hpp>
#include <asmstudio/driver/ConsolePresenter.hpp>
#include <asmstudio/sim/Simulator.hpp>
#include <asmstudio/analysis/CfgDot.hpp>

// Optimizer
#include <asmstudio/transforms/Optimizer.hpp>


#endif // ASMSTUDIO_ASMSTUDIO_HPP
