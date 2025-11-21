# Improved Louvain Algorithm for Community Detection

This repository contains a high-performance C++ implementation of the Louvain Algorithm and improved Fast Louvain for community detection in large-scale networks. 
<img width="1147" height="750" alt="output_facebook" src="https://github.com/user-attachments/assets/3bd14f6c-d727-4e49-a4de-6b84f636e84d" />
<img width="1857" height="1010" alt="output_twitter_tooHeavytoBeColored" src="https://github.com/user-attachments/assets/90e0b910-05d1-4210-b6d5-6bc900b0ddd2" />

##  Prerequisites

* **Compiler:** `g++` with C++17 support.
* **Visualization Tool:** [Gephi](https://gephi.org/) (Recommended for large graphs).

## Project Structure

* `main.cpp`: Entry point. Handles file I/O, runs the hierarchy benchmark, and exports results.
* `Louvain.h / .cpp`: Core implementation. Contains the `run_phase_one` (modularity optimization) and `run_phase_two` (aggregation) logic, including the pruning and dynamic iteration features.
* `Graph.h / .cpp`: Efficient Adjacency List graph data structure.
* `facebook.txt` / `karate.txt`: Sample datasets (Edge lists).


## Build & Run

### 1. Compilation
Open your terminal in the project directory and run the following command:

```bash g++ -std=c++17 -Wall -o louvain_dsa main.cpp Graph.cpp Louvain.cpp```

### 2. Execution
Ensure your input graph file (e.g., `facebook.txt`) is in the same directory and run:

```bash
./louvain_dsa
````
<img width="487" height="619" alt="output_twitter_code" src="https://github.com/user-attachments/assets/8773c37a-1c58-4da0-8aa5-f6c86bda3fe0" />
Warning: The Louvain algorithm (both Standard and Fast versions) relies on randomized node ordering during the optimization phase to avoid local optima and bias. As a result, running the algorithm multiple times on the same dataset may yield slightly different community structures (number of communities or specific node assignments) and modularity scores.

### 3\. Output

The program will output benchmark statistics to the console, showing the number of calculation steps saved by the Fast implementation. It also generates two CSV files for visualization:

  * `communities_std.csv` (Standard Result)
  * `communities_fast.csv` (Optimized Result)

## 📊 Visualization Workflow (Gephi)

For large networks, use Gephi to visualize the community structure.

### Step 1: Prepare Data

  * Rename your input graph file from `.txt` to `.csv` (e.g., `facebook.csv`).

### Step 2: Import Edges

1.  Open Gephi and navigate to the **Data Laboratory** tab.
2.  Click **Import Spreadsheet**.
3.  Select `facebook.csv`.
4.  **Important:** In the settings, change the **Separator** to **Space**.
5.  Ensure the table type is **Edges table** and click **Finish**.

### Step 3: Import Communities

1.  Click **Import Spreadsheet** again.
2.  Select the output file `communities_fast.csv`.
3.  Ensure the table type is **Nodes table**.
4.  Click **Finish**.
5.  **Critical:** In the import report, select **Append to existing workspace** to merge the community data with the existing nodes.

### Step 4: Color & Layout

1.  Go to the **Overview** tab.
2.  **Color:** In the **Appearance** pane (top-left), select **Nodes** -\> **Partition** (icon) -\> **CommunityId** -\> **Apply**.
3.  **Layout:** In the **Layout** pane (bottom-left), select **ForceAtlas 2**. Click **Run** and wait for the clusters to separate, then click **Stop**.

<!-- end list -->

```
```
