// Compile: mpicxx -o lb1 lb1_source.cpp
// Launch: ./lb1 N M
// Но по умолчанию N ограничено количеством физических ядер.
// Для увеличения количества слотов нужно
// в mpirun передать опцию --hostfile с файлом с содержимым:
// localhost slots=<MAX_AMOUNT_OF_SLOTS>

/**
  Реализуйте функцию ring, которая создаёт N процессов и посылает
  сообщение первому процессу, который посылает сообщение второму,
  второй - третьему, и так далее. Наконец, процесс N посылает сообщение
  обратно процессу 1. После того, как сообщение обежало вокруг кольца M
  раз, все процессы заканчивают работу.
*/

#include <iostream>
#include <unistd.h>
#include "mpi.h"

int ring(int argc, char* argv[]){
  int proc_num, proc_rank, recv, m = 0;
  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &proc_num);
  MPI_Comm_rank(MPI_COMM_WORLD, &proc_rank);

  if (proc_num < 2){
    std::cout << "Error: only one process started. The program ends." << std::endl;
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  // take M from arguments
  m = std::atoi(argv[2]);

  if (proc_rank == 0){
    std::cout << "proc_num = " << proc_num << ", M = " << m << std::endl;

    for (int b = 0; b < m; b++){
      MPI_Send(&m, 1, MPI_INT, proc_rank + 1, 0, MPI_COMM_WORLD);
      std::cout << "bypass(" << b << ") > process(" << proc_rank << ") sended to process (" << proc_rank + 1 << ")"  << std::endl;

      MPI_Recv(&recv, 1, MPI_INT, proc_num - 1, 0, MPI_COMM_WORLD, &status);
      std::cout << "bypass(" << b << ") > process (" << proc_rank << ") received a message from process (" << status.MPI_SOURCE << "): " << recv << std::endl;
    }
  }

  for (int proc = 1, proc_dst, proc_src; proc < proc_num; proc++){
      if (proc_rank == proc){
        proc_dst = proc_rank + 1;
        proc_src = proc_rank - 1;
        if (proc_rank == proc_num - 1)
          proc_dst = 0;

        for (int b = 0; b < m; b++){
          MPI_Recv(&recv, 1, MPI_INT, proc_src, 0, MPI_COMM_WORLD, &status);
          std::cout << "bypass(" << b << ") > process (" << proc_rank << ") received a message from process (" << status.MPI_SOURCE << "): " << recv << std::endl;

          MPI_Send(&recv, 1, MPI_INT, proc_dst, 0, MPI_COMM_WORLD);
          std::cout << "bypass(" << b << ") > process(" << proc_rank << ") sended to process (" << proc_dst << ")"  << std::endl;
        }
      }
  }

  MPI_Finalize();
  return EXIT_SUCCESS;
}

/*
  Для того чтобы количество процессов N было аргументом программы,
  а не аргументом mpirun -np, далее реализуется следущий алгоритм:
  1. Проверяется количество аргументов командной строки (должно быть 3)
      название исполяемого файла, N и М.
  2. Проверяется корректность N и M.
  3. Запуск программы mpirun с аргументом -np N (задает количество процессов)
  4. Завершение программы.
  После чего mpirun запускает программу и сразу переходит в функцию ring минуя if.

  MPI экспортирует переменную среды OMPI_COMM_WORLD_RANK в запущенные процессы.
  Таким образом отслеживаю как была запущена программа (через mpirun или напрямую)
*/
int main(int argc, char* argv[]){
  if (getenv("OMPI_COMM_WORLD_RANK") == NULL){
    if (argc < 3) {
      std::cout << "Error: not enough arguments" << std::endl;
      std::cout << "Usage: ./lb1 <N> <M>" << std::endl;
      return EXIT_FAILURE;
    }

    int n = std::atoi(argv[1]), m = std::atoi(argv[2]);
    if (n == 0 || m == 0){
      std::cout << "Error: incorrectly arguments (need int)" << std::endl;
      return EXIT_FAILURE;
    }

    char **args = (char **)calloc(6, sizeof(char *));
    args[0] = "mpirun";
    args[1] = "-np";
    args[2] = argv[1];
    args[3] = argv[0];
    args[4] = argv[1];
    args[5] = argv[2];

    execvp("mpirun", args);

    return EXIT_SUCCESS;
  }

  return ring(argc, argv);
}
