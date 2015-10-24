/*
 * Copyright(C) 2015 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 *
 * This file is part of Mapper.
 *
 * Mapper is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MyLib. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdlib.h>

#include <mylib/algorithms.h>
#include <mylib/matrix.h>
#include <mylib/vector.h>
#include <mylib/ai.h>
#include <mylib/util.h>
#include <mylib/table.h>
#include <mylib/queue.h>
 
#include "mapper.h"

/**
 * @brief Destroys centroids.
 * 
 * @param centroids Centroids.
 * @param ncentroids Number of centroids.
 */
static void destroy_centroids(vector_t *centroids, int ncentroids)
{
	for (int i = 0; i < ncentroids; i++)
		vector_destroy(centroids[i]);
	free(centroids);
}

/**
 * @brief Balances processes evenly among clusters using auction's algorithm.
 * 
 * @param procs  Processes.
 * @param nprocs Number of processes.
 * @param map    Unbalanced process map.
 * 
 * @returns A balanced cluster map.
 */
static int *balance(const vector_t *procs, int nprocs, int *map, int nclusters)
{
	matrix_t m;            /* Auction's matrix.    */
	int *balanced_map;     /* Process map.         */
	int ncentroids;        /* Number of centroids. */
	vector_t *centroids;   /* Centroids.           */
	int procs_per_cluster; /* Gotcha?              */
	
	centroids = kmeans_centroids(procs, nprocs, map);
	ncentroids = kmeans_count_centroids(map, nprocs);
	
	if (nprocs%nclusters)
		error("invalid number of clusters");
		
	if (ncentroids != nclusters)
		error("bad number of centroids");
	
	procs_per_cluster = nprocs/nclusters;
	
	m = matrix_create(nprocs, nprocs);
	
	/* Build auction's matrix. */
	for (int i = 0; i < nprocs; i++)
	{
		for (int j = 0; j < nclusters; j++)
		{
			double distance;
			
			distance = vector_distance(procs[i], centroids[j]);
			for (int k = 0; k < procs_per_cluster; k++)
				matrix_set(m, i, procs_per_cluster*j + k, distance);
		}
	}
	
	balanced_map = auction(m, 0.0001);
	
	/* Fix map. */
	for (int i = 0; i < nprocs; i++)
		balanced_map[i] /= procs_per_cluster;
	
	/* House keeping. */
	matrix_destroy(m);
	destroy_centroids(centroids, nclusters);
	
	return (balanced_map);
}

/**
 * @brief Internal implementation of table_split().
 */
static void _table_split
(struct table *t, int i0, int j0, int height, int width, int size)
{
	static int count = 0;
	
	/* Stop condition reached. */
	if (width*height <= size)
	{
		/* Enumerate region. */
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				table_set(t, i0 + i, j0 + j, smalloc(sizeof(int)));
				*INTP(table_get(t, i0 + i, j0 + j)) = count;
			}
		}
		
		count++;
		
		return;
	}
	
	/* Split vertically. */
	if (width > height)
	{
		_table_split(t, i0, j0, height, width/2, size);
		_table_split(t, i0, j0 + width/2, height, width/2, size);
	}
	
	/* Split horizontally. */
	else
	{
		_table_split(t, i0, j0, height/2, width, size);
		_table_split(t, i0 + height/2, j0, height/2, width, size);
	}
}

/**
 * @brief Splits a table recursively.
 * 
 * @param t Target table.
 */
static void table_split(struct table *t, int size)
{
	_table_split(t, 0, 0, table_height(t), table_width(t), size);
}

/**
 * @brief Places processes in the processor.
 * 
 * @param mesh       Processor's topology.
 * @param clustermap Process map.
 * @param nprocs     Number of processes.
 * 
 * @returns Process map.
 */
static int *place
(struct topology *mesh, int *clustermap, int nprocs, int nclusters)
{
	int *map;               /* Process map.        */
	struct table *clusters;	/* Clustered topology. */
	
	map = smalloc(nprocs*sizeof(int));
	clusters = table_create(&integer, mesh->height, mesh->width);
	
	table_split(clusters, nprocs/nclusters);
	
	/* Place processes in the processor. */
	for (int i = 0; i < mesh->height; i++)
	{
		for (int j = 0; j < mesh->width; j++)
		{
			int c;
			
			c = *INTP(table_get(clusters, i, j));
			for (int k = 0; k < nprocs; k++)
			{
				if (clustermap[k] == c)
				{
					clustermap[k] = -1;
					map[k] = i*mesh->width + j;
					break;
				}
			}
		}
	}
	
	/* House keeping. */
	for (int i = 0; i < mesh->height; i++)
	{
		for (int j = 0; j < mesh->width; j++)
			free(table_get(clusters, i, j));
	}
	table_destroy(clusters);
	
	return (map);
}

/**
 * @brief (Balanced) Kmeans clustering.
 * 
 * @param data Data that shall be clustered.
 * @param npoints Number of points that shall be clustered.
 * @param ncentroids Number of centroids
 * 
 * @returns A map that indicates in which cluster each data point is located.
 */
static int *kmeans_balanced(const vector_t *data, int npoints, int ncentroids)
{
	int *clustermap;   /* Cluster map.          */
	int *balanced_map; /* Balanced cluster map. */
	
	clustermap = kmeans(data, npoints, ncentroids, 0.0);
	balanced_map = balance(data, npoints, clustermap, ncentroids);
		
	/* House keeping. */
	free(clustermap);
	
	return (balanced_map);
}

/**
 * @brief Task.
 */
struct task
{
	int mask;
	int depth;
	int npoints;
	int *ids; 
	vector_t *data;
};


/**
 * @brief Creates task.
 */
static struct task *task_create(int depth, unsigned mask, int npoints)
{
	struct task *t;
	
	t = smalloc(sizeof(struct task));
	t->data = smalloc(npoints*sizeof(vector_t));
	t->depth = depth;
	t->mask = mask;
	t->ids = smalloc(npoints*sizeof(int));
	t->npoints = npoints;
	
	return (t);
}

/**
 * @brief Destroys a task.
 */
static void task_destroy(struct task *t)
{
	free(t->data);
	free(t->ids);
	free(t);
}

/**
 * @brief (Hierarchical) Kmeans clustering.
 * 
 * @param data Data that shall be clustered.
 * @param npoints Number of points that shall be clustered.
 * @param ncentroids Number of centroids
 * 
 * @returns A map that indicates in which cluster each data point is located.
 */
static int *kmeans_hierarchical(const vector_t *data, int npoints)
{
	queue_t tasks;   /* Tasks.              */
	struct task *t;  /* Working task.       */
	int *clustermap; /* Current clustermap. */
	
	clustermap = smalloc(npoints*sizeof(int));
	tasks = queue_create(NULL);
	
	/* Create task. */
	t = task_create(0, 0, npoints);
	for (int i = 0; i < npoints; i++)
		t->ids[i] = i, t->data[i] = data[i];
	
	queue_enqueue(tasks, t);
	
	/* Walk through the hierarychy. */
	while (!queue_empty(tasks))
	{
		int *partialmap;
		
		t = queue_dequeue(tasks);
		
		fprintf(stderr, "===\n");
		
		partialmap = kmeans_balanced(t->data, t->npoints, 2);
		
		/* Fix cluster map. */
		for (int i = 0; i < t->npoints; i++)
			clustermap[t->ids[i]] = (partialmap[i] << t->depth) | t->mask;
			
		for (int i = 0; i < npoints; i++)
			fprintf(stderr, "%x ", clustermap[i]);
		fprintf(stderr, "\n");
		
		/* Enqueue child tasks. */
		if (0)
		{
			int mask0, mask1;
			struct task *t0, *t1;
			
			mask0 = (0 << t->depth) | t->mask;
			mask1 = (1 << t->depth) | t->mask;
			
			/* Create tasks. */
			t0 = task_create(t->depth + 1, mask0, t->npoints/2);
			t1 = task_create(t->depth + 1, mask1, t->npoints/2);
			
			for (int i = 0, i0 = 0, i1 = 0; i < npoints; i++)
			{
				if ((clustermap[i] & mask0) == mask0)
					t0->ids[i0] = i, t0->data[i0] = data[i], i0++;
				else if ((clustermap[i] & mask1) == mask1)
					t1->ids[i1] = i, t1->data[i1] = data[i], i1++;
			}
			
			queue_enqueue(tasks, t0);
			queue_enqueue(tasks, t1);
		}
		
		/* House keeping. */
		free(partialmap);
		task_destroy(t);
		fprintf(stderr, "---\n");
	}
	
	/* House keeping. */
	queue_destroy(tasks);
	
	return (clustermap);
}

/**
 * @brief Maps processes using kmeans algorithm.
 *
 * @param procs       Processes.
 * @param nprocs      Number of processes.
 * @param nclusters   Number of clusters.
 * @param use_auction Use auction balancer?
 *
 * @returns A process map.
 */
int *map_kmeans
(const vector_t *procs, int nprocs, void *args)
{
	int *map;              /* Process map.           */
	int *clustermap;       /* Balanced cluster map.  */
	int nclusters;         /* Number of clusters.    */
	struct topology *mesh; /* Processor's topology.  */
	
	UNUSED(kmeans_hierarchical);
	
	/* Extract arguments. */
	nclusters = ((struct kmeans_args *)args)->nclusters;
	mesh = ((struct kmeans_args *)args)->mesh;
	
	/* Sanity check. */
	assert(nclusters > 0);
	assert(nprocs == (mesh->height*mesh->width));
	
	clustermap = kmeans_balanced(procs, nprocs, nclusters);
	
	map = place(mesh, clustermap, nprocs, nclusters);

	/* House keeping. */
	free(clustermap);
	
	return (map);
}
