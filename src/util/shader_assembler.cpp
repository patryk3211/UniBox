#include <util/shader_assembler.hpp>

#include <istream>
#include <fstream>
#include <sstream>

#include <spdlog/spdlog.h>
#include <glslang/SPIRV/GlslangToSpv.h>

using namespace unibox;

ShaderAssembler::ShaderAssembler(const std::string& baseFile) {
    std::ifstream fileStream = std::ifstream(baseFile, std::ios::binary);
    std::stringstream fileString;
    fileString << fileStream.rdbuf();

    code = fileString.str();
}

ShaderAssembler::~ShaderAssembler() {

}

void ShaderAssembler::pragmaInsert(const std::string& pragmaName, const std::string& value) {
    size_t pos = code.find("#pragma " + pragmaName);
    if(pos == std::string::npos) {
        spdlog::error("Could not find '#pragma " + pragmaName + "' in code.");
        return;
    }
    
    code.replace(pos, 8+pragmaName.length(), value);
}

std::vector<uint32_t>& ShaderAssembler::compile(EShLanguage language) {
    if(bytecode.size() != 0) return bytecode;
    
    glslang::TProgram program;
    glslang::TShader shader(language);

    const char* source = code.c_str();
    const char* names[] = { "shader" };
    shader.setStringsWithLengthsAndNames(&source, 0, names, 1);

    auto version = glslang::EShTargetClientVersion::EShTargetVulkan_1_0;

    shader.setEnvInput(glslang::EShSource::EShSourceGlsl, language, glslang::EShClient::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClient::EShClientVulkan, version);
    shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_0);

    auto messages = static_cast<EShMessages>(EShMessages::EShMsgDefault | EShMessages::EShMsgVulkanRules | EShMessages::EShMsgSpvRules);

    TBuiltInResource resources = getResources();

    //std::string str;
    //if(!shader.preprocess(&resources, version, EProfile::ENoProfile, false, false, messages, &str,))

    if(!shader.parse(&resources, version, true, messages)) {
        spdlog::info(shader.getInfoLog());
        spdlog::info(shader.getInfoDebugLog());
        spdlog::error("Shader parsing failed.");
        return bytecode;
    }

    program.addShader(&shader);

    if(!program.link(messages) || !program.mapIO()) {
        spdlog::error("Shader linking failed.");
        return bytecode;
    }
    
    glslang::GlslangToSpv(*program.getIntermediate(language), bytecode);
    return bytecode;
}

cl::Program* ShaderAssembler::compile(const cl::Context& context, const cl::Device& device) {
	cl::Program::Sources source(1, std::make_pair(code.c_str(), code.size()+1));
	cl::Program* program = new cl::Program(context, source);
	
	if(program->build() != CL_BUILD_SUCCESS) {
		spdlog::error("Could not build OpenCL program. Status: " + program->getBuildInfo<CL_PROGRAM_BUILD_STATUS>(device));
		spdlog::error("Log: " + program->getBuildInfo<CL_PROGRAM_BUILD_LOG>(device));
		delete program;
		return 0;
	}

	return program;
}

void ShaderAssembler::dump(std::ostream& stream) {
    stream << code;
}

TBuiltInResource ShaderAssembler::getResources() {
    TBuiltInResource resources = {};
	resources.maxLights = 32;
	resources.maxClipPlanes = 6;
	resources.maxTextureUnits = 32;
	resources.maxTextureCoords = 32;
	resources.maxVertexAttribs = 64;
	resources.maxVertexUniformComponents = 4096;
	resources.maxVaryingFloats = 64;
	resources.maxVertexTextureImageUnits = 32;
	resources.maxCombinedTextureImageUnits = 80;
	resources.maxTextureImageUnits = 32;
	resources.maxFragmentUniformComponents = 4096;
	resources.maxDrawBuffers = 32;
	resources.maxVertexUniformVectors = 128;
	resources.maxVaryingVectors = 8;
	resources.maxFragmentUniformVectors = 16;
	resources.maxVertexOutputVectors = 16;
	resources.maxFragmentInputVectors = 15;
	resources.minProgramTexelOffset = -8;
	resources.maxProgramTexelOffset = 7;
	resources.maxClipDistances = 8;
	resources.maxComputeWorkGroupCountX = 65535;
	resources.maxComputeWorkGroupCountY = 65535;
	resources.maxComputeWorkGroupCountZ = 65535;
	resources.maxComputeWorkGroupSizeX = 1024;
	resources.maxComputeWorkGroupSizeY = 1024;
	resources.maxComputeWorkGroupSizeZ = 64;
	resources.maxComputeUniformComponents = 1024;
	resources.maxComputeTextureImageUnits = 16;
	resources.maxComputeImageUniforms = 8;
	resources.maxComputeAtomicCounters = 8;
	resources.maxComputeAtomicCounterBuffers = 1;
	resources.maxVaryingComponents = 60;
	resources.maxVertexOutputComponents = 64;
	resources.maxGeometryInputComponents = 64;
	resources.maxGeometryOutputComponents = 128;
	resources.maxFragmentInputComponents = 128;
	resources.maxImageUnits = 8;
	resources.maxCombinedImageUnitsAndFragmentOutputs = 8;
	resources.maxCombinedShaderOutputResources = 8;
	resources.maxImageSamples = 0;
	resources.maxVertexImageUniforms = 0;
	resources.maxTessControlImageUniforms = 0;
	resources.maxTessEvaluationImageUniforms = 0;
	resources.maxGeometryImageUniforms = 0;
	resources.maxFragmentImageUniforms = 8;
	resources.maxCombinedImageUniforms = 8;
	resources.maxGeometryTextureImageUnits = 16;
	resources.maxGeometryOutputVertices = 256;
	resources.maxGeometryTotalOutputComponents = 1024;
	resources.maxGeometryUniformComponents = 1024;
	resources.maxGeometryVaryingComponents = 64;
	resources.maxTessControlInputComponents = 128;
	resources.maxTessControlOutputComponents = 128;
	resources.maxTessControlTextureImageUnits = 16;
	resources.maxTessControlUniformComponents = 1024;
	resources.maxTessControlTotalOutputComponents = 4096;
	resources.maxTessEvaluationInputComponents = 128;
	resources.maxTessEvaluationOutputComponents = 128;
	resources.maxTessEvaluationTextureImageUnits = 16;
	resources.maxTessEvaluationUniformComponents = 1024;
	resources.maxTessPatchComponents = 120;
	resources.maxPatchVertices = 32;
	resources.maxTessGenLevel = 64;
	resources.maxViewports = 16;
	resources.maxVertexAtomicCounters = 0;
	resources.maxTessControlAtomicCounters = 0;
	resources.maxTessEvaluationAtomicCounters = 0;
	resources.maxGeometryAtomicCounters = 0;
	resources.maxFragmentAtomicCounters = 8;
	resources.maxCombinedAtomicCounters = 8;
	resources.maxAtomicCounterBindings = 1;
	resources.maxVertexAtomicCounterBuffers = 0;
	resources.maxTessControlAtomicCounterBuffers = 0;
	resources.maxTessEvaluationAtomicCounterBuffers = 0;
	resources.maxGeometryAtomicCounterBuffers = 0;
	resources.maxFragmentAtomicCounterBuffers = 1;
	resources.maxCombinedAtomicCounterBuffers = 1;
	resources.maxAtomicCounterBufferSize = 16384;
	resources.maxTransformFeedbackBuffers = 4;
	resources.maxTransformFeedbackInterleavedComponents = 64;
	resources.maxCullDistances = 8;
	resources.maxCombinedClipAndCullDistances = 8;
	resources.maxSamples = 4;
	resources.limits.nonInductiveForLoops = true;
	resources.limits.whileLoops = true;
	resources.limits.doWhileLoops = true;
	resources.limits.generalUniformIndexing = true;
	resources.limits.generalAttributeMatrixVectorIndexing = true;
	resources.limits.generalVaryingIndexing = true;
	resources.limits.generalSamplerIndexing = true;
	resources.limits.generalVariableIndexing = true;
	resources.limits.generalConstantMatrixVectorIndexing = true;
	return resources;
}