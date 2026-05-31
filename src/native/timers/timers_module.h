#pragma once

#include <qjs/plugin.h>

class TimersPlugin final : public qjs::IPlugin {
public:
    const char* name() const override;
    void install(qjs::Context& ctx, qjs::Module& root) override;
};
