#include "Graph.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Define the static empty vector
const std::vector<std::pair<int, double>> Graph::s_empty_neighbor_list = {};

Graph::Graph() : m_total_weight(0.0) {
    // Constructor initializes total weight to 0.
    // The maps are default-constructed (empty).
}

void Graph::add_edge(int u, int v, double weight) {
    // This is the core logic.
    // We assume the file format doesn't list the same edge twice (e.g., u v AND v u)

    if (u == v) {
        // Handle self-loop
        // Add edge to adjacency list
        m_adj_list[u].push_back({u, weight});
        // A self-loop adds 2*weight to the degree (k_i)
        m_node_degrees[u] += 2 * weight;
        // A self-loop adds 2*weight to the total graph weight (2m)
        m_total_weight += 2 * weight;
    } else {
        // Handle normal undirected edge
        // Add (u, v)
        m_adj_list[u].push_back({v, weight});
        // Add (v, u)
        m_adj_list[v].push_back({u, weight});

        // Update degrees
        m_node_degrees[u] += weight;
        m_node_degrees[v] += weight;

        // Update total graph weight (2m)
        // The edge (u, v) with weight 'w' adds 'w' to k_u and 'w' to k_v.
        // The sum of degrees (2m) increases by 2w.
        m_total_weight += 2 * weight;
    }
}

bool Graph::load_from_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return false;
    }

    std::string line;
    int u, v;
    double weight;

    // Note: We assume node IDs in the file are integers.
    // We also assume the file lists each *undirected* edge exactly ONCE.
    while (std::getline(file, line)) {
        // Skip empty lines or comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        if (!(ss >> u >> v >> weight)) {
            // Handle optional weight. If not provided, default to 1.0
            std::stringstream ss_unweighted(line);
            if(ss_unweighted >> u >> v) {
                weight = 1.0;
            } else {
                std::cerr << "Warning: Skipping malformed line: " << line << std::endl;
                continue;
            }
        }
        
        add_edge(u, v, weight);
    }

    file.close();
    return true;
}

double Graph::get_self_loop_weight(int u) const {
    double self_loop_weight = 0.0;

    auto it = m_adj_list.find(u);
    if (it != m_adj_list.end()) {
        // Iterate through all neighbors of node u
        for (const auto& neighbor_pair : it->second) {
            // If the neighbor is itself, it's a self-loop
            if (neighbor_pair.first == u) {
                self_loop_weight += neighbor_pair.second;
            }
        }
    }
    return self_loop_weight;
}

const std::vector<std::pair<int, double>>& Graph::get_neighbors(int u) const {
    // Use .find() for const-correctness and to avoid inserting a new element
    auto it = m_adj_list.find(u);
    if (it != m_adj_list.end()) {
        return it->second; // Return the vector of neighbors
    }
    return s_empty_neighbor_list; // Return the static empty vector
}

double Graph::get_degree(int u) const {
    auto it = m_node_degrees.find(u);
    if (it != m_node_degrees.end()) {
        return it->second; // Return the pre-computed degree
    }
    return 0.0; // Node doesn't exist or has 0 degree
}

double Graph::get_total_weight() const {
    return m_total_weight;
}

std::vector<int> Graph::get_all_nodes() const {
    std::vector<int> nodes;
    nodes.reserve(m_node_degrees.size()); // Pre-allocate memory

    // m_node_degrees holds all nodes that have at least one edge.
    for (const auto& pair : m_node_degrees) {
        nodes.push_back(pair.first);
    }
    return nodes;
}