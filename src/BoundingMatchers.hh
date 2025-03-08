#pragma once

#include "DeclStmtTypeLoc.hh"
#include "MacroExpansionNode.hh"

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/ASTContext.h"

#include <algorithm>
#include <optional>

namespace cpp2c
{
    using namespace clang::ast_matchers;

    void storeChildren(cpp2c::DeclStmtTypeLoc DSTL,
                       std::set<const clang::Stmt *> &MatchedStmts,
                       std::set<const clang::Decl *> &MatchedDecls,
                       std::set<const clang::TypeLoc *> &MatchedTypeLocs);

    // Matches all AST nodes that bounds the given range.
    AST_POLYMORPHIC_MATCHER_P2(
        boundsRange,
        AST_POLYMORPHIC_SUPPORTED_TYPES(clang::Decl,
                                        clang::Stmt,
                                        clang::TypeLoc),
        clang::ASTContext *, Ctx,
        clang::SourceRange, Range)
    {
        // Can't match an invalid location
        if (Node.getBeginLoc().isInvalid() || Node.getEndLoc().isInvalid())
            return false;

        auto DefB = Range.getBegin();
        auto DefE = Range.getEnd();
        if (DefB.isInvalid() || DefE.isInvalid())
            return false;

        // These sets keep track of nodes we have already matched,
        // so that we do not match their subtrees as well
        static std::set<const clang::Stmt *> MatchedStmts; // Also includes Exprs, can be casted
        static std::set<const clang::Decl *> MatchedDecls;
        static std::set<const clang::TypeLoc *> MatchedTypeLocs;
        // Note these are STATIC, so they will persist between invocations
        // This matcher cannot be used twice in the same translation unit

        // Collect a bunch of SourceLocation information up front that may be
        // useful later

        auto &SM = Ctx->getSourceManager();

        // Pre-preprocessing locations
        auto NodeSpB = SM.getSpellingLoc(Node.getBeginLoc());
        auto NodeSpE = SM.getSpellingLoc(Node.getEndLoc());
        // Post-preprocessing locations
        auto NodeExB = SM.getExpansionLoc(Node.getBeginLoc());
        auto NodeExE = SM.getExpansionLoc(Node.getEndLoc());
        auto ImmMacroCallerLocSpB = SM.getSpellingLoc(
            SM.getImmediateMacroCallerLoc(Node.getBeginLoc()));
        auto ImmMacroCallerLocSpE = SM.getSpellingLoc(
            SM.getImmediateMacroCallerLoc(Node.getEndLoc()));
        auto ImmMacroCallerLocExB = SM.getExpansionLoc(
            SM.getImmediateMacroCallerLoc(Node.getBeginLoc()));
        auto ImmMacroCallerLocExE = SM.getExpansionLoc(
            SM.getImmediateMacroCallerLoc(Node.getEndLoc()));
        DeclStmtTypeLoc DSTL(&Node);

        static const constexpr bool debug = false;

        // Preliminary check to ensure that the spelling range of the top
        // level expansion includes the expansion range of the given node
        // NOTE: We may not need this check, but I should doublecheck
        if (!Range.fullyContains(NodeExE))
        {
            if (debug)
            {
                llvm::errs() << "Node mismatch <exp end not in given "
                                "spelling range>\n";
                if (DSTL.ST)
                    DSTL.ST->dumpColor();
                else if (DSTL.D)
                    DSTL.D->dumpColor();
                else if (DSTL.TL)
                {
                    auto QT = DSTL.TL->getType();
                    if (!QT.isNull())
                        QT.dump();
                    else
                        llvm::errs() << "<Null type>\n";
                }
                llvm::errs() << "Given spelling range: ";
                Range.dump(SM);
                llvm::errs() << "NodeSpB: ";
                NodeSpB.dump(SM);
                llvm::errs() << "NodeSpE: ";
                NodeSpE.dump(SM);
                llvm::errs() << "Given range end: ";
                NodeExE.dump(SM);
                llvm::errs() << "Imm macro caller loc: ";
                SM.getImmediateMacroCallerLoc(Node.getBeginLoc()).dump(SM);
                llvm::errs() << "Imm macro caller loc end: ";
                SM.getImmediateMacroCallerLoc(Node.getEndLoc()).dump(SM);
            }
            return false;
        }

        // Check that the beginning of the node we are considering
        // aligns with the beginning of the macro expansion.
        // There are three cases to consider:
        // 1. The node begins with a token that that comes directly from
        //    the body of the macro's definition
        // 2. The node begins with a token that comes from an expansion of
        //    an argument passed to the call to the macro
        // 3. The node begins with a token that comes from an expansion of a
        //    nested macro invocation in the body of the macro's definition

        // bool frontAligned = (DefB == NodeSpB);
        // bool backAligned = (NodeSpE == DefE);
        bool frontBounded = (DefB <= NodeSpB);
        bool backBounded = (NodeSpE <= DefE);

        // Either the node aligns with the macro itself,
        // or one of its arguments.
        if (!frontBounded)
        {
            if (debug)
            {
                llvm::errs() << "Node mismatch <not front bounded>\n";
                if (DSTL.ST)
                    DSTL.ST->dumpColor();
                else if (DSTL.D)
                    DSTL.D->dumpColor();
                else if (DSTL.TL)
                {
                    auto QT = DSTL.TL->getType();
                    if (!QT.isNull())
                        QT.dump();
                    else
                        llvm::errs() << "<Null type>\n";
                }

                auto ImmSpellingLoc = SM.getImmediateSpellingLoc(Node.getBeginLoc());

                llvm::errs() << "Node begin loc: ";
                Node.getBeginLoc().dump(SM);
                llvm::errs() << "NodeExB: ";
                NodeExB.dump(SM);
                llvm::errs() << "NodeSpB: ";
                NodeSpB.dump(SM);
                auto L1 = SM.getImmediateMacroCallerLoc(Node.getBeginLoc());
                auto L2 = SM.getImmediateMacroCallerLoc(L1);
                auto L3 = SM.getImmediateMacroCallerLoc(L2);
                auto L4 = SM.getImmediateMacroCallerLoc(L3);
                llvm::errs() << "Imm macro caller loc x1 (macro id: " << L1.isMacroID() << "): ";
                L1.dump(SM);
                llvm::errs() << "Imm macro caller loc x2 (macro id: " << L2.isMacroID() << "): ";
                L2.dump(SM);
                llvm::errs() << "Imm macro caller loc x3 (macro id: " << L3.isMacroID() << "): ";
                L3.dump(SM);
                llvm::errs() << "Imm macro caller loc x4 (macro id: " << L4.isMacroID() << "): ";
                L4.dump(SM);
                llvm::errs() << "Top macro caller loc: ";
                SM.getTopMacroCallerLoc(Node.getBeginLoc()).dump(SM);
                llvm::errs() << "Imm macro caller expansion loc: ";
                ImmMacroCallerLocExB.dump(SM);
                llvm::errs() << "Imm macro caller spelling loc: ";
                ImmMacroCallerLocSpB.dump(SM);
                llvm::errs() << "Immediate spelling loc: ";
                ImmSpellingLoc.dump(SM);
                llvm::errs() << "DefB: ";
                DefB.dump(SM);
                // if (Expansion->ArgDefBeginsWith &&
                //     !(Expansion->ArgDefBeginsWith->Tokens.empty()))
                // {
                //     llvm::errs() << "ArgDefBeginsWith front token loc: ";
                //     Expansion->ArgDefBeginsWith->Tokens.front()
                //         .getLocation()
                //         .dump(SM);
                // }
            }
            return false;
        }
        if (!backBounded)
        {
            if (debug)
            {
                llvm::errs() << "Node mismatch <not back bounded>\n";
                if (DSTL.ST)
                    DSTL.ST->dumpColor();
                else if (DSTL.D)
                    DSTL.D->dumpColor();
                else if (DSTL.TL)
                {
                    auto QT = DSTL.TL->getType();
                    if (!QT.isNull())
                        QT.dump();
                    else
                        llvm::errs() << "<Null type>\n";
                }

                auto ImmSpellingEndLoc = SM.getImmediateSpellingLoc(Node.getEndLoc());

                llvm::errs() << "Node end loc: ";
                Node.getEndLoc().dump(SM);
                llvm::errs() << "NodeExE: ";
                NodeExE.dump(SM);
                llvm::errs() << "NodeSpE: ";
                NodeSpE.dump(SM);
                llvm::errs() << "Imm macro caller end loc: ";
                SM.getImmediateMacroCallerLoc(Node.getEndLoc()).dump(SM);
                llvm::errs() << "Imm macro caller expansion end loc: ";
                ImmMacroCallerLocExE.dump(SM);
                llvm::errs() << "Imm macro caller spelling end loc: ";
                ImmMacroCallerLocSpE.dump(SM);
                llvm::errs() << "Immediate spelling end loc: ";
                ImmSpellingEndLoc.dump(SM);
                llvm::errs() << "DefE: ";
                DefE.dump(SM);
                // if (Expansion->ArgDefEndsWith &&
                //     !(Expansion->ArgDefEndsWith->Tokens.empty()))
                // {
                //     llvm::errs() << "ArgDefEndsWith back token loc: ";
                //     Expansion->ArgDefEndsWith->Tokens.back()
                //         .getLocation()
                //         .dump(SM);
                // }
            }
            return false;
        }

        if (debug)
        {
            llvm::errs() << "Matched Range with:\n";
            if (DSTL.ST)
                DSTL.ST->dumpColor();
            else if (DSTL.D)
                DSTL.D->dumpColor();
            else if (DSTL.TL)
            {
                auto QT = DSTL.TL->getType();
                if (!QT.isNull())
                    QT.dump();
                else
                    llvm::errs() << "<Null type>\n";
            }
        }
        return true;
    }

    std::optional<DeclStmtTypeLoc> findBoundingASTNodesForRange(
        clang::SourceRange Range,
        clang::ASTContext &Ctx);

}
