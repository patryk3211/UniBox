#include <simulator/particle.hpp>

#include <spdlog/spdlog.h>

#include <istream>
#include <fstream>
#include <algorithm>
#include <regex>

#include <util/base64.hpp>
#include <stb/stb_image.h>

using namespace unibox;
using namespace nlohmann;

std::unordered_map<std::string, Particle> Particle::particles = std::unordered_map<std::string, Particle>();
std::unordered_map<std::string, std::list<Particle**>> Particle::deferredPointers = std::unordered_map<std::string, std::list<Particle**>>();
std::vector<Particle*> Particle::particlesIdArray = std::vector<Particle*>();
std::string Particle::updateInclude;
std::string Particle::drawInclude;
TextureAtlas* Particle::iconAtlas = 0;
TextureAtlas::Coordinate Particle::missingIconCoord;
TextureAtlas::Coordinate Particle::invalidIconCoord;

Particle::Particle() {
    valid = false;
}

Particle::Particle(const std::string& particleDir) {
    valid = false;

    if(iconAtlas == 0) {
        iconAtlas = new TextureAtlas(2048, 2048);
        uint8_t pixels[] = {
            0xFF, 0x00, 0xFF, 0xFF,   0x00, 0x00, 0x00, 0xFF,
            0x00, 0x00, 0x00, 0xFF,   0xFF, 0x00, 0xFF, 0xFF
        };
        missingIconCoord = iconAtlas->storeTexture(2, 2, pixels);
        uint8_t pixels2[] = {
            0xFF, 0x00, 0x00, 0xFF,   0x00, 0x00, 0x00, 0xFF,
            0x00, 0x00, 0x00, 0xFF,   0xFF, 0x00, 0x00, 0xFF
        };
        invalidIconCoord = iconAtlas->storeTexture(2, 2, pixels2);
    }

    json propJson;
    std::ifstream propStream = std::ifstream(particleDir + "/properties.json", std::ios::binary);
    if(!propStream.is_open()) {
        spdlog::error("Failed to open particle properties file in directory '" + particleDir + "'.");
        return;
    }
    propStream >> propJson;
    propStream.close();

    {
        // Load the element general information.
        auto displayNameJson = propJson.find("displayName");
        if(displayNameJson == propJson.end()) {
            spdlog::warn("Failed to get the display name of a particle in directory '" + particleDir + "'.");
            displayName = "MISSING_NAME";
        } else displayName = displayNameJson->get<std::string>();

        auto descriptionJson = propJson.find("description");
        if(descriptionJson == propJson.end()) description = displayName;
        else description = descriptionJson->get<std::string>();
    }

    {
        // Load the element properties.
        auto gravityJson = propJson.find("gravity");
        if(gravityJson == propJson.end()) properties.affectedByGravity = false;
        else properties.affectedByGravity = gravityJson->get<bool>();

        auto stateJson = propJson.find("state");
        if(stateJson == propJson.end()) properties.state = POWDER;
        else properties.state = stateJson->get<ParticleState>();

        auto colorJson = propJson.find("color");
        if(colorJson == propJson.end()) properties.color = glm::vec4(1.0, 0.0, 1.0, 1.0);
        else {
            if(!colorJson->is_array()) {
                spdlog::warn("Color property is not a JSON Array in particle '" + displayName + "' (directory '" + particleDir + "').");
                properties.color = glm::vec4(1.0, 0.0, 1.0, 1.0);
            } else {
                if(colorJson->size() < 3) {
                    spdlog::warn("Color property contains less than 3 entries in '" + displayName + "' (directory '" + particleDir + "').");
                    properties.color = glm::vec4(1.0, 0.0, 1.0, 1.0);
                } else if(colorJson->size() == 3) {
                    auto el0 = colorJson->at(0);
                    auto el1 = colorJson->at(1);
                    auto el2 = colorJson->at(2);

                    if(el0.is_number() && el1.is_number() && el2.is_number()) {
                        properties.color = glm::vec4(el0.get<float>()/255.0, el1.get<float>()/255.0, el2.get<float>()/255.0, 1.0);
                    } else {
                        spdlog::warn("One or more entry in the color property is not a number in '" + displayName + "' (directory '" + particleDir + "').");
                        properties.color = glm::vec4(1.0, 0.0, 1.0, 1.0);
                    }
                } else {
                    auto el0 = colorJson->at(0);
                    auto el1 = colorJson->at(1);
                    auto el2 = colorJson->at(2);
                    auto el3 = colorJson->at(3);

                    if(el0.is_number() && el1.is_number() && el2.is_number() && el3.is_number()) {
                        properties.color = glm::vec4(el0.get<float>()/255.0, el1.get<float>()/255.0, el2.get<float>()/255.0, el3.get<float>()/255.0);
                    } else {
                        spdlog::warn("One or more entry in the color property is not a number in '" + displayName + "' (directory '" + particleDir + "').");
                        properties.color = glm::vec4(1.0, 0.0, 1.0, 1.0);
                    }
                }
            }
        }

        auto phasesJson = propJson.find("phases");
        if(phasesJson != propJson.end()) {
            if(phasesJson->is_array()) {
                if(phasesJson->size() > 2) spdlog::error("Phases property contains more than 2 entries in '" + displayName + "' (directory '" + particleDir + "').");
                else {
                    std::for_each(phasesJson->begin(), phasesJson->end(), [&](json item) {
                        ParticlePhaseChangeProperty phase;
                        
                        auto temperatureJson = item.at("temperature");
                        auto conditionJson = item.at("condition");
                        auto resultJson = item.at("element");
                        if(temperatureJson.is_null() || conditionJson.is_null() || resultJson.is_null()) {
                            spdlog::error("Every element phase property must contain all three properties ('temperature', 'condition', 'element').");
                            return;
                        }

                        phase.condition = conditionJson.get<ParticlePhaseChangeCondition>();
                        phase.temperature = temperatureJson.get<float>();
                        phase.result = 0;
                        
                        properties.phases.push_back(phase);
                        getParticlePtr(resultJson.get<std::string>(), &properties.phases.data()[properties.phases.size()-1].result);
                    });
                }
            } else spdlog::warn("Phases property in '" + displayName + "' (directory '" + particleDir + "') is not an array.");
        }

        auto densityJson = propJson.find("density");
        if(densityJson != propJson.end()) {
            if(densityJson->is_number()) properties.density = densityJson->get<float>();
            else spdlog::warn("Density of an element must be a number.");
        } else properties.density = 1;
    }

    {
        // Load the element scripts.
        auto initJson = propJson.find("init");
        if(initJson == propJson.end()) initScript = std::nullopt;
        else {
            std::ifstream initScriptStream = std::ifstream(particleDir + "/" + initJson->get<std::string>(), std::ios::binary);
            if(!initScriptStream.is_open()) spdlog::error("Could not open file '" + particleDir + "/" + initJson->get<std::string>() + "'.");
            else {
                std::string data;
                std::stringstream strStream;
                strStream << initScriptStream.rdbuf();
                initScript = std::optional(strStream.str());
                initScriptStream.close();
            }
        }

        auto updateJson = propJson.find("update");
        if(updateJson == propJson.end()) updateScript = std::nullopt;
        else {
            const std::string value = updateJson->get<std::string>();
            if(value.find(':') != std::string::npos) updateScript = std::optional(particleDir + "/" + value);
            else updateScript = std::optional(value);
        }

        auto drawJson = propJson.find("draw");
        if(drawJson == propJson.end()) drawScript = std::nullopt;
        else {
            const std::string value = drawJson->get<std::string>();
            if(value.find(':') != std::string::npos) drawScript = std::optional(particleDir + "/" + value);
            else drawScript = std::optional(value);
        }
    }
    {
        auto iconJson = propJson.find("icon");
        if(iconJson == propJson.end()) iconCoordinates = missingIconCoord;
        else {
            if(iconJson->is_string()) {
                // Open the given file.
                int width, height, channels;
                auto file = iconJson->get<std::string>();
                stbi_uc* pixels = stbi_load((particleDir + "/" + file).c_str(), &width, &height, &channels, STBI_rgb_alpha);
                if(pixels != 0) iconCoordinates = iconAtlas->storeTexture(width, height, pixels);
                else {
                    spdlog::error("Failed to load image from file");
                    iconCoordinates = missingIconCoord;
                }
            } else if(iconJson->is_object()) {
                // Parse the in json image.
                auto widthJson = iconJson->find("width");
                auto heightJson = iconJson->find("height");
                auto dataJson = iconJson->find("data");
                if(widthJson == iconJson->end() || heightJson == iconJson->end() || dataJson == iconJson->end()) {
                    spdlog::error("A JSON icon object must contain 3 fields (width, height and data)");
                    iconCoordinates = invalidIconCoord;
                } else {
                    int width = widthJson->get<int>();
                    int height = heightJson->get<int>();
                    if(width > 0 && height > 0) {
                        if(!dataJson->is_string()) {
                            spdlog::error("The data property in icon must be a string.");
                            iconCoordinates = invalidIconCoord;
                        } else {
                            auto data = base64::decode(dataJson->get<std::string>());
                            iconCoordinates = iconAtlas->storeTexture(width, height, data.data());
                        }
                    } else {
                        spdlog::error("Width and height must be greater than 0.");
                        iconCoordinates = invalidIconCoord;
                    }
                }
            } else {
                spdlog::error("The icon property must either be a path to an image or an image encoded in JSON object.");
                iconCoordinates = invalidIconCoord;
            }
        }
    }

    typeId = 0;

    valid = true;
}

Particle::~Particle() {

}

const TextureAtlas::Coordinate& Particle::getTextureAtlasIcon() {
    return iconCoordinates;
}

bool Particle::isValid() {
    return valid;
}

void Particle::getParticlePtr(const std::string& name, Particle** particle) {
    auto part = particles.find(name);
    if(part != particles.end()) *particle = &part->second;
    else {
        auto deferredList = deferredPointers.find(name);
        if(deferredList != deferredPointers.end()) deferredList->second.push_back(particle);
        else {
            std::list<Particle**> deferredParticlePointers = std::list<Particle**>();
            deferredParticlePointers.push_back(particle);
            deferredPointers.insert({ name, deferredParticlePointers });
        }
    }
}

Particle& Particle::getParticle(const std::string& name) {
    return particles[name];
}

void Particle::loadParticle(const std::string& name, const std::string& particleDir) {
    Particle particle = Particle(particleDir);
    if(!particle.isValid()) return;
    particles.insert({ name, particle });
    auto deferredParticles = deferredPointers.find(name);
    if(deferredParticles != deferredPointers.end()) {
        Particle* part = &particles[name];
        std::for_each(deferredParticles->second.begin(), deferredParticles->second.end(), [part](Particle** particlePtr) {
            *particlePtr = part;
        });
        deferredPointers.erase(name);
    }
}

void Particle::loadParticles() {
    std::ifstream enabledPacks = std::ifstream("particles/packs.conf");
    if(!enabledPacks.is_open()) {
        spdlog::error("Could not read the enabled packs file. Assuming default configuration.");

        // Write the default configuration.
        std::ofstream enabledPacksOut = std::ofstream("particles/packs.conf");
        enabledPacksOut << "unibox\n";
        enabledPacksOut.close();

        loadParticlePack("particles/unibox");
    } else {
        while(!enabledPacks.eof()) {
            std::string pack;
            enabledPacks >> pack;
            if(pack.empty()) continue;
            loadParticlePack("particles/" + pack);
        }
    }

    if(!deferredPointers.empty()) {
        spdlog::warn("Some particles could not be loaded but are referenced by other particles:");
        for(auto part = deferredPointers.begin(); part != deferredPointers.end(); part++) {
            size_t size = part->second.size();
            spdlog::warn("  " + part->first + ", " + std::to_string(size) + (size == 1 ? " reference" : " references"));
        }
    }

    // Generate IDs
    spdlog::info("Particle ID mapping:");
    for(auto particle = particles.begin(); particle != particles.end(); particle++) {
        ushort id = particlesIdArray.size()+1;
        particle->second.typeId = id;
        particlesIdArray.push_back(&particle->second);
        spdlog::info("  " + particle->first + " -> " + std::to_string(id));
    }
    spdlog::info("Loaded " + std::to_string(particles.size()) + " particles.");
}

void Particle::loadParticlePack(const std::string& packRoot) {
    std::ifstream particlesStream = std::ifstream(packRoot + "/manifest.json", std::ios::binary);
    if(!particlesStream.is_open()) {
        spdlog::error("Could not open particles file for pack");
        return;
    }
    json manifestJson;
    particlesStream >> manifestJson;
    particlesStream.close();

    std::string namespaceId;

    {
        auto nameJson = manifestJson.find("displayName");
        if(nameJson == manifestJson.end()) spdlog::info("Loading particle pack 'MISSING_NAME'.");
        else spdlog::info("Loading particle pack '" + nameJson->get<std::string>() + "'.");

        auto descriptionJson = manifestJson.find("description");
        if(descriptionJson != manifestJson.end()) spdlog::info("Description: " + descriptionJson->get<std::string>());

        auto namespaceJson = manifestJson.find("namespace");
        if(namespaceJson == manifestJson.end()) {
            spdlog::error("Could not locate a namespace entry in '" + packRoot + "', cannot load this pack.");
            return;
        } else namespaceId = namespaceJson->get<std::string>();
    }

    auto particlesJson = manifestJson.find("particles");
    if(particlesJson != manifestJson.end()) {
        if(particlesJson->is_array()) {
            std::for_each(particlesJson->begin(), particlesJson->end(), [packRoot, namespaceId](json item) {
                if(!item.is_object()) {
                    spdlog::error("Every entry of the manifest particles property must be an object.");
                    return;
                }
                auto nameJson = item.find("name");
                auto locationJson = item.find("location");
                if(nameJson == item.end() || locationJson == item.end()) {
                    spdlog::error("Every entry of the manifest particles property must contain a name and a location property.");
                    return;
                }
                spdlog::info("Loading particle '" + nameJson->get<std::string>() + "'.");
                loadParticle(namespaceId + ":" + nameJson->get<std::string>(), packRoot + "/" + locationJson->get<std::string>());
            });
        } else spdlog::error("Particles property in manifest file must be an array.");
    } else spdlog::error("Every manifest must contain a particles property.");

    auto includeJson = manifestJson.find("include");
    if(includeJson != manifestJson.end()) {
        if(includeJson->is_object()) {
            auto updateIncludeJson = includeJson->find("update");
            if(updateIncludeJson != includeJson->end()) {
                if(updateIncludeJson->is_array()) {
                    std::for_each(updateIncludeJson->begin(), updateIncludeJson->end(), [&packRoot](json item) {
                        updateInclude += "\n";
                        std::string value = readFile(packRoot + "/" + item.get<std::string>());
                        updateInclude += value;
                    });
                } else spdlog::error("Every item in the include object must be an array.");
            }

            auto drawIncludeJson = includeJson->find("draw");
            if(drawIncludeJson != includeJson->end()) {
                if(drawIncludeJson->is_array()) {
                    std::for_each(drawIncludeJson->begin(), drawIncludeJson->end(), [&packRoot](json item) {
                        drawInclude += "\n";
                        std::string value = readFile(packRoot + "/" + item.get<std::string>());
                        drawInclude += value;
                    });
                } else spdlog::error("Every item in the include object must be an array.");
            }
        } else spdlog::error("Include property in manifest file must be an object.");
    }
}

void Particle::fillPip(ParticleInfoPacket& pip) {
    pip.color = properties.color;
}

void Particle::fillSimPip(SimulationParticleInfoPacket& simPip) {
    simPip.density = properties.density;
    simPip.state = properties.state | (properties.affectedByGravity << 2);
}

std::string Particle::constructSwitchCode() {
    std::string output;
    for(auto& particle : particles) {
        if(!particle.second.updateScript.has_value()) continue;
        const std::string& scriptStr = particle.second.updateScript.value();
        const std::string funcName = scriptStr.find(':') == std::string::npos ? scriptStr : scriptStr.substr(scriptStr.find(':')+1);
        output.append("case ");
        output.append(std::to_string(particle.second.typeId));
        output.append(": ");
        output.append(funcName);
        output.append("(structs, &particle); break;\n");
    }
    return output;
}

std::string Particle::constructFunctions() {
    std::string output;
    for(auto& particle : particles) {
        if(!particle.second.updateScript.has_value()) continue;
        const std::string& scriptStr = particle.second.updateScript.value();
        if(scriptStr.find(':') == std::string::npos) continue;

        const std::string funcName = scriptStr.substr(scriptStr.find(':')+1);
        const std::string scriptPath = scriptStr.substr(0, scriptStr.find(':'));
        std::string script;

        {
            std::ifstream fileStream = std::ifstream(scriptPath, std::ios::binary);
            std::stringstream strStream;
            strStream << fileStream.rdbuf();
            script = strStream.str();
        }

        output.append(script);
        output.append("\n");
    }
    return output;
}

uint Particle::getParticleId(const std::string& name) {
    return getParticle(name).typeId;
}

std::string Particle::constructMeshSwitchCode() {
    std::string output;
    for(auto& particle : particles) {
        if(!particle.second.drawScript.has_value()) continue;
        const std::string& scriptStr = particle.second.drawScript.value();
        const std::string funcName = scriptStr.find(':') == std::string::npos ? scriptStr : scriptStr.substr(scriptStr.find(':')+1);
        output.append("case ");
        output.append(std::to_string(particle.second.typeId));
        output.append(": baseColor = (float4)(");
        output.append(funcName);
        output.append("(baseColor.xyz, particle), baseColor.w); break;\n");
    }
    return output;
}

std::string Particle::constructMeshFunctions() {
    std::string output;
    for(auto& particle : particles) {
        if(!particle.second.drawScript.has_value()) continue;
        std::string& scriptStr = particle.second.drawScript.value();
        if(scriptStr.find(':') == std::string::npos) continue;

        const std::string funcName = scriptStr.substr(scriptStr.find(':')+1);
        const std::string scriptPath = scriptStr.substr(0, scriptStr.find(':'));
        std::string script;

        {
            std::ifstream fileStream = std::ifstream(scriptPath, std::ios::binary);
            std::stringstream strStream;
            strStream << fileStream.rdbuf();
            script = strStream.str();
        }

        output.append(script);
        output.append("\n");
    }
    return output;
}

std::string Particle::constructTypeDefinitions() {
    std::string output;
    for(auto& particle : particles) {
        output.append("#define ");

        std::string name = particle.first;
        name.replace(name.find(':'), 1, "_");
        for(int i = 0; i < name.length(); i++) name[i] = toupper(name[i]);
        output.append(name);

        output.append(" ");
        output.append(std::to_string(particle.second.typeId));

        output.append("\n");
    }
    return output;
}

std::string& Particle::getIncludeCode() {
    return updateInclude;
}

std::string& Particle::getMeshIncludeCode() {
    return drawInclude;
}

std::string Particle::readFile(const std::string& filepath) {
    std::ifstream stream = std::ifstream(filepath, std::ios::binary);
    if(!stream.is_open()) return "";
    std::stringstream strstream;
    strstream << stream.rdbuf();
    return strstream.str();
}

void* Particle::getIconImage(uint* width, uint* height) {
    *width = 2048;
    *height = 2048;
    return iconAtlas->getAtlasData();
}
