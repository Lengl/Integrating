# Integrating
Integrate f(x)=sin(1/x)*sin(1/x)/(x*x) on multiple cores

To run use parametrs in following order: number of threads (int), left border (double), right border (double)
To compile you will need keys -lpthread -lrt -lm

Results: There is a slight speed-up on 2 cores instead of one, but there is almost no speed-up on higher amount of cores. This happens because parametrs of dropping global and local stacks are not optimized.
