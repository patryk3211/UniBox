#pragma once

#include <string>
#include <unordered_map>
#include <any>
#include <algorithm>
#include <optional>

namespace unibox::util {
    class GlobalResources {
        struct Resource {
            const std::type_info& type;
            std::any value;
            std::optional<std::function<void(const Resource&)>> destructor;

            template<typename T> Resource(const T& value) : 
                type(typeid(T)), value(value), destructor(std::nullopt) { }

            template<typename T> Resource(const T& value, const std::function<void(const Resource&)> destructor) :
                type(typeid(T)), value(value), destructor(destructor) { }

            ~Resource() { if(destructor) (*destructor)(*this); }

            template<typename T> const std::optional<T> get() const {
                if(type == typeid(T)) return std::any_cast<T>(value);
                else if(type == typeid(Resource*)) return std::any_cast<Resource*>(value)->get<T>();
                return std::nullopt;
            }

            template<typename T> std::optional<T> get() {
                if(type == typeid(T)) return std::any_cast<T>(value);
                else if(type == typeid(Resource*)) return std::any_cast<Resource*>(value)->get<T>();
                return std::nullopt;
            }
        };

        static GlobalResources* instance;

        std::unordered_map<std::string, Resource*> resources;

    public:
        GlobalResources() { instance = this; }
        ~GlobalResources() {
            for(auto& [id, res] : resources) delete res;
            resources.clear();
        }

        template<typename T> void store(const std::string& id, const T& value) { resources.insert({ id, new Resource(value) }); }
        template<typename T> void store(const std::string& id, const T& value, const std::function<void(const Resource&)> destructor)
        { resources.insert({ id, new Resource(value, destructor) }); }

        template<typename T> std::optional<T> get(const std::string& id) {
            auto res = resources.find(id);
            if(res == resources.end()) return std::nullopt;
            return res->second->get<T>();
        }

        void remove(const std::string& id) {
            auto value = resources.find(id);
            if(value != resources.end()) {
                delete value->second;
                resources.erase(id);
            }
        }

        void link(const std::string& id, const std::string& dest) {
            auto value = resources.find(dest);
            if(value != resources.end()) {
                Resource* res = new Resource(value->second);
                resources.insert({ id, res });
            }
        }

        static GlobalResources* getInstance() { return instance; }
    };
}