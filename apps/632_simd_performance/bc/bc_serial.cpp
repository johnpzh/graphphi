#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <omp.h>
#include <unistd.h>
#include <algorithm>
#include "../../include/peg.h"

using std::string;
using std::getline;
using std::cout;
using std::endl;
using std::to_string;
using std::vector;

unsigned NNODES;
unsigned NEDGES;
unsigned NUM_THREADS;
unsigned TILE_WIDTH;
unsigned SIDE_LENGTH;
unsigned NUM_TILES;
unsigned ROW_STEP;
unsigned CHUNK_SIZE;
unsigned T_RATIO;

double start;
double now;
FILE *time_out;
char *time_file = "timeline.txt";

void input(
		char filename[], 
		unsigned *&graph_heads, 
		unsigned *&graph_tails, 
		unsigned *&graph_vertices,
		unsigned *&graph_edges,
		unsigned *&graph_degrees,
		unsigned *&tile_offsets,
		unsigned *&tile_sizes,
		unsigned *&graph_heads_reverse,
		unsigned *&graph_tails_reverse,
		unsigned *&graph_vertices_reverse,
		unsigned *&graph_edges_reverse,
		unsigned *&graph_degrees_reverse,
		unsigned *&tile_offsets_reverse,
		unsigned *&tile_sizes_reverse)
{
	//printf("data: %s\n", filename);
	string prefix = string(filename) + "_col-" + to_string(ROW_STEP) + "-coo-tiled-" + to_string(TILE_WIDTH);
	string prefix_reverse = string(filename) + "_col-" + to_string(ROW_STEP) + "-coo-tiled-" + to_string(TILE_WIDTH) + "_reverse";
	string fname = prefix + "-0";
	FILE *fin = fopen(fname.c_str(), "r");
	if (!fin) {
		fprintf(stderr, "Error: cannot open file %s.\n", fname.c_str());
		exit(1);
	}
	fscanf(fin, "%u %u", &NNODES, &NEDGES);
	fclose(fin);
	if (NNODES % TILE_WIDTH) {
		SIDE_LENGTH = NNODES / TILE_WIDTH + 1;
	} else {
		SIDE_LENGTH = NNODES / TILE_WIDTH;
	}
	NUM_TILES = SIDE_LENGTH * SIDE_LENGTH;

	// Read tile Offsets, and get tile sizes
	fname = prefix + "-offsets";
	fin = fopen(fname.c_str(), "r");
	if (!fin) {
		fprintf(stderr, "cannot open file: %s\n", fname.c_str());
		exit(1);
	}
	tile_offsets = (unsigned *) malloc(NUM_TILES * sizeof(unsigned));
	for (unsigned i = 0; i < NUM_TILES; ++i) {
		fscanf(fin, "%u", tile_offsets + i);
	}
	fclose(fin);
	tile_sizes = (unsigned *) malloc(NUM_TILES * sizeof(unsigned));
	for (unsigned i = 0; i < NUM_TILES; ++i) {
		if (NUM_TILES - 1 != i) {
			tile_sizes[i] = tile_offsets[i + 1] - tile_offsets[i];
		} else {
			tile_sizes[i] = NEDGES - tile_offsets[i];
		}
	}
	// Reverse
	fname = prefix_reverse + "-offsets";
	fin = fopen(fname.c_str(), "r");
	if (!fin) {
		fprintf(stderr, "cannot open file: %s\n", fname.c_str());
		exit(1);
	}
	tile_offsets_reverse = (unsigned *) malloc(NUM_TILES * sizeof(unsigned));
	for (unsigned i = 0; i < NUM_TILES; ++i) {
		fscanf(fin, "%u", tile_offsets_reverse + i);
	}
	fclose(fin);
	tile_sizes_reverse = (unsigned *) malloc(NUM_TILES * sizeof(unsigned));
	for (unsigned i = 0; i < NUM_TILES; ++i) {
		if (NUM_TILES - 1 != i) {
			tile_sizes_reverse[i] = tile_offsets_reverse[i + 1] - tile_offsets_reverse[i];
		} else {
			tile_sizes_reverse[i] = NEDGES - tile_offsets_reverse[i];
		}
	}

	graph_heads = (unsigned *) malloc(NEDGES * sizeof(unsigned));
	graph_tails = (unsigned *) malloc(NEDGES * sizeof(unsigned));
	graph_vertices = (unsigned *) malloc(NNODES * sizeof(unsigned));
	graph_edges = (unsigned *) malloc(NEDGES * sizeof(unsigned));
	graph_degrees = (unsigned *) malloc(NNODES * sizeof(unsigned));
	graph_heads_reverse = (unsigned *) malloc(NEDGES * sizeof(unsigned));
	graph_tails_reverse = (unsigned *) malloc(NEDGES * sizeof(unsigned));
	graph_vertices_reverse = (unsigned *) malloc(NNODES * sizeof(unsigned));
	graph_edges_reverse = (unsigned *) malloc(NEDGES * sizeof(unsigned));
	graph_degrees_reverse = (unsigned *) malloc(NNODES * sizeof(unsigned));

	// For heads and tails
	NUM_THREADS = 64;
	unsigned edge_bound = NEDGES / NUM_THREADS;
#pragma omp parallel num_threads(NUM_THREADS) private(fname, fin)
{
	unsigned tid = omp_get_thread_num();
	unsigned offset = tid * edge_bound;
	fname = prefix + "-" + to_string(tid);
	fin = fopen(fname.c_str(), "r");
	if (!fin) {
		fprintf(stderr, "Error: cannot open file %s\n", fname.c_str());
		exit(1);
	}
	if (0 == tid) {
		fscanf(fin, "%u %u", &NNODES, &NEDGES);
	}
	unsigned bound_index;
	if (NUM_THREADS - 1 != tid) {
		bound_index = offset + edge_bound;
	} else {
		bound_index = NEDGES;
	}
	for (unsigned index = offset; index < bound_index; ++index) {
		unsigned n1;
		unsigned n2;
		fscanf(fin, "%u %u", &n1, &n2);
		n1--;
		n2--;
		graph_heads[index] = n1;
		graph_tails[index] = n2;
	}
	fclose(fin);
}
//Reverse
#pragma omp parallel num_threads(NUM_THREADS) private(fname, fin)
{
	unsigned tid = omp_get_thread_num();
	unsigned offset = tid * edge_bound;
	fname = prefix_reverse + "-" + to_string(tid);
	fin = fopen(fname.c_str(), "r");
	if (!fin) {
		fprintf(stderr, "Error: cannot open file %s\n", fname.c_str());
		exit(1);
	}
	if (0 == tid) {
		fscanf(fin, "%u %u", &NNODES, &NEDGES);
	}
	unsigned bound_index;
	if (NUM_THREADS - 1 != tid) {
		bound_index = offset + edge_bound;
	} else {
		bound_index = NEDGES;
	}
	for (unsigned index = offset; index < bound_index; ++index) {
		unsigned n1;
		unsigned n2;
		fscanf(fin, "%u %u", &n1, &n2);
		n1--;
		n2--;
		graph_heads_reverse[index] = n1;
		graph_tails_reverse[index] = n2;
	}
	fclose(fin);
}

	//For graph CSR
	prefix = string(filename) + "_untiled";
	prefix_reverse = string(filename) + "_untiled_reverse";

	// Read degrees
	fname = prefix + "-nneibor";
	fin = fopen(fname.c_str(), "r");
	if (!fin) {
		fprintf(stderr, "cannot open file: %s\n", fname.c_str());
		exit(1);
	}
	for (unsigned i = 0; i < NNODES; ++i) {
		fscanf(fin, "%u", graph_degrees + i);
	}
	fclose(fin);
	//Reverse
	fname = prefix_reverse + "-nneibor";
	fin = fopen(fname.c_str(), "r");
	if (!fin) {
		fprintf(stderr, "cannot open file: %s\n", fname.c_str());
		exit(1);
	}
	for (unsigned i = 0; i < NNODES; ++i) {
		fscanf(fin, "%u", graph_degrees_reverse + i);
	}
	fclose(fin);

	NUM_THREADS = 64;
	edge_bound = NEDGES / NUM_THREADS;
#pragma omp parallel num_threads(NUM_THREADS) private(fname, fin)
{
	unsigned tid = omp_get_thread_num();
	unsigned offset = tid * edge_bound;
	fname = prefix + "-" + to_string(tid);
	fin = fopen(fname.c_str(), "r");
	if (!fin) {
		fprintf(stderr, "Error: cannot open file %s\n", fname.c_str());
		exit(1);
	}
	if (0 == tid) {
		fscanf(fin, "%u %u", &NNODES, &NEDGES);
	}
	unsigned bound_index;
	if (NUM_THREADS - 1 != tid) {
		bound_index = offset + edge_bound;
	} else {
		bound_index = NEDGES;
	}
	for (unsigned index = offset; index < bound_index; ++index) {
		unsigned n1;
		unsigned n2;
		fscanf(fin, "%u %u", &n1, &n2);
		n1--;
		n2--;
		graph_edges[index] = n2;
	}
	fclose(fin);
}
//Reverse
#pragma omp parallel num_threads(NUM_THREADS) private(fname, fin)
{
	unsigned tid = omp_get_thread_num();
	unsigned offset = tid * edge_bound;
	fname = prefix_reverse + "-" + to_string(tid);
	fin = fopen(fname.c_str(), "r");
	if (!fin) {
		fprintf(stderr, "Error: cannot open file %s\n", fname.c_str());
		exit(1);
	}
	if (0 == tid) {
		fscanf(fin, "%u %u", &NNODES, &NEDGES);
	}
	unsigned bound_index;
	if (NUM_THREADS - 1 != tid) {
		bound_index = offset + edge_bound;
	} else {
		bound_index = NEDGES;
	}
	for (unsigned index = offset; index < bound_index; ++index) {
		unsigned n1;
		unsigned n2;
		fscanf(fin, "%u %u", &n1, &n2);
		n1--;
		n2--;
		graph_edges_reverse[index] = n2;
	}
	fclose(fin);
}
	// CSR
	unsigned edge_start = 0;
	for (unsigned i = 0; i < NNODES; ++i) {
		graph_vertices[i] = edge_start;
		edge_start += graph_degrees[i];
	}
//Reverse
	edge_start = 0;
	for (unsigned i = 0; i < NNODES; ++i) {
		graph_vertices_reverse[i] = edge_start;
		edge_start += graph_degrees_reverse[i];
	}

}

class Serial_bc {
private:

inline unsigned update_visited(
		int *h_graph_mask,
		int *h_graph_visited)
{
	unsigned count = 0;
#pragma omp parallel for reduction(+: count)
	for (unsigned i = 0; i < NNODES; ++i) {
		if (h_graph_mask[i]) {
			h_graph_visited[i] = 1;
			++count;
		}
	}
	return count;
}

inline void update_visited_reverse(
		int *h_graph_mask,
		int *h_graph_visited,
		float *dependencies,
		float *inverse_num_paths)
{
#pragma omp parallel for 
	for (unsigned i = 0; i < NNODES; ++i) {
		if (h_graph_mask[i]) {
			h_graph_visited[i] = 1;
			dependencies[i] += inverse_num_paths[i];
		}
	}
}

//inline int *BFS_kernel(
//		unsigned *graph_vertices,
//		unsigned *graph_edges,
//		unsigned *graph_degrees,
//		int *h_graph_mask,
//		int *h_graph_visited,
//		unsigned *num_paths)
//{
//	int *new_frontier = (int *) calloc(NNODES, sizeof(int));
//	//unsigned new_frontier_size = 0;
//
////#pragma omp parallel for schedule(dynamic, 512) reduction(+: new_frontier_size)
//#pragma omp parallel for schedule(dynamic, 512)
//	for(unsigned int head_id = 0; head_id < NNODES; head_id++ )
//	{
//		if (!h_graph_mask[head_id]) {
//			continue;
//		}
//		//unsigned num_paths_head = num_paths[head_id];
//		unsigned bound_edge_i = graph_vertices[head_id] + graph_degrees[head_id];
//		for (unsigned edge_i = graph_vertices[head_id]; edge_i < bound_edge_i; ++edge_i) {
//			unsigned tail_id = graph_edges[edge_i];
//			if (h_graph_visited[tail_id]) {
//				continue;
//			}
//			// Change in new_frontier
//			if (!new_frontier[tail_id]) {
//				new_frontier[tail_id] = 1;
//			}
//			// Update num_paths
//			volatile unsigned old_val = num_paths[tail_id];
//			volatile unsigned new_val = old_val + num_paths[head_id];
//			while (!__sync_bool_compare_and_swap(num_paths + tail_id, old_val, new_val)) {
//				old_val = num_paths[tail_id];
//				new_val = old_val + num_paths[head_id];
//			}
//		}
//	}
//
//	return new_frontier;
//}
//
//void BFS_kernel_reverse(
//			unsigned *graph_vertices_reverse,
//			unsigned *graph_edges_reverse,
//			unsigned *graph_degrees_reverse,
//			int *h_graph_mask,
//			int *h_graph_visited,
//			unsigned *num_paths,
//			float *dependencies)
//
//{
//#pragma omp parallel for schedule(dynamic, 512)
//	for(unsigned int head_id = 0; head_id < NNODES; head_id++ )
//	{
//		if (!h_graph_mask[head_id]) {
//			continue;
//		}
//		unsigned bound_edge_i = graph_vertices_reverse[head_id] + graph_degrees_reverse[head_id];
//		for (unsigned edge_i = graph_vertices_reverse[head_id]; edge_i < bound_edge_i; ++edge_i) {
//			unsigned tail_id = graph_edges_reverse[edge_i];
//			if (h_graph_visited[tail_id]) {
//				continue;
//			}
//			volatile float old_val;
//			volatile float new_val;
//			do {
//				old_val = dependencies[tail_id];
//				new_val = old_val + dependencies[head_id];
//			} while (!__sync_bool_compare_and_swap(
//											(int *) (dependencies + tail_id), 
//											*((int *) &old_val), 
//											*((int *) &new_val)));
//		}
//	}
//}
///////////////////////////////////////////////////////
// Sparse
void update_visited_sparse_reverse(
		unsigned *h_graph_queue,
		const unsigned &queue_size,
		int *h_graph_visited,
		float *dependencies,
		float *inverse_num_paths)
{
#pragma omp parallel for
	for (unsigned i = 0; i < queue_size; ++i) {
		unsigned tail_id = h_graph_queue[i];
		h_graph_visited[tail_id] = 1;
		dependencies[tail_id] += inverse_num_paths[tail_id];
	}
}
unsigned *to_sparse(
		const unsigned &frontier_size,
		unsigned *h_graph_mask)
{
	unsigned *new_frontier = (unsigned *) malloc(frontier_size * sizeof(unsigned));

	const unsigned block_size = 1 << 12;
	unsigned num_blocks = (NNODES - 1)/block_size + 1;
	unsigned *nums_in_blocks = nullptr;
	
	if (num_blocks > 1) {
		nums_in_blocks = (unsigned *) malloc(num_blocks * sizeof(unsigned));
		memset(nums_in_blocks, 0, num_blocks * sizeof(unsigned));
		// The start locations where the vertices are put in the frontier.
#pragma omp parallel for
		for (unsigned block_i = 0; block_i < num_blocks; ++block_i) {
			unsigned offset = block_i * block_size;
			unsigned bound;
			if (num_blocks - 1 != block_i) {
				bound = offset + block_size;
			} else {
				bound = NNODES;
			}
			for (unsigned vertex_i = offset; vertex_i < bound; ++vertex_i) {
				if (h_graph_mask[vertex_i]) {
					nums_in_blocks[block_i]++;
				}
			}
		}
		//TODO: blocked parallel for
		// Scan to get the offsets as start locations.
		unsigned offset_sum = 0;
		for (unsigned block_i = 0; block_i < num_blocks; ++block_i) {
			unsigned tmp = nums_in_blocks[block_i];
			nums_in_blocks[block_i] = offset_sum;
			offset_sum += tmp;
		}
		// Put vertices into the frontier
#pragma omp parallel for
		for (unsigned block_i = 0; block_i < num_blocks; ++block_i) {
			unsigned base = nums_in_blocks[block_i];
			unsigned offset = block_i * block_size;
			unsigned bound;
			if (num_blocks - 1 != block_i) {
				bound = offset + block_size;
			} else {
				bound = NNODES;
			}
			for (unsigned vertex_i = offset; vertex_i < bound; ++vertex_i) {
				if (h_graph_mask[vertex_i]) {
					new_frontier[base++] = vertex_i;
				}
			}
		}
		free(nums_in_blocks);
	} else {
		unsigned k = 0;
		for (unsigned i = 0; i < NNODES; ++i) {
			if (h_graph_mask[i]) {
				new_frontier[k++] = i;
			}
		}
	}
	return new_frontier;
}
inline unsigned *BFS_kernel_sparse(
				unsigned *graph_vertices,
				unsigned *graph_edges,
				unsigned *graph_degrees,
				int *h_graph_visited,
				unsigned *h_graph_queue,
				unsigned &queue_size,
				unsigned *num_paths)
{
	// From h_graph_queue, get the degrees (para_for)
	unsigned *degrees = (unsigned *) malloc(sizeof(unsigned) *  queue_size);
	unsigned new_queue_size = 0;
//#pragma omp parallel for schedule(dynamic) reduction(+: new_queue_size)
#pragma omp parallel for reduction(+: new_queue_size)
	for (unsigned i = 0; i < queue_size; ++i) {
		degrees[i] = graph_degrees[h_graph_queue[i]];
		new_queue_size += degrees[i];
	}
	if (0 == new_queue_size) {
		free(degrees);
		queue_size = 0;
		//h_graph_queue = nullptr;
		return nullptr;
	}

	// From degrees, get the offset (stored in degrees) (block_para_for)
	// TODO: blocked parallel for
	unsigned offset_sum = 0;
	for (unsigned i = 0; i < queue_size; ++i) {
		unsigned tmp = degrees[i];
		degrees[i] = offset_sum;
		offset_sum += tmp;
	}

	// From offset, get active vertices (para_for)
	unsigned *new_frontier_tmp = (unsigned *) malloc(sizeof(unsigned) * new_queue_size);
#pragma omp parallel for schedule(dynamic, CHUNK_SIZE)
	for (unsigned i = 0; i < queue_size; ++i) {
		unsigned start = h_graph_queue[i];
		unsigned offset = degrees[i];
		unsigned out_degree = graph_degrees[start];
		unsigned base = graph_vertices[start];
		for (unsigned k = 0; k < out_degree; ++k) {
			unsigned end = graph_edges[base + k];
			if (0 == h_graph_visited[end]) {
				//bool unvisited = __sync_bool_compare_and_swap(h_graph_visited + end, 0, 1); //update h_graph_visited
				//if (unvisited) {
				//	new_frontier_tmp[offset + k] = end;
				//} else {
				//	new_frontier_tmp[offset + k] = (unsigned) -1;
				//}
				// Update num_paths
				volatile unsigned old_val;
				volatile unsigned new_val;
				do {
					old_val = num_paths[end];
					new_val = old_val + num_paths[start];
				} while (!__sync_bool_compare_and_swap(num_paths + end, old_val, new_val));
				if (old_val == 0.0) {
					new_frontier_tmp[offset + k] = end;
				} else {
					new_frontier_tmp[offset + k] = (unsigned) -1;
				}
			} else {
				new_frontier_tmp[offset + k] = (unsigned) -1;
			}
		}
	}


	// Refine active vertices, removing visited and redundant (block_para_for)
	unsigned block_size = 1024 * 2;
	unsigned num_blocks = (new_queue_size - 1)/block_size + 1;

	unsigned *nums_in_blocks = NULL;
	if (num_blocks > 1) {
	nums_in_blocks = (unsigned *) malloc(sizeof(unsigned) * num_blocks);
	unsigned new_queue_size_tmp = 0;
//#pragma omp parallel for schedule(dynamic) reduction(+: new_queue_size_tmp)
#pragma omp parallel for reduction(+: new_queue_size_tmp)
	for (unsigned block_i = 0; block_i < num_blocks; ++block_i) {
		unsigned offset = block_i * block_size;
		unsigned bound;
		if (num_blocks - 1 != block_i) {
			bound = offset + block_size;
		} else {
			bound = new_queue_size;
		}
		unsigned base = offset;
		for (unsigned end_i = offset; end_i < bound; ++end_i) {
			if ((unsigned) - 1 != new_frontier_tmp[end_i]) {
				new_frontier_tmp[base++] = new_frontier_tmp[end_i];
			}
		}
		nums_in_blocks[block_i] = base - offset;
		new_queue_size_tmp += nums_in_blocks[block_i];
	}
	new_queue_size = new_queue_size_tmp;
	} else {
		unsigned base = 0;
		for (unsigned i = 0; i < new_queue_size; ++i) {
			if ((unsigned) -1 != new_frontier_tmp[i]) {
				new_frontier_tmp[base++] = new_frontier_tmp[i];
			}
		}
		new_queue_size = base;
	}
	
	if (0 == new_queue_size) {
		free(degrees);
		free(new_frontier_tmp);
		if (nums_in_blocks) {
			free(nums_in_blocks);
		}
		queue_size = 0;
		return nullptr;
	}

	// Get the final new frontier
	unsigned *new_frontier = (unsigned *) malloc(sizeof(unsigned) * new_queue_size);
	if (num_blocks > 1) {
	//TODO: blocked parallel for
	offset_sum = 0;
	for (unsigned i = 0; i < num_blocks; ++i) {
		unsigned tmp = nums_in_blocks[i];
		nums_in_blocks[i] = offset_sum;
		offset_sum += tmp;
	}
//#pragma omp parallel for schedule(dynamic)
#pragma omp parallel for
	for (unsigned block_i = 0; block_i < num_blocks; ++block_i) {
		unsigned offset = nums_in_blocks[block_i];
		unsigned bound;
		if (num_blocks - 1 != block_i) {
			bound = nums_in_blocks[block_i + 1];
		} else {
			bound = new_queue_size;
		}
		unsigned base = block_i * block_size;
		for (unsigned i = offset; i < bound; ++i) {
			new_frontier[i] = new_frontier_tmp[base++];
		}
	}
	} else {
		unsigned base = 0;
		for (unsigned i = 0; i < new_queue_size; ++i) {
			new_frontier[i] = new_frontier_tmp[base++];
		}
	}

	// Return the results
	free(degrees);
	free(new_frontier_tmp);
	if (nums_in_blocks) {
		free(nums_in_blocks);
	}
	queue_size = new_queue_size;
	return new_frontier;
}
inline unsigned *BFS_sparse(
				unsigned *h_graph_queue,
				unsigned &queue_size,
				unsigned *graph_vertices,
				unsigned *graph_edges,
				unsigned *graph_degrees,
				int *h_graph_visited,
				unsigned *num_paths)
{
	return BFS_kernel_sparse(
				graph_vertices,
				graph_edges,
				graph_degrees,
				h_graph_visited,
				h_graph_queue,
				queue_size,
				num_paths);
}

inline void BFS_kernel_sparse_reverse(
				unsigned *graph_vertices,
				unsigned *graph_edges,
				unsigned *graph_degrees,
				int *h_graph_visited,
				unsigned *h_graph_queue,
				unsigned &queue_size,
				float *dependencies)
{
	// From offset, get active vertices (para_for)
#pragma omp parallel for schedule(dynamic, CHUNK_SIZE)
	for (unsigned i = 0; i < queue_size; ++i) {
		unsigned start = h_graph_queue[i];
		unsigned base_edge_i = graph_vertices[start];
		unsigned bound_edge_i = base_edge_i + graph_degrees[start];
		for (unsigned edge_i = base_edge_i; edge_i < bound_edge_i; ++edge_i) {
			unsigned end = graph_edges[edge_i];
			if (0 == h_graph_visited[end]) {
				volatile float old_val;
				volatile float new_val;
				do {
					old_val = dependencies[end];
					new_val = old_val + dependencies[start];
				} while (!__sync_bool_compare_and_swap(
												(int *) (dependencies + end), 
												*((int *) &old_val), 
												*((int *) &new_val)));
			}
		}
	}
}
inline void BFS_sparse_reverse(
				unsigned *h_graph_queue,
				unsigned &queue_size,
				unsigned *graph_vertices,
				unsigned *graph_edges,
				unsigned *graph_degrees,
				int *h_graph_visited,
				float *dependencies)
{
	BFS_kernel_sparse_reverse(
				graph_vertices,
				graph_edges,
				graph_degrees,
				h_graph_visited,
				h_graph_queue,
				queue_size,
				dependencies);
}
// End Sparse
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Dense

void update_visited_dense_reverse(
		unsigned *h_graph_mask,
		int *h_graph_visited,
		int *is_active_side,
		float *dependencies,
		float *inverse_num_paths)
{
#pragma omp parallel for
	for (unsigned i = 0; i < NNODES; ++i) {
		if (h_graph_mask[i]) {
			h_graph_visited[i] = 1;
			is_active_side[i / TILE_WIDTH] = 1;
			dependencies[i] += inverse_num_paths[i];
		}
	}
}

unsigned *to_dense(
		int *is_active_side, 
		unsigned *h_graph_queue, 
		unsigned frontier_size)
{
	unsigned *new_mask = (unsigned *) calloc(NNODES, sizeof(unsigned));
#pragma omp parallel for
	for (unsigned i = 0; i < frontier_size; ++i) {
		unsigned vertex_id = h_graph_queue[i];
		new_mask[vertex_id] = 1;
		is_active_side[vertex_id / TILE_WIDTH] = 1;
	}
	return new_mask;
}
inline void bfs_kernel_dense(
		const unsigned &start_edge_i,
		const unsigned &bound_edge_i,
		unsigned *h_graph_heads,
		unsigned *h_graph_tails,
		unsigned *h_graph_mask,
		unsigned *h_updating_graph_mask,
		int *h_graph_visited,
		//unsigned *h_graph_parents,
		//int *h_cost,
		int *is_updating_active_side,
		unsigned *num_paths)
{
	for (unsigned edge_i = start_edge_i; edge_i < bound_edge_i; ++edge_i) {
		unsigned head = h_graph_heads[edge_i];
		if (0 == h_graph_mask[head]) {
			//++edge_i;
			continue;
		}
		unsigned end = h_graph_tails[edge_i];
		//if ((unsigned) -1 == h_graph_parents[end]) {
		//	h_cost[end] = h_cost[head] + 1;
		//	h_updating_graph_mask[end] = 1;
		//	is_updating_active_side[end/TILE_WIDTH] = 1;
		//	h_graph_parents[end] = head; // addition
		//}
		if (0 == h_graph_visited[end]) {
			volatile unsigned old_val;
			volatile unsigned new_val;
			do {
				old_val = num_paths[end];
				new_val = old_val + num_paths[head];
			} while (!__sync_bool_compare_and_swap(num_paths + end, old_val, new_val));
			if (old_val == 0.0) {
				h_updating_graph_mask[end] = 1;
				is_updating_active_side[end/TILE_WIDTH] = 1;
			}
		}
	}
}
inline void scheduler_dense(
		const unsigned &start_row_index,
		const unsigned &tile_step,
		unsigned *h_graph_heads,
		unsigned *h_graph_tails,
		unsigned *h_graph_mask,
		unsigned *h_updating_graph_mask,
		int *h_graph_visited,
		//unsigned *h_graph_parents,
		//int *h_cost,
		unsigned *tile_offsets,
		//int *is_empty_tile,
		unsigned *tile_sizes,
		int *is_active_side,
		int *is_updating_active_side,
		unsigned *num_paths)
{
	unsigned start_tile_id = start_row_index * SIDE_LENGTH;
	//unsigned bound_row_id = start_row_index + tile_step;
	unsigned end_tile_id = start_tile_id + tile_step * SIDE_LENGTH;
#pragma omp parallel for schedule(dynamic, 1)
	for (unsigned tile_index = start_tile_id; tile_index < end_tile_id; tile_index += tile_step) {
		unsigned bound_tile_id = tile_index + tile_step;
		for (unsigned tile_id = tile_index; tile_id < bound_tile_id; ++tile_id) {
			unsigned row_id = (tile_id - start_tile_id) % tile_step + start_row_index;
			//if (is_empty_tile[tile_id] || !is_active_side[row_id]) {
			//	continue;
			//}
			if (!tile_sizes[tile_id] || !is_active_side[row_id]) {
				continue;
			}
			// Kernel
			unsigned bound_edge_i;
			if (NUM_TILES - 1 != tile_id) {
				bound_edge_i = tile_offsets[tile_id + 1];
			} else {
				bound_edge_i = NEDGES;
			}
			bfs_kernel_dense(
					tile_offsets[tile_id],
					bound_edge_i,
					h_graph_heads,
					h_graph_tails,
					h_graph_mask,
					h_updating_graph_mask,
					h_graph_visited,
					//h_graph_parents,
					//h_cost,
					is_updating_active_side,
					num_paths);
		}

	}
}

inline unsigned *BFS_dense(
		unsigned *h_graph_heads,
		unsigned *h_graph_tails,
		unsigned *h_graph_mask,
		//int *h_updating_graph_mask,
		int *h_graph_visited,
		//unsigned *h_graph_parents,
		//int *h_cost,
		unsigned *tile_offsets,
		//int *is_empty_tile,
		unsigned *tile_sizes,
		int *is_active_side,
		int *is_updating_active_side,
		unsigned *num_paths)
{
	unsigned *new_mask = (unsigned *) calloc(NNODES, sizeof(unsigned));
	unsigned side_id;
	for (side_id = 0; side_id + ROW_STEP <= SIDE_LENGTH; side_id += ROW_STEP) {
		scheduler_dense(
				//side_id * SIDE_LENGTH,\
				//(side_id + ROW_STEP) * SIDE_LENGTH,
				side_id,
				ROW_STEP,
				h_graph_heads,
				h_graph_tails,
				h_graph_mask,
				new_mask,
				h_graph_visited,
				//h_graph_parents,
				//h_cost,
				tile_offsets,
				//is_empty_tile,
				tile_sizes,
				is_active_side,
				is_updating_active_side,
				num_paths);
	}
	scheduler_dense(
			//side_id * SIDE_LENGTH,\
			//NUM_TILES,
			side_id,
			SIDE_LENGTH - side_id,
			h_graph_heads,
			h_graph_tails,
			h_graph_mask,
			new_mask,
			h_graph_visited,
			//h_graph_parents,
			//h_cost,
			tile_offsets,
			//is_empty_tile,
			tile_sizes,
			is_active_side,
			is_updating_active_side,
			num_paths);

	return new_mask;
}

inline void bfs_kernel_dense_reverse(
		const unsigned &start_edge_i,
		const unsigned &bound_edge_i,
		unsigned *h_graph_heads,
		unsigned *h_graph_tails,
		unsigned *h_graph_mask,
		//unsigned *h_updating_graph_mask,
		int *h_graph_visited,
		//unsigned *h_graph_parents,
		//int *h_cost,
		//int *is_updating_active_side,
		float *dependencies)
{
	for (unsigned edge_i = start_edge_i; edge_i < bound_edge_i; ++edge_i) {
		unsigned head = h_graph_heads[edge_i];
		if (0 == h_graph_mask[head]) {
			//++edge_i;
			continue;
		}
		unsigned end = h_graph_tails[edge_i];
		if (0 == h_graph_visited[end]) {
			volatile float old_val;
			volatile float new_val;
			do {
				old_val = dependencies[end];
				new_val = old_val + dependencies[head];
			} while (!__sync_bool_compare_and_swap(
											(int *) (dependencies + end), 
											*((int *) &old_val), 
											*((int *) &new_val)));
			//do {
			//	old_val = num_paths[end];
			//	new_val = old_val + num_paths[head];
			//} while (!__sync_bool_compare_and_swap(num_paths + end, old_val, new_val));
		}
	}
}
inline void scheduler_dense_reverse(
		const unsigned &start_row_index,
		const unsigned &tile_step,
		unsigned *h_graph_heads,
		unsigned *h_graph_tails,
		unsigned *h_graph_mask,
		//unsigned *h_updating_graph_mask,
		int *h_graph_visited,
		//unsigned *h_graph_parents,
		//int *h_cost,
		unsigned *tile_offsets,
		//int *is_empty_tile,
		unsigned *tile_sizes,
		int *is_active_side,
		//int *is_updating_active_side,
		float *dependencies)
{
	unsigned start_tile_id = start_row_index * SIDE_LENGTH;
	//unsigned bound_row_id = start_row_index + tile_step;
	unsigned end_tile_id = start_tile_id + tile_step * SIDE_LENGTH;
#pragma omp parallel for schedule(dynamic, 1)
	for (unsigned tile_index = start_tile_id; tile_index < end_tile_id; tile_index += tile_step) {
		unsigned bound_tile_id = tile_index + tile_step;
		for (unsigned tile_id = tile_index; tile_id < bound_tile_id; ++tile_id) {
			unsigned row_id = (tile_id - start_tile_id) % tile_step + start_row_index;
			if (!tile_sizes[tile_id] || !is_active_side[row_id]) {
				continue;
			}
			//if (!tile_sizes[tile_id]) {
			//	continue;
			//}
			// Kernel
			unsigned bound_edge_i;
			if (NUM_TILES - 1 != tile_id) {
				bound_edge_i = tile_offsets[tile_id + 1];
			} else {
				bound_edge_i = NEDGES;
			}
			bfs_kernel_dense_reverse(
					tile_offsets[tile_id],
					bound_edge_i,
					h_graph_heads,
					h_graph_tails,
					h_graph_mask,
					//h_updating_graph_mask,
					h_graph_visited,
					//h_graph_parents,
					//h_cost,
					//is_updating_active_side,
					dependencies);
		}

	}
}

inline void BFS_dense_reverse(
		unsigned *h_graph_heads,
		unsigned *h_graph_tails,
		unsigned *h_graph_mask,
		//int *h_updating_graph_mask,
		int *h_graph_visited,
		//unsigned *h_graph_parents,
		//int *h_cost,
		unsigned *tile_offsets,
		//int *is_empty_tile,
		unsigned *tile_sizes,
		int *is_active_side,
		//int *is_updating_active_side,
		float *dependencies)
{
	//unsigned *new_mask = (unsigned *) calloc(NNODES, sizeof(unsigned));
	unsigned remainder = SIDE_LENGTH % ROW_STEP;
	unsigned bound_side_id = SIDE_LENGTH - remainder;
	//unsigned side_id;
	for (unsigned side_id = 0; side_id < bound_side_id; side_id += ROW_STEP) {
		scheduler_dense_reverse(
				//side_id * SIDE_LENGTH,\
				//(side_id + ROW_STEP) * SIDE_LENGTH,
				side_id,
				ROW_STEP,
				h_graph_heads,
				h_graph_tails,
				h_graph_mask,
				//new_mask,
				h_graph_visited,
				//h_graph_parents,
				//h_cost,
				tile_offsets,
				//is_empty_tile,
				tile_sizes,
				is_active_side,
				//is_updating_active_side,
				dependencies);
	}
	scheduler_dense_reverse(
			//side_id * SIDE_LENGTH,\
			//NUM_TILES,
			bound_side_id,
			remainder,
			h_graph_heads,
			h_graph_tails,
			h_graph_mask,
			//new_mask,
			h_graph_visited,
			//h_graph_parents,
			//h_cost,
			tile_offsets,
			//is_empty_tile,
			tile_sizes,
			is_active_side,
			//is_updating_active_side,
			dependencies);

	//return new_mask;
}
// End Dense
////////////////////////////////////////////////////////////////////////

public:
void BC(
		unsigned *graph_heads, 
		unsigned *graph_tails, 
		unsigned *graph_vertices,
		unsigned *graph_edges,
		unsigned *graph_degrees,
		unsigned *tile_offsets,
		unsigned *tile_sizes,
		unsigned *graph_heads_reverse, 
		unsigned *graph_tails_reverse, 
		unsigned *graph_vertices_reverse,
		unsigned *graph_edges_reverse,
		unsigned *graph_degrees_reverse,
		unsigned *tile_offsets_reverse,
		unsigned *tile_sizes_reverse,
		const unsigned &source)
{
	omp_set_num_threads(NUM_THREADS);
	unsigned *num_paths = (unsigned *) calloc(NNODES, sizeof(unsigned));
	int *h_graph_visited = (int *) calloc(NNODES, sizeof(int));
	int *is_active_side = (int *) calloc(SIDE_LENGTH, sizeof(int));
	int *is_updating_active_side = (int *) calloc(SIDE_LENGTH, sizeof(int));
	unsigned *h_graph_mask = nullptr;
	float *dependencies = (float *) calloc(NNODES, sizeof(float));
	vector<unsigned *> frontiers;
	vector<unsigned> frontier_sizes;
	vector<bool> is_dense_frontier;
	unsigned frontier_size;

	num_paths[source] = 1;
	// First is the Sparse
	frontier_size = 1;
	h_graph_visited[source] = 1;
	unsigned *h_graph_queue = (unsigned *) malloc(frontier_size * sizeof(unsigned));
	h_graph_queue[0] = source;
	frontiers.push_back(h_graph_queue);
	is_dense_frontier.push_back(false);
	frontier_sizes.push_back(1);

	double start_time = omp_get_wtime();
	// First Phase
	unsigned *new_queue = BFS_sparse(
								h_graph_queue,
								frontier_size,
								graph_vertices,
								graph_edges,
								graph_degrees,
								h_graph_visited,
								num_paths);
	h_graph_queue = new_queue;
	frontiers.push_back(h_graph_queue);
	frontier_sizes.push_back(frontier_size);
	bool last_is_dense = false;
	is_dense_frontier.push_back(last_is_dense);
	// Update the h_graph_visited, get the sum of the number of active nodes and their out degrees.
	unsigned out_degree = 0;
#pragma omp parallel for reduction(+: out_degree)
	for (unsigned i = 0; i < frontier_size; ++i)
	{
		unsigned vertex_id = h_graph_queue[i];
		out_degree += graph_degrees[vertex_id];
		h_graph_visited[vertex_id] = 1;
	}
	// According the sum, determing to run Sparse or Dense, and then change the last_is_dense.
	unsigned bfs_threshold = NEDGES / T_RATIO;
	while (true) {
		if (frontier_size + out_degree > bfs_threshold) {
			if (!last_is_dense) {
				free(is_active_side);
				is_active_side = (int *) calloc(NNODES, sizeof(int));
				h_graph_mask = to_dense(
					is_active_side, 
					h_graph_queue, 
					frontier_size);
			}
			unsigned *new_mask = BFS_dense(
					graph_heads,
					graph_tails,
					h_graph_mask,
					//h_updating_graph_mask,
					h_graph_visited,
					//h_graph_parents,
					//h_cost,
					tile_offsets,
					//is_empty_tile,
					tile_sizes,
					is_active_side,
					is_updating_active_side,
					num_paths);
			if (!last_is_dense) {
				free(h_graph_mask);
			}
			h_graph_mask = new_mask;
			last_is_dense = true;
			frontiers.push_back(h_graph_mask);
			is_dense_frontier.push_back(last_is_dense);
		} else {
			// Sparse
			if (last_is_dense) {
				h_graph_queue = to_sparse(
					frontier_size,
					h_graph_mask);
			}
			new_queue = BFS_sparse(
								h_graph_queue,
								frontier_size,
								graph_vertices,
								graph_edges,
								graph_degrees,
								h_graph_visited,
								num_paths);
			if (last_is_dense) {
				free(h_graph_queue);
			}
			h_graph_queue = new_queue;
			last_is_dense = false;
			frontiers.push_back(h_graph_queue);
			is_dense_frontier.push_back(last_is_dense);
		}
		// Update h_graph_visited; Get the sum again.
		if (last_is_dense) {
			frontier_size = 0;
			out_degree = 0;
#pragma omp parallel for reduction(+: frontier_size, out_degree)
			for (unsigned side_id = 0; side_id < SIDE_LENGTH; ++side_id) {
				if (!is_updating_active_side[side_id]) {
					is_active_side[side_id] = 0;
					continue;
				}
				is_updating_active_side[side_id] = 0;
				is_active_side[side_id] = 1;
				unsigned bound_vertex_id;
				if (SIDE_LENGTH - 1 != side_id) {
					bound_vertex_id = side_id * TILE_WIDTH + TILE_WIDTH;
				} else {
					bound_vertex_id = NNODES;
				}
				for (unsigned vertex_id = side_id * TILE_WIDTH; vertex_id < bound_vertex_id; ++vertex_id) {
					if (1 == h_graph_mask[vertex_id]) {
						frontier_size++;
						out_degree += graph_degrees[vertex_id];
						h_graph_visited[vertex_id] = 1;
					}
				}
			}
			frontier_sizes.push_back(frontier_size);
			if (0 == frontier_size) {
				break;
			}
		} else {
			out_degree = 0;
			frontier_sizes.push_back(frontier_size);
			if (0 == frontier_size) {
				break;
			}
#pragma omp parallel for reduction(+: out_degree)
			for (unsigned i = 0; i < frontier_size; ++i) {
				unsigned end = h_graph_queue[i];
				out_degree += graph_degrees[end];
				h_graph_visited[end] = 1;
			}
		}
	}
		
	//// First phase
	//while (0 != frontier_size) {
	//	int *new_frontier = BFS_kernel(
	//							graph_vertices,
	//							graph_edges,
	//							graph_degrees,
	//							h_graph_mask,
	//							h_graph_visited,
	//							num_paths);
	//	h_graph_mask = new_frontier;
	//	frontiers.push_back(h_graph_mask);
	//	frontier_size = update_visited(h_graph_mask, h_graph_visited);
	//	frontiers_sizes.push_back(frontier_size);
	//}
	
//	// Second phase
//	int level_count = frontiers.size() - 1;
//	float *inverse_num_paths = (float *) malloc(NNODES * sizeof(float));
//#pragma omp parallel for num_threads(NUM_THREADS)
//	for (unsigned i = 0; i < NNODES; ++i) {
//		if (num_paths[i] == 0) {
//			inverse_num_paths[i] = 0.0;
//		} else {
//			inverse_num_paths[i] = 1/ (1.0 * num_paths[i]);
//		}
//	}
//#pragma omp parallel for num_threads(NUM_THREADS)
//	for (unsigned i = 0; i < NNODES; ++i) {
//		h_graph_visited[i] = 0;
//	}
//
//	for (int lc = level_count - 1; lc >= 0; --lc) {
//		h_graph_mask = frontiers[lc];
//		//undate_visited();
//		update_visited_reverse(
//						h_graph_mask, 
//						h_graph_visited,
//						dependencies,
//						inverse_num_paths);
//		BFS_kernel_reverse(
//					graph_vertices_reverse,
//					graph_edges_reverse,
//					graph_degrees_reverse,
//					h_graph_mask,
//					h_graph_visited,
//					num_paths,
//					dependencies);
//	}
//
//#pragma omp parallel for num_threads(NUM_THREADS)
//	for (unsigned i = 0; i < NNODES; ++i) {
//		if (inverse_num_paths[i] == 0.0) {
//			dependencies[i] = 0.0;
//		} else {
//			dependencies[i] = (dependencies[i] - inverse_num_paths[i]) / inverse_num_paths[i];
//		}
//	}

	int level_count = frontiers.size() - 1;
	float *inverse_num_paths = (float *) malloc(NNODES * sizeof(float));
#pragma omp parallel for num_threads(NUM_THREADS)
	for (unsigned i = 0; i < NNODES; ++i) {
		if (num_paths[i] == 0) {
			inverse_num_paths[i] = 0.0;
		} else {
			inverse_num_paths[i] = 1/ (1.0 * num_paths[i]);
		}
	}
	free(h_graph_visited);
	h_graph_visited = (int *) calloc(NNODES, sizeof(int));

	unsigned *frontier;
	for (int lc = level_count - 1; lc >= 0; --lc) {
		frontier = frontiers[lc];
		if (is_dense_frontier[lc]) {
			update_visited_dense_reverse(
										frontier,
										h_graph_visited,
										is_active_side,
										dependencies,
										inverse_num_paths);
			BFS_dense_reverse(
					graph_heads_reverse,
					graph_tails_reverse,
					frontier,
					h_graph_visited,
					tile_offsets_reverse,
					tile_sizes_reverse,
					is_active_side,
					dependencies);
		} else {
			unsigned queue_size = frontier_sizes[lc];
			update_visited_sparse_reverse(
										frontier,
										queue_size,
										h_graph_visited,
										dependencies,
										inverse_num_paths);
			BFS_sparse_reverse(
							frontier,
							queue_size,
							graph_vertices_reverse,
							graph_edges_reverse,
							graph_degrees_reverse,
							h_graph_visited,
							dependencies);
		}
	}
	////Test
	//puts("Before:");
	//for (unsigned i = 0; i < NNODES; ++i) {
	//	printf("dependencies[%d]: %f\n",i, dependencies[i]);
	//}
	////End Test

#pragma omp parallel for num_threads(NUM_THREADS)
	for (unsigned i = 0; i < NNODES; ++i) {
		if (inverse_num_paths[i] == 0.0) {
			dependencies[i] = 0.0;
		} else {
			dependencies[i] = (dependencies[i] - inverse_num_paths[i]) / inverse_num_paths[i];
		}
	}

	double run_time;
	printf("%u %f\n", NUM_THREADS, run_time = omp_get_wtime() - start_time);
	record_best_performance(run_time, NUM_THREADS);

	//////Test
	//////puts("After:");
	//FILE *fout = fopen("output.txt", "w");
	//for (unsigned i = 0; i < NNODES; ++i) {
	//	fprintf(fout, "d[%u]: %f\n", i, dependencies[i]);
	//}
	//fclose(fout);
	//////End Test
	
	// Free memory
	for (auto f = frontiers.begin(); f != frontiers.end(); ++f) {
		free(*f);
	}
	free(num_paths);
	free(h_graph_visited);
	free(dependencies);
	free(is_active_side);
	free(is_updating_active_side);
	free(inverse_num_paths);
}
}; // class Serial_bc
int main(int argc, char *argv[]) 
{
	start = omp_get_wtime();
	char *filename;
	if (argc > 3) {
		filename = argv[1];
		TILE_WIDTH = strtoul(argv[2], NULL, 0);
		ROW_STEP = strtoul(argv[3], NULL, 0);
	} else {
		filename = "/home/zpeng/benchmarks/data/pokec_combine/soc-pokec";
		//filename = "/sciclone/scr-mlt/zpeng01/pokec_combine/soc-pokec";
		TILE_WIDTH = 1024;
		ROW_STEP = 16;
	}
	// Input
	unsigned *graph_heads;
	unsigned *graph_tails;
	unsigned *graph_vertices;
	unsigned *graph_edges;
	unsigned *graph_degrees;
	unsigned *tile_offsets;
	unsigned *tile_sizes;

	unsigned *graph_heads_reverse;
	unsigned *graph_tails_reverse;
	unsigned *graph_vertices_reverse;
	unsigned *graph_edges_reverse;
	unsigned *graph_degrees_reverse;
	unsigned *tile_offsets_reverse;
	unsigned *tile_sizes_reverse;

	input(
		filename, 
		graph_heads, 
		graph_tails, 
		graph_vertices,
		graph_edges,
		graph_degrees,
		tile_offsets,
		tile_sizes,
		graph_heads_reverse,
		graph_tails_reverse,
		graph_vertices_reverse,
		graph_edges_reverse,
		graph_degrees_reverse,
		tile_offsets_reverse,
		tile_sizes_reverse);

	unsigned source = 0;

	now = omp_get_wtime();
	time_out = fopen(time_file, "w");
	fprintf(time_out, "input end: %lf\n", now - start);
#ifdef ONEDEBUG
	printf("Input finished: %s\n", filename);
	unsigned run_count = 7;
#else
	unsigned run_count = 9;
#endif
	T_RATIO = 40;
	CHUNK_SIZE = 2048;
	// BFS
	for (int cz = 0; cz < 25; ++cz) {
	for (unsigned i = 6; i < run_count; ++i) {
		NUM_THREADS = (unsigned) pow(2, i);
#ifndef ONEDEBUG
		//sleep(10);
#endif
		for (int k = 0; k < 3; ++k) {
		BC(
			graph_heads, 
			graph_tails, 
			graph_vertices,
			graph_edges,
			graph_degrees,
			tile_offsets,
			tile_sizes,
			graph_heads_reverse, 
			graph_tails_reverse, 
			graph_vertices_reverse,
			graph_edges_reverse,
			graph_degrees_reverse,
			tile_offsets_reverse,
			tile_sizes_reverse,
			source);
		//// Re-initializing
		now = omp_get_wtime();
		fprintf(time_out, "Thread %u end: %lf\n", NUM_THREADS, now - start);
		}
	}
	}
	print_best_performance();
	fclose(time_out);

	// Free memory
	free(graph_heads);
	free(graph_tails);
	free(graph_vertices);
	free(graph_edges);
	free(graph_degrees);
	free(tile_offsets);
	free(tile_sizes);
	free(graph_heads_reverse);
	free(graph_tails_reverse);
	free(graph_vertices_reverse);
	free(graph_edges_reverse);
	free(graph_degrees_reverse);
	free(tile_offsets_reverse);
	free(tile_sizes_reverse);

	return 0;
}
