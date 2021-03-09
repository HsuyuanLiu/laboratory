/*
 由于内存限制，参数选择尽量选择较大的块，较高的相连度，否则可能无法运行
 这不属于代码实现的问题，而是受程序的内存限制
 */
#include <iostream>
#include <random>
#include <time.h>
using namespace std;
const int range_of = pow(2, 20);
const int smaller_range = pow(2, 14);
int groups_shared[4] = { 1,2,4,8 };         //组相联数目表
int block_numbers[4] = { 256,128,64,32 };   //块的数目
int sizeofblock[4] = { 16,32,64,128 };         //块的大小  将cache的大小固定为4KB，基于其可以做出改变
float time_punish[4] = { 600,700,800,900 };//设置一个时间惩罚，块越大，强制缺失代价越大，也可以考虑计算读入时间，查阅资料后设置
int block2cache;    //块的大小标记
int num_of_group;   //组的数目
float time_per_access_group[4] = { 3.2,3.6,4.0,4.4 }; //读入时间，一般而言，读时间与相联度有关.选取平均时间
float time_per_access_block[4] = { 4.5,5.5,6.5,7.5 }; //读入时间，一般而言，读时间与块大小相关？
class block {
    friend class cache;
public:
    int tag = 0;         //标志位
    int valid;         //有效位
    int order = 0;      //载入顺序位
    int numofhit;
    bool notrencent = 1; //初始化时默认均可选
    int size_block;   //块的大小
    block() = default;
    void set_block(int tag1) { //将该块重新写入,完成一次替换
        tag = tag1;
        valid = 1;
        numofhit = 0;
        order = 0;
    }
    void init_block(int num) {
        size_block = num;
        valid = 0;
        numofhit = 0;
        order = 0;
    }
};
class cache {
public:
    int successhit;//记录该方式命中次数
    int testnum;//记录测试数据个数
    void LRU(int*, int count);
    void MRU(int*, int count);
    void RANDOM(int*, int count);
    void LFU(int*, int count);
    void NRU(int*, int count);
    int forLRU(int index);
    int forLFU(int);
    int forMRU(int);
    int search4NRU(int);
    int group_linked; //组相联数
    int storage_len;  //一个有多少个组
    block* start;
    cache(int i) :group_linked(i) {};
    void ini_block();    //cache的初始化
    int search(int);     //实际上，开出了与原始数目相同的block
};
void cache::ini_block() {
    start = new block[storage_len];
    for (int i = 0; i < storage_len; i++) {
        start[i].init_block(block2cache);
    }
}
int cache::search(int index) {
    testnum += 1;
    int first_not_full = -1;
    int mark = 0;
    int q = index % num_of_group;           //q得到组号,即它应该写入哪一个组/块
    int to_found = q * group_linked;        //得到该组的起始地址
    int tag_4insert = index / group_linked;    
    for (int i = to_found; i < to_found + group_linked; i++) {
        if (start[i].valid == 0 && mark == 0) {
            first_not_full = i;   
            mark = 1;
        }
        else if (start[i].tag == tag_4insert) {
            successhit += 1;
            start[i].numofhit += 1;
            start[i].order = 0;
            while (i < to_found + group_linked) {
                if (start[i].valid == 1) {
                    start[i].order += 1;
                }
                i++;
            }
            return -2;   
        }
        if (start[i].valid == 1);
        start[i].order += 1;
    }
    return first_not_full - to_found;
}
int cache::search4NRU(int index) {  //nru位为0，最近可能访问到，否则不能访问到，应该替换
    testnum += 1;
    int first_not_full = -1;
    int mark = 0;
    int q = index % num_of_group;           //q得到组号,即它应该写入哪一个组/块
    int to_found = q * group_linked;        //得到该组的起始地址
    int tag_4insert = index / group_linked;    //得到其索引
    for (int i = to_found; i < to_found + group_linked; i++) {
        if (start[i].valid == 0 && mark == 0) {
            first_not_full = i;   //是否为空，找到第一个不为空的点,记录第一个位置
            mark = 1;
        }
        else if (start[i].tag == tag_4insert) { //hit
            successhit += 1;
            start[i].notrencent = 0;
            while (i < to_found + group_linked) {
                if (start[i].valid == 1) {
                    start[i].notrencent = 1;//设置nru位
                }
                i++;
            }
            return -2;   //命中则置为-2；
        }
        if (start[i].valid == 1);
        start[i].notrencent = 1;
    }
    return first_not_full - to_found;
}
void cache::NRU(int* ind, int count) { 
    int i = 0;
    while (i < count) {
        int block_address = ind[i] / block2cache;
        int isfound = search(block_address);
        if (isfound >= 0) {
            start[block_address % num_of_group + isfound].set_block(block_address / num_of_group);
            start[block_address % num_of_group + isfound].notrencent = 0;
        }
        else {
            int q = 0;
            while (true) {
                int load = rand() % num_of_group;
                q++;
                if (start[load + (block_address % num_of_group)].notrencent == 1) {
                    start[load + (block_address % num_of_group)].set_block(block_address / num_of_group);
                    start[block_address % num_of_group + isfound].notrencent = 0;
                    break;
                }
                else if (q > 4) {
                    load = 0;
                    start[load + (block_address % num_of_group)].set_block(block_address / num_of_group);
                    start[block_address % num_of_group + isfound].notrencent = 0;
                    break;
                }
            }
        }
        i++;
    }
}
int cache::forLFU(int index) {   //LFU寻址
    int q = index % num_of_group;    //q得到组号
    int to_found = q * group_linked - 1;
    int location = 0;
    for (int i = to_found; i < to_found + group_linked; i++) {
        if (start[i].numofhit < start[location].numofhit)
            location = i;   
    }
    return location;
}
void cache::LFU(int* ind, int count) { //最近最少
    int i = 0;
    while (i < count) {
        int block_address = ind[i] / block2cache;
        int isfound = search(block_address);
        if (isfound >= 0) {
            start[block_address % num_of_group + isfound].set_block(block_address / num_of_group);
        }
        else {
            int to_loadin = forLFU(block_address);
            start[to_loadin + block_address % num_of_group].set_block(block_address / num_of_group);
        }
        i++;
    }
}
int cache::forLRU(int index) {   //LFU寻址
    int q = index % num_of_group;
    int to_found = q * group_linked - 1;
    int location = 0;
    for (int i = to_found; i < to_found + group_linked; i++) {
        if (start[i].order > start[location].order)
            location = i;
    }
    return location;
}
void cache::LRU(int* ind, int count) {
    int i = 0;
    while (i < count) {
        int block_address = ind[i] / block2cache; //得出数据所对应的块的地址
        int isfound = search(block_address);      //搜索块
        if (isfound >= 0) {
            start[block_address % num_of_group + isfound].set_block(block_address / num_of_group);
        }
        else {
            int to_loadin = forLRU(block_address);
            start[to_loadin + block_address % num_of_group].set_block(block_address / num_of_group);
        }
        i++;
    }
}
void cache::RANDOM(int* ind, int count) {
    int i = 0;
    while (i < count) {
        int block_address = ind[i] / block2cache;
        int isfound = search(block_address);
        if (isfound >= 0) {
            start[block_address % num_of_group + isfound].set_block(block_address / num_of_group);
        }
        else {
            int to_loadin = rand() % group_linked;
            start[to_loadin + block_address % num_of_group].set_block(block_address / num_of_group);
        }
        i++;
    }
}
int cache::forMRU(int index) {   
    int q = index % num_of_group;    
    int to_found = q * group_linked - 1;
    int location = 0;
    for (int i = to_found; i < to_found + group_linked; i++) {
        if (start[i].order < start[location].order)
            location = i;   
    }
    return location;
}
void cache::MRU(int* ind, int count) {
    int i = 0;
    while (i < count) {
        int block_address = ind[i] / block2cache;
        int isfound = search(block_address);
        if (isfound >= 0) {
            start[block_address % num_of_group + isfound].set_block(block_address / num_of_group);
        }
        else {
            int to_loadin = forMRU(block_address);
            start[to_loadin + block_address % num_of_group].set_block(block_address / num_of_group);
        }
        i++;
    }
}
float time_punish4miss(int group_index, float hittime, int block_index, int try_time) {
    float total_time = 0;
    total_time = time_per_access_block[block_index] * try_time + time_punish[block_index] * (try_time - hittime) + time_per_access_group[group_index] * try_time;
    //总时间=每块访问时间+惩罚时间+组相联相关访问时间
    return total_time;
}
int main() {
    unsigned seed;
    seed = time(0);
    srand(seed);
    float total_hit = 0;
    float average = 0;
    float cost_time = 0;
    cout << "Select the size of block: 0 for 16,1 for 32,2 for 64,3 for 128" << endl;
    int i;          //选择块大小 { 16,32,64,128 }
    cin >> i;
    cout << "Select the substitute algorithm: 0 for LRU,1 for LFU,2 for MRU,3 for RANDOM,4 for NRU." << endl;
    int algorithm;
    cin >> algorithm;
    cout << "Select the set associative: 0 for directed mapped,1 for 2-way,2 for 4-way,3 for 8-way" << endl;
    int group_selecton = 0; //{ 1,2,4,8 }
    cin >> group_selecton;
    int test_len = 5000;
    if (i >= 0 && i <= 3 && group_selecton >= 0 && group_selecton <= 3) {
        for (int q = 0; q < 10; q++) {//20组实验
            cache A(groups_shared[group_selecton]);//初始化，设置不同的组相联数目
            A.storage_len = block_numbers[i];
            num_of_group = block_numbers[i] / groups_shared[i];
            block2cache = sizeofblock[i];
            A.ini_block();
            int* for_test = new int[test_len];
            for (int j = 0; j < test_len; j++) {
                if (rand() % 3 == 1)
                    for_test[j] = rand() % range_of;  //地址范围从0~2^16;
                else
                    for_test[j] = for_test[j - 1] + rand() % smaller_range;//如果希望考虑局部性，则再设置一个随机数作加减法
            }
            switch (algorithm) {
            case 0:
                A.LRU(for_test, test_len);  //选择替换算法,在此修改参数
                cout << "use LRU" << endl;
                break;
            case 1:
                A.LFU(for_test, test_len);
                cout << "use LFU" << endl;
                break;
            case 2:
                A.MRU(for_test, test_len);
                cout << "use MRU" << endl;
                break;
            case 3:
                A.RANDOM(for_test, test_len);
                cout << "use RANDOM" << endl;
                break;
            case 4:
                A.NRU(for_test, test_len);
                cout << "use NRU" << endl;
                break;
            default:
                A.LRU(for_test, test_len);
                cout << "use LRU" << endl;
            }
            total_hit += A.successhit;
            average += cost_time;
            cout << A.successhit << ",";
        }
        cout << total_hit << endl;
        float average_hit = total_hit / 20;
        cout << "Average successful hit:" << average_hit << endl;
        cost_time = time_punish4miss(group_selecton, total_hit / 20, i, test_len);
        cout << "The whole time cost is " << cost_time / 1000 << endl;
    }
    else cout << "Select the appropriate parameters" << endl;
}