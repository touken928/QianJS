#pragma once

#include <qjs/plugin.h>

class FsPlugin final : public qjs::IPlugin {
public:
    const char* name() const override;
    void install(qjs::Context& ctx, qjs::Module& root) override;
};
