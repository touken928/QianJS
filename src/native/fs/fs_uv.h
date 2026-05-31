#pragma once

#include <qjs/value.h>

#include <string>
#include <vector>

namespace qjs {
class Engine;
}

qjs::Value fsReadFileAsync(qjs::Engine& engine, std::string path, bool asBuffer);

qjs::Value fsWriteFileAsync(qjs::Engine& engine, std::string path, std::vector<uint8_t> data);

qjs::Value fsMkdirAsync(qjs::Engine& engine, std::string path, bool recursive);

qjs::Value fsReaddirAsync(qjs::Engine& engine, std::string path);

qjs::Value fsStatAsync(qjs::Engine& engine, std::string path);

qjs::Value fsUnlinkAsync(qjs::Engine& engine, std::string path);

qjs::Value fsRmdirAsync(qjs::Engine& engine, std::string path);
