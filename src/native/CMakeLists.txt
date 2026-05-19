# Native modules: catalog in native_modules.cmake (included by root); attach here.

qianjs_native_attach(qianjs_impl)

set(QIANJS_NATIVE_NEED_LIBUV "${QIANJS_NATIVE_NEED_LIBUV}" PARENT_SCOPE)
set(QIANJS_NATIVE_NEED_UI "${QIANJS_NATIVE_NEED_UI}" PARENT_SCOPE)
