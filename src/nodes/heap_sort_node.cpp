#include "node.hpp"

#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace {

bool read_numbers(const autorob::Json& args, std::vector<double>& numbers) {
  const autorob::Json* input = args.get("numbers");
  if (!input || !input->is_array()) return false;
  for (const autorob::Json& item : input->as_array()) {
    if (!item.is_number() || !std::isfinite(item.as_number())) return false;
    numbers.push_back(item.as_number());
  }
  return true;
}

// TODO 1: Restore the max-heap rule below index.
void sift_down_max(std::vector<double>& heap, std::size_t index, std::size_t heap_size) {
  std::size_t i = index;
  while (heap_size >= 2*i+2+1 && (heap[i] < heap[2*i+1] || heap[i] < heap[2*i+2])){
    if (heap[2*i+1] > heap[2*i+2]){
      if (heap[i] < heap[2*i+1]){
        double temp_parent_for_left_child = heap[i];
        heap[i] = heap[2*i+1];
        heap[2*i+1] = temp_parent_for_left_child;
        i = 2*i+1;
      }
    }
    else {
      if (heap[i] < heap[2*i+2]){
        double temp_parent_for_right_child = heap[i];
        heap[i] = heap[2*i+2];
        heap[2*i+2] = temp_parent_for_right_child;
        i = 2*i+2;
      }
    }
  }
  if (heap_size >= 2*i+1+1 && heap[i] < heap[2*i+1]){
    double temp_parent_for_left_child = heap[i];
    heap[i] = heap[2*i+1];
    heap[2*i+1] = temp_parent_for_left_child;
  }
}

// TODO 2: Build a max heap, repeatedly swap index 0 with the last heap item,
// shrink heap_size, and restore the heap. The final vector must be ascending.
void heap_sort(std::vector<double>& numbers) {
  for (std::size_t i = numbers.size() / 2; i > 0; i--) {
    sift_down_max(numbers, i - 1, numbers.size());
  }
  for (std::size_t heap_size = numbers.size(); heap_size > 1; heap_size--) {
    std::swap(numbers[0], numbers[heap_size - 1]);
    sift_down_max(numbers, 0, heap_size - 1);
  }
}

autorob::ServiceResult heap_sort_callback(const autorob::Json& args) {
  std::vector<double> numbers;
  if (!read_numbers(args, numbers)) {
    return {false, autorob::Json::Object{}, "expected args.numbers number array"};
  }

  heap_sort(numbers);

  // TODO 3: Convert numbers to Json::Array and return:
  // return {true, autorob::Json::Object{{"sorted", autorob::Json(array)}}, ""};
  autorob::Json::Array result;
  for (int i=0;i<numbers.size();i++){
    result.push_back(autorob::Json(numbers[i]));
  }
  return {true, autorob::Json::Object{{"sorted", autorob::Json(result)}}, ""};
}

}  // namespace

int main() {
  autorob::Node node("heap_sort_node");
  node.advertise_service("/heap_sort", "autorob/heap_sort", heap_sort_callback);
  node.wait();
}
