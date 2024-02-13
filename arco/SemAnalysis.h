#ifndef ARCO_SEM_ANALYSIS_H
#define ARCO_SEM_ANALYSIS_H

#include "AST.h"
#include "Logger.h"

namespace arco {

    class ArcoContext;

    class SemAnalyzer {
    public:

        explicit SemAnalyzer(ArcoContext& Context, Decl* D);

        static void ReportStatementsInInvalidContext(FileScope* FScope);

        static void ResolveImports(FileScope* FScope, ArcoContext& Context);

        static void CheckForDuplicateFuncDeclarations(Module* Mod);
        static void CheckForDuplicateFuncDeclarations(Namespace* NSpace);

        void CheckFuncDecl(FuncDecl* Func);
        
        void CheckStructDecl(StructDecl* Struct);
        void CheckEnumDecl(EnumDecl* Enum);

        void CheckVarDecl(VarDecl* Var);

    private:
        ArcoContext& Context;
        Module*      Mod;
        Logger       Log;

        FileScope*  FScope;
        FuncDecl*   CFunc   = nullptr;
        StructDecl* CStruct = nullptr;
        VarDecl*    CGlobal = nullptr;
        VarDecl*    CField  = nullptr;

        // Every time a loop is entered this is incremented,
        // and decremented when existed
        ulen LoopDepth = 0;

        struct Scope {
            Scope* Parent = nullptr;
            // This refers to anything that
            // prevents the usual flow of
            // execution such as 'break',
            // 'continue', 'return'
            bool FoundTerminal = false;
            // Keeping track of returns
            // as a way of find out
            // if a function definitatively
            // returns.
            bool AllPathsReturn = false;
        }  * LocScope = nullptr;

        void CheckFuncParams(FuncDecl* Func);

        void CheckNode(AstNode* Node);

        //===-------------------------------===//
        // Statements
        //===-------------------------------===//

        void CheckScopeStmts(LexScope& LScope, Scope& NewScope);
        void CheckReturn(ReturnStmt* Return);
        void CheckLoopControl(LoopControlStmt* LoopControl);
        void CheckPredicateLoop(PredicateLoopStmt* Loop);
        void CheckRangeLoop(RangeLoopStmt* Loop);
        void CheckIteratorLoop(IteratorLoopStmt* Loop);
        void CheckDeleteStmt(DeleteStmt* Delete);
        bool CheckIf(IfStmt* If);
        bool CheckNestedScope(NestedScopeStmt* NestedScope);

        //===-------------------------------===//
        // Expressions
        //===-------------------------------===//

        void CheckBinaryOp(BinaryOp* BinOp);
        void CheckUnaryOp(UnaryOp* UniOp);
        void CheckIdentRef(IdentRef* IRef,
                           bool ExpectsFuncCall,
                           Namespace* NamespaceToLookup,
                           StructDecl* StructToLookup = nullptr);
        void CheckFieldAccessor(FieldAccessor* FieldAcc, bool ExpectsFuncCall);
        void CheckThisRef(ThisRef* This);
        void CheckFuncCall(FuncCall* Call);
        FuncDecl* CheckCallToCanidates(SourceLoc ErrorLoc,
                                       FuncsList* Canidates,
                                       llvm::SmallVector<NonNamedValue, 2>& Args);
        FuncDecl* FindBestFuncCallCanidate(FuncsList* Canidates,
                                           const llvm::SmallVector<NonNamedValue, 2>& Args);
        bool CompareAsCanidate(FuncDecl* Canidate,
                               const llvm::SmallVector<NonNamedValue, 2>& Args,
                               ulen& NumConflicts,
                               ulen& EnumImplicitConflicts);
        void DisplayErrorForNoMatchingFuncCall(SourceLoc ErrorLoc,
                                               FuncsList* Canidates,
                                               const llvm::SmallVector<NonNamedValue, 2>& Args);
        void DisplayErrorForSingleFuncForFuncCall(
            const char* CallType,
            SourceLoc Loc,
            const llvm::SmallVector<TypeInfo>& ParamTypes,
            const llvm::SmallVector<NonNamedValue, 2>& Args,
            ulen NumDefaultArgs = 0,
            FuncDecl* CalledFunc = nullptr
        );
        std::string GetFuncDefForError(const llvm::SmallVector<TypeInfo>& ParamTypes, FuncDecl* CalledFunc);
        std::string GetCallMismatchInfo(const llvm::SmallVector<TypeInfo>& ParamTypes,
                                        const llvm::SmallVector<NonNamedValue, 2>& Args,
                                        ulen NumDefaultArgs);
        
        void CheckArray(Array* Arr);
        void CheckArrayAccess(ArrayAccess* Access);
        void CheckTypeCast(TypeCast* Cast);
        void CheckStructInitializer(StructInitializer* StructInit);
        FuncDecl* CheckStructInitArgs(StructDecl* Struct,
                                      SourceLoc ErrorLoc,
                                      llvm::SmallVector<NonNamedValue, 2>& Args);
        void CheckHeapAlloc(HeapAlloc* Alloc);
        void CheckSizeOf(SizeOf* SOf);
        void CheckTypeOf(TypeOf* TOf);

        void CheckCondition(Expr* Cond, const char* PreErrorText);

        void CreateCast(Expr* E, Type* ToType);

        bool IsAssignableTo(Type* ToTy, Expr* FromExpr);
        bool IsAssignableTo(Type* ToTy, Type* FromTy, Expr* FromExpr);
        bool IsCastableTo(Type* ToTy, Type* FromTy);
        bool ViolatesConstAssignment(VarDecl* DestVar, Expr* Assignment);
        bool ViolatesConstAssignment(Type* DestTy, bool DestConstAddress, Expr* Assignment);

        bool FixupType(Type* Ty, bool AllowDynamicArrays = false);
        bool FixupArrayType(ArrayType* ArrayTy, bool AllowDynamic);
        bool FixupStructType(StructType* StructTy);

        void CheckModifibility(Expr* LValue);
        bool IsLValue(Expr* E);

        void EnsureChecked(SourceLoc ErrLoc, VarDecl* Var);

        void DisplayCircularDepError(SourceLoc ErrLoc, VarDecl* StartDep, const char* ErrHeader);

        static void CheckForDuplicateFuncs(const FuncsList& FuncList);

        bool IsComparable(Type* Ty);

        void DisplayNoteInfoForTypeMismatch(Expr* FromExpr, Type* ToTy);
        void DisplayErrorForTypeMismatch(const char* ErrMsg, SourceLoc ErrorLoc,
                                         Expr* FromExpr, Type* ToTy);

        void Error(SourceLoc Loc, const char* Msg) {
            Log.BeginError(Loc, Msg);
            Log.EndError();
        }

        template<typename... TArgs>
        void Error(SourceLoc Loc, const char* Fmt, TArgs&&... Args) {
            Log.BeginError(Loc, Fmt, std::forward<TArgs>(Args)...);
            Log.EndError();
        }

        void Error(AstNode* Node, const char* Msg) {
            Error(Node->Loc, Msg);
        }

        template<typename... TArgs>
        void Error(AstNode* Node, const char* Fmt, TArgs&&... Args) {
            Error(Node->Loc, Fmt, std::forward<TArgs>(Args)...);
        }
    };

}

#endif // ARCO_SEM_ANALYSIS_H