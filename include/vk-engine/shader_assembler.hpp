#pragma once

#include <string>
#include <vector>
#include <ostream>

#include <glslang/Public/ShaderLang.h>

namespace unibox {
    class ShaderAssembler {
        std::string code;
        std::vector<uint32_t> bytecode;

        static TBuiltInResource getResources();
    public:
        ShaderAssembler(const std::string& baseFile);
        ~ShaderAssembler();

        void pragmaInsert(const std::string& pragmaName, const std::string& value);

        std::vector<uint32_t>& compile(EShLanguage language);

        void dump(std::ostream& stream);
    };
}