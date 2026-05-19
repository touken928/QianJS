#include <gtest/gtest.h>

#include "app/cli.h"

namespace {

char arg_prog[] = "qianjs";

} // namespace

TEST(Cli, SingleArgPrintsUsageReturns1) {
    char* argv[] = {arg_prog};
    EXPECT_EQ(qianjs_cli_run(1, argv), 1);
}

TEST(Cli, HelpAliasReturn0) {
    char h0[] = "help";
    char* a0[] = {arg_prog, h0};
    EXPECT_EQ(qianjs_cli_run(2, a0), 0);

    char h1[] = "-h";
    char* a1[] = {arg_prog, h1};
    EXPECT_EQ(qianjs_cli_run(2, a1), 0);

    char h2[] = "--help";
    char* a2[] = {arg_prog, h2};
    EXPECT_EQ(qianjs_cli_run(2, a2), 0);
}

TEST(Cli, UnknownCommandReturns1) {
    char c[] = "nope";
    char* argv[] = {arg_prog, c};
    EXPECT_EQ(qianjs_cli_run(2, argv), 1);
}

TEST(Cli, BuildMissingFileArgReturns1) {
    char cmd[] = "build";
    char* argv[] = {arg_prog, cmd};
    EXPECT_EQ(qianjs_cli_run(2, argv), 1);
}

TEST(Cli, EmbedMissingFileArgReturns1) {
    char cmd[] = "embed";
    char* argv[] = {arg_prog, cmd};
    EXPECT_EQ(qianjs_cli_run(2, argv), 1);
}

TEST(Cli, RunMissingFileArgReturns1) {
    char cmd[] = "run";
    char* argv[] = {arg_prog, cmd};
    EXPECT_EQ(qianjs_cli_run(2, argv), 1);
}

TEST(Cli, BuildNonexistentPathReturns1) {
    char cmd[] = "build";
    char path[] = "/nonexistent/path/that/does/not/exist/file.js";
    char* argv[] = {arg_prog, cmd, path};
    EXPECT_EQ(qianjs_cli_run(3, argv), 1);
}

TEST(Cli, RunNonexistentPathReturns1) {
    char cmd[] = "run";
    char path[] = "/nonexistent/qianjs_run_target.js";
    char* argv[] = {arg_prog, cmd, path};
    EXPECT_EQ(qianjs_cli_run(3, argv), 1);
}
