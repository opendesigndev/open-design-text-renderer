
#include <gtest/gtest.h>

#include <ode-essentials.h>

#include "TextRendererApiTests.h"

namespace {

/// Parsed command line input.
struct CommandLineInput {
    ode::FilePath fontDirectory;
};

/// Parse command line input and return it. Or print version info (if specified) and return nullopt.
CommandLineInput parseCommandLineArguments(int argc, const char *const *argv) {
    CommandLineInput input;

    for (int i = 1; i < argc; ++i) {
        std::string curArg(argv[i]);
        if (curArg == "--font-assets" || curArg == "--fontpath" || curArg == "--fonts") {
            if (i+1 < argc)
                input.fontDirectory = argv[++i];
        }
    }

    return input;
}

} // namespace

int main(int argc, char** argv) {
    const CommandLineInput commandLineInput = parseCommandLineArguments(argc, argv);
    odtr::test::gFontsDirectory = (std::string)commandLineInput.fontDirectory;

    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
