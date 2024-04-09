#include "cachelab.h"
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
    int valid_bits;
    int tag;
    int LRU_count;
}cache_line,*cache_asso,**cache;

int hit_count,
    miss_count,
    eviction_count;

int h,v,s,E,b,S;

char t[1000];

cache __cache = NULL;

void printUsage()
{
    printf("Usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n"
            "-h: Optional help flag that prints usage info"
            "-v: Optional verbose flag that displays trace info"
            "-s <s>: Number of set index bits (S = 2s is the number of sets)"
            "-E <E>: Associativity (number of lines per set)"
            "-b <b>: Number of block bits (B = 2b is the block size)"
            "-t <tracefile>: Name of the valgrind trace to replay");
}

void init_cache()
{
    __cache = (cache)malloc(sizeof(cache_asso) * S);

    for(int i=0;i<S;i++)
    {
        __cache[i] = (cache_asso)malloc(sizeof(cache_line) * E);
        for(int j=0;j<E;j++)
        {
            __cache[i][j].LRU_count = -1;
            __cache[i][j].tag = -1;
            __cache[i][j].valid_bits = 0;
        }
    }
}

void update(unsigned int address)
{
    int seting_add = (address >> b) & ((-1U) >> (64-s));
    int tag_add = address >> (s+b);

    for(int i=0;i<E;i++)
    {
        if(__cache[seting_add][i].tag == tag_add)
        {
            __cache[seting_add][i].LRU_count = 0;
            ++hit_count;
            return;
        }
    }

    for(int i=0;i<E;i++)
    {
        if(__cache[seting_add][i].valid_bits == 0)
        {
            __cache[seting_add][i].valid_bits = 1;
            __cache[seting_add][i].tag = tag_add;
            __cache[seting_add][i].LRU_count = 0;
            ++miss_count;
            return;
        }
    }

    ++miss_count;
    ++eviction_count;

    int max_stacmp = __cache[seting_add][0].LRU_count;
    int max_stacmp_index = 0;
    for(int i=1;i<E;i++)
    {
        if(__cache[seting_add][i].LRU_count > max_stacmp)
        {
            max_stacmp = __cache[seting_add][i].LRU_count;
            max_stacmp_index = i;
        }
    }
    __cache[seting_add][max_stacmp_index].tag =tag_add;
    __cache[seting_add][max_stacmp_index].LRU_count = 0;
    return;
}

void update_strcmp()
{
    for(int i=0;i<S;i++)
    {
        for(int j=0;j<E;j++)
        {
            if(__cache[i][j].valid_bits == 1)
                ++(__cache[i][j].LRU_count);
        }
    }
}

void parse_trace()
{
    FILE* fp = fopen(t,"r");
    if(fp == NULL)
    {
        printf("open error!\n");
        exit(1);
    }
    char operation;
    unsigned int address;
    int size;

    while(fscanf(fp,"%c %xu,%d",&operation,&address,&size) > 0)
    {
        switch (operation)
        {
        case 'I':
            continue;
        
        case 'L':
            update(address);
            break;
        case 'M':
            update(address);
        case 'S':
            update(address);
        }
        update_strcmp(); 
    }
    fclose(fp);

    for(int i=0;i<E;i++)
    {
        free(__cache[i]);
    }
    free(__cache);
}

int main(int argc,char *argv[])
{
    h = 0;
    v = 0;
    hit_count = miss_count = eviction_count = 0;

    int opt;
    while(-1 != (opt = getopt(argc,argv,"hvs:E:b:t:")))
    {
        switch(opt)
        {
            case 'h':
                h = 1;
                printUsage();
                break;
            case 'v':
                v = 1;
                printUsage();
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 't':
                strcpy(t,optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            default:
                printUsage();
                break;
        }
    }
    if(s<0||E<0||b<0||t==NULL)
        return -1;
    S = 1 << s;

    FILE* fp = fopen(t,"r");
    if(fp == NULL)
    {
        printf("open error!\n");
        exit(1);
    }

    init_cache();
    parse_trace();

    printSummary(hit_count, miss_count, eviction_count);

    return 0;
}
