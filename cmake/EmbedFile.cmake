# Generates a .cpp defining `sml::ByteView <FUNC>()` over the bytes of a file.
# embedFile(<input> <output.cpp> <function-name> <header-include>)
function(embedFile input output func header)
    file(READ "${input}" hex HEX)
    string(LENGTH "${hex}" len)
    math(EXPR size "${len} / 2")
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${hex}")
    string(REGEX REPLACE "(0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,)" "\\1\n" bytes "${bytes}")
    file(WRITE "${output}.tmp"
        "// Generated from ${input}. Do not edit.\n"
        "#include \"${header}\"\n\n"
        "namespace {\nalignas(16) unsigned char const kData[${size}] = {\n${bytes}};\n}\n\n"
        "sml::ByteView sml::${func}()\n{\n    return {kData, ${size}};\n}\n")
    file(COPY_FILE "${output}.tmp" "${output}" ONLY_IF_DIFFERENT)
    file(REMOVE "${output}.tmp")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${input}")
endfunction()
