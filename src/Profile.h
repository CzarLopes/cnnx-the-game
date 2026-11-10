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
    // Fases concluidas, pelo id "w1_f1". Concluir libera a proxima fase do mundo.
    bool isPhaseComplete(const std::string& phase) const;
    void completePhase(const std::string& phase);

    // Ha algo para carregar: bloco salvo, fase concluida ou conquista.
    bool hasProgress() const { return !level_.empty() || !upgrades_.empty() || !phases_.empty(); }
    // Novo jogo: apaga bloco, fases e conquistas e salva na hora.
    void reset();

private:
    void save() const;

    std::string path_;
    std::vector<std::string> upgrades_;
    std::string level_;
    std::vector<std::string> phases_;
};
