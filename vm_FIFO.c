/*
vm_LRU.c
注释:
    本文件解决1.1(2)的问题
    TLB使用FIFO更新
    Page Replacement使用FIFO

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>


int bin2int(int*,int, int);//将二进制码start-end位转换为int
void TLB_update_FIFO(int [][2], int, int, int);// 用FIFO更新TLB数组
int page_replacement_FIFO(const char*, int, char [128][256], int [128][2]); //该函数用FIFO策略更新page_table数组和物理内存中存放的内容


int main(int argc, char *argv[]){

    char* back_store_name = argv[1];
    char* input_address_name = argv[2];
    
    //TLB[i][0]为page number，TLB[i][1]为该page number对应的frame number
    int TLB[16][2];
    //page_table[i][0]为page number，page_table[i][1]为该page number对应的frame number
    int page_table[128][2];
    //将物理内存定义为128*256的二维数组，即有128frames，每个fram有256个字节
    char physical_memory[128][256];
    //初始化TLB，-1表示为空
    for(int i=0; i<16; ++i)
        *TLB[i] = -1;
    //初始化page table，-1表示为空
    for(int i=0; i<128; ++i)
        *page_table[i] = -1;

    //TLB_hit变量用于记录TLB hit的次数
    int TLB_hit = 0;
    //Page_fault变量用于记录Page-fault的次数
    int Page_fault = 0;
    //total变量用于记录虚拟地址的个数
    int total = 0;

    //读取addresses.txt文件
    FILE *fp;
    if((fp = fopen(input_address_name,"r"))== NULL){
        printf("File read failed: %s", input_address_name);
        return -1;
    }
    int addresses[15000];
    int i = 0;
    while(fscanf(fp, "%d", &addresses[i]) != EOF) {
        i++;
    }
    fclose(fp);

    //循环遍历addresses.txt文件中记录的每个虚拟地址，将其转换成物理地址，并获取该物理地址中储存的值
    for(int i = 0; i < 1000; i++){
        //获取page和offset
        int temp = addresses[i];
        int count = 0;
        int binary[100];
        for(int l = 0; l < 100; l++){
            binary[l] = 0;
        }
        while(temp != 0){
            binary[count] = temp & 1;
            temp = (temp>>1);
            count++;
        }
        int page_number = 0;
        int off_set = 0;
        page_number = bin2int(binary,8,16);
        for (int m = 0; m < 8; m++)
        {
            off_set = off_set + pow(2,m) * binary[m];
        }

        //定义变量frame_number，用于储存当前page number对应的frame number
        int frame_number;
        //设置标志变量flag1，如果flag1=0代表TLB miss，如果flag1=1代表TLB hit
        int flag1 = 0;
        //设置标志变量flag2，如果flag2=0代表page fault，如果flag2=1代表page success
        int flag2 = 0;
        total++;
        //查询TLB
        for(int i=0; i<16; i++){
            if(TLB[i][0] == -1)
                break;
            //如果TLB hit
            if(TLB[i][0] == page_number){
                //获取当前page number对应的frame number
                frame_number = TLB[i][1];
                flag1 = 1;
                TLB_hit++;
                break;
            }
        }
        //如果TLB miss，查询page table
        if(flag1 == 0){
            for(int i=0; i<128; ++i){
                if(page_table[i][0] == -1)
                    break;
                //如果page success
                if(page_table[i][0] == page_number){
                    //获取当前page number对应的frame number
                    frame_number = page_table[i][1];
                    flag2 = 1;
                    break;
                }
            }
            //如果标志变量flag2=1，说明page success
            if(flag2 == 1){                    
                TLB_update_FIFO(TLB, 16, page_number, frame_number);
            }
            //如果标志变量flag2=0，说明page fault
            else{
                //Page-fault的次数加一
                Page_fault++;
                frame_number = page_replacement_FIFO(back_store_name, page_number, physical_memory, page_table);
                //frame_number为-1表示打开文件失败，此时返回-1
                if(frame_number == -1)
                    return -1;
                TLB_update_FIFO(TLB, 16, page_number, frame_number);
            }
        }
        //根据frame number和offset来计算当前虚拟地址对应的物理地址
        int physical_address = (frame_number << 8) + off_set;
        //打印当前的虚拟地址、当前的虚拟地址对应的物理地址、该物理地址中储存的值
        printf("Virtual address: %d Physical address: %d Value: %d\n", addresses[i], physical_address, physical_memory[frame_number][off_set]);

    }
    float TLB_hit_rate = 100 * (float)TLB_hit / total;
    float Page_fault_rate = 100 * (float)Page_fault / total;
    printf("TLB hit rate is %.4f%%, Page-fault rate is %.4f%%.\n", TLB_hit_rate, Page_fault_rate);
    return 0;
}

int bin2int(int binary[],int start,int end){
    int i=0;
    int ans = 0;
    for ( i = start; i < end; i++)
    {
        ans = ans + pow(2,i-start) * binary[i];
    }
    return ans;
}

//定义TLB_update_FIFO函数，该函数用于更新TLB数组，
//如果TLB数组未满则按顺序储存，如果满了则使用FIFO替换策略进行替换
void TLB_update_FIFO(int TLB[][2], int len, int page_number, int frame_number){
    static int record = 0; //静态变量record用于记录当前TLB数组应该更新的行，TLB初始为空，因此record初始为0
   
    //更新TLB中索引为record的行
    TLB[record][0] = page_number;
    TLB[record][1] = frame_number;
    record++;
    //如果TLB的最后一行已被更新，则下一次调用该函数应该更新第一行，record为0
    if(record >= len)
        record = record % len;
       
    return;
}

//定义page_replacement_FIFO函数，该函数用于更新page_table数组和物理内存中存放的内容，
//如果物理内存、page_table数组未满则按顺序储存，如果满了则使用FIFO替换策略进行替换
int page_replacement_FIFO(const char* back_store_name, int page_number, char physical_memory[128][256], int page_table[128][2]){
    
    static int frame_number = -1; //静态变量frame_number用于记录物理内存当前应该更新的frame对应的frame number
    //index指示应该将文件指针定位到何处，这里是page number对应的内容的首地址
    int index = 256 * page_number;
    frame_number++;
    //如果frame_number为128，则将其置为0
    if(frame_number >= 128)
        frame_number = frame_number % 128;
    //定义文件指针，初始化为空
    FILE *fp = NULL;
    //打开BACKING_STORE.bin文件
    if((fp = fopen(back_store_name, "rb")) == NULL){
        printf("Cannot open file %s!\n", back_store_name);
        return -1;
    }
    //将文件指针定位到page number对应的内容的首地址
    fseek(fp, index, 0);
    //将page number对应的内容写入物理地址
    fread(physical_memory[frame_number],1,256,fp);
    //关闭BACKING_STORE.bin文件
    fclose(fp);
    fp = NULL;
    //更改page_table中相应位置的映射关系
    page_table[frame_number][0] = page_number;
    page_table[frame_number][1] = frame_number;
    //返回page number对应的frame number
    return frame_number;
}