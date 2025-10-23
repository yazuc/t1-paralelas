cp06@atlantica:~/monte-carlo$ srun -N 2 -n 32 --exclusive mpiMCpi
[MASTER] PI ≈ 3.141614 com 10000000000 pontos (10000 tarefas)
Tempo de execucao: 33.056857

cp06@atlantica:~/monte-carlo$ srun -N 2 -n 16 --exclusive mpiMCpi
[MASTER] PI ≈ 3.141637 com 10000000000 pontos (10000 tarefas)
Tempo de execucao: 37.017729

cp06@atlantica:~/monte-carlo$ srun -N 2 -n 8 --exclusive mpiMCpi
[MASTER] PI ≈ 3.141621 com 10000000000 pontos (10000 tarefas)
Tempo de execucao: 75.164280

cp06@atlantica:~/monte-carlo$ srun -N 2 -n 4  --exclusive mpiMCpi
[MASTER] PI ≈ 3.141612 com 10000000000 pontos (10000 tarefas)
Tempo de execucao: 174.784784
