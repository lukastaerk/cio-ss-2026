#include <format>
#include <iostream>
#include <set>
#include <string>
#include <vector>

struct Node {
  std::string value;
  std::vector<Node*> successors;
  explicit Node(const std::string& v = "") : value(v) {}
};

bool search_from(Node* node, Node* goalnode, std::set<Node*>& nodesvisited) {
  if (nodesvisited.count(node)) {
    return false;
  } else if (node == goalnode) {
    return true;
  } else {
    nodesvisited.insert(node);
    for (Node* nextnode : node->successors) {
      if (search_from(nextnode, goalnode, nodesvisited)) {
        return true;
      }
    }
    return false;
  }
}
bool depth_first_search(Node* startnode, Node* goalnode) {
  std::set<Node*> nodesvisited;
  return search_from(startnode, goalnode, nodesvisited);
}

static void report(const std::string& name, bool got, bool expected,
                   int& failed) {
  bool ok = (got == expected);
  std::cout << std::format("{} = {} expected {} {}", name,
                           got ? "Path found!" : "Path not found!",
                           expected ? "Path found!" : "Path not found!",
                           ok ? "  PASS" : "  FAIL")
            << "\n";
  if (!ok) ++failed;
}

int main() {
  int failed = 0;

  // Case 1: Strongly connected (acyclic) graph -> Path found!
  Node station1("Westminster");
  Node station2("Waterloo");
  Node station3("Trafalgar Square");
  Node station4("Canary Wharf");
  Node station5("London Bridge");
  Node station6("Tottenham Court Road");
  station2.successors = {&station1};
  station3.successors = {&station1, &station2};
  station4.successors = {&station2, &station3};
  station5.successors = {&station4, &station3};
  station6.successors = {&station5, &station4};
  report("case 1", depth_first_search(&station6, &station1), true, failed);

  // Case 2: Branching graph -> Path found!
  Node nodef("F"), nodee("E"), noded("D");
  Node nodec("C"), nodeb("B"), nodea("A");
  nodec.successors = {&nodef};
  nodeb.successors = {&nodee};
  nodea.successors = {&nodeb, &nodec, &noded};
  report("case 2", depth_first_search(&nodea, &nodee), true, failed);

  // Case 3: Two unconnected nodes -> Path not found
  report("case 3", depth_first_search(&nodef, &nodee), false, failed);

  // Case 4: One node graph -> Path found!
  report("case 4", depth_first_search(&nodef, &nodef), true, failed);

  // Case 5: Graph with cycles -> Path found!
  nodee.successors = {&nodea};
  report("case 5", depth_first_search(&nodea, &nodef), true, failed);

  std::cout << (failed == 0 ? "running tests PASSED\n" : "SOME TESTS FAILED\n");
  return failed == 0 ? 0 : 1;
}