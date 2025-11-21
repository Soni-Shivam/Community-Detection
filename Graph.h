#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <unordered_map>
#include <string>
#include <utility> // For std::pair

/**
 * @class Graph
 * @brief Represents a weighted, undirected graph using an adjacency list.
 *
 * This class is designed to be efficient for sparse graphs, which are common
 * in real-world networks. It stores the graph and pre-computes node degrees
 * and the total graph weight (2m) for fast access by the Louvain algorithm.
 */
class Graph {
public:
    /**
     * @brief Default constructor.
     */
    Graph();

    /**
     * @brief Adds an edge to the graph.
     *
     * This method handles both normal edges (u, v) and self-loops (u, u).
     * It correctly updates node degrees and the total graph weight (2m).
     *
     * @param u The source node ID.
     * @param v The target node ID.
     * @param weight The weight of the edge.
     */
    void add_edge(int u, int v, double weight);

    /**
     * @brief Loads a graph from an edge list file.
     *
     * The file format is assumed to be:
     * <source_node> <target_node> <weight>
     * per line.
     *
     * @param filename The path to the graph file.
     * @return true if loading was successful, false otherwise.
     */
    bool load_from_file(const std::string& filename);

    /**
     * @brief Gets the neighbors of a given node.
     *
     * @param u The node ID.
     * @return A const reference to the vector of neighbors (pair<node, weight>).
     * Returns a static empty vector if the node has no neighbors or doesn't exist.
     */
    const std::vector<std::pair<int, double>>& get_neighbors(int u) const;

    /**
     * @brief Gets the weighted degree of a node (k_i).
     *
     * For a self-loop (u, u) with weight 'w', it adds '2w' to the degree.
     *
     * @param u The node ID.
     * @return The weighted degree of the node. Returns 0 if the node doesn't exist.
     */
    double get_degree(int u) const;

    /**
     * @brief Gets the total weight of the graph (2m).
     *
     * This is the sum of all node degrees (k_i) in the graph.
     *
     * @return The total weight (2m).
     */
    double get_total_weight() const;

    /**
     * @brief Gets a list of all unique nodes in the graph.
     *
     * @return A vector of node IDs.
     */
    std::vector<int> get_all_nodes() const;


    /**
     * @brief Gets the sum of weights of all self-loops on a node.
     *
     * @param u The node ID.
     * @return The total weight of self-loops. Returns 0 if none.
     */
    double get_self_loop_weight(int u) const;

private:
    // The adjacency list: map<node, vector<pair<neighbor, weight>>>
    std::unordered_map<int, std::vector<std::pair<int, double>>> m_adj_list;

    // Map of node degrees: map<node, weighted_degree>
    std::unordered_map<int, double> m_node_degrees;

    // Total graph weight (2m)
    double m_total_weight;

    // A static empty vector to return for nodes with no neighbors.
    // This avoids creating a new empty vector on every failed lookup.
    static const std::vector<std::pair<int, double>> s_empty_neighbor_list;
};

#endif // GRAPH_H