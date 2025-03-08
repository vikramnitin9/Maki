#include "BoundingMatchers.hh"
#include "ExpansionMatchHandler.hh"

#include <optional>

namespace cpp2c
{
    std::optional<DeclStmtTypeLoc> findBoundingASTNodesForRange(
        clang::SourceRange Range,
        clang::ASTContext &Ctx)
    {
        using namespace clang::ast_matchers;
        // Find AST nodes aligned with the entire invocation

        std::vector<DeclStmtTypeLoc> Matches;

        // Match stmts
        {
            MatchFinder Finder;
            ExpansionMatchHandler Handler;
            auto Matcher = stmt(unless(anyOf(implicitCastExpr(),
                                             implicitValueInitExpr())),
                                boundsRange(&Ctx, Range))
                               .bind("root");
            Finder.addMatcher(Matcher, &Handler);
            Finder.matchAST(Ctx);
            for (auto &&M : Handler.Matches)
                Matches.push_back(M);
        }

        // Match decls
        {
            MatchFinder Finder;
            ExpansionMatchHandler Handler;
            auto Matcher = decl(boundsRange(&Ctx, Range))
                               .bind("root");
            Finder.addMatcher(Matcher, &Handler);
            Finder.matchAST(Ctx);
            for (auto &&M : Handler.Matches)
                Matches.push_back(M);
        }

        // Match type locs
        {
            MatchFinder Finder;
            ExpansionMatchHandler Handler;
            auto Matcher = typeLoc(boundsRange(&Ctx, (Range)))
                               .bind("root");
            Finder.addMatcher(Matcher, &Handler);
            Finder.matchAST(Ctx);
            for (auto &&M : Handler.Matches)
                Matches.push_back(M);
        }

        // In all matches, find the smallest range that fully contains the given range

        std::optional<DeclStmtTypeLoc> BestMatch;
        for (auto &&M : Matches)
        {
            if (!BestMatch)
            {
                BestMatch = M;
                continue;
            }

            auto BestRange = BestMatch->getSourceRange();
            auto CurRange = M.getSourceRange();

            if (CurRange.getBegin() < BestRange.getBegin() &&
                CurRange.getEnd() > BestRange.getEnd())
                BestMatch = M;
        }

        return BestMatch;
    }
} // namespace cpp2c
