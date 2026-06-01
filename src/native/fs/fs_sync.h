#pragma once

namespace qjs {
class Engine;
class Module;
}

void install_fs_sync(qjs::Engine& engine, qjs::Module& sync);
