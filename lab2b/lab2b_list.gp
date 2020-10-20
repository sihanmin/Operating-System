#NAME: Sihan Min
#EMAIL: sihanmin@yeah.net
#ID: 504807176
#! /usr/bin/gnuplot

# general plot parameters
set terminal png
set datafile separator ","

# lab2b_1.png
set title "lab2b.1: Throughput vs Number of Threads for Mutex and Spin-lock"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_1.png'
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000)/($7) \
     title 'list w/mutex' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000)/($7) \
     title 'list w/spin-lock' with linespoints lc rgb 'green'


# lab2b_2.png
set title "lab2b.2: Mean Time per Mutex and\nper Operation for Mutex-synchronize"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Time (nanoseconds)"
set logscale y
set output 'lab2b_2.png'
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
     title 'list w/mutex average time per operation' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
     title 'list w/mutex wait-for-lock-time' with linespoints lc rgb 'green'


# lab2b_3.png
set title "lab2b.3: Successful Iterations vs\nThreads for Each Synchronization Method."
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
set key left top
plot \
     "< grep list-id-none lab2b_list.csv" using ($2):($3) \
     title 'w/o synchronization' with points lc rgb 'red', \
     "< grep list-id-m lab2b_list.csv" using ($2):($3) \
     title 'w/mutex lock' with points lc rgb 'green', \
     "< grep list-id-s lab2b_list.csv" using ($2):($3) \
     title 'w/spin-lock' with points lc rgb 'blue'

# lab2b_4.png
set title "lab2b.4: Throughput vs Threads Number for\nMutex Synchronized Partitioned Lists."
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Operations per Second"
set logscale y 10
set output 'lab2b_4.png'
set key right top
plot \
     "< grep -e 'list-none-m,[0-9]2*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'list = 1' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'list = 4' with linespoints lc rgb 'green', \
     "< grep -e 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'list = 8' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'list = 16' with linespoints lc rgb 'orange'

# lab2b_5.png
set title "lab2b.5: Throughput vs threads Number for\nSpin-lock-synchronized Partitioned Lists."
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Operations per Second"
set logscale y 10
set output 'lab2b_5.png'
set key left bottom
plot \
     "< grep -e 'list-none-s,[0-9]2*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'list = 1' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'list = 4' with linespoints lc rgb 'green', \
     "< grep -e 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'list = 8' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     title 'list = 16' with linespoints lc rgb 'orange'