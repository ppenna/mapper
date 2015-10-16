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

#include <inttypes.h> 
#include <mylib/util.h>
#include <mylib/cache.h>
#include <mylib/object.h>
#include <mylib/matrix.h>
#include "access.h"


 void trace_read(struct cache *c, FILE * trace, int th){
	
	char rw;
	int size;
	uint64_t addr;	
	char line[80];
	char ponto1; //recebe o ponto e vírgula
	char ponto2; //recebe o ponto e vírgula
	
	int y;
	
	struct access *accessMem;

	
	while( (fscanf(trace,"%s", line))!=EOF ){
				
			sscanf(line, "%c%c%d%c%" PRIx64 "%*d", &rw, &ponto1, &size, &ponto2, &addr);	
	

			if (rw != 'R' && rw != 'W') //Não faz nada se não for leitura e escrita
				continue;
			
			uint64_t x;
			for( x = addr; x < (addr+size); x++ ){
		
				//Buscar o access do addr lido na cache
				if (sizeof(key_t) < sizeof(uint64_t)){
					printf ("uint64_t= %u \n key_t= %u \n", sizeof(uint64_t), sizeof(key_t));
					error("Mapeamento de variáveis de tamanhos diferentes");
				}
				accessMem = (struct access*) cache_get(c, x);
				
								
				if (accessMem !=NULL){

					//Acrescentar o acesso lido 
					if (rw == 'W')
						accessMem->access[th] += 2;
					else
						accessMem->access[th] += 1;
					
					//Atualizar a cache
					//fprintf(stderr, "\nAtualizar acesso na cache\n");
					cache_update(c, accessMem);

				}else{
					//Criar um novo access
					accessMem = access_create();
					accessMem->addr = x;
					//Inicializar vetor de acessos
					for(y=0; y<QTD_THREADS; y++){
							accessMem->access[y] = 0;
					}
										
					if (rw == 'W')
						accessMem->access[th] += 2;
					else
						accessMem->access[th] += 1;
					//Inserir na cache
					if (sizeof(key_t) < sizeof(uint64_t)){
						printf ("uint64_t= %u \n key_t= %u \n", sizeof(uint64_t), sizeof(key_t));
						error("Mapeamento de variáveis de tamanhos diferentes");
					}
					//fprintf(stderr, "\nInserir acesso na cache\n");
					cache_insert(c, accessMem);
				}
			}
			
		}//Fim while
	
}


void matrix_generate(FILE *swp, struct matrix *m){
	
	//Pensar e entender matrix , percorrer cache 
	int x;
	int y;
	int e;
	
	
	struct access *a;
	a = smalloc(sizeof(struct access));
	
	//Ler acessos gravados na swap
	a = access_read(swp);
		
	while(!feof(swp)){


		if (a==NULL){
				fprintf(stderr,"\nErro ao ler o acesso do arquivo\n");
				break;
		}
		
		//fprintf(stderr,"\na->addr=%x, a->access[0]=%d, a->access[1]=%d, a->access[2]=%d \n",a->addr, a->access[0], a->access[1], a->access[2]);


		//Verificar os compartilhamentos entre cada par de threads
		for(x=0; x <QTD_THREADS; x++ ){
			for(y=0; y<QTD_THREADS; y++){
				if(a->access[x] != 0 && a->access[y] != 0){
					//Obter a quantidade de bytes compartilhados pelas
					//threads X, Y até o momento
					e = matrix_get(m, x, y);
					//fprintf(stderr, "\nx=%d, y=%d, e=%d\n", x, y, e);
					if (a->access[x] <= a->access[y])
						e += a->access[x];
					else
						e += a->access[y];	
					
					matrix_set(m, x, y, e);	
				
				}			
			}
		}
		
		//Ler acessos gravados na swap
		a = access_read(swp);
		
	}
	
	
}
