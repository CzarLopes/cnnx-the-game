#include "Profile.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <utility>

using nlohmann::json;

Profile::Profile(std::string path) : path_(std::move(path)) {
    std::ifstream file(path_);
    if (!file) {
        return;
    }
    const json save = json::parse(file, nullptr, false);
    if (save.is_object() && save.contains("level") && save["level"].is_string()) {
        level_ = save["level"].get<std::string>();
    }
    const auto readList = [&save](const char* key, std::vector<std::string>& list) {
        if (save.is_object() && save.contains(key) && save[key].is_array()) {
            for (const json& id : save[key]) {
                if (id.is_string()) {
                    list.push_back(id.get<std::string>());
                }
            }
        }
    };
    readList("hpUpgrades", upgrades_);
    readList("phases", phases_);
}

bool Profile::isPhaseComplete(const std::string& phase) const {
    return std::find(phases_.begin(), phases_.end(), phase) != phases_.end();
}

void Profile::completePhase(const std::string& phase) {
    if (!isPhaseComplete(phase)) {
        phases_.push_back(phase);
    }
    // A fase terminou: entrar nela de novo comeca do primeiro bloco
    level_.clear();
    save();
}

bool Profile::hasUpgrade(const std::string& id) const {
    return std::find(upgrades_.begin(), upgrades_.end(), id) != upgrades_.end();
}

bool Profile::addUpgrade(const std::string& id) {
    if (hasUpgrade(id)) {
        return false;
    }
    upgrades_.push_back(id);
    save();
    return true;
}

int Profile::maxHp() const {
    return std::min(kBaseMaxHp + kHpPerUpgrade * static_cast<int>(upgrades_.size()), kMaxHpLimit);
}

void Profile::setLevel(const std::string& id) {
    if (level_ == id) {
        return;
    }
    level_ = id;
    save();
}

void Profile::reset() {
    upgrades_.clear();
    level_.clear();
    phases_.clear();
    save();
}

void Profile::save() const {
    if (path_.empty()) {
        return;
    }
    const std::filesystem::path path(path_);
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream file(path);
    file << json{{"level", level_}, {"phases", phases_}, {"hpUpgrades", upgrades_}}.dump(2);
}
