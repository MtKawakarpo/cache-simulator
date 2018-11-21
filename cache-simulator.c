/*
 * created by haiping wang
 *
 */
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <inttypes.h>
#include <stdlib.h>

#define MAX_NUM_LOCATION_PER_SET 16 * 1024
#define MAX_NUM_SETS 16 * 1024

/*
 * a struct describe a location in a set
 * */
struct cacheline{
    uint8_t valid;
    uint64_t tag;
    uint64_t counter;
};
/*
 * a set contains of multiple location to store data blocks
 * */
struct set{
    struct cacheline cachelines[MAX_NUM_LOCATION_PER_SET];
    int last_hit_line;
};

/* global variables */
struct set sets[MAX_NUM_SETS];
uint16_t tag_len;//len of bits used for tag in address
uint16_t set_len;//len of bits used for set index in address
uint16_t nb_line_per_set;// the number of locations in a set
/* statistics */
uint64_t access_total = 0;
uint64_t hit_total = 0;
uint64_t first_hit = 0;
uint64_t nonfirst_hit = 0;
uint64_t miss_total = 0;
uint64_t eviction_total = 0;

void init_cache(int num_set){
    int i, j;
    for(i = 0;i<num_set; i++){
        sets[i].last_hit_line = 0;
        for(j = 0;j< nb_line_per_set; j++){
            sets[i].cachelines[j].valid = 0;
            sets[i].cachelines[j].tag = 0;
            sets[i].cachelines[j].counter = 0;
        }
    }
    printf("finish init cache\n");
}
/*
 * helper function
 * */
void inc_counter(long long set_index){
    int j;
    //update counter
    for(j = 0;j<nb_line_per_set; j++){
        if(sets[set_index].cachelines[j].valid == 1)
            sets[set_index].cachelines[j].counter += 1;
    }
}
void swap_location(long long set_index, int major_loc, int selcted_loc){
    struct cacheline tmp_line;
    tmp_line = sets[set_index].cachelines[major_loc];
    sets[set_index].cachelines[major_loc] = sets[set_index].cachelines[selcted_loc];
    sets[set_index].cachelines[selcted_loc] = tmp_line;
    return;
}
int cmp_location_by_tag(long long set_index, uint64_t tag){
    int j;
    for(j = 0; j<nb_line_per_set; j++){
        if(sets[set_index].cachelines[j].tag == tag && sets[set_index].cachelines[j].valid == 1){
            sets[set_index].cachelines[j].counter = 0;
            return j;
        }
    }
    return -1;
}
int evict_location_lru(long long set_index, uint64_t tag){
    int j,line_index;
    uint64_t max_counter = sets[set_index].cachelines[0].counter;
    line_index = 0;
    for(j = 0;j<nb_line_per_set;j++){
        if(max_counter<sets[set_index].cachelines[j].counter){
            max_counter = sets[set_index].cachelines[j].counter;
            line_index = j;
        }
    }
    sets[set_index].cachelines[line_index].tag = tag;
    sets[set_index].cachelines[line_index].counter = 0;
    return line_index;
}
int find_null_location(long long set_index, uint64_t tag){

    int j;
    for(j = 0;j<nb_line_per_set; j++){
        if(sets[set_index].cachelines[j].valid == 0){
            sets[set_index].cachelines[j].valid = 1;
            sets[set_index].cachelines[j].tag = tag;
            sets[set_index].cachelines[j].counter = 0;
            return j;
        }
    }
    return -1;
}

/*
 * access algorithm: general method (LRU)
 *
 * return positive: first/nonfirst hit
 * return -1: miss
 *
 * */
int access_generally(long long set_index, uint64_t tag){

    int i,j, ret;
    inc_counter(set_index);
    //search target
    ret = cmp_location_by_tag(set_index, tag);
    if(ret >= 0){
        hit_total += 1;
        return ret;
    }
    //miss
    miss_total += 1;
    ret = find_null_location(set_index, tag);
    //replace by LRU algorithm
    if(ret < 0){
        eviction_total += 1;
        evict_location_lru(set_index, tag);
    }
    return -1;
}

/*
 * algorithm 2: MRU way prediction
 * */
int access_with_way_predict(long long set_index, uint64_t tag){
    int i,j, line_index;

    inc_counter(set_index);
    line_index = sets[set_index].last_hit_line;
//    if(line_index > 0)
//        printf("> predict : %d ", line_index);
    if(line_index>=0 && sets[set_index].cachelines[line_index].valid == 1 && sets[set_index].cachelines[line_index].tag == tag){
        first_hit += 1;
        sets[set_index].cachelines[line_index].counter = 0;
//        if(line_index>0)
//            printf(" right!\n");
        return line_index;
    }


    line_index = cmp_location_by_tag(set_index, tag);

    if(line_index >= 0){
        nonfirst_hit += 1;
        sets[set_index].last_hit_line = line_index;
        return line_index;
    }
    //miss
    miss_total += 1;
    line_index = find_null_location(set_index, tag);
    if(line_index < 0){
        eviction_total += 1;
        line_index = evict_location_lru(set_index, tag);
    }
    sets[set_index].last_hit_line = line_index;

    return -1;

}

/*
 * algorithm 3: multi-column predication
 * */
int access_with_mc_predict(long long set_index, uint64_t tag){
    int i,j, line_index, location_len;

    location_len = log(nb_line_per_set)/log(2);
    uint32_t major_loc = (tag & ((1<<location_len)-1));

    inc_counter(set_index);
    if(sets[set_index].cachelines[major_loc].valid == 1 && sets[set_index].cachelines[major_loc].tag == tag){
        first_hit += 1;
        sets[set_index].cachelines[major_loc].counter = 0;
        return major_loc;//hit major location
    }
    line_index = cmp_location_by_tag(set_index, tag);
    if(line_index >= 0){
        nonfirst_hit += 1;
        swap_location(set_index, major_loc, line_index);
        return line_index;
    }
    //miss
    miss_total += 1;
    line_index = find_null_location(set_index, tag);
    if(line_index < 0){
        eviction_total += 1;
        line_index = evict_location_lru(set_index, tag);
    }
    swap_location(set_index, major_loc, line_index);
    return -1;

}

int main() {
    char file_name[40]="traces/astar.trace";
    char log_name[40]="traces/astar/2-way-mcpredict.txt";
    int num_way = 2;

    long long i, j, ret, set_index, word_index;
    char w_or_r;
    char addr64[64]={0};
    uint64_t addr, tag;
    int block_offset = 3;
    int set_num = 16 * 1024 / num_way;
    int set_len = log(set_num)/log(2);
    int tag_len = 64 - block_offset - set_len;
    nb_line_per_set = (128*1024)/8/set_num;

    FILE* fp = freopen(file_name, "r", stdin);
    if(fp == NULL){
        printf("cannot open file\n");
    }else {
        printf("suc to open file\n");
    }

    init_cache(set_num);

    printf("tag len = %d, set len = %d, %d line per set , %d set\n\n", tag_len, set_len, nb_line_per_set, set_num);

    if(freopen(log_name,"w",stdout)==NULL)
        fprintf(stderr,"error redirecting stdout\n");

    while(scanf("%c %s\n", &w_or_r, addr64)!=EOF){

        addr = strtoull(addr64, NULL, 16);
        word_index = addr & 7;
        set_index = addr & ((1<<(set_len+3))-1);
        set_index = set_index>>3;
        tag = addr >> (set_len+3);
        access_total += 1;
//        printf("R: tag = %"PRIu64", set index = %lld, block offset = %lld\n", tag, set_index, word_index);
//        ret = access_generally(set_index, tag);
//        ret = access_with_way_predict(set_index, tag);
        ret = access_with_mc_predict(set_index, tag);
        if(ret <0){
        printf("MISS: reference: %c %s\n", w_or_r, addr64);
        }
    }
    //total hit
//    printf("hit ratio = %f, total = %"PRIu64", miss = %"PRIu64", hit = %"PRIu64"\n",
//           (double)(hit_total*1.0)/access_total , access_total, miss_total, hit_total);
    //first hit
    printf("firt hit ratio = %f, nonfirst hit radio = %f, hit radio = %f\n",
           (double)(first_hit*1.0)/access_total ,(double)(nonfirst_hit*1.0)/access_total , (double)((first_hit + nonfirst_hit)*1.0)/access_total);

    return 0;
}