#include <qianjs_modules.h>

#if QIANJS_MODULE_FS

#include <gtest/gtest.h>

#include "native/js_test_harness.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>

namespace {

namespace fs = std::filesystem;

fs::path make_temp_root(const char* prefix) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    return fs::temp_directory_path() / (std::string(prefix) + "_" + std::to_string(stamp));
}

void cleanup(fs::path dir) {
    std::error_code ec;
    fs::remove_all(dir, ec);
}

} // namespace

TEST(NativeFsModule, AsyncReadFileAndReadFileBytes) {
    const fs::path root = make_temp_root("qianjs_fs_rf");
    std::error_code ec;
    fs::create_directories(root, ec);
    ASSERT_FALSE(ec) << ec.message();
    const fs::path file = root / "bin.dat";
    {
        FILE* f = std::fopen(file.string().c_str(), "wb");
        ASSERT_NE(f, nullptr);
        const unsigned char b[] = {0xffu, 0u, 0x80u};
        ASSERT_EQ(std::fwrite(b, 1, 3, f), 3u);
        std::fclose(f);
    }

    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    const std::string p = qianjs::test::js_string_literal(file.string());
    const std::string js = "import { readFileBytes, unlink } from 'fs';\nconst p = " + p
        + ";\nreadFileBytes(p).then((buf) => {\n"
          "  const u = new Uint8Array(buf);\n"
          "  globalThis.__b0 = u[0];\n"
          "  globalThis.__b1 = u[1];\n"
          "  globalThis.__b2 = u[2];\n"
          "  return unlink(p);\n"
          "}).then(() => { globalThis.__rf_bytes_done = 1; });\n";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_fs_async_bytes.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    bool ok = false;
    EXPECT_EQ(qianjs::test::global_int(c, "__rf_bytes_done", &ok), 1);
    EXPECT_TRUE(ok);
    EXPECT_EQ(qianjs::test::global_int(c, "__b0", &ok), 255);
    EXPECT_TRUE(ok);
    EXPECT_EQ(qianjs::test::global_int(c, "__b1", &ok), 0);
    EXPECT_TRUE(ok);
    EXPECT_EQ(qianjs::test::global_int(c, "__b2", &ok), 128);
    EXPECT_TRUE(ok);

    engine.cleanup();
    cleanup(root);
}

TEST(NativeFsModule, AsyncWriteFileStringUnicodeAndReadBack) {
    const fs::path root = make_temp_root("qianjs_fs_utf");
    std::error_code ec;
    fs::create_directories(root, ec);
    ASSERT_FALSE(ec) << ec.message();

    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    const std::string p = qianjs::test::js_string_literal((root / "u.txt").string());
    const std::string js =
        "import { writeFile, readFile, unlink } from 'fs';\nconst p = " + p + ";\n"
        "writeFile(p, '\\u4f60\\u597d').then(() => readFile(p)).then((t) => {\n"
        "  globalThis.__utf = t;\n"
        "  return unlink(p);\n"
        "}).then(() => { globalThis.__utf_done = 1; });\n";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_fs_utf8.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    const std::string want = u8"\u4f60\u597d";
    EXPECT_EQ(qianjs::test::global_string(c, "__utf"), want);
    bool ok = false;
    EXPECT_EQ(qianjs::test::global_int(c, "__utf_done", &ok), 1);
    EXPECT_TRUE(ok);

    engine.cleanup();
    cleanup(root);
}

TEST(NativeFsModule, AsyncWriteFileUint8Array) {
    const fs::path root = make_temp_root("qianjs_fs_u8");
    std::error_code ec;
    fs::create_directories(root, ec);
    ASSERT_FALSE(ec) << ec.message();

    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    const std::string p = qianjs::test::js_string_literal((root / "raw.bin").string());
    const std::string js = "import { writeFile, readFileBytes, unlink } from 'fs';\nconst p = " + p + ";\n"
                           "writeFile(p, new Uint8Array([1, 2, 3]))\n"
                           "  .then(() => readFileBytes(p))\n"
                           "  .then((buf) => {\n"
                           "    const u = new Uint8Array(buf);\n"
                           "    globalThis.__u8 = u[0] + u[1] + u[2];\n"
                           "    return unlink(p);\n"
                           "  })\n"
                           "  .then(() => { globalThis.__u8_done = 1; });\n";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_fs_u8.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    bool ok = false;
    EXPECT_EQ(qianjs::test::global_int(c, "__u8", &ok), 6);
    EXPECT_TRUE(ok);
    EXPECT_EQ(qianjs::test::global_int(c, "__u8_done", &ok), 1);
    EXPECT_TRUE(ok);

    engine.cleanup();
    cleanup(root);
}

TEST(NativeFsModule, AsyncMkdirReaddirStatUnlinkRmdir) {
    const fs::path root = make_temp_root("qianjs_fs_tree");
    std::error_code ec;
    fs::create_directories(root, ec);
    ASSERT_FALSE(ec) << ec.message();

    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    const std::string b = qianjs::test::js_string_literal(root.string());
    const std::string js =
        "import { mkdir, writeFile, readdir, stat, unlink, rmdir } from 'fs';\n"
        "const base = " +
        b +
        ";\n"
        "mkdir(base + '/d')\n"
        "  .then(() => writeFile(base + '/d/a.txt', 'a'))\n"
        "  .then(() => writeFile(base + '/d/b.txt', 'b'))\n"
        "  .then(() => readdir(base + '/d'))\n"
        "  .then((names) => {\n"
        "    globalThis.__rd = JSON.stringify(names.slice().sort());\n"
        "    return stat(base + '/d/a.txt');\n"
        "  })\n"
        "  .then((st) => {\n"
        "    globalThis.__st = JSON.stringify({ isFile: st.isFile, isDirectory: st.isDirectory, size: "
        "st.size });\n"
        "    return unlink(base + '/d/a.txt');\n"
        "  })\n"
        "  .then(() => unlink(base + '/d/b.txt'))\n"
        "  .then(() => rmdir(base + '/d'))\n"
        "  .then(() => { globalThis.__tree_done = 1; });\n";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_fs_tree.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    EXPECT_EQ(qianjs::test::global_string(c, "__rd"), R"(["a.txt","b.txt"])");
    const std::string st = qianjs::test::global_string(c, "__st");
    EXPECT_NE(st.find("\"isFile\":true"), std::string::npos);
    EXPECT_NE(st.find("\"isDirectory\":false"), std::string::npos);
    EXPECT_NE(st.find("\"size\":1"), std::string::npos);
    bool ok = false;
    EXPECT_EQ(qianjs::test::global_int(c, "__tree_done", &ok), 1);
    EXPECT_TRUE(ok);

    engine.cleanup();
    cleanup(root);
}

TEST(NativeFsModule, AsyncMkdirRecursive) {
    const fs::path root = make_temp_root("qianjs_fs_mkrec");
    std::error_code ec;
    fs::create_directories(root, ec);
    ASSERT_FALSE(ec) << ec.message();

    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    const std::string b = qianjs::test::js_string_literal(root.string());
    const std::string js = "import { mkdirRecursive, stat, rmdir } from 'fs';\nconst base = " + b + ";\n"
                           "mkdirRecursive(base + '/x/y/z')\n"
                           "  .then(() => stat(base + '/x/y/z'))\n"
                           "  .then((st) => {\n"
                           "    globalThis.__zdir = st.isDirectory;\n"
                           "  })\n"
                           "  .then(() => rmdir(base + '/x/y/z'))\n"
                           "  .then(() => rmdir(base + '/x/y'))\n"
                           "  .then(() => rmdir(base + '/x'))\n"
                           "  .then(() => { globalThis.__mkrec_done = 1; });\n";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_fs_mkrec.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    bool ok = false;
    EXPECT_EQ(qianjs::test::global_int(c, "__zdir", &ok), 1);
    EXPECT_TRUE(ok);
    EXPECT_EQ(qianjs::test::global_int(c, "__mkrec_done", &ok), 1);
    EXPECT_TRUE(ok);

    engine.cleanup();
    cleanup(root);
}

TEST(NativeFsModule, AsyncWriteFileInvalidDataRejects) {
    const fs::path root = make_temp_root("qianjs_fs_badwf");
    std::error_code ec;
    fs::create_directories(root, ec);
    ASSERT_FALSE(ec) << ec.message();

    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    const std::string p = qianjs::test::js_string_literal((root / "x.bin").string());
    const std::string js = "import { writeFile } from 'fs';\nconst p = " + p + ";\n"
                           "try {\n"
                           "  writeFile(p, 123);\n"
                           "  globalThis.__wf_bad = 'no_throw';\n"
                           "} catch (e) {\n"
                           "  globalThis.__wf_bad = String(e);\n"
                           "}\n";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_fs_badwf.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    EXPECT_NE(qianjs::test::global_string(c, "__wf_bad").find("writeFile"), std::string::npos);

    engine.cleanup();
    cleanup(root);
}

TEST(NativeFsModule, AsyncReadMissingRejects) {
    const fs::path root = make_temp_root("qianjs_fs_miss");
    std::error_code ec;
    fs::create_directories(root, ec);
    ASSERT_FALSE(ec) << ec.message();

    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    const std::string p = qianjs::test::js_string_literal((root / "nope_not_here.txt").string());
    const std::string js = "import { readFile } from 'fs';\nconst p = " + p + ";\n"
                           "readFile(p).catch((e) => { globalThis.__rej = String(e); });\n";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_fs_rej.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    EXPECT_NE(qianjs::test::global_string(c, "__rej"), "");

    engine.cleanup();
    cleanup(root);
}

TEST(NativeFsModule, SyncReadWriteMkdirReaddirStatUnlinkRmdir) {
    const fs::path root = make_temp_root("qianjs_fs_sync");
    std::error_code ec;
    fs::create_directories(root, ec);
    ASSERT_FALSE(ec) << ec.message();

    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    const std::string b = qianjs::test::js_string_literal(root.string());
    const std::string js =
        "import { sync as fss } from 'fs';\n"
        "const base = " +
        b +
        ";\n"
        "fss.mkdirRecursive(base + '/s/a');\n"
        "fss.writeFile(base + '/s/a/t.txt', 'sync');\n"
        "globalThis.__sr = fss.readFile(base + '/s/a/t.txt');\n"
        "const buf = fss.readFileBytes(base + '/s/a/t.txt');\n"
        "globalThis.__sblen = buf.byteLength;\n"
        "fss.writeFile(base + '/s/a/u.bin', new Uint8Array([9]));\n"
        "const names = fss.readdir(base + '/s/a').slice().sort();\n"
        "globalThis.__sdir = JSON.stringify(names);\n"
        "const st = fss.stat(base + '/s/a/t.txt');\n"
        "globalThis.__sst = JSON.stringify({ isFile: st.isFile, size: st.size });\n"
        "fss.unlink(base + '/s/a/u.bin');\n"
        "fss.unlink(base + '/s/a/t.txt');\n"
        "fss.rmdir(base + '/s/a');\n"
        "fss.rmdir(base + '/s');\n"
        "globalThis.__sync_ok = 1;\n";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_fs_sync.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    EXPECT_EQ(qianjs::test::global_string(c, "__sr"), "sync");
    bool ok = false;
    EXPECT_EQ(qianjs::test::global_int(c, "__sblen", &ok), 4);
    EXPECT_TRUE(ok);
    EXPECT_EQ(qianjs::test::global_string(c, "__sdir"), R"(["t.txt","u.bin"])");
    EXPECT_NE(qianjs::test::global_string(c, "__sst").find("\"isFile\":true"), std::string::npos);
    EXPECT_EQ(qianjs::test::global_int(c, "__sync_ok", &ok), 1);
    EXPECT_TRUE(ok);

    engine.cleanup();
    cleanup(root);
}

TEST(NativeFsModule, SyncStatDirectoryShape) {
    const fs::path root = make_temp_root("qianjs_fs_stdir");
    std::error_code ec;
    fs::create_directories(root / "sub", ec);
    ASSERT_FALSE(ec) << ec.message();

    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    const std::string d = qianjs::test::js_string_literal((root / "sub").string());
    const std::string js = "import { sync as fss } from 'fs';\nconst d = " + d + ";\n"
                           "const st = fss.stat(d);\n"
                           "globalThis.__stdir = JSON.stringify({ isFile: st.isFile, isDirectory: st.isDirectory, "
                           "isSymbolicLink: st.isSymbolicLink });\n";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_fs_stdir.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    const std::string s = qianjs::test::global_string(c, "__stdir");
    EXPECT_NE(s.find("\"isFile\":false"), std::string::npos);
    EXPECT_NE(s.find("\"isDirectory\":true"), std::string::npos);

    engine.cleanup();
    cleanup(root);
}

TEST(NativeFsModule, SyncRmdirNonemptyThrows) {
    const fs::path root = make_temp_root("qianjs_fs_rmfull");
    std::error_code ec;
    fs::create_directories(root / "k", ec);
    ASSERT_FALSE(ec) << ec.message();
    {
        FILE* f = std::fopen((root / "k" / "f.txt").string().c_str(), "wb");
        ASSERT_NE(f, nullptr);
        std::fputc('x', f);
        std::fclose(f);
    }

    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    const std::string d = qianjs::test::js_string_literal((root / "k").string());
    const std::string js = "import { sync as fss } from 'fs';\nconst d = " + d + ";\n"
                           "try { fss.rmdir(d); globalThis.__rmdir_ne = 'no'; }\n"
                           "catch (e) { globalThis.__rmdir_ne = String(e); }\n";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_fs_rmfull.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    EXPECT_NE(qianjs::test::global_string(c, "__rmdir_ne").find("rmdir"), std::string::npos);

    engine.cleanup();
    cleanup(root);
}

TEST(NativeFsModule, SyncReadMissingThrows) {
    const fs::path root = make_temp_root("qianjs_fs_sync_miss");
    std::error_code ec;
    fs::create_directories(root, ec);
    ASSERT_FALSE(ec) << ec.message();

    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    const std::string p = qianjs::test::js_string_literal((root / "missing.txt").string());
    const std::string js = "import { sync as fss } from 'fs';\nconst p = " + p + ";\n"
                           "try { fss.readFile(p); globalThis.__sync_miss = 'no'; }\n"
                           "catch (e) { globalThis.__sync_miss = String(e); }\n";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_fs_sync_miss.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    EXPECT_NE(qianjs::test::global_string(c, "__sync_miss").find("readFile"), std::string::npos);

    engine.cleanup();
    cleanup(root);
}

#endif // QIANJS_MODULE_FS
