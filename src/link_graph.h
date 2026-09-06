#pragma once
#include <string>
#include <vector>

// Graful orientat de linkuri dintre documente.
// Nodurile sunt doc_id-uri 0..N-1; o muchie from -> to inseamna
// "documentul `from` linkeaza spre documentul `to`".
class LinkGraph {
public:
    explicit LinkGraph(int num_nodes) : adj_(num_nodes) {}

    // Adauga o muchie orientata. Ignora self-loop-urile (o pagina care se
    // linkeaza pe ea insasi nu-si transmite autoritate).
    void add_edge(int from, int to);

    int num_nodes() const { return (int)adj_.size(); }
    const std::vector<int>& out_links(int node) const { return adj_.at(node); }
    int out_degree(int node) const { return (int)adj_.at(node).size(); }

private:
    std::vector<std::vector<int>> adj_;
};

// Extrage tintele linkurilor de forma [[Titlu]] din text brut.
// Ex: "vezi [[PageRank]] si [[BM25 Ranking]]" -> {"PageRank", "BM25 Ranking"}
std::vector<std::string> extract_links(const std::string& text);
