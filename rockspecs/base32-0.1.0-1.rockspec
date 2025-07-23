package = "base32"
version = "0.1.0-1"
source = {
    url = "git+https://github.com/mah0x211/lua-base32.git",
    tag = "v0.1.0",
}
description = {
    summary = "base32 encode/decode module",
    homepage = "https://github.com/mah0x211/lua-base32",
    license = "MIT/X11",
    maintainer = "Masatoshi Fukunaga",
}
dependencies = {
    "lua >= 5.1",
    "errno >= 0.5.0",
}
build = {
    type = "make",
    build_variables = {
        SRCDIR = "src",
        CFLAGS = "$(CFLAGS)",
        WARNINGS = "-Wall -Wno-trigraphs -Wmissing-field-initializers -Wreturn-type -Wmissing-braces -Wparentheses -Wno-switch -Wunused-function -Wunused-label -Wunused-parameter -Wunused-variable -Wunused-value -Wuninitialized -Wunknown-pragmas -Wshadow -Wsign-compare",
        CPPFLAGS = "-I$(LUA_INCDIR)",
        LDFLAGS = "$(LIBFLAG)",
        LIB_EXTENSION = "$(LIB_EXTENSION)",
        BASE32_COVERAGE = "$(BASE32_COVERAGE)",
    },
    install_variables = {
        SRCDIR = "src",
        INST_LIBDIR = "$(LIBDIR)",
        LIB_EXTENSION = "$(LIB_EXTENSION)",
    },
}
