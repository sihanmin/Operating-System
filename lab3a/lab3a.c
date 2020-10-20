#include <sys/types.h> 
#include <stdint.h>
#include <inttypes.h>
#include "ext2_fs.h"
#include <stdlib.h>    
#include <string.h>    
#include <sys/types.h>  
#include <sys/stat.h>   
#include <fcntl.h>     
#include <unistd.h>     
#include <stdio.h>     
#include <time.h>      

#define ERR_EXIT_CODE 2
#define SUPER_BLOCK_OFFSET 1024
#define SUPER_BLOCK_LENGTH 1024
#define GROUP_DESC_OFFSET 2048
#define GROUP_DESC_LENGTH 1024
#define SIZE_BITMAP 1024

//global variables 
int image_fd = 0;
struct ext2_super_block superblock;
struct ext2_group_desc groupdesc;
struct ext2_inode inode;
void report_error_and_exit(char* msg);
void report_error_and_exit_args(char* msg);
void read_superblock();
void print_superblock();
void read_group();
void print_group();
char findType(__u16 i_mode);
void timeForm(__u32 time, char * result);
void read_inode(int inode_num);
void print_inode(int inode_num);
void process_inode(int inode_num);
void process_directory(int inode_num);
void print_indirect(int parent_inode, int level, int offset, int block_num, int block_num_ref);
void process_indirect(int inode_num);
void read_block_bitmap();
void read_inode_bitmap();

int main(int argc, char* argv[]){
  if(argc != 2){
    report_error_and_exit_args("Wrong number of argument\n");
  }
  image_fd = open(argv[1], O_RDONLY);
  if(image_fd == -1){
    report_error_and_exit_args("Cannot open the image\n");
  }

  //Then analyze
  read_superblock();
  print_superblock();
  read_group();
  print_group();
  read_block_bitmap();
  read_inode_bitmap();
}

void report_error_and_exit(char* msg){
  fprintf(stderr, "%s", msg);
  exit(ERR_EXIT_CODE);
}


void report_error_and_exit_args(char* msg){
  fprintf(stderr, "%s", msg);
  exit(1);
}


void read_superblock(){
  if(pread( image_fd, &superblock, SUPER_BLOCK_LENGTH, SUPER_BLOCK_OFFSET ) == -1 ){
    report_error_and_exit("Cannot read from superblock\n");
  }    
}

void print_superblock(){
  printf("%s,%d,%d,%d,%hd,%d,%d,%d\n", "SUPERBLOCK", 
	 superblock.s_blocks_count, 
	 superblock.s_inodes_count,
	 SUPER_BLOCK_LENGTH << superblock.s_log_block_size, 
	 superblock.s_inode_size,
	 superblock.s_blocks_per_group,
	 superblock.s_inodes_per_group,
	 superblock.s_first_ino);
}

void read_group(){
  if(pread( image_fd, &groupdesc, 32, 2048 ) == -1 ){
    report_error_and_exit("Cannot read from group descriptor table\n");
  }
}

void print_group(){
  printf("%s,%d,%d,%d,%hd,%hd,%d,%d,%d\n", "GROUP",
	 0,
	 superblock.s_blocks_count,
	 superblock.s_inodes_count,
	 groupdesc.bg_free_blocks_count,
	 groupdesc.bg_free_inodes_count,
	 groupdesc.bg_block_bitmap,
	 groupdesc.bg_inode_bitmap,
	 groupdesc.bg_inode_table);
}

char findType(__u16 i_mode)
{
  char file_type;    
  if((i_mode & 0x4000) == 0x4000)
    file_type = 'd';
  else if((i_mode & 0xA000) == 0xA000)
    file_type = 's';
  else if((i_mode & 0x8000) == 0x8000)
    file_type = 'f';
  else
    file_type = '?';
    
  return file_type;
}

void timeForm(__u32 time, char * result)
{
  time_t raw = time;
  struct tm* gtime = gmtime(&raw);
  strftime(result, 30, "%m/%d/%g %H:%M:%S", gtime);
}


void read_inode(int inode_num)
{
  int inode_size = superblock.s_inode_size;
  int inode_offset = groupdesc.bg_inode_table * 1024 + (inode_num - 1) * inode_size;
  int ret = pread(image_fd, &inode, inode_size, inode_offset);
  if(ret == -1 ){
    report_error_and_exit("Cannot read from inode\n");
  }
}


void print_inode(int inode_num){
  char inode_file_type = findType(inode.i_mode);
  char ctime[30], atime[30], mtime[30];
  timeForm(inode.i_ctime, ctime);
  timeForm(inode.i_atime, atime);
  timeForm(inode.i_mtime, mtime);
    
  printf("INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u",
	 inode_num,
	 inode_file_type,
	 inode.i_mode & 511,
	 inode.i_uid,
	 inode.i_gid,
	 inode.i_links_count,
	 ctime,
	 mtime,
	 atime,
	 inode.i_size,
	 inode.i_blocks
	 );
    
  for (int i = 0; i < 15; i++){
    printf(",%d",inode.i_block[i]);
  }
    
  printf("\n");
}

void process_inode(int inode_num){
  read_inode(inode_num);
  char inode_file_type = findType(inode.i_mode);
  if(inode.i_mode != 0 && inode.i_links_count != 0){
    print_inode(inode_num); 
  }
  if(inode_file_type == 'd'){
    process_directory(inode_num); 
  }
  if(inode_file_type == 'f' || inode_file_type == 'd'){
    process_indirect(inode_num); 
  }
}

void process_directory(int inode_num){
  int logical_directory_offset = 0;
  for(int i = 0; i < EXT2_NDIR_BLOCKS; i++){
    int directory_offset = 0;
    struct ext2_dir_entry directory;
    while(directory_offset < (1024 << superblock.s_log_block_size)){
      int inode_offset = inode.i_block[i] * (1024 << superblock.s_log_block_size) + directory_offset;
      int ret = pread(image_fd, &directory, sizeof(directory), inode_offset);
      if(ret < 0){
	report_error_and_exit("cannot read");
      }
      if(directory.inode != 0){
	char* file_name = directory.name;
	file_name[directory.name_len] = '\0';
	printf("%s,%u,%u,%u,%u,%u,'%s'\n", "DIRENT", 
	       inode_num,
	       logical_directory_offset,
	       directory.inode,
	       directory.rec_len,
	       directory.name_len,
	       file_name);
      }
      if(directory.rec_len == 0){
	break;
      }
      directory_offset += directory.rec_len;
      logical_directory_offset += directory.rec_len;
    }
  }
}

//Just a wrapper function
void print_indirect(int parent_inode, int level, int offset, int block_num, int block_num_ref){
  printf("%s,%d,%d,%d,%d,%d\n", "INDIRECT", parent_inode, level, offset, block_num, block_num_ref);
}

void process_indirect(int inode_num){
  int block_size = 1024 << superblock.s_log_block_size;
  //first level
  if(inode.i_block[12] != 0){
    int inode_offset = inode.i_block[12] * (1024 << superblock.s_log_block_size);
    int* buffer = malloc(block_size);//TODO:check return val
    if(buffer == NULL){
      report_error_and_exit("cannnot alloc");
    }
    int ret = pread(image_fd, buffer, block_size, inode_offset);
    if(ret < 0){
      report_error_and_exit("cannot read");
    }
    for(int i = 0; i < block_size/4; i++){
      //printf("%d", buffer[i]);
      if(buffer[i] != 0){	
	print_indirect(inode_num, 1, 12+i, inode.i_block[12], buffer[i]);
	inode_offset++;
      }
    }
    free(buffer);
  }
  
  //second level
  if(inode.i_block[13] != 0){
    int* first_level = malloc(block_size);
    if(first_level == NULL){
      report_error_and_exit("cannot alloc");
    }
    int first_offset = inode.i_block[13] * (1024 << superblock.s_log_block_size);
    int ret = pread(image_fd, first_level, block_size, first_offset);
    if(ret < 0){
      report_error_and_exit("cannot read");
    }
    for(int i = 0; i < block_size/4; i++){
      int* second_level = malloc(block_size);
      if(second_level == NULL){
	report_error_and_exit("cannot alloc");
      }
      int second_offset =  first_level[i] * (1024 << superblock.s_log_block_size);
      ret = pread(image_fd, second_level, block_size,second_offset) ;
      if(ret < 0){
	report_error_and_exit("cannot read");
      }
      for(int j = 0; j < block_size/4; j++){
	if(second_level[j] != 0){
	  print_indirect(inode_num, 2, 268, inode.i_block[13], first_level[i]);	  
	  int block_offset = 0;
	  struct ext2_dir_entry dir;
	  while(block_offset <  (1024 << superblock.s_log_block_size)/4){ //TODO: /4 boudary
	    ret = pread(image_fd, &dir, sizeof(dir), second_offset + block_offset);
	    if(ret < 0){
	      report_error_and_exit("read err");
	    }
	    print_indirect(inode_num, 1, 268+j, first_level[i], second_level[j]);
	    block_offset += 268;
	  }//end while
	}//end if
      } //end for
    }//end for 
  }//end if

  //third level
  if(inode.i_block[14] != 0){
    int* first_level = malloc(block_size);
    if(first_level == NULL){
      report_error_and_exit("cannot alloc");
    }
    int first_offset = inode.i_block[14] * (1024 << superblock.s_log_block_size);
    int ret = pread(image_fd, first_level, block_size, first_offset);
    if(ret < 0) {
      report_error_and_exit("cannot read");
    }
    for(int i = 0; i < block_size/4; i++){
      int* second_level = malloc(block_size);
      if(second_level == NULL){
	report_error_and_exit("cannot alloc");
      }
      int second_offset =  first_level[i] * (1024 << superblock.s_log_block_size);
      ret = pread(image_fd, second_level, block_size,second_offset);
      if(ret < 0){
	report_error_and_exit("cannot read");
      }
      for(int j = 0; j < block_size/4; j++){
	int* third_level = malloc(block_size);
	int third_offset = second_level[j] * block_size;
	ret = pread(image_fd, third_level, block_size,third_offset);
	for(int k = 0; k < block_size/4; k++){
	  if(third_level[k] != 0){
	    struct ext2_dir_entry dir;
	    int block_offset = 0;
	    while(block_offset <  (1024 << superblock.s_log_block_size)/4 ){ //TODO: /4 boudary
	      ret = pread(image_fd, &dir, sizeof(dir), third_offset + block_offset);
	      if(ret < 0){report_error_and_exit("read err");}
	      //int constant = (256-i) * (256 - j) + (256 - k) + 14;
	      int constant = 65804 + 65536 * i + 256 * j * k;
	      print_indirect(inode_num, 3, constant+ i, inode.i_block[14], first_level[i]);	      
	      print_indirect(inode_num, 2, constant + i + j , first_level[i], second_level[j]); 
	      print_indirect(inode_num, 1, constant + i + j + k , second_level[j], third_level[k]); 
	      block_offset += 65536;
	    }//end while
	  }//end if
	}
      } //end for
    }//end for 
  }//end if
  
}//end process_indirect

void read_block_bitmap(){
  int blockBitmap_offset = groupdesc.bg_block_bitmap * (1024 << ((int) superblock.s_log_block_size));
  unsigned char* buffer = malloc(SIZE_BITMAP);
  int count = 0;
  if( buffer == NULL ){
    report_error_and_exit("Cannot alloc");
  }
  if( pread( image_fd, buffer, SIZE_BITMAP, blockBitmap_offset ) == -1 )
    {
      report_error_and_exit("Cannot read from bit map");
    }
  for(int i = 0; i < SIZE_BITMAP; i++){
    char byte = buffer[i];
    for(int j = 0; j < 8; j ++){
      int mask = (1 << j);
      count ++; 
      if((mask & byte) == 0){
	printf("%s,%d\n", "BFREE", count);
      }
    }
  }
}

void read_inode_bitmap(){
  int blockBitmap_offset = (1024 << superblock.s_log_block_size) * groupdesc.bg_inode_bitmap;
  int buffer_size = SIZE_BITMAP;
  unsigned char* buffer = malloc(SIZE_BITMAP);
  if( buffer == NULL ){
    report_error_and_exit("Cannot alloc");
  }
  int ret = pread( image_fd, buffer, SIZE_BITMAP, blockBitmap_offset);
  if(ret == -1){
    report_error_and_exit("Cannot read from bit map");
  }
    
  int not_free_inodes[superblock.s_inodes_per_group];
  for(int i = 0; i < buffer_size; i++){
    char c = buffer[i];
    for(int j = 0; j < 8; j ++){
      unsigned int inode_count = i * 8 + j;
      //printf("%d", inode_count);
      if(inode_count < superblock.s_inodes_per_group){
	int mask = (0x01 << j);
	//printf("\nFor inode %d", inode_count);
	if((mask & c) == 0){
	  printf("%s,%d\n", "IFREE", inode_count + 1);
	  not_free_inodes[inode_count] = 0;
	}else{
	  not_free_inodes[inode_count] = 1;
	  //printf(" The %d is not free", inode_count);
	}	  
      }
    }
  }

  for(unsigned int i = 0; i < superblock.s_inodes_per_group ;i++ )
    {
      if(not_free_inodes[i] == 1){
	//printf("%d ", i);
        process_inode(i+1);
      }
    }
}
