#include "link_graph.h"

void LinkGraph::add_edge(int from, int to) {
    if (from == to) return;  // ignoram self-loop-urile
    adj_.at(from).push_back(to);
}

std::vector<std::string> extract_links(const std::string& text) {
    std::vector<std::string> links;
    size_t i = 0;
    while (i + 1 < text.size()) {
        // Cautam deschiderea "[["
        if (text[i] == '[' && text[i + 1] == '[') {
            size_t start = i + 2;
            size_t end = text.find("]]", start);
            if (end == std::string::npos) break;  // "[[" fara inchidere

            std::string target = text.substr(start, end - start);

            // Trim spatii la capete.
            size_t a = target.find_first_not_of(" \t\n\r");
            size_t b = target.find_last_not_of(" \t\n\r");
            if (a != std::string::npos) {
                links.push_back(target.substr(a, b - a + 1));
            }
            i = end + 2;  // sarim peste "]]"
        } else {
            ++i;
        }
    }
    return links;
}
