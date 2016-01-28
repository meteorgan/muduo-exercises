#include "Queen.h"

#include <iostream>
#include <algorithm>
#include <thread>
#include <future>
#include <cassert>

bool is_conflict(const std::vector<int>& columns, int column) {
  if(std::find(columns.begin(), columns.end(), column) != columns.end())
    return true;

  int r = columns.size();
  for(int row = 0; row < r; ++row) {
    if((columns[row] - column ==  row - r) || (columns[row] - column == abs(row - r)))
      return true;
  }

  return false;
}

std::list<std::vector<int>> put_n_row(const std::vector<int>& queen, int column_n) {
  std::list<std::vector<int>> result;  

  for(int i = 0; i < column_n; ++i) {
    if(!is_conflict(queen, i)) {
      std::vector<int> new_row(queen);
      new_row.push_back(i);
      result.push_back(new_row);
    }
  }

  return result;
}

void put_n_row(std::list<std::vector<int>>& results, int column_n) {
  int size = results.size();
  while(size-- > 0) {
    std::vector<int> current = results.front();
    std::list<std::vector<int>> new_rows = put_n_row(current, column_n);
    results.insert(results.end(), new_rows.begin(), new_rows.end());

    results.pop_front();
  }
}

void solve_queens(std::list<std::vector<int>>& queens, int size) {
  if(queens.size() == 0) {
    for(int i = 0; i < size; ++i)
      queens.push_back(std::vector<int>(1, i));
  }

  if(queens.front().size() != static_cast<size_t>(size)) {
    size_t row = queens.front().size();
    for(;row < static_cast<size_t>(size); ++row) {
      put_n_row(queens, size);
    }
  }
}

std::list<std::vector<int>> solve_queens(int size) {
  std::list<std::vector<int>> results;
  solve_queens(results, size);
  
  return results;
}

std::list<std::vector<int>> complete_queens(const std::vector<int>& queen, int size) { 
  assert(queen.size() > 0);
  std::list<std::vector<int>> results;
  results.push_back(queen);
  for(int i = queen.size(); i < size; ++i) {
    put_n_row(results, size);
  }

  return results;
}

std::list<std::vector<int>> solve_queens_multi_thread(int size) {
  std::list<std::vector<int>> temp;
  for(int i = 0; i < size; ++i)
    temp.push_back(std::vector<int>(1, i));

  std::vector<std::future<std::list<std::vector<int>>>> futures(size);
  int i = 0;
  for(auto &queen : temp) {
    futures[i] = std::async(std::launch::async, complete_queens, std::ref(queen), size);
    ++i;
  }

  std::list<std::vector<int>> results;
  for(auto &rf : futures) {
    std::list<std::vector<int>> queens = rf.get();
    results.insert(results.end(), queens.begin(), queens.end());
  }
  
  return results; 
}
