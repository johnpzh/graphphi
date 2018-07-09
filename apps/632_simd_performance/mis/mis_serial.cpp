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

class Serial_mis {
private:

enum Status {
		UNDECIDED,
		SEMI_IN,
		OUT,
		IN
};

bool mis_check(
			unsigned *graph_vertices,
			unsigned *graph_edges,
			unsigned *graph_degrees,
			Status *h_graph_flags)
{
	bool correct = true;
#pragma omp parallel for
	for (unsigned head = 0; head < NNODES; ++head) {
		unsigned num_in_ngh = 0;
		unsigned start_tail_i = graph_vertices[head];
		unsigned bound_tail_i = start_tail_i + graph_degrees[head];
		for (unsigned tail_i = start_tail_i; tail_i < bound_tail_i; ++tail_i) {
			unsigned tail = graph_edges[tail_i];
			
			if (IN == h_graph_flags[head] && IN == h_graph_flags[tail]) {
				correct = false;
				puts("head and tail?");//test
			}

			if (IN == h_graph_flags[tail]) {
				num_in_ngh++;
			}
		}
		if (IN != h_graph_flags[head] && num_in_ngh == 0) {
			correct = false;
			puts("no head and no tail?");//test
		}
	}

	return correct;
}


///////////////////////////////////////////////////////
// Sparse
inline unsigned *update_flags_sparse(
		unsigned *h_graph_queue,
		unsigned &_queue_size,
		unsigned &_out_degree,
		unsigned *graph_degrees,
		Status *h_graph_flags)
{
	unsigned *new_queue_m = (unsigned *) malloc(_queue_size * sizeof(unsigned));
	unsigned queue_size = 0;
	unsigned out_degree = 0;
#pragma omp parallel for reduction(+: queue_size, out_degree)
	for (unsigned i = 0; i < queue_size; ++i) {
		unsigned v_id = h_graph_queue[i];
		if (SEMI_IN == h_graph_flags[v_id]) {
			h_graph_flags[v_id] = IN;
			new_queue_m[i] = 0;
			//h_graph_mask[v_id] = 0;
		} else if (OUT == h_graph_flags[v_id]) {
			new_queue_m[i] = 0;
			//h_graph_mask[v_id] = 0;
		} else {
			h_graph_flags[v_id] = SEMI_IN;
			out_degree += graph_degrees[v_id];
			++queue_size;
			new_queue_m[i] = 1;
		}

	}
	
	unsigned *new_queue = (unsigned *) malloc(queue_size * sizeof(unsigned));
	unsigned num_blocks = NUM_THREADS;
	unsigned block_size = queue_size / num_blocks;
	if (block_size < 2) {
		// Serial
		unsigned k = 0;
		for (unsigned i = 0; i < queue_size; ++i) {
			if (new_queue_m[i]) {
				new_queue[k++] = h_graph_queue[i];
			}
		}
	} else {
		// Parallel blocks
		// Get number of one in every block, also 
		unsigned *nums_in_blocks = (unsigned *) calloc(num_blocks, sizeof(unsigned));
#pragma omp parallel for
		for (unsigned block_i = 0; block_i < num_blocks; ++block_i) {
			unsigned offset = block_i * block_size;
			unsigned bound_v_i;
			if (num_blocks - 1 != block_i) {
				bound_v_i = offset + block_size;
			} else {
				bound_v_i = _queue_size;
			}
			unsigned base = offset;
			for (unsigned v_i = offset; v_i < bound_v_i; ++v_i) {
				if (new_queue_m[v_i]) {
					h_graph_queue[base++] = h_graph_queue[v_i];
				}
			}
			nums_in_blocks[block_i] = base - offset;
		}

		// Get the offsets of each blocks
		// TODO: prefix sum
		unsigned prefix_sum = 0;
		for (unsigned i = 0; i < num_blocks; ++i) {
			unsigned tmp = nums_in_blocks[i];
			nums_in_blocks[i] = prefix_sum;
			prefix_sum += tmp;
		}

		// Put vertices into the new queue
#pragma omp parallel for
		for (unsigned block_i = 0; block_i < num_blocks; ++block_i) {
			unsigned offset = nums_in_blocks[block_i];
			unsigned bound_v_i;
			if (num_blocks - 1 != block_i) {
				bound_v_i = nums_in_blocks[block_i + 1];
			} else {
				bound_v_i = queue_size;
			}
			unsigned base = block_i * block_size;
			for (unsigned v_i = offset; v_i < bound_v_i; ++v_i) {
				new_queue[v_i] = h_graph_queue[base++];
			}
		}

		free(nums_in_blocks);
	}

	_queue_size = queue_size;
	_out_degree = out_degree;

	free(new_queue_m);
	return new_queue;
}
unsigned *to_sparse(
		int *h_graph_mask,
		const unsigned &frontier_size)
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
inline void BFS_kernel_sparse(
				unsigned *graph_vertices,
				unsigned *graph_edges,
				unsigned *graph_degrees,
				//int *h_graph_visited,
				unsigned *h_graph_queue,
				const unsigned &queue_size,
				//unsigned *num_paths)
				Status *h_graph_flags)
{
#pragma omp parallel for
	for (unsigned i = 0; i < queue_size; ++i) {
		unsigned head = h_graph_queue[i];
		unsigned start_edge_i = graph_vertices[head];
		unsigned bound_edge_i = start_edge_i + graph_degrees[head];
		for (unsigned edge_i = start_edge_i; edge_i < bound_edge_i; ++edge_i) {
			unsigned tail = graph_edges[edge_i];
			if (IN == h_graph_flags[tail]) {
				if (OUT != h_graph_flags[head]) {
					h_graph_flags[head] = OUT;
				}
			} else if (tail < head && h_graph_flags[tail] < OUT && SEMI_IN == h_graph_flags[head]) {
				h_graph_flags[head] = UNDECIDED;
			}
			//if (IN == h_graph_flags[tail]) {
			//	if (OUT != h_graph_flags[head]) {
			//		h_graph_flags[head] = OUT;
			//	}
			//} else if (SEMI_IN == h_graph_flags[tail] && SEMI_IN == h_graph_flags[head]) {
			//	h_graph_flags[head] = UNDECIDED;
			//}
		}
	}
}
inline void BFS_sparse(
				unsigned *h_graph_queue,
				unsigned &queue_size,
				unsigned *graph_vertices,
				unsigned *graph_edges,
				unsigned *graph_degrees,
				//int *h_graph_visited,
				//unsigned *num_paths)
				Status *h_graph_flags)
{
	BFS_kernel_sparse(
				graph_vertices,
				graph_edges,
				graph_degrees,
				//h_graph_visited,
				h_graph_queue,
				queue_size,
				//num_paths);
				h_graph_flags);
}
// End Sparse
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// Dense

void update_flags_dense(
				unsigned &_frontier_size,
				unsigned &_out_degree,
				int *h_graph_mask,
				int *is_active_side,
				Status *h_graph_flags,
				unsigned *graph_degrees)
{
	unsigned out_degree = 0;
	unsigned frontier_size = 0;
	memset(is_active_side, 0, SIDE_LENGTH * sizeof(int));
#pragma omp parallel for reduction(+: frontier_size, out_degree)
	for (unsigned v_id = 0; v_id < NNODES; ++v_id) {
		if (!h_graph_mask[v_id]) {
			continue;
		}
		if (SEMI_IN == h_graph_flags[v_id]) {
			h_graph_flags[v_id] = IN;
			h_graph_mask[v_id] = 0;
		} else if (OUT == h_graph_flags[v_id]) {
			h_graph_mask[v_id] = 0;
		} else {
			h_graph_flags[v_id] = SEMI_IN;
			out_degree += graph_degrees[v_id];
			++frontier_size;
			unsigned side_id = v_id / TILE_WIDTH;
			if (!is_active_side[side_id]) {
				is_active_side[side_id] = 1;
			}
		}
	}
	_frontier_size = frontier_size;
	_out_degree = out_degree;
}

void to_dense(
			unsigned *h_graph_queue,
			unsigned frontier_size,
			int *h_graph_mask,
			int *is_active_side)
{
	memset(h_graph_mask, 0, NNODES * sizeof(int));
	memset(is_active_side, 0, SIDE_LENGTH * sizeof(int));
#pragma omp parallel for
	for (unsigned i = 0; i < frontier_size; ++i) {
		unsigned vertex_id = h_graph_queue[i];
		h_graph_mask[vertex_id] = 1;
		is_active_side[vertex_id / TILE_WIDTH] = 1;
	}
}
inline void bfs_kernel_dense(
		const unsigned &start_edge_i,
		const unsigned &bound_edge_i,
		unsigned *h_graph_heads,
		unsigned *h_graph_tails,
		int *h_graph_mask,
		//unsigned *h_updating_graph_mask,
		//int *h_graph_visited,
		//unsigned *h_graph_parents,
		//int *h_cost,
		//int *is_updating_active_side,
		Status *h_graph_flags)
{
	for (unsigned edge_i = start_edge_i; edge_i < bound_edge_i; ++edge_i) {
		unsigned head = h_graph_heads[edge_i];
		if (0 == h_graph_mask[head]) {
			//++edge_i;
			continue;
		}
		unsigned tail = h_graph_tails[edge_i];
		if (IN == h_graph_flags[tail]) {
			if (OUT != h_graph_flags[head]) {
				h_graph_flags[head] = OUT;
			}
		} else if (tail < head && h_graph_flags[tail] < OUT && SEMI_IN == h_graph_flags[head]) {
			h_graph_flags[head] = UNDECIDED;
		}
		//if (IN == h_graph_flags[tail]) {
		//	if (OUT != h_graph_flags[head]) {
		//		h_graph_flags[head] = OUT;
		//	}
		//} else if (SEMI_IN == h_graph_flags[tail] && SEMI_IN == h_graph_flags[head]) {
		//	h_graph_flags[head] = UNDECIDED;
		//}
	}
}
inline void scheduler_dense(
		const unsigned &start_col_index,
		const unsigned &tile_step,
		unsigned *h_graph_heads,
		unsigned *h_graph_tails,
		int *h_graph_mask,
		//unsigned *h_updating_graph_mask,
		//int *h_graph_visited,
		//unsigned *h_graph_parents,
		//int *h_cost,
		unsigned *tile_offsets,
		unsigned *tile_sizes,
		int *is_active_side,
		//int *is_updating_active_side,
		Status *h_graph_flags)
{
#pragma omp parallel for schedule(dynamic, 1)
	for (unsigned row_id = 0; row_id < SIDE_LENGTH; ++row_id) {
		unsigned bound_col_id = start_col_index + tile_step;
		for (unsigned col_id = start_col_index; col_id < bound_col_id; ++col_id) {
			unsigned tile_id = row_id * SIDE_LENGTH + col_id;
			if (0 == tile_sizes[tile_id] || !is_active_side[col_id]) {
				continue;
			}
			// Kernel
			unsigned start_edge_i = tile_offsets[tile_id];
			unsigned bound_edge_i = start_edge_i + tile_sizes[tile_id];
			//unsigned bound_edge_i;
			//if (NUM_TILES - 1 != tile_id) {
			//	bound_edge_i = tile_offsets[tile_id + 1];
			//} else {
			//	bound_edge_i = NEDGES;
			//}
			bfs_kernel_dense(
					//tile_offsets[tile_id],
					start_edge_i,
					bound_edge_i,
					h_graph_heads,
					h_graph_tails,
					h_graph_mask,
					//h_updating_graph_mask,
					//h_graph_visited,
					//h_graph_parents,
					//h_cost,
					//is_updating_active_side,
					h_graph_flags);
		}
	}
}

inline void BFS_dense(
		unsigned *h_graph_heads,
		unsigned *h_graph_tails,
		int *h_graph_mask,
		unsigned *tile_offsets,
		unsigned *tile_sizes,
		int *is_active_side,
		//int *is_updating_active_side,
		Status *h_graph_flags)
{
	//unsigned *new_mask = (unsigned *) calloc(NNODES, sizeof(unsigned));
	//unsigned side_id;
	unsigned remainder = SIDE_LENGTH % ROW_STEP;
	unsigned bound_side_id = SIDE_LENGTH - remainder;
	for (unsigned side_id = 0; side_id < bound_side_id; side_id += ROW_STEP) {
		scheduler_dense(
				side_id,
				ROW_STEP,
				h_graph_heads,
				h_graph_tails,
				h_graph_mask,
				//new_mask,
				//h_graph_visited,
				//h_graph_parents,
				//h_cost,
				tile_offsets,
				tile_sizes,
				is_active_side,
				//is_updating_active_side,
				h_graph_flags);
	}
	if (remainder) {
		scheduler_dense(
				bound_side_id,
				remainder,
				h_graph_heads,
				h_graph_tails,
				h_graph_mask,
				//new_mask,
				//h_graph_visited,
				//h_graph_parents,
				//h_cost,
				tile_offsets,
				tile_sizes,
				is_active_side,
				//is_updating_active_side,
				h_graph_flags);
	}

	//return new_mask;
}
// End Dense
////////////////////////////////////////////////////////////////////////

public:
void MIS(
		unsigned *graph_heads, 
		unsigned *graph_tails, 
		unsigned *graph_vertices,
		unsigned *graph_edges,
		unsigned *graph_degrees,
		unsigned *tile_offsets,
		unsigned *tile_sizes,
		const unsigned &source)
{
	omp_set_num_threads(NUM_THREADS);
	int *h_graph_mask = (int *) malloc(NNODES * sizeof(int));
	Status *h_graph_flags = (Status *) malloc(NNODES * sizeof(Status));
#pragma omp parallel for
	for (unsigned i = 0; i < NNODES; ++i) {
		h_graph_mask[i] = 1;
		h_graph_flags[i] = SEMI_IN;
	}
	unsigned frontier_size = NNODES;
	unsigned out_degree;
	unsigned dense_threshold = NEDGES / T_RATIO;
	int *is_active_side = (int *) malloc(SIDE_LENGTH * sizeof(int));
#pragma omp parallel for
	for (unsigned i = 0; i < SIDE_LENGTH; ++i) {
		is_active_side[i] = 1;
	}
	//int *is_updating_active_side = (int *) calloc(SIDE_LENGTH, sizeof(int));
	bool last_is_dense;
	unsigned *h_graph_queue = nullptr;
	unsigned *new_queue = nullptr;

	double start_time = omp_get_wtime();
	// Fisrt is Dense
	BFS_dense(
			graph_heads,
			graph_tails,
			h_graph_mask,
			tile_offsets,
			tile_sizes,
			is_active_side,
			//is_updating_active_side,
			h_graph_flags);
	update_flags_dense(
					frontier_size,
					out_degree,
					h_graph_mask,
					is_active_side,
					h_graph_flags,
					graph_degrees);
	last_is_dense = true;
	//printf("frontier_size: %u\n", frontier_size);//test

	while (0 != frontier_size) {
		if (frontier_size + out_degree > dense_threshold) {
			// Dense
			if (!last_is_dense) {
				to_dense(
					h_graph_queue,
					frontier_size,
					h_graph_mask,
					is_active_side);
			}
			BFS_dense(
					graph_heads,
					graph_tails,
					h_graph_mask,
					tile_offsets,
					tile_sizes,
					is_active_side,
					//is_updating_active_side,
					h_graph_flags);
			update_flags_dense(
							frontier_size,
							out_degree,
							h_graph_mask,
							is_active_side,
							h_graph_flags,
							graph_degrees);
			last_is_dense = true;
		} else {
			// Sparse
			if (last_is_dense) {
				 new_queue = to_sparse(
						 			h_graph_mask,
						 			frontier_size);
				 free(h_graph_queue);
				 h_graph_queue = new_queue;
			}
			BFS_sparse(
					h_graph_queue,
					frontier_size,
					graph_vertices,
					graph_edges,
					graph_degrees,
					h_graph_flags);
			new_queue = update_flags_sparse(
					h_graph_queue,
					frontier_size,
					out_degree,
					graph_degrees,
					h_graph_flags);
			free(h_graph_queue);
			h_graph_queue = new_queue;
			last_is_dense = false;
		}
		//printf("frontier_size: %u\n", frontier_size);//test
	}

	printf("%u %f\n", NUM_THREADS, omp_get_wtime() - start_time);
//	unsigned mis_count = 0;
//#pragma omp parallel for reduction(+: mis_count)
//	for (unsigned i = 0; i < NNODES; ++i) {
//		if (IN == h_graph_flags[i]) {
//			mis_count++;
//		}
//	}
//	printf("mis_count: %u\n", mis_count);
	//if (!mis_check(
	//		graph_vertices,
	//		graph_edges,
	//		graph_degrees,
	//		h_graph_flags)) {
	//	printf("Check failed.\n");
	//} else {
	//	puts("Congrats.\n");
	//}

	free(h_graph_mask);
	free(h_graph_flags);
	free(is_active_side);
}
}; // class Serial_mis

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

	input(
		filename, 
		graph_heads, 
		graph_tails, 
		graph_vertices,
		graph_edges,
		graph_degrees,
		tile_offsets,
		tile_sizes);

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
	T_RATIO = 20;
	CHUNK_SIZE = 2048;
	// BFS
	for (int cz = 0; cz < 3; ++cz) {
	for (unsigned i = 6; i < run_count; ++i) {
		NUM_THREADS = (unsigned) pow(2, i);
#ifndef ONEDEBUG
		//sleep(10);
#endif
		for (int k = 0; k < 3; ++k) {
		MIS(
			graph_heads, 
			graph_tails, 
			graph_vertices,
			graph_edges,
			graph_degrees,
			tile_offsets,
			tile_sizes,
			source);
		//// Re-initializing
		now = omp_get_wtime();
		fprintf(time_out, "Thread %u end: %lf\n", NUM_THREADS, now - start);
		}
	}
	}
	fclose(time_out);

	// Free memory
	free(graph_heads);
	free(graph_tails);
	free(graph_vertices);
	free(graph_edges);
	free(graph_degrees);
	free(tile_offsets);
	free(tile_sizes);

	return 0;
}
