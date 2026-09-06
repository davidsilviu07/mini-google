#pragma once
#include <string>
#include <vector>

// Transforma text brut intr-o lista de termeni normalizati:
//  - lowercase
//  - pastreaza doar [a-z0-9]
//  - elimina stopwords uzuale
// Pozitia fiecarui token in vectorul rezultat = pozitia lui in document
// (folosita mai tarziu pentru cautare pe fraza).
std::vector<std::string> tokenize(const std::string& text);
