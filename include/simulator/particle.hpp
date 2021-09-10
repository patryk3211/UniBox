#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <list>
#include <optional>

#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
#include <simulator/voxel.hpp>
#include <glm/vec4.hpp>

namespace unibox {
    enum ParticlePhaseChangeCondition {
        GREATER,
        LESS
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(ParticlePhaseChangeCondition, {
        { GREATER, "greater" },
        { LESS, "less" }})

    class Particle;
    struct ParticlePhaseChangeProperty {
        float temperature;
        ParticlePhaseChangeCondition condition;
        Particle* result;
    };

    enum ParticleState {
        SOLID = 0b00,
        LIQUID = 0b01,
        GAS = 0b10,
        POWDER = 0b11
    };

    NLOHMANN_JSON_SERIALIZE_ENUM(ParticleState, {
        { GAS, "gas" },
        { LIQUID, "liquid" },
        { SOLID, "solid" },
        { POWDER, "powder" }})

    struct ParticleProperties {
        bool affectedByGravity;
        ParticleState state;
        glm::vec4 color;
        std::vector<ParticlePhaseChangeProperty> phases;
        float density;
    };

    struct ParticleInfoPacket {
        glm::vec4 color;
    };

    struct SimulationParticleInfoPacket {
        float density;
        uint state;
    };

    class Particle {
        static std::unordered_map<std::string, Particle> particles;
        static std::unordered_map<std::string, std::list<Particle**>> deferredPointers;
        static std::vector<Particle*> particlesIdArray;

        static std::string updateInclude, drawInclude;

        ushort typeId;

        std::string displayName;
        std::string description;

        ParticleProperties properties;
        std::optional<std::string> initScript;
        std::optional<std::string> updateScript;
        std::optional<std::string> drawScript;

        bool valid;

        Particle(const std::string& particleDir);

        static void getParticlePtr(const std::string& name, Particle** particle);
        static std::string readFile(const std::string& filepath);
    public:
        Particle();
        ~Particle();

        void initializeVoxel(Voxel& voxel);
        void fillPip(ParticleInfoPacket& pip);
        void fillSimPip(SimulationParticleInfoPacket& simPip);

        bool isValid();

        static Particle& getParticle(const std::string& name);
        static uint getParticleId(const std::string& name);
        static void loadParticle(const std::string& name, const std::string& particleDir);

        static void loadParticles();
        static void loadParticlePack(const std::string& packRoot);

        static std::vector<Particle*>& getParticleArray() { return particlesIdArray; }

        static std::string constructSwitchCode();
        static std::string constructFunctions();
        static std::string& getIncludeCode();

        static std::string constructMeshSwitchCode();
        static std::string constructMeshFunctions();
        static std::string& getMeshIncludeCode();

        static std::string constructTypeDefinitions();
    };
}