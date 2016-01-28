#ifndef QUEEN_H
#define QUEEN_H

#include <vector>
#include <list>

bool is_conflict(const std::vector<int>& columns, int column);

std::list<std::vector<int>> put_n_row(const std::vector<int>& queen, int column_n);

void put_n_row(std::list<std::vector<int>>& results, int column_n);

void solve_queens(std::list<std::vector<int>>& queens, int size);

std::list<std::vector<int>> solve_queens(int size = 8);

std::list<std::vector<int>> complete_queens(const std::vector<int>& queen, int size);

std::list<std::vector<int>> solve_queens_multi_thread(int size = 8);


#endif
