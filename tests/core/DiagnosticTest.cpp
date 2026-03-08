#include <asmstudio/core/Diagnostic.hpp>

#include <gtest/gtest.h>
#include <sstream>

using namespace asmstudio;

// ---------------------------------------------------------------------------
// Basic bag state
// ---------------------------------------------------------------------------

TEST(DiagnosticBag, StartsEmpty)
{
    // Given
    DiagnosticBag bag;

    // Then
    EXPECT_TRUE(bag.empty());
    EXPECT_FALSE(bag.hasErrors());
    EXPECT_TRUE(bag.all().empty());
}

TEST(DiagnosticBag, EmitNote)
{
    DiagnosticBag bag;
    bag.emit(Severity::Note, "just a note");
    EXPECT_FALSE(bag.empty());
    EXPECT_FALSE(bag.hasErrors());
    EXPECT_EQ(bag.all().size(), 1u);
    EXPECT_EQ(bag.all()[0].message, "just a note");
    EXPECT_EQ(bag.all()[0].severity, Severity::Note);
}

TEST(DiagnosticBag, EmitWarning)
{
    DiagnosticBag bag;
    bag.emit(Severity::Warning, "watch out");
    EXPECT_FALSE(bag.hasErrors());
    EXPECT_EQ(bag.all()[0].severity, Severity::Warning);
}

TEST(DiagnosticBag, EmitErrorSetsHasErrors)
{
    DiagnosticBag bag;
    bag.emit(Severity::Error, "something went wrong");
    EXPECT_TRUE(bag.hasErrors());
}

TEST(DiagnosticBag, EmitFatalSetsHasErrors)
{
    DiagnosticBag bag;
    bag.emit(Severity::Fatal, "unrecoverable");
    EXPECT_TRUE(bag.hasErrors());
}

TEST(DiagnosticBag, MultipleEntries)
{
    // Given
    DiagnosticBag bag;

    // When
    bag.emit(Severity::Note, "first");
    bag.emit(Severity::Warning, "second");
    bag.emit(Severity::Error, "third");

    // Then
    EXPECT_EQ(bag.all().size(), 3u);
    EXPECT_TRUE(bag.hasErrors());
}

TEST(DiagnosticBag, ClearResetsState)
{
    DiagnosticBag bag;
    bag.emit(Severity::Error, "oops");
    bag.clear();
    EXPECT_TRUE(bag.empty());
    EXPECT_FALSE(bag.hasErrors());
}

TEST(DiagnosticBag, PrintOutputContainsMessage)
{
    // Given
    DiagnosticBag bag;
    bag.emit(Severity::Error, "bad operand");
    std::ostringstream oss;

    // When
    bag.print(oss);

    // Then
    EXPECT_NE(oss.str().find("bad operand"), std::string::npos);
    EXPECT_NE(oss.str().find("error"), std::string::npos);
}

TEST(DiagnosticBag, PrintIncludesLineColWhenSet)
{
    DiagnosticBag bag;
    bag.emit(Severity::Warning, "missing newline", SourceRange{ .line = 10, .col = 5, .length = 1 });
    std::ostringstream oss;
    bag.print(oss);
    EXPECT_NE(oss.str().find("10"), std::string::npos);
    EXPECT_NE(oss.str().find("5"), std::string::npos);
}
