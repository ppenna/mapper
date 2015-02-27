/*
* Copyright (C) 2013 Pedro H. Penna <pedrohenriquepenna@gmail.com>
*
* <kmeans/kmeans_read.c> - kmeans_read() implementation.
*/

#include <assert.h>
#include <list.h>
#include <matrix.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector.h>
#include "kmeans.h"

/*
 * Reads kmeans() data from a file.
 */
int kmeans_read(FILE *input)
{
        unsigned i, j;
        unsigned size;
        unsigned xsrc, ysrc, src;
        unsigned xdest, ydest, dest;
        struct matrix *m;
        struct process *proc;
        struct list_node *node;
        
        procs = list_create();
        assert(procs != NULL);
        
        nprocs = noc.height*noc.width;
        
        assert((nprocs & 2) == 0);
        assert((noc.height*noc.width) == nprocs);

        m = matrix_create(nprocs, nprocs);
        assert(m != NULL);
        
        /* Read communication matrix. */
        fseek(input, 0, SEEK_SET);
        fscanf(input, "%*u %u %u %*u %u %u %*u %u\n",
			&xsrc, &ysrc, &xdest, &ydest, &size);
        while (!feof(input))
        {
			src = xsrc*noc.width + ysrc;
			dest = xdest*noc.width + ydest;
			
        	MATRIX(m, src, dest) += size;
        	MATRIX(m, dest, src) += size;
        	
			fscanf(input, "%*u %u %u %*u %u %u %*u %u\n",
				&xsrc, &ysrc, &xdest, &ydest, &size);
        }
        
        /* Create processes. */
        for (i = 0; i < nprocs; i++)
        {
			proc = malloc(sizeof(struct process));
			assert(proc != NULL);
                
                proc->traffic = vector_create(nprocs*nprocs);
                assert(proc->traffic);
                
                proc->id = i;
                
                for (j = 0; j < nprocs; j++)
                {
                	VECTOR(proc->traffic, i*nprocs + j) = MATRIX(m, i, j);
                	VECTOR(proc->traffic, j*nprocs + i) = MATRIX(m, j, i);
                }
                
                node = list_node_create(proc);
                assert(node != NULL);
                
                list_insert(procs, node);
        }
        
        matrix_destroy(m);
        
        return (0);
}
