#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define SIZE 650
#define PRINT_SIZE 650
#define PERCENT_ZEROS 99
#define MAX_NUMBER 10
#define SUMMETRIC_MATRIX

int flop = 0;
int flop_cho = 0;
int count_nz = 0;

pthread_mutex_t lock;

enum Mode {
  ROW,
  COLUMN
};

struct Non_zero_1dim_elems {
  int i;
  double x;
  struct Non_zero_1dim_elems *next;
};

struct Non_zero_2dim_elems {
  int i;
  int j;
  double x;
  struct Non_zero_2dim_elems *next;
};

struct Non_zero_2dim_elems *l_matrix_head,
                           *u_matrix_head,
                           *l_matrix_temp,
                           *u_matrix_temp;

struct Graph_dep {
  int i;
  int level;
  int *deps;
  int count_deps;
  int *up_deps;
  int count_up_deps;
};

struct Pthread_args {
  int k;
  struct Non_zero_2dim_elems *a_matrix_head;
};

struct Non_zero_1dim_elems *init_1dim_node()
{
  struct Non_zero_1dim_elems *node = (struct Non_zero_1dim_elems*)malloc(sizeof(struct Non_zero_1dim_elems));
  node->i = 0;
  node->x = 0;
  node->next = NULL;

  return node;
}

struct Non_zero_2dim_elems *init_2dim_node()
{
  struct Non_zero_2dim_elems *node = (struct Non_zero_2dim_elems*)malloc(sizeof(struct Non_zero_2dim_elems));
  node->i = 0;
  node->j = 0;
  node->x = 0;
  node->next = NULL;

  return node;
}

/*void print_1dim_matrix(struct Non_zero_1dim_elems *matrix)
{
  for (int i = 0; matrix->next != NULL && i < PRINT_SIZE; matrix = matrix->next, i++) {
    printf("(%d)=%lf\n", matrix->i, matrix->x);
  }
  printf("\n");
}*/

void print_2dim_matrix(struct Non_zero_2dim_elems *matrix)
{
  for (int i = 0; matrix->next != NULL && i < PRINT_SIZE * PRINT_SIZE; matrix = matrix->next, i++) {
    printf("(%d:%d)=%lf\n", matrix->i, matrix->j, matrix->x);
  }
  printf("\n");
}

void print_2dim_matrix2(struct Non_zero_2dim_elems *matrix)
{
  double matrix_temp[SIZE][SIZE] = {};
  
  for (; matrix->next != NULL; matrix = matrix->next) {
    matrix_temp[matrix->i][matrix->j] = matrix->x;
  }
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      printf("%.3lf\t", matrix_temp[i][j]);
    }
    printf("\n\n");
  }
  printf("\n");
}

void cmp_out_1dim_in_1dim(struct Non_zero_1dim_elems *out_matrix, struct Non_zero_1dim_elems *in_matrix)
{
  for (; out_matrix->next != NULL; out_matrix = out_matrix->next) {
    in_matrix->i = out_matrix->i;
    in_matrix->x = out_matrix->x;
    in_matrix->next = init_1dim_node();
    in_matrix = in_matrix->next;
  }
}

void cmp_out_2dim_in_1dim(struct Non_zero_2dim_elems *out_matrix, double *in_matrix, int index, enum Mode mode)
{
  memset(in_matrix, 0, sizeof(double) * SIZE);
  switch(mode) {
  case ROW:
    for (; out_matrix->next != NULL; out_matrix = out_matrix->next) {
      if (out_matrix->i == index) {
        in_matrix[out_matrix->j] = out_matrix->x;
      }
    }
    break;
  case COLUMN:
    for (; out_matrix->next != NULL; out_matrix = out_matrix->next) {
      if (out_matrix->j == index) {
        in_matrix[out_matrix->i] = out_matrix->x;
      }
    }
    break;
  }
}

void sort_2dim_matrix(struct Non_zero_2dim_elems *matrix_head, enum Mode mode)
{
  struct Non_zero_2dim_elems *sorted_matrix_head = init_2dim_node(),
                             *sorted_matrix = sorted_matrix_head,
                             *matrix = NULL;
  
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      matrix = matrix_head;
      for (; matrix->next != NULL; matrix = matrix->next) {
        switch(mode) {
        case ROW:
          if (matrix->i == i && matrix->j == j) {
            sorted_matrix->i = i;
            sorted_matrix->j = j;
            sorted_matrix->x = matrix->x;
            sorted_matrix->next = init_2dim_node();
            sorted_matrix = sorted_matrix->next;
          }
          break;
        case COLUMN:
          if (matrix->i == j && matrix->j == i) {
            sorted_matrix->i = j;
            sorted_matrix->j = i;
            sorted_matrix->x = matrix->x;
            sorted_matrix->next = init_2dim_node();
            sorted_matrix = sorted_matrix->next;
          }
          break;
        }
      }
    }
  }
  matrix_head->i = sorted_matrix_head->i;
  matrix_head->j = sorted_matrix_head->j;
  matrix_head->x = sorted_matrix_head->x;
  matrix_head->next = sorted_matrix_head->next;
}

void sort_for_graph(struct Non_zero_2dim_elems *matrix_head)
{
  struct Non_zero_2dim_elems *sorted_matrix_head = init_2dim_node(),
                             *sorted_matrix = sorted_matrix_head,
                             *matrix = NULL;
  
  for (int j = 0; j < SIZE; j++) {
    for (int i = SIZE - 1; i >= 0; i--) {
      matrix = matrix_head;
      for (; matrix->next != NULL; matrix = matrix->next) {
        if (matrix->i == i && matrix->j == j) {
          sorted_matrix->i = i;
          sorted_matrix->j = j;
          sorted_matrix->x = matrix->x;
          sorted_matrix->next = init_2dim_node();
          sorted_matrix = sorted_matrix->next;
        }
      }
    }
  }
  matrix_head->i = sorted_matrix_head->i;
  matrix_head->j = sorted_matrix_head->j;
  matrix_head->x = sorted_matrix_head->x;
  matrix_head->next = sorted_matrix_head->next;
}

int *sort_1dim_matrix(int *array, int size)
{
  for (int index = 0; index < size - 1; index++) {
    int min = 9000, pos = 0;
    for (int k = index; k < size; k++) {
      if (array[k] < min) {
        min = array[k];
        pos = k;
      }
    }
    if (min != 9000) {
      int temp = array[index];
      array[index] = array[pos];
      array[pos] = temp;
    }
  }

  return array;
}

void reverse_1dim_matrix(struct Non_zero_1dim_elems *matrix_head)
{
  struct Non_zero_1dim_elems *reverse_matrix_head = init_1dim_node(),
                             *reverse_matrix = reverse_matrix_head,
                             *matrix = matrix_head;

  for (; matrix->next != NULL;) {
    for (; matrix->next->next != NULL; matrix = matrix->next);
    reverse_matrix->i = matrix->i;
    reverse_matrix->x = matrix->x;
    reverse_matrix->next = init_1dim_node();
    reverse_matrix = reverse_matrix->next;

    matrix->i = 0;
    matrix->x = 0;
    matrix->next = NULL;
    matrix = matrix_head;
  }

  matrix_head->i = reverse_matrix_head->i;
  matrix_head->x = reverse_matrix_head->x;
  matrix_head->next = reverse_matrix_head->next;
}

void reverse_2dim_matrix(struct Non_zero_2dim_elems *matrix_head)
{
  struct Non_zero_2dim_elems *reverse_matrix_head = init_2dim_node(),
                             *reverse_matrix = reverse_matrix_head,
                             *matrix = matrix_head;

  for (; matrix->next != NULL;) {
    for (; matrix->next->next != NULL; matrix = matrix->next);
    reverse_matrix->i = matrix->i;
    reverse_matrix->j = matrix->j;
    reverse_matrix->x = matrix->x;
    reverse_matrix->next = init_2dim_node();
    reverse_matrix = reverse_matrix->next;

    matrix->i = 0;
    matrix->j = 0;
    matrix->x = 0;
    matrix->next = NULL;
    matrix = matrix_head;
  }

  matrix_head->i = reverse_matrix_head->i;
  matrix_head->j = reverse_matrix_head->j;
  matrix_head->x = reverse_matrix_head->x;
  matrix_head->next = reverse_matrix_head->next;
}

void sparse_forward_substitution(struct Non_zero_2dim_elems *l_matrix, double *x_matrix)
{
  int *non_zero = (int*)malloc(sizeof(int) * SIZE), count = 0;
  for (int i = 0; i < SIZE; i++) {
    if (x_matrix[i]) {
      non_zero[count++] = i;
    }
  }

  for (int j = 0; j < count; j++) {
    for (int i = non_zero[j] + 1; i < SIZE; i++) {
      for (; l_matrix->next != NULL; l_matrix = l_matrix->next) {
        if (l_matrix->i > i || l_matrix->j > non_zero[j]) {
          break;
        }
        if (l_matrix->i != l_matrix->j && l_matrix->i == i && l_matrix->j == non_zero[j]) {
          if (!x_matrix[i]) {
            non_zero[count++] = i;
            non_zero = sort_1dim_matrix(non_zero, count);
            /*printf("\n|| ");
            for (int i = 0; i < count; i++) {
              printf("%d ", non_zero[i]);
            }
            printf("||\n");*/
          }
          x_matrix[i] -= l_matrix->x * x_matrix[non_zero[j]];
          flop += 2;
          break;
        }
      }
    }
  }

        /*for (int i = j + 1; i < SIZE; i++) {
        if (l_matrix[i][j]) {
          x_matrix[i] -= l_matrix[i][j] * x_matrix[j];
        }
      }*/
}

void backward_substitution(struct Non_zero_2dim_elems *u_matrix, double *b_matrix)
{
  /*struct Non_zero_1dim_elems *x_matrix = x_matrix_head,
                             *x_matrix_temp = NULL,
                             *node = NULL;*/

  /*for (; u_matrix->next != NULL; u_matrix = u_matrix->next) {
    if (u_matrix->i < x_matrix->i) {
      for (; u_matrix->i < x_matrix->i && x_matrix->next != NULL; x_matrix = x_matrix->next);
      if (x_matrix->next == NULL) {
        x_matrix->i = u_matrix->i;
        x_matrix->x = 0;
        x_matrix->next = init_1dim_node();
      }
    }
    if (u_matrix->i > x_matrix->i) {
      node = init_1dim_node();
      node->i = x_matrix->i;
      node->x = x_matrix->x;
      node->next = x_matrix->next;

      x_matrix->i = u_matrix->i;
      x_matrix->x = 0;
      x_matrix->next = node;
    }

    x_matrix_temp = x_matrix_head;
    for (; x_matrix_temp->i != u_matrix->j; x_matrix_temp = x_matrix_temp->next);
    if (u_matrix->i != u_matrix->j) {
      x_matrix->x -= u_matrix->x * x_matrix_temp->x;
    } else {
      x_matrix->x /= u_matrix->x;
    }
  }*/



  for (int i = SIZE - 1; u_matrix->next != NULL; u_matrix = u_matrix->next) {
    for (; u_matrix->i < i; i--);
    if (u_matrix->i != u_matrix->j) {
      b_matrix[i] -= u_matrix->x * b_matrix[u_matrix->j];
    } else {
      b_matrix[i] /= u_matrix->x;
    }
  }

      /*if (u_matrix[i][j]) {
        if (i != j) {
          b_matrix[i] -= u_matrix[i][j] * x_matrix[j];
        } else {
          x_matrix[i] = b_matrix[i] / u_matrix[i][j];
        }
      }*/
}

void init_graph_dep(struct Graph_dep *dag, struct Non_zero_2dim_elems *a)
{
  for (int i = 0; i < SIZE; i++) {
    dag[i].i = i;
    dag[i].level = 0;
    dag[i].deps = NULL;
    dag[i].count_deps = 0;
    dag[i].up_deps = NULL;
    dag[i].count_up_deps = 0;
  }

  for (; a->next != NULL; a = a->next) {
    if (a->i == a->j) {                                                   //diagonal
      bool copy = false;
      for (int i = 0; i < dag[a->i].count_deps; i++) {
        if (dag[a->i].deps[i] == a->i) {
          copy = true;
          break;
        }
      }
      if (!copy) {
        dag[a->i].count_deps++;
        dag[a->i].deps = realloc(dag[a->i].deps, sizeof(int) * dag[a->i].count_deps);
        dag[a->i].deps[dag[a->i].count_deps - 1] = a->i;
      }
    } else if (a->i < a->j) {                                             //upper
      for (int i = 0; i < dag[a->i].count_deps; i++) {
        bool copy = false;
        for (int j = 0; j < dag[a->j].count_deps; j++) {
          if (dag[a->i].deps[i] == dag[a->j].deps[j]) {
            copy = true;
            break;
          }
        }
        for (int j = 0; j < dag[a->j].count_up_deps; j++) {
          if (dag[a->i].deps[i] == dag[a->j].up_deps[j]) {
            copy = true;
            break;
          }
        }
        if (!copy) {
          dag[a->j].count_up_deps++;
          dag[a->j].up_deps = realloc(dag[a->j].up_deps, sizeof(int) * dag[a->j].count_up_deps);
          dag[a->j].up_deps[dag[a->j].count_up_deps - 1] = dag[a->i].deps[i];
          /*dag[a->j].count_deps++;
          dag[a->j].deps = realloc(dag[a->j].deps, sizeof(int) * dag[a->j].count_deps);
          dag[a->j].deps[dag[a->j].count_deps - 1] = dag[a->i].deps[i];*/
        }
      }
    } else if (a->i > a->j) {                                             //lower
      bool copy = false;
      for (int i = 0; i < dag[a->j].count_deps; i++) {
        if (dag[a->j].deps[i] == a->i) {
          copy = true;
          break;
        }
      }
      if (!copy) {
        dag[a->j].count_deps++;
        dag[a->j].deps = realloc(dag[a->j].deps, sizeof(int) * dag[a->j].count_deps);
        dag[a->j].deps[dag[a->j].count_deps - 1] = a->i;
      }
    }
  }

  for (int i = 0; i < SIZE; i++) {
    int max_lvl = 0;
    for (int j = 0; j < dag[i].count_up_deps; j++) {
      if (max_lvl <= dag[dag[i].up_deps[j]].level) {
        max_lvl = dag[dag[i].up_deps[j]].level;
      }
    }
    dag[i].level = max_lvl + 1;
  }

  /*for (bool copy = false; a->next != NULL; a = a->next) {
    if (a->i == a->j) {                                                   //diagonal
      dag[a->i].level = 1;
    } else if (a->i < a->j) {                                             //upper
      copy = false;
      for (int i = 0; i < dag[a->j].count_deps; i++) {                   //check copy
        if (dag[a->j].deps[i] == a->i) {
          copy = true;
          break;
        }
      }
      if (!copy) {                                                        //if copy not exist
        dag[a->j].count_deps++;
        dag[a->j].deps = realloc(dag[a->j].deps, sizeof(int) * dag[a->j].count_deps);
        dag[a->j].deps[dag[a->j].count_deps - 1] = a->i;

        for (int i = 0; i < dag[dag[a->j].deps[dag[a->j].count_deps - 1]].count_deps; i++) {
          int temp = dag[dag[a->j].deps[dag[a->j].count_deps - 1]].deps[i];
          for (int j = 0; j < dag[a->j].count_deps; j++) {

          }
          if (temp)
        }

        for (int i = 0; i < dag[a->j].count_deps; i++) {
          if (dag[a->j].level <= dag[dag[a->j].deps[i]].level) {
            dag[a->j].level = dag[dag[a->j].deps[i]].level + 1;
          }
        }
      }
    } else if (a->i > a->j) {                                             //lower
      dag[a->j].count_deps++;
      dag[a->j].deps = realloc(dag[a->j].deps, sizeof(int) * dag[a->j].count_deps);
      dag[a->j].deps[dag[a->j].count_deps - 1] = a->i;

      dag[a->i].count_deps++;
      dag[a->i].deps = realloc(dag[a->i].deps, sizeof(int) * dag[a->i].count_deps);
      dag[a->i].deps[dag[a->i].count_deps - 1] = a->j;
      if (dag[a->i].count_deps > 1) {
        if (dag[a->j].level <= dag[dag[a->i].deps[dag[a->i].count_deps - 2]].level) {
          dag[a->j].level = dag[dag[a->i].deps[dag[a->i].count_deps - 2]].level + 1;
        }
      }
      for (int i = 0; i < dag[a->i].count_deps; i++) {
        if (dag[a->i].level <= dag[dag[a->i].deps[i]].level) {
          dag[a->i].level = dag[dag[a->i].deps[i]].level + 1;
        }
      }
    }
  }*/

  /*for (int i = 0; i < SIZE; i++) {                                        //init levels
    for (int j = 0; j < dag[i].count_deps; j++) {
      if (dag[dag[i].deps[j]].level < dag[i].level + 1) {
        dag[dag[i].deps[j]].level = dag[i].level + 1;
      }
    }
  }*/
}

void *gilbert_peierls_parallel(void *args)
{
  struct Pthread_args *arg = (struct Pthread_args*)args;
  struct Non_zero_2dim_elems *l_matrix = l_matrix_temp,
                             *u_matrix = u_matrix_temp;
  double *b_matrix = (double*)malloc(sizeof(double) * SIZE);
  memset(b_matrix, 0, sizeof(double) * SIZE);

  cmp_out_2dim_in_1dim(arg->a_matrix_head, b_matrix, arg->k, COLUMN);

  /*printf("L\n");
  print_2dim_matrix2(l_matrix_head);
  printf("B\n");
  for (int i = 0; i < SIZE; i++) {
    printf("%lf\n", b_matrix[i]);
  }*/
  sparse_forward_substitution(l_matrix_head, b_matrix);
  /*printf("B\n");
  for (int i = 0; i < SIZE; i++) {
    printf("%lf\n", b_matrix[i]);
  }*/

//printf("IN%d\n", arg->k);
pthread_mutex_lock(&lock);
  for (; u_matrix->next != NULL; u_matrix = u_matrix->next);
  u_matrix_temp = u_matrix;
  for (int i = 0; i <= arg->k; i++) {
    if (b_matrix[i]) {
      u_matrix->i = i;
      u_matrix->j = arg->k;
      u_matrix->x = b_matrix[i];
      u_matrix->next = init_2dim_node();
      u_matrix = u_matrix->next;
    }
  }
  u_matrix = u_matrix_temp;

  for (; l_matrix->next != NULL; l_matrix = l_matrix->next);
  for (int i = arg->k + 1; i < SIZE; i++) {
    if (b_matrix[i]) {
      l_matrix->i = i;
      l_matrix->j = arg->k;
      for (; u_matrix->i != u_matrix->j || u_matrix->i != arg->k; u_matrix = u_matrix->next);
      l_matrix->x = b_matrix[i] / u_matrix->x;
      flop++;
      l_matrix->next = init_2dim_node();
      l_matrix = l_matrix->next;
    }
  }
  l_matrix_temp = l_matrix;
pthread_mutex_unlock(&lock);
//printf("OUT%d\n", arg->k);
}

void gilbert_peierls_lu_factorization(struct Non_zero_2dim_elems *a_matrix_head)
{
  struct Graph_dep *dag = (struct Graph_dep*)malloc(sizeof(struct Graph_dep) * SIZE);
  memset(dag, 0, sizeof(struct Graph_dep) * SIZE);
  sort_for_graph(a_matrix_head);
  init_graph_dep(dag, a_matrix_head);
  sort_2dim_matrix(a_matrix_head, COLUMN);

  /*
  for (int i = 0; i < SIZE; i++) {                          //output graph
    printf("index: %d\tlevel: %d\tcount deps: %d\ndeps: ", dag[i].i, dag[i].level, dag[i].count_deps);
    for (int j = 0; j < dag[i].count_deps; j++) {
      printf("%d ", dag[i].deps[j]);
    }
    printf("\ncount up deps: %d\nup deps: ", dag[i].count_up_deps);
    for (int j = 0; j < dag[i].count_up_deps; j++) {
      printf("%d ", dag[i].up_deps[j]);
    }
    printf("\n\n");
  }
  */

  int level_max = 0;                                        //find max level
  for (int i = 0; i < SIZE; i++) {
    if (level_max < dag[i].level) {
      level_max = dag[i].level;
    }
  }

  pthread_t threads[SIZE];
  pthread_mutex_init(&lock, NULL);
  int status = 0;
  struct Pthread_args *args = (struct Pthread_args*)malloc(sizeof(struct Pthread_args) * SIZE);

  u_matrix_temp = u_matrix_head;
  for (int level = 1, count_on_lvl = 0; level <= level_max; level++, count_on_lvl = 0) {
    sort_2dim_matrix(l_matrix_head, COLUMN);
    l_matrix_temp = l_matrix_head;
    for (; l_matrix_temp->next != NULL; l_matrix_temp = l_matrix_temp->next);
    for (; u_matrix_temp->next != NULL; u_matrix_temp = u_matrix_temp->next);

    for (int k = 0, thread_id = 0; k < SIZE; k++) {
      if (level != dag[k].level) {
        continue;
      }
      args[k].k = k;
      args[k].a_matrix_head = a_matrix_head;
      status = pthread_create(&threads[thread_id++], NULL, (void *)gilbert_peierls_parallel, (void *)&args[k]);
      if (status != 0) {
          printf("Can't create thread, status = %d\n", status);
          exit(-1);
      }
      count_on_lvl++;
    }
    printf("lvl: %d\n", count_on_lvl);
    for (int thread_id = 0; thread_id < count_on_lvl; thread_id++) {
      status = pthread_join(threads[thread_id], NULL);
      if (status != 0) {
          printf("Can't join thread, status = %d\n", status);
          exit(-1);
      }
    }
  }
  
  /*for (int k = 0; k < SIZE; k++) {
    cmp_out_2dim_in_1dim(a_matrix_head, b_matrix, k, COLUMN);
    
    /*printf("L matrix\n");
    print_2dim_matrix(l_matrix_head);
    print_2dim_matrix2(l_matrix_head);
    printf("B matrix\n");
    for (int i = 0; i < SIZE; i++) {
      printf("%lf\n", b_matrix[i]);
    }
    printf("\n");
    printf("IN%d\n", k);
    sparse_forward_substitution(l_matrix_head, b_matrix);
    /*printf("X matrix\n");
    for (int i = 0; i < SIZE; i++) {
      printf("%lf\n", b_matrix[i]);
    }
    printf("\n");*/
    //printf("OUT%d\n", k);

    /*for (; x_matrix->i <= k && x_matrix->next != NULL; x_matrix = x_matrix->next) {
      u_matrix->i = x_matrix->i;
      u_matrix->j = k;
      u_matrix->x = x_matrix->x;
      u_matrix->next = init_2dim_node();
      u_matrix = u_matrix->next;
    }*/
    //for (int i = 0; i <= k; i++) {
//      u_matrix[i][k] = x_matrix[i];
      //if (b_matrix[i]) {
      //if (x_matrix[i]) {
        //u_matrix->i = i;
        //u_matrix->j = k;
        //u_matrix->x = x_matrix[i];
        //u_matrix->x = b_matrix[i];
        //u_matrix->next = init_2dim_node();
        //u_matrix = u_matrix->next;
/*      }
    }
    sort_2dim_matrix(u_matrix_head, ROW);
    u_matrix = u_matrix_head;
    for (; u_matrix->next != NULL; u_matrix = u_matrix->next);
    /*printf("U matrix\n");
    print_2dim_matrix(u_matrix_head);*/

    /*struct Non_zero_2dim_elems *u_matrix_temp = u_matrix_head;
    struct Non_zero_2dim_elems *l_matrix = l_matrix_head;
    for (; l_matrix->next != NULL; l_matrix = l_matrix->next);
    
    /*for (; x_matrix->next != NULL; x_matrix = x_matrix->next) {
      l_matrix->i = x_matrix->i;
      l_matrix->j = k;
      for (; u_matrix_temp->i != u_matrix_temp->j || u_matrix_temp->i != k; u_matrix_temp = u_matrix_temp->next);
      l_matrix->x = x_matrix->x / u_matrix_temp->x;
      l_matrix->next = init_2dim_node();
      l_matrix = l_matrix->next;
    }*/
    /*for (int i = k + 1; i < SIZE; i++) {
      //l_matrix[i][k] = x_matrix[i] / u_matrix[k][k];
      //if (x_matrix[i]) {
      if (b_matrix[i]) {
        l_matrix->i = i;
        l_matrix->j = k;
        for (; u_matrix_temp->i != u_matrix_temp->j || u_matrix_temp->i != k; u_matrix_temp = u_matrix_temp->next);
        //l_matrix->x = x_matrix[i] / u_matrix_temp->x;
        l_matrix->x = b_matrix[i] / u_matrix_temp->x;
        l_matrix->next = init_2dim_node();
        l_matrix = l_matrix->next;
      }
    }
    sort_2dim_matrix(l_matrix_head, COLUMN);

    /*for (int i = 0; i <= k; i++) {
      u_matrix[i][k] = x_matrix[i];
    }
    for (int i = k + 1; i < SIZE; i++) {
      l_matrix[i][k] = x_matrix[i] / u_matrix[k][k];
    }*/
  //}
}

void cholesky_lu_factorization(double a_matrix[SIZE][SIZE], double l_matrix[SIZE][SIZE])
{
  double luk = 0;

  for (int i = 0; i < SIZE; i++) {
    for (int j = i; j < SIZE; j++) {
      luk = 0;
      for (int k = 0; k < i; k++) {
        if (l_matrix[i][k] != 0 && l_matrix[j][k] != 0) {
          luk += l_matrix[i][k] * l_matrix[j][k];
          flop_cho += 2;
        }
      }
      if (i == j) {
        l_matrix[j][i] = sqrt(a_matrix[j][i] - luk);
        flop_cho++;
      } else if (i < j) {
        l_matrix[j][i] = (a_matrix[j][i] - luk) / l_matrix[i][i];
        flop_cho += 2;
      }
    }
  }
}

void start_cholesky()
{
  double **a = (double**)malloc(sizeof(double*) * SIZE);
  double **l = (double**)malloc(sizeof(double*) * SIZE);
  double **u = (double**)malloc(sizeof(double*) * SIZE);
  for (int i = 0; i < SIZE; i++) {
    a[i] = (double*)malloc(sizeof(double) * SIZE);
    l[i] = (double*)malloc(sizeof(double) * SIZE);
    u[i] = (double*)malloc(sizeof(double) * SIZE);
  }

  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j <= i; j++) {
      a[i][j] = 0;
      l[i][j] = 0;
      u[i][j] = 0;
      if((rand() % 100) + 1 > PERCENT_ZEROS) {
        u[i][j] = (rand() % MAX_NUMBER) + 1;
      }
    }
  }

  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      for (int k = 0; k < SIZE; k++) {
        a[i][j] += u[i][k] * u[k][j];
      }
    }
  }

  /*for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      printf("%lf\t", a[i][j]);
    }
    printf("\n");
  }*/

  cholesky_lu_factorization(a,l);
  printf("\n\nflop cho: %d\n\n", flop_cho);
}

/*void block_lu_factorization(double a_matrix[SIZE][SIZE], double l_matrix[SIZE][SIZE], double u_matrix[SIZE][SIZE])
{
  double luk = 0;

  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      luk = 0;
      for (int k = 0; k < i; k++) {
        luk += l_matrix[i][k] * u_matrix[k][j];
      }
      if (j >= i) {
        u_matrix[i][j] = a_matrix[i][j] - luk;
      } else if (i > j) {
        l_matrix[i][j] = (a_matrix[i][j] - luk) / u_matrix[j][j];
      }
    }
  }
}*/

void init_matrixes(struct Non_zero_2dim_elems *a_matrix, double *b_matrix)
{
  for (int j = 0; j < SIZE; j++) {
    for (int i = 0; i < SIZE; i++) {
      if ((rand() % 100) + 1 > PERCENT_ZEROS || i == j) {
        a_matrix->i = i;
        a_matrix->j = j;
        a_matrix->x = (rand() % MAX_NUMBER) + 1;
        a_matrix->next = init_2dim_node();
        a_matrix = a_matrix->next;
        count_nz++;
      }
    }
    if ((rand() % 100) + 1 > PERCENT_ZEROS) {
      b_matrix[j] = (rand() % MAX_NUMBER) + 1;
    }
  }
}

void init_eden_matrix(struct Non_zero_2dim_elems *matrix)
{
  for (int i = 0; i < SIZE; i++) {
    matrix->i = i;
    matrix->j = i;
    matrix->x = 1;
    matrix->next = init_2dim_node();
    matrix = matrix->next;
  }
}

void launch_gilbert_peierls(struct Non_zero_2dim_elems *a_matrix, double *b_matrix)
{
  l_matrix_head = init_2dim_node();
  u_matrix_head = init_2dim_node();

  init_eden_matrix(l_matrix_head);
  
  printf("GILBERT-PEIERLS LU-FACTORIZATION\n\n");

  clock_t time = clock();
  gilbert_peierls_lu_factorization(a_matrix);
  time = clock() - time;

  /*printf("ENDFACT\n");
  printf("L matrix\n");
  print_2dim_matrix2(l_matrix_head);
  printf("U matrix\n");
  print_2dim_matrix2(u_matrix_head);*/
  
  sort_2dim_matrix(l_matrix_head, COLUMN);
  sparse_forward_substitution(l_matrix_head, b_matrix);

  /*print_2dim_matrix(l_matrix_head);
  printf("ENDSPARSE\n");
  printf("B matrix\n");
  for (int i = 0; i < SIZE; i++) {
    printf("%lf\n", b_matrix[i]);
  }*/

  sort_2dim_matrix(u_matrix_head, ROW);
  reverse_2dim_matrix(u_matrix_head);
  backward_substitution(u_matrix_head, b_matrix);
  printf("X matrix\n");
  for (int i = 0; i < SIZE; i++) {
    printf("%lf\n", b_matrix[i]);
  }
  printf("\nTIME: %lf\n", (double)time / CLOCKS_PER_SEC);
}

void init_matrixes2(struct Non_zero_2dim_elems *a_matrix, double *b_matrix)
{
  a_matrix->i = 0; a_matrix->j = 0; a_matrix->x = 6; a_matrix->next = init_2dim_node(); a_matrix = a_matrix->next;
  a_matrix->i = 0; a_matrix->j = 3; a_matrix->x = 2; a_matrix->next = init_2dim_node(); a_matrix = a_matrix->next;
  a_matrix->i = 1; a_matrix->j = 1; a_matrix->x = 2; a_matrix->next = init_2dim_node(); a_matrix = a_matrix->next;
  a_matrix->i = 1; a_matrix->j = 2; a_matrix->x = 5; a_matrix->next = init_2dim_node(); a_matrix = a_matrix->next;
  a_matrix->i = 1; a_matrix->j = 3; a_matrix->x = 4; a_matrix->next = init_2dim_node(); a_matrix = a_matrix->next;
  a_matrix->i = 1; a_matrix->j = 4; a_matrix->x = 7; a_matrix->next = init_2dim_node(); a_matrix = a_matrix->next;
  a_matrix->i = 2; a_matrix->j = 2; a_matrix->x = 5; a_matrix->next = init_2dim_node(); a_matrix = a_matrix->next;
  a_matrix->i = 3; a_matrix->j = 0; a_matrix->x = 4; a_matrix->next = init_2dim_node(); a_matrix = a_matrix->next;
  a_matrix->i = 3; a_matrix->j = 3; a_matrix->x = 2; a_matrix->next = init_2dim_node(); a_matrix = a_matrix->next;
  a_matrix->i = 4; a_matrix->j = 1; a_matrix->x = 4; a_matrix->next = init_2dim_node(); a_matrix = a_matrix->next;
  a_matrix->i = 4; a_matrix->j = 3; a_matrix->x = 9; a_matrix->next = init_2dim_node(); a_matrix = a_matrix->next;
  a_matrix->i = 4; a_matrix->j = 4; a_matrix->x = 10; a_matrix->next = init_2dim_node(); a_matrix = a_matrix->next;

  /*b_matrix->i = 0; b_matrix->x = 6; b_matrix->next = init_1dim_node(); b_matrix = b_matrix->next;
  b_matrix->i = 2; b_matrix->x = 5; b_matrix->next = init_1dim_node(); b_matrix = b_matrix->next;
  b_matrix->i = 4; b_matrix->x = 9; b_matrix->next = init_1dim_node(); b_matrix = b_matrix->next;*/
  b_matrix[0] = 6;
  b_matrix[2] = 5;
  b_matrix[4] = 9;
}

void init_matrixes3(struct Non_zero_2dim_elems *a_matrix, double *b_matrix)
{
  double number = 0;

  FILE *data = fopen("data.txt", "r");
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      fscanf(data, "%lf", &number);
      if (number) {
        a_matrix->i = i;
        a_matrix->j = j;
        a_matrix->x = number;
        a_matrix->next = init_2dim_node();
        a_matrix = a_matrix->next;
      }
    }
  }

  for (int i = 0; i < SIZE; i++) {
    fscanf(data, "%lf", &number);
    if (number) {
      b_matrix[i] = number;
    }
  }  
  fclose(data);
}

int main(void)
{
  srand(time(NULL));

  struct Non_zero_2dim_elems *a_matrix = init_2dim_node();
  double *b_matrix = (double*)malloc(sizeof(double) * SIZE);
  memset(b_matrix, 0, sizeof(double) * SIZE);
  
#ifdef SUMMETRIC_MATRIX
  init_matrixes(a_matrix, b_matrix);
#else
  init_matrixes2(a_matrix, b_matrix);
#endif
  sort_2dim_matrix(a_matrix, COLUMN);

  //print_2dim_matrix2(a_matrix);
  //print_1dim_matrix(b_matrix);
  /*for (int i = 0; i < SIZE; i++) {
    printf("%lf\n", b_matrix[i]);
  }*/
  //start_cholesky();
  launch_gilbert_peierls(a_matrix, b_matrix);

  printf("FLOP: %d\nCOUNT: %d\n", flop, count_nz);
  printf("FINISH");
  getchar();
  return 0;
}