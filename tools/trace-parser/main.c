/*
 * Copyright(C) 2015 Amanda Amorim <amandamp.amorim@gmail.com>
 *                   Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * This file is part of Mapper.
 * 
 * Mapper is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Mapper is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Mapper. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <mylib/object.h>
#include <mylib/hash.h>
#include <mylib/cache.h>

#include "access.h"
#include "trace-parser.h"




int main(int argc, char **argv)
{
	unsigned size_cache = 256;
	int th = 0;
	
	FILE * swp;
	
	swp = fopen("swap.swp", "w+");
	
	if(swp == NULL){
		printf("Arquivo de swp não pode ser aberto");
		return(EXIT_FAILURE);
	}
	
	FILE * trace;
	
	trace = fopen("/home/amanda/teste.out", "r");
	if(trace == NULL){
		printf("Arquivo de trace não pode ser aberto");
		return(EXIT_FAILURE);
	}
	
	struct cache * c = cache_create( &access_info, swp, size_cache);
	
	
	trace_read(c, trace, th);
	
		
	fclose(swp);
	fclose(trace);

	return (EXIT_SUCCESS);
}

	

