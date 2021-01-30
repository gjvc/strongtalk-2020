//
//  (C) 1994 - 2021, The Strongtalk authors and contributors
//  Refer to the "COPYRIGHTS" file at the root of this source tree for complete licence and copyright terms
//

#include "vm/system/asserts.hpp"
#include "vm/memory/allocation.hpp"
#include "vm/utilities/DebugNotifier.hpp"
#include "vm/runtime/flags.hpp"
#include "vm/runtime/ResourceMark.hpp"

#include <gtest/gtest.h>
#include "test/utilities/TestNotifier.hpp"


class errorTests : public ::testing::Test {

protected:
    void SetUp() override {
        mark              = new HeapResourceMark();
        saved             = Notifier::current;
        notifier          = new TestNotifier;
        Notifier::current = notifier;
    }


    void TearDown() override {
        Notifier::current = saved;
        delete mark;
        mark = nullptr;
    }


    HeapResourceMark *mark;
    Notifier         *saved;
    TestNotifier     *notifier;


};


TEST_F( errorTests, strcmp
) {
    ASSERT_EQ( 0, strcmp( "format arg1", "format arg1" ) );
}


TEST_F( errorTests, errorShouldReportErrorWithOneArgToNotifier
) {
    ::error( "format %s", "arg1" );

    ASSERT_EQ( 1, notifier->
        errorCount()
    );
    ASSERT_EQ( 0, strcmp( "format arg1", notifier->errorAt( 0 ) ) );
}


TEST_F( errorTests, errorShouldReportErrorWithTwoArgToNotifier
) {
    ::error( "format %s %s", "arg1", "arg2" );

    ASSERT_EQ( 1, notifier->
        errorCount()
    );
    ASSERT_EQ( 0, strcmp( "format arg1 arg2", notifier->errorAt( 0 ) ) );
}


TEST_F( errorTests, warningShouldReportWarningWithOneArgToNotifier
) {
    ::spdlog::warn( "format %s", "arg1" );

    ASSERT_EQ( 1, notifier->
        warningCount()
    );
    ASSERT_EQ( 0, strcmp( "format arg1", notifier->warningAt( 0 ) ) );
}


TEST_F( errorTests, compiler_warningShouldReportWarningWithOneArgToNotifier
) {
    ::compiler_warning( "format %s", "arg1" );

    ASSERT_EQ( 1, notifier->
        compilerWarningCount()
    );
    ASSERT_EQ( 0, strcmp( "format arg1", notifier->compilerWarningAt( 0 ) ) );
}


TEST_F( errorTests, compiler_warningShouldNotReportWarningWhenPrintCompilerWarningsIsFalse
) {
    FlagSetting fl( PrintCompilerWarnings, false );

    ASSERT_EQ( 0, notifier->
        compilerWarningCount()
    );
    ::compiler_warning( "format %s", "arg1" );
    ASSERT_EQ( 0, notifier->
        compilerWarningCount()
    );
}
