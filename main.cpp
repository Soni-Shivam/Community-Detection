#include "Graph.h"
#include "Louvain.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <map>
#include <set>
#include <fstream>
#include <cstdlib>
#include <chrono> 

// --- Helper: Calculate Final Modularity ---
double calculate_final_modularity(const Graph& g, const std::unordered_map<int, int>& partition) {
    double m2 = g.get_total_weight();
    if (m2 == 0.0) return 0.0;
    double modularity = 0.0;
    
    for (int node_u : g.get_all_nodes()) {
        double k_u = g.get_degree(node_u);
        for (const auto& neighbor_pair : g.get_neighbors(node_u)) {
            int node_v = neighbor_pair.first;
            double edge_weight_uv = neighbor_pair.second;
            double k_v = g.get_degree(node_v);

            if (partition.at(node_u) == partition.at(node_v)) {
                modularity += (edge_weight_uv - (k_u * k_v) / m2);
            }
        }
    }
    return modularity / m2;
}

// --- Helper: Write Results to CSV ---
void write_results_to_file(const std::unordered_map<int, int>& partition, const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }
    outfile << "NodeId,CommunityId" << std::endl;
    std::map<int, int> sorted_partition(partition.begin(), partition.end());
    for (const auto& pair : sorted_partition) {
        outfile << pair.first << "," << pair.second << std::endl;
    }
    outfile.close();
    std::cout << "Results successfully written to " << filename << std::endl;
}

// --- Helper: Run Full Hierarchy ---
long long run_hierarchy(const Graph& original_graph, bool fast_mode, std::unordered_map<int, int>& final_partition) {
    std::vector<int> original_nodes = original_graph.get_all_nodes();
    final_partition.clear();
    for (int node_id : original_nodes) {
        final_partition[node_id] = node_id;
    }

    Graph current_graph = original_graph;
    long long total_steps = 0;
    bool done = false;
    std::string mode_name = fast_mode ? "[FAST MODE]" : "[STANDARD]";
    std::cout << "\nStarting " << mode_name << " Run..." << std::endl;

    while (!done) {
        Louvain louvain(current_graph, fast_mode);
        bool moved = louvain.run_phase_one();
        total_steps += louvain.get_step_count();

        if (!moved) break;

        std::unordered_map<int, int> this_level_comm_map = louvain.get_community_assignments();
        Graph new_graph = louvain.run_phase_two();
        const auto& comm_remap = louvain.get_comm_remap();
        
        std::unordered_map<int, int> old_partition = final_partition;
        for (int original_node_id : original_nodes) {
            int old_comm_id = old_partition.at(original_node_id);
            int new_comm_id = this_level_comm_map.at(old_comm_id);
            int new_node_id = comm_remap.at(new_comm_id);
            final_partition[original_node_id] = new_node_id;
        }

        if (new_graph.get_all_nodes().size() == current_graph.get_all_nodes().size()) break;
        current_graph = new_graph;
    }
    return total_steps;
}

int main() {
    Graph g;
    std::string graph_filename = "facebook.txt"; 
    
    // --- DEFINE OUTPUT FILES ---
    std::string output_std = "communities_std.csv";
    std::string output_fast = "communities_fast.csv";

    std::cout << "Loading '" << graph_filename << "'..." << std::endl;
    if (!g.load_from_file(graph_filename)) {
        std::cerr << "Failed to load graph." << std::endl;
        return 1;
    }
    std::cout << "Graph loaded. Nodes: " << g.get_all_nodes().size() << ". 2m = " << g.get_total_weight() << std::endl;

    // --- 1. Run Standard Louvain ---
    std::unordered_map<int, int> partition_std;
    auto start_std = std::chrono::high_resolution_clock::now();
    long long steps_std = run_hierarchy(g, false, partition_std);
    auto end_std = std::chrono::high_resolution_clock::now();
    double q_std = calculate_final_modularity(g, partition_std);

    // --- 2. Run Fast Louvain ---
    std::unordered_map<int, int> partition_fast;
    auto start_fast = std::chrono::high_resolution_clock::now();
    long long steps_fast = run_hierarchy(g, true, partition_fast);
    auto end_fast = std::chrono::high_resolution_clock::now();
    double q_fast = calculate_final_modularity(g, partition_fast);

    // --- 3. Report ---
    auto ms_std = std::chrono::duration_cast<std::chrono::milliseconds>(end_std - start_std).count();
    auto ms_fast = std::chrono::duration_cast<std::chrono::milliseconds>(end_fast - start_fast).count();

    std::cout << "\n======================================" << std::endl;
    std::cout << "       BENCHMARK RESULTS              " << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "1. Standard Louvain:" << std::endl;
    std::cout << "   - Total Steps:   " << steps_std << std::endl;
    std::cout << "   - Modularity:    " << std::fixed << std::setprecision(5) << q_std << std::endl;
    std::cout << "   - Time (ms):     " << ms_std << std::endl;

    std::cout << "\n2. Fast Louvain (Tree Split + Dynamic Iteration):" << std::endl;
    std::cout << "   - Total Steps:   " << steps_fast << std::endl;
    std::cout << "   - Modularity:    " << std::fixed << std::setprecision(5) << q_fast << std::endl;
    std::cout << "   - Time (ms):     " << ms_fast << std::endl;

    std::cout << "\n--------------------------------------" << std::endl;
    double reduction = 0.0;
    if (steps_std > 0) {
        reduction = 100.0 * (double)(steps_std - steps_fast) / (double)steps_std;
    }
    std::cout << "IMPROVEMENT:" << std::endl;
    std::cout << "   Saved " << (steps_std - steps_fast) << " calculation steps." << std::endl;
    std::cout << "   Efficiency Gain: " << std::fixed << std::setprecision(2) << reduction << "% fewer calculations." << std::endl;
    std::cout << "======================================" << std::endl;

    // --- SAVE BOTH RESULTS ---
    write_results_to_file(partition_std, output_std);
    write_results_to_file(partition_fast, output_fast);

    return 0;
}