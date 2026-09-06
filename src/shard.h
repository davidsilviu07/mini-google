#pragma once
#include <string>
#include <utility>
#include <vector>

// --- Index distribuit: sharding + scatter-gather ---
//
// In loc de un index monolit, spargem documentele in N shard-uri disjuncte.
// Fiecare shard e un index independent, salvat ca snapshot propriu. La cautare,
// coordonatorul intreaba toate shard-urile in PARALEL (scatter), primeste top-k
// de la fiecare, apoi le combina intr-un top-k global (gather). Asa functioneaza
// un motor de cautare la scara: niciun index nu incape pe o singura masina.

// Construieste `num_shards` snapshot-uri dintr-un corpus. Documentul i merge in
// shard-ul (i % num_shards). Fisierele: <prefix>.shard0.bin, .shard1.bin, ...
// Intoarce numarul de shard-uri scrise.
int build_shards(const std::string& corpus_dir, int num_shards,
                 const std::string& prefix);

// Interogheaza UN shard: incarca snapshot-ul, ruleaza BM25, intoarce (titlu, scor).
std::vector<std::pair<std::string, double>> query_shard(
    const std::string& snapshot, const std::string& query, int k);

// Combina rezultatele mai multor shard-uri intr-un top-k global, dupa scor.
// Fiecare vector interior e rezultatul (titlu, scor) al unui shard.
std::vector<std::pair<std::string, double>> merge_by_score(
    const std::vector<std::vector<std::pair<std::string, double>>>& shard_results,
    int k);

// Cautare distribuita: interogheaza toate shard-urile IN PARALEL si le combina.
std::vector<std::pair<std::string, double>> distributed_search(
    const std::vector<std::string>& shard_snapshots, const std::string& query,
    int k);
