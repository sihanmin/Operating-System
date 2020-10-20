#!/usr/local/cs/bin/python3
import sys
import csv

#helper functions 
def report_error_and_exit(msg):
    sys.stderr.write(msg)
    exit(1)

class Check():
    def __init__(self, data_file):
        #block information
        self.group = dict()
        self.bfree = list()
        self.ifree = list()
        self.inode = list()
        self.dirent = list()
        self.indirect = list()
        #parse csv file
        for line in data_file:
            ch = line[0]
            if(ch == "SUPERBLOCK"):
                #TODO:what to append
                self.superblock = line
                self.total_blocks = int(line[1])
                self.total_inodes = int(line[2])
                self.block_size = int(line[3])
                self.inode_size = int(line[4])
                self.first_inode = int(line[7])
            elif(ch == "GROUP"):
                #TODO: what to append
                self.first_inode_block = int(line[8])
            elif(ch == "INODE"):
                self.inode.append(line)
            elif(ch == "BFREE"):
                self.bfree.append(int(line[1]))
            elif(ch == "IFREE"):
                self.ifree.append(int(line[1]))
            elif(ch == "DIRENT"):
                self.dirent.append(line)
            elif(ch == "INDIRECT"):
                self.indirect.append(line)
            else:
                report_error_and_exit("Unknown entry in csv: " + ch)
        self.dump()
    def check_block(self):
        return None
    def check_inode(self):

        return None
    def check_dir(self):
        return None
    def dump(self):
        print(self.superblock)
        print(self.ifree)
        print(self.bfree)
        print(self.inode)

    
def main():
    if(len(sys.argv) != 2):
        report_error_and_exit("Wrong number of arguments\n")
    try:
        with open(sys.argv[1]) as data_file:
            c = Check(csv.reader(data_file, delimiter=','))
            c.check_block()
            c.check_inode()
            c.check_dir()
            data_file.close()
    except IOError:
        report_error_and_exit("Cannot read data file");
    exit(0)

if __name__ == "__main__":
    main()
