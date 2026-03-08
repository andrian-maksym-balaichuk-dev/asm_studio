#ifndef ASMSTUDIO_ASMSTUDIO_HPP
#define ASMSTUDIO_ASMSTUDIO_HPP


#include <asmstudio/core/Compat.hpp>

// Core
#include <asmstudio/core/Diagnostic.hpp>
#include <asmstudio/core/SourceLocation.hpp>
#include <asmstudio/core/Types.hpp>

// OOP DSL API
#include <asmstudio/api/Condition.hpp>
#include <asmstudio/api/Expr.hpp>
#include <asmstudio/api/Function.hpp>
#include <asmstudio/api/Program.hpp>
#include <asmstudio/api/Stmt.hpp>
#include <asmstudio/api/Variable.hpp>

// IR
#include <asmstudio/ir/IRTypes.hpp>

// Assembly parser
#include <asmstudio/parser/AsmParse.hpp>

// Studio tools
#include <asmstudio/backend/Arm64Emitter.hpp>
#include <asmstudio/backend/PseudoEmitter.hpp>
#include <asmstudio/explain/Explain.hpp>
#include <asmstudio/presentation/ConsolePresenter.hpp>
#include <asmstudio/simulator/Simulator.hpp>
#include <asmstudio/visualization/CfgDot.hpp>

// Optimizer
#include <asmstudio/optimizer/Optimizer.hpp>


#endif // ASMSTUDIO_ASMSTUDIO_HPP
