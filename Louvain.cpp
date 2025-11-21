#include "Louvain.h"
#include <random>
#include <algorithm>
#include <iostream>
#include <cmath> 
#include <numeric>
#include <queue> 

Louvain::Louvain(const Graph& g, bool fast_mode) 
    : graph(g), use_fast_mode(fast_mode), calculation_steps(0) {
    
    m2 = graph.get_total_weight();
    std::vector<int> original_nodes = graph.get_all_nodes();
    n = original_nodes.size();

    // Resize vectors
    community_of_node.resize(n);
    community_total_degree.resize(n);
    community_internal_weight.resize(n);
    internal_id_to_node.resize(n);
    is_active.assign(n, true); // All nodes start as active

    int internal_id = 0;
    for (int node_id : original_nodes) {
        node_to_internal_id[node_id] = internal_id;
        internal_id_to_node[internal_id] = node_id;
        
        // Initialize state (t=0): Each node is its own community
        community_of_node[internal_id] = internal_id;
        community_total_degree[internal_id] = graph.get_degree(node_id);
        community_internal_weight[internal_id] = graph.get_self_loop_weight(node_id);
        
        internal_id++;
    }
}

// --- Improvement 1: Tree Splitting Logic ---
void Louvain::prune_local_trees() {
    if (!use_fast_mode) return;

    // Track dynamic degrees as we prune
    std::vector<int> current_degree(n, 0);
    std::queue<int> leaf_queue;

    // 1. Initialize degrees and identify initial leaves
    for (int i = 0; i < n; ++i) {
        int original_id = internal_id_to_node[i];
        const auto& neighbors = graph.get_neighbors(original_id);
        
        // Count only neighbors that are NOT self-loops
        int deg = 0;
        for(auto& p : neighbors) {
            if(node_to_internal_id.at(p.first) != i) deg++;
        }
        current_degree[i] = deg;

        if (deg == 1) {
            leaf_queue.push(i);
        }
    }

    // 2. Peeling process (Recursive removal)
    while (!leaf_queue.empty()) {
        int leaf = leaf_queue.front();
        leaf_queue.pop();

        // If already processed/pruned, skip
        if (!is_active[leaf]) continue;

        // Find the "parent" (the only remaining active neighbor)
        int parent = -1;
        int original_id = internal_id_to_node[leaf];
        const auto& neighbors = graph.get_neighbors(original_id);

        for (const auto& p : neighbors) {
            int neighbor_idx = node_to_internal_id.at(p.first);
            // Parent must be active and not the node itself
            if (neighbor_idx != leaf && is_active[neighbor_idx]) {
                parent = neighbor_idx;
                break;
            }
        }

        // If parent found, prune leaf
        if (parent != -1) {
            is_active[leaf] = false; // Deactivate node so it's skipped in Phase 1
            pruned_nodes_stack.push({leaf, parent}); // Store for re-attachment

            // Decrease parent's degree
            current_degree[parent]--;
            
            // If parent becomes a new leaf, add to queue
            if (current_degree[parent] == 1) {
                leaf_queue.push(parent);
            }
        }
    }
}

void Louvain::reattach_trees() {
    // Process stack in LIFO order (reverse of pruning)
    while (!pruned_nodes_stack.empty()) {
        std::pair<int, int> pair = pruned_nodes_stack.top();
        pruned_nodes_stack.pop();

        int child = pair.first;
        int parent = pair.second;

        // Assign child to parent's final community
        int parent_comm = community_of_node[parent];
        
        // Execute the move to keep state consistent
        move_node(child, parent_comm);
    }
}

bool Louvain::run_phase_one() {
    // Step A: Prune Trees (Fast Mode only)
    prune_local_trees();
    // std::cout << "Running Phase 1" << std::endl;

    bool any_move_made = false;
    bool a_move_happened_in_pass = true;

    std::vector<int> nodes(n);
    std::iota(nodes.begin(), nodes.end(), 0);

    std::random_device rd;
    std::mt19937 g(rd());

    // Improvement 2: Dynamic Iteration Threshold
    // Paper suggests stopping if moved nodes < 0.1% of total active nodes
    double stop_threshold = use_fast_mode ? 0.001 : 0.0; 
    
    int total_active_nodes = 0;
    for(bool b : is_active) if(b) total_active_nodes++;

    while (a_move_happened_in_pass) {
        a_move_happened_in_pass = false;
        int moves_in_this_pass = 0;

        std::shuffle(nodes.begin(), nodes.end(), g);

        for (int node : nodes) {
            // CRITICAL: Skip pruned nodes
            if (!is_active[node]) continue;

            int original_node_id = internal_id_to_node[node];
            int current_community = community_of_node[node];
            double max_delta_Q = 0.0;
            int best_community = current_community;

            std::unordered_map<int, double> neighbor_communities;
            
            // Aggregate edge weights to neighboring communities
            for (const auto& neighbor_pair : graph.get_neighbors(original_node_id)) {
                int neighbor_internal_id = node_to_internal_id.at(neighbor_pair.first);
                int neighbor_community = community_of_node[neighbor_internal_id];
                neighbor_communities[neighbor_community] += neighbor_pair.second;
            }

            // Find best community to move to
            for (const auto& pair : neighbor_communities) {
                int target_community = pair.first;
                if (target_community == current_community) continue;
                
                double delta_Q = calculate_delta_modularity(node, target_community);

                if (delta_Q > max_delta_Q) {
                    max_delta_Q = delta_Q;
                    best_community = target_community;
                }
            }

            // Execute move if gain is positive
            if (max_delta_Q > 0.0 && best_community != current_community) {
                move_node(node, best_community);
                a_move_happened_in_pass = true;
                any_move_made = true;
                moves_in_this_pass++;
            }
        }

        // Check Dynamic Iteration Threshold
        if (use_fast_mode && total_active_nodes > 0) {
            double move_ratio = (double)moves_in_this_pass / total_active_nodes;
            if (move_ratio < stop_threshold) {
                // Stop early if fewer than 0.1% of nodes moved
                a_move_happened_in_pass = false; 
            }
        }
    }

    // Step B: Re-attach Trees (Fast Mode only)
    if (use_fast_mode) {
        reattach_trees();
    }

    return any_move_made;
}

Graph Louvain::run_phase_two() {
    // Standard aggregation logic
    std::cout << "Running Phase 2: Community aggregation..." << std::endl;
    Graph aggregated_graph;
    m_comm_to_new_node.clear(); 

    int new_node_id = 0;
    std::unordered_map<long long, double> new_edges;
    
    // Helper for edge key
    auto edge_key = [](int u, int v) -> long long {
        return (u < v) ? ((long long)u << 32) | v : ((long long)v << 32) | u;
    };

    // Create new nodes for active communities
    std::vector<int> active_communities(n, 0); 
    for (int i = 0; i < n; ++i) {
        active_communities[community_of_node[i]] = 1;
    }
    
    for (int i = 0; i < n; ++i) {
        if (active_communities[i]) {
            m_comm_to_new_node[i] = new_node_id++; 
        }
    }
    
    std::cout << "Phase 2: Aggregating " << n << " nodes into " 
              << new_node_id << " communities." << std::endl;

    // Build new edges
    for (int original_u_id : graph.get_all_nodes()) {
        int internal_u = node_to_internal_id.at(original_u_id);
        int comm_u = community_of_node[internal_u];
        int new_u = m_comm_to_new_node.at(comm_u);

        for (const auto& neighbor_pair : graph.get_neighbors(original_u_id)) {
            int original_v_id = neighbor_pair.first;
            double edge_weight = neighbor_pair.second;
            int internal_v = node_to_internal_id.at(original_v_id);
            int comm_v = community_of_node[internal_v];
            int new_v = m_comm_to_new_node.at(comm_v);

            if (original_u_id <= original_v_id) {
                 new_edges[edge_key(new_u, new_v)] += edge_weight;
            }
        }
    }

    for (const auto& edge_pair : new_edges) {
        long long key = edge_pair.first;
        double weight = edge_pair.second;
        int new_u = (int)(key >> 32);
        int new_v = (int)(key & 0xFFFFFFFF);
        aggregated_graph.add_edge(new_u, new_v, weight);
    }
    return aggregated_graph;
}

std::unordered_map<int, int> Louvain::get_community_assignments() const {
    std::unordered_map<int, int> assignments;
    for (int i = 0; i < n; ++i) {
        int original_node_id = internal_id_to_node[i];
        int community_id = community_of_node[i];
        assignments[original_node_id] = community_id;
    }
    return assignments;
}

double Louvain::calculate_delta_modularity(int node, int target_community) {
    // --- BENCHMARKING COUNTER ---
    calculation_steps++; 
    // ----------------------------

    int current_community = community_of_node[node];
    if (current_community == target_community) return 0.0;

    int original_node_id = internal_id_to_node[node];
    double k_i = graph.get_degree(original_node_id); 
    
    double sum_tot_target = community_total_degree[target_community];
    double sum_tot_current = community_total_degree[current_community];

    double k_i_target = 0.0; 
    double k_i_current = 0.0; 

    for (const auto& neighbor_pair : graph.get_neighbors(original_node_id)) {
        int neighbor_original_id = neighbor_pair.first;
        double edge_weight = neighbor_pair.second;
        
        int neighbor_internal_id = node_to_internal_id.at(neighbor_original_id);
        int neighbor_community = community_of_node[neighbor_internal_id];

        if (neighbor_community == target_community) {
            k_i_target += edge_weight;
        }
        if (neighbor_community == current_community) {
            k_i_current += edge_weight; 
        }
    }

    double m = m2 / 2.0;
    // Robust formula for dQ (including self-loop handling implicitly via k_i_current)
    double dQ = (k_i_target - k_i_current) / m2;
    dQ += (k_i * (sum_tot_current - sum_tot_target - k_i)) / (m2 * m2);
    
    return dQ;
}

void Louvain::move_node(int node, int target_community) {
    int current_community = community_of_node[node];
    if (current_community == target_community) return;

    int original_node_id = internal_id_to_node[node];
    double k_i = graph.get_degree(original_node_id);
    double self_loop_weight = graph.get_self_loop_weight(original_node_id);

    double k_i_target = 0.0;
    double k_i_current = 0.0;

    for (const auto& neighbor_pair : graph.get_neighbors(original_node_id)) {
        int neighbor_original_id = neighbor_pair.first;
        double edge_weight = neighbor_pair.second;
        int neighbor_internal_id = node_to_internal_id.at(neighbor_original_id);
        int neighbor_community = community_of_node[neighbor_internal_id];

        if (neighbor_community == target_community) k_i_target += edge_weight;
        if (neighbor_community == current_community) k_i_current += edge_weight;
    }
    
    double k_i_current_ext = k_i_current - self_loop_weight;

    // Update sums
    community_total_degree[current_community] -= k_i;
    community_total_degree[target_community] += k_i;

    community_internal_weight[current_community] -= (2 * k_i_current_ext + self_loop_weight);
    community_internal_weight[target_community] += (2 * k_i_target + self_loop_weight);

    community_of_node[node] = target_community;
}