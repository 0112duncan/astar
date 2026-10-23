#include "node.hpp"

#include <cmath>
#include <cstddef>
#include <vector>

namespace {

bool read_values(const autorob::Json& args, std::vector<double>& values) {
  const autorob::Json* input = args.get("values");
  if (!input || !input->is_array()) return false;
  for (const autorob::Json& item : input->as_array()) {
    if (!item.is_number() || !std::isfinite(item.as_number())) return false;
    values.push_back(item.as_number());
  }
  return true;
}

// TODO 1: Move heap[index] downward until the min-heap rule is restored.
void sift_down_min(std::vector<double>& heap, std::size_t index) {
  std::size_t i = index;
  while (heap.size() >= 2*i+2+1 && (heap[i] > heap[2*i+1] || heap[i] > heap[2*i+2])){
    if (heap[2*i+1] < heap[2*i+2]){
      if (heap[i] > heap[2*i+1]){
        double temp_parent_for_left_child = heap[i];
        heap[i] = heap[2*i+1];
        heap[2*i+1] = temp_parent_for_left_child;
        i = 2*i+1;
      }
    }
    else {
      if (heap[i] > heap[2*i+2]){
        double temp_parent_for_right_child = heap[i];
        heap[i] = heap[2*i+2];
        heap[2*i+2] = temp_parent_for_right_child;
        i = 2*i+2;
      }
    }
  }
  if (heap.size() >= 2*i+1+1 && heap[i] > heap[2*i+1]){
    double temp_parent_for_left_child = heap[i];
    heap[i] = heap[2*i+1];
    heap[2*i+1] = temp_parent_for_left_child;
  }
}

// TODO 2: Call sift_down_min from the last parent back to index 0.
void build_min_heap(std::vector<double>& heap) {
  for (std::size_t i = heap.size() / 2; i > 0; --i) {
    sift_down_min(heap, i - 1);
  }
}

autorob::ServiceResult min_heap_callback(const autorob::Json& args) {
  std::vector<double> heap;
  if (!read_values(args, heap)) {
    return {false, autorob::Json::Object{}, "expected args.values number array"};
  }

  build_min_heap(heap);

  // TODO 3: After build_min_heap works, convert heap to a Json::Array and return:
  // return {true, autorob::Json::Object{{"heap", autorob::Json(array)}}, ""};
  autorob::Json::Array result;
  for (int i=0;i<heap.size();i++){
    result.push_back(autorob::Json(heap[i]));
  }
  return {true, autorob::Json::Object{{"heap", autorob::Json(result)}}, ""};
}

}  // namespace

int main() {
  autorob::Node node("min_heap_node");

  // This one line turns heapify_callback into the /heapify service.
  node.advertise_service("/heapify", "autorob/min_heap", min_heap_callback);
  node.wait();
}
