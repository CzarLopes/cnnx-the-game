#pragma once

#include <string>
#include <vector>

// Progresso permanente do jogador, salvo em arquivo.
class Profile {
public:
    static constexpr int kBaseMaxHp = 30;
    static constexpr int kHpPerUpgrade = 5;
    static constexpr int kMaxHpLimit = 100;

    Profile() = default;
    // Carrega do arquivo, se existir. Arquivo invalido comeca do zero.
    explicit Profile(std::string path);

    bool hasUpgrade(const std::string& id) const;
    // Retorna true se a conquista era nova; salva na hora para nao se perder.
    bool addUpgrade(const std::string& id);
    int maxHp() const;

    // Bloco onde o jogador parou, usado pelo "Carregar jogo". Vazio se nao houver.
    const std::string& savedLevel() const { return level_; }
    void setLevel(const std::string& id);
    // Ha algo para carregar: um bloco salvo ou alguma conquista.
    bool hasProgress() const { return !level_.empty() || !upgrades_.empty(); }
    // Novo jogo: apaga bloco e conquistas e salva na hora.
    void reset();

private:
    void save() const;

    std::string path_;
    std::vector<std::string> upgrades_;
    std::string level_;
};
