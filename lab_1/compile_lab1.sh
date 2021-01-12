gcc -O3 -Wall -Werror -lm -o lab1-seq lab1.c
gcc -O3 -Wall -Werror -lm -floop-parallelize-all -ftree-parallelize-loops=1 -o lab1-par-1 lab1.c
gcc -O3 -Wall -Werror -lm -floop-parallelize-all -ftree-parallelize-loops=2 -o lab1-par-2 lab1.c
gcc -O3 -Wall -Werror -lm -floop-parallelize-all -ftree-parallelize-loops=3 -o lab1-par-3 lab1.c
gcc -O3 -Wall -Werror -lm -floop-parallelize-all -ftree-parallelize-loops=4 -o lab1-par-4 lab1.c
gcc -O3 -Wall -Werror -lm -floop-parallelize-all -ftree-parallelize-loops=5 -o lab1-par-5 lab1.c
gcc -O3 -Wall -Werror -lm -floop-parallelize-all -ftree-parallelize-loops=6 -o lab1-par-6 lab1.c
