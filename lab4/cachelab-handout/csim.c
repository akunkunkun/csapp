#include "cachelab.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <getopt.h>
#include <string.h>

#define OP_LENGTH 128 

int hit_count , miss_count , eviction_count , timestamp;

typedef struct per_block{
    char isvalid;
    unsigned int tag; // index_line 
    int timestamp;
}per_block;

typedef per_block* set;

typedef struct cache{
    int nsets;
    int nelines;
    int nblock;
    int index_s;
    set* head;
}cache;

typedef struct operation{
    char op ;
    int size ;
    unsigned int address;
}operation; 

cache initial_cache(int s , int e ,int b ){
    int nsets = 1 << s;
    set* head = calloc(nsets, sizeof (set));
    for (int i = 0 ; i < nsets ; i++){
        head[i] = calloc(e,sizeof(per_block));
    } 
    cache c = { nsets , e , b , s,head};
    return c;
}

per_block initial_block(){
    per_block block;
    block.isvalid = 0 ;
    block.tag = -1;
    block.timestamp = 0;
    return block;
}

void free_cache(cache* c ){
    int nsets = c->nsets;
    for (int i = 0 ; i < nsets; i++){
        free(c->head[i]);
        c->head[i] = NULL;
    }
    free(c->head);
    c->head = NULL;
}

unsigned gets_S(unsigned int address , int s , int b){
    return (address >> b) & ((-1U) >> (64 - s));
}

unsigned gets_tag(unsigned address , int s , int b ){
    return address >> (s + b );
}


operation fetch_operation(FILE * infp){
    char temp[OP_LENGTH];

    operation oper={ 0, 0, 0 };
    if ( !fgets(temp,OP_LENGTH,infp)){
        return oper;
    }
    sscanf(temp, "\n%c %x,%d" ,&oper.op, &oper.address ,&oper.size );
    return oper;
}

per_block* ismatch_block(set s, int e , int tag){
    for (int i = 0 ; i < e ; i++){
        //printf("%d %d\n" , s[i].tag , s[i].isvalid);
        if (s[i].tag == tag &&s[i].isvalid)
            return s + i;
    }
    return NULL;
}

per_block* find_new_block(set s , int e){
    for (int i = 0 ; i < e ;i++){
        if (!(s[i].isvalid))
            return s + i;
    }
    return NULL;
}

per_block* find_evict_block(set s , int e ){
    int index = 0;
    for(int i = 1 ; i < e ;i++){
        if (s[i].timestamp < s[index].timestamp ){
            index = i ;
        }
    }
    return s + index;
}

void  load_or_store(cache * c , operation* op , int display , int isDisplay){
    int S = gets_S(op->address , c->index_s , c->nblock );
    int tag = gets_tag(op->address , c->index_s , c->nblock);
    //printf("%d %d \n",S,tag);
    if ( display && isDisplay)
        printf("%c %x,%d ",op->op , op->address,op->size);
    per_block* appro_block = ismatch_block(c->head[S],c->nelines,tag);
    if (appro_block){
        hit_count++;
        if (display)
            printf("hit ");
    }
    else{
        miss_count++;
        if (display )
            printf("miss ");
        appro_block = find_new_block(c->head[S],c->nelines);

        if (appro_block)
            appro_block->isvalid = 1;
        else{
            ++eviction_count;
            appro_block = find_evict_block(c->head[S], c->nelines);
            if (display )
                printf("eviction ");
        }

        appro_block->tag = tag;
    }

    appro_block->timestamp = timestamp;
    timestamp++;
    
    if (display && isDisplay)
        printf("\n");
}

void modify(cache* c, operation* op, int isDisplay)
{
    if (isDisplay)
        printf("%c %x,%d ", op->op, op->address, op->size);
    op->op = 'L';
    load_or_store(c, op, isDisplay,0);
    op->op = 'S';
    load_or_store(c, op, isDisplay,0);
    if (isDisplay)
        printf("\n");
}

void printUsage(){
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
            "Options:\n"
            "  -h         Print this help message.\n"
            "  -v         Optional verbose flag.\n"
            "  -s <num>   Number of set index bits.\n"
            "  -E <num>   Number of lines per set.\n"
            "  -b <num>   Number of block offset bits.\n"
            "  -t <file>  Trace file.\n\n"
            "Examples:\n"
            "  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
            "  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void get_argument(int arc , char ** ars , int* s ,int* e ,int* b , char** path , int* display){
    int opt ; 
    while ( (opt = getopt(arc,ars ,"hvs:E:b:t:")) != -1){
        switch(opt){
			case 'v':
				*display = 1;
				//printUsage();
				break;
			case 's':
				*s = atoi(optarg);
				break;
			case 'E':
				*e = atoi(optarg);
				break;
			case 'b':
				*b = atoi(optarg);
				break;
			case 't':
                *path = calloc(2,sizeof(optarg));
				strcpy(*path, optarg);
				break;
			default:
				//printUsage();
				break;
        }
    }
    if(*s<=0 || *e<=0 || *b<=0 || *path==NULL) // 如果选项参数不合格就退出
	        exit(-1);
}

int main(int argc , char ** argvs )
{
    for (int i = 0 ; i < argc ;i++){
        if (strcmp(argvs[i],"-h") == 0){
            printUsage();
            return 0;
        }
    }

    char *path = NULL;
    int display = 0;
    int s , e ,b ;
    get_argument(argc , argvs , &s , &e , &b , &path ,&display);

    cache cach = initial_cache(s, e, b);
    FILE* pfile = fopen(path, "r");
    setvbuf(pfile, NULL, _IOFBF, 128);
    if(pfile == NULL)
	{
		printf("open error\n");
		exit(-1);
	}

    operation o;
    while ((o = fetch_operation(pfile)).op)
    {
        if (o.op == 'M')
            modify(&cach, &o, display);
        else if (o.op == 'L' || o.op == 'S')
            load_or_store(&cach, &o, display, 1);
        else if (o.op == 'I')
            continue;
    }

    fclose(pfile);
    free_cache(&cach);

    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
