#pragma once
#include <string>
#include <vector>

#include "inverted_index.h"

// Salveaza / incarca un "snapshot" complet al motorului intr-un singur fisier
// binar: indexul inversat + vectorul de scoruri PageRank. Fisierul incepe cu un
// magic number ("MGIX") si o versiune, ca sa respingem fisiere corupte sau
// scrise de un format incompatibil.
//
// Intorc false daca fisierul lipseste, are magic gresit sau versiune necunoscuta.

bool save_snapshot(const std::string& path, const InvertedIndex& index,
                   const std::vector<double>& pagerank);

bool load_snapshot(const std::string& path, InvertedIndex& index,
                   std::vector<double>& pagerank);
