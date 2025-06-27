#ifndef __PROGTEST__
#include <cassert>
#include <iomanip>
#include <cstdint>
#include <iostream>
#include <memory>
#include <limits>
#include <optional>
#include <algorithm>
#include <bitset>
#include <list>
#include <array>
#include <vector>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <random>
#include <type_traits>
#include <utility>

#endif

template < typename T >
struct Queue {

    class AVL{

        struct Node{
            Node() : totalDepth(1), rightSon(0), leftSon(0), totalRight(0), totalLeft(0), parent(0) {};
            Node(T iVal) : value(iVal), totalDepth(1) ,rightSon(0), leftSon(0), totalRight(0), totalLeft(0), parent(0) {};
            T value;
            unsigned int totalDepth;
            unsigned int rightSon;
            unsigned int leftSon;
            unsigned int totalRight;
            unsigned int totalLeft;
            unsigned int parent;
        };

    public:

        AVL() : size(0), root(1), min(0), field() {
            // Null node creation
            field.reserve(512);
            field.emplace_back();
            field[0].totalDepth = 0;
        };

        unsigned int Size() const {
            return size;
        }

        size_t getSizeOfNode(){
            return sizeof(Node);
        }

        unsigned int insertMax(T Value){
            if(size == UINT_MAX - 2)
                throw std::out_of_range("Out of memory MAX UINT!");

            unsigned int newNodeIndex = getFreeNode();
            field[newNodeIndex] = Node(Value);
            size++;

            if(size == 1){
                root = newNodeIndex;
                min = newNodeIndex;
                return root;
            }

            unsigned int current = root;

            while (true){
                if(field[current].rightSon == 0){
                    field[current].rightSon = newNodeIndex;
                    field[newNodeIndex].parent = current;
                    field[current].totalRight += 1;
                    break;
                }

                field[current].totalRight += 1;
                current = field[current].rightSon;
            }

            //Propagate change of depth

            while (true){
                unsigned int newDepth = std::max(field[field[current].leftSon].totalDepth, field[field[current].rightSon].totalDepth) + 1;
                if(field[current].totalDepth == newDepth){
                    break;
                }

                field[current].totalDepth = newDepth;

                current = fixTree(current);

                if(current == root)
                    break;
                current = field[current].parent;
            }

            return newNodeIndex;
        }

        T getMin(){
            if(size == 0)
                throw std::logic_error("Size is 0, cannot call getMin()!");
            return field[min].value;
        }

        void extractMin(){
            if(size == 0)
                return;

            if(field[min].parent != 0){
                field[field[min].parent].leftSon = field[min].rightSon;
                field[field[min].parent].totalLeft = field[min].totalRight;

                unsigned int current = field[min].parent;

                // Propagate change of total nodes count
                while(current){
                    unsigned int sum = field[current].totalRight + field[current].totalLeft + 1;
                    unsigned int parent = field[current].parent;

                    if(parent == 0)
                        break;
                    if(field[parent].leftSon == current)
                        field[parent].totalLeft = sum;
                    else
                        field[parent].totalRight = sum;
                    current = parent;
                }
            }
            else
                root = field[min].rightSon;

            if(field[min].rightSon != 0)
                field[field[min].rightSon].parent = field[min].parent;

            unsigned int oldDepth = field[min].parent == 0 ? field[field[min].rightSon].totalDepth : field[field[min].parent].totalDepth;
            unsigned int head = field[min].parent ? field[min].parent : field[min].rightSon;
            unsigned int current = head;

            while (field[current].leftSon) {
                current = field[current].leftSon;
            }

            freeNode(min);
            min = current;

            current = head;
            while(current){
                evalDepth(current);
                if(field[current].totalDepth == oldDepth && std::abs(getDepthDiff(current)) < 2){
                    break;
                }

                current = fixTree(current);

                if(current == root){
                    break;
                }

                current = field[current].parent;
                oldDepth = field[current].totalDepth;
            }

        }

        unsigned int getPos(unsigned int index) const{
            //Check in range
            if(!nodeExists(index)){
                throw std::out_of_range("Out of range getPos()!");
            }

            unsigned int parent = field[index].parent;
            unsigned int current = index;
            unsigned int pos = field[current].totalLeft;
            while (parent){
                parent = field[current].parent;

                if(field[parent].rightSon == current) {
                    pos += 1 + field[parent].totalLeft;
                }

                if(parent == root)
                    break;

                current = parent;
            }

            return pos;
        }

        void jumpAhead(unsigned int index, unsigned int by){

            if(!nodeExists(index))
                throw std::logic_error("jumpAhead() out of range index!");

            if(!by)
                return;

            unsigned int target;

            if(by < size - 1){
                // Find the predecessor
                unsigned int current = index;
                bool rightEdge = true;
                int jmp = static_cast<int>(by);
                while(current){
                    unsigned int parent = field[current].parent;
                    if(((jmp - static_cast<int>(field[current].totalLeft)) <= 0) && rightEdge){
                        break;
                    }
                    if(current == root){
                        jmp = field[root].totalLeft;
                        break;
                    }
                    if(rightEdge)
                        jmp -= field[current].totalLeft;
                    rightEdge = false;

                    if(field[parent].rightSon == current){
                        jmp -= 1;
                        rightEdge = true;
                    }

                    current = parent;
                }

                if(jmp != 0){
                    jmp -= 1;
                    current = field[current].leftSon;
                    while(current){
                        if(jmp - static_cast<int>(field[current].totalRight) < 0){
                            current = field[current].rightSon;
                            continue;
                        }
                        if(jmp - static_cast<int>(field[current].totalRight) > 0){
                            jmp -= field[current].totalRight + 1;
                            current = field[current].leftSon;
                            continue;
                        }
                        break;
                    }
                }

                target = current;
            }
            else{
                target = min;
            }

            if(target == min)
                min = index;

            if(target == index)
                return;
            // Delete index node
            deleteNode(index);

            // Insert as Predecessor
            if(!field[target].leftSon) {

                field[target].leftSon = index;
            }
            else {
                target = field[target].leftSon;
                while (field[target].rightSon) {
                    target = field[target].rightSon;
                }
                field[target].rightSon = index;
            }

            field[index].leftSon = 0;
            field[index].rightSon = 0;
            field[index].totalRight = 0;
            field[index].totalLeft = 0;
            field[index].totalDepth = 1;
            field[index].parent = target;

            while(target){
                field[target].totalLeft = field[field[target].leftSon].totalLeft + field[field[target].leftSon].totalRight + 1;
                if(!field[target].leftSon)
                    field[target].totalLeft = 0;
                field[target].totalRight = field[field[target].rightSon].totalLeft + field[field[target].rightSon].totalRight + 1;
                if(!field[target].rightSon)
                    field[target].totalRight = 0;
                evalDepth(target);
                target = fixTree(target);

                target = field[target].parent;
            }

        }

    private:

        bool nodeExists(unsigned int index) const{
            if(index >= field.size() || index == 0)
                return false;

            if(field[index].leftSon == 0 && field[index].totalLeft > 0)
                return false;
            else
                return true;
        }

        // Evaluates depth of node from its children
        void evalDepth(unsigned int index){
            field[index].totalDepth = std::max(field[field[index].leftSon].totalDepth, field[field[index].rightSon].totalDepth) + 1;
        }

        // Returns the new root node
        unsigned int fixTree(unsigned int index){

            int diff = getDepthDiff(index);

            if(std::abs(diff) > 1){
                // Fix tree
                if(diff == -2){
                    int subDiff = getDepthDiff(field[index].leftSon);
                    if (subDiff > 0){
                        rotateLeft(field[index].leftSon);
                    }
                    index = rotateRight(index);
                }

                if(diff == 2){
                    int subDiff = getDepthDiff(field[index].rightSon);
                    if (subDiff < 0){
                        rotateRight(field[index].rightSon);
                    }
                    index = rotateLeft(index);
                }
            }

            return index;
        }

        // Evaluates depth difference node from its children
        int getDepthDiff(unsigned int index){
            return field[field[index].rightSon].totalDepth - field[field[index].leftSon].totalDepth;
        }

        unsigned int rotateRight(unsigned int index){
            unsigned int leftSon = field[index].leftSon;
            unsigned int originalParent = field[index].parent;

            field[index].leftSon = field[leftSon].rightSon;
            if(field[leftSon].rightSon != 0)
                field[field[leftSon].rightSon].parent = index;
            field[index].totalLeft = field[leftSon].totalRight;
            field[index].totalDepth = std::max(field[field[index].leftSon].totalDepth, field[field[index].rightSon].totalDepth) + 1;

            field[index].parent = leftSon;
            field[leftSon].rightSon = index;
            field[leftSon].parent = originalParent;
            field[leftSon].totalRight = field[index].totalRight + field[index].totalLeft + 1;
            field[leftSon].totalDepth = std::max(field[field[leftSon].leftSon].totalDepth, field[field[leftSon].rightSon].totalDepth) + 1;

            if(index == root){
                root = leftSon;
            }
            else {
                if(field[originalParent].leftSon == index){
                    field[originalParent].leftSon = leftSon;
                } else {
                    field[originalParent].rightSon = leftSon;
                }
            }
            return leftSon;
        }

        unsigned int rotateLeft(unsigned int index){
            unsigned int rightSon = field[index].rightSon;
            unsigned int originalParent = field[index].parent;

            field[index].rightSon = field[rightSon].leftSon;
            if(field[rightSon].leftSon != 0)
                field[field[rightSon].leftSon].parent = index;
            field[index].totalRight = field[rightSon].totalLeft;
            field[index].totalDepth = std::max(field[field[index].rightSon].totalDepth, field[field[index].leftSon].totalDepth) + 1;

            field[index].parent = rightSon;
            field[rightSon].leftSon = index;
            field[rightSon].parent = originalParent;
            field[rightSon].totalLeft = field[index].totalLeft + field[index].totalRight + 1;
            field[rightSon].totalDepth = std::max(field[field[rightSon].leftSon].totalDepth, field[field[rightSon].rightSon].totalDepth) + 1;

            if(index == root) {
                root = rightSon;
            }
            else {
                if (field[originalParent].leftSon == index) {
                    field[originalParent].leftSon = rightSon;
                } else {
                    field[originalParent].rightSon = rightSon;
                }
            }
            return rightSon;
        }

        unsigned int getFreeNode(){

            if(!freeList.empty()){
                unsigned int res = freeList.front();
                freeList.erase(freeList.begin());
                return res;
            }

            field.emplace_back();
            return field.size() - 1;
        }

        // index node is still valid and need to be free by freeNode
        void deleteNode(unsigned int index){

            unsigned int replaceNode;

            if(!field[index].rightSon){
                replaceNode = field[index].leftSon;
                if(field[index].leftSon)
                    field[field[index].leftSon].parent = field[index].parent;
            }

            else {
                replaceNode = field[index].rightSon;
                while(field[replaceNode].leftSon){
                    replaceNode = field[replaceNode].leftSon;
                }

                if(field[replaceNode].parent){
                    if(field[field[replaceNode].parent].rightSon == replaceNode) {
                        field[field[replaceNode].parent].rightSon = field[replaceNode].rightSon;
                        field[field[replaceNode].parent].totalRight = field[replaceNode].totalRight;
                    }
                    else {
                        field[field[replaceNode].parent].leftSon = field[replaceNode].rightSon;
                        field[field[replaceNode].parent].totalLeft = field[replaceNode].totalRight;
                    }
                    if(field[replaceNode].rightSon)
                        field[field[replaceNode].rightSon].parent = field[replaceNode].parent;
                }


                if(replaceNode && field[index].rightSon != replaceNode && field[index].leftSon != replaceNode) {
                    field[replaceNode].rightSon = field[index].rightSon;
                    field[replaceNode].totalRight = field[index].totalRight;
                    if(field[index].rightSon)
                        field[field[index].rightSon].parent = replaceNode;

                    field[replaceNode].leftSon = field[index].leftSon;
                    field[replaceNode].totalLeft = field[index].totalLeft;
                    if(field[index].leftSon)
                        field[field[index].leftSon].parent = replaceNode;
                }
            }

            if(field[index].parent){
                if(field[field[index].parent].leftSon == index)
                    field[field[index].parent].leftSon = replaceNode;
                else
                    field[field[index].parent].rightSon = replaceNode;
            }
            else
                root = replaceNode;

            unsigned int parent = field[replaceNode].parent;
            if(replaceNode)
                field[replaceNode].parent = field[index].parent;
            else
                parent = field[index].parent;

            if(parent == index)
                parent = replaceNode;

            // Propagate change to up
            while(parent){
                field[parent].totalLeft = field[field[parent].leftSon].totalLeft + field[field[parent].leftSon].totalRight + 1;
                if(!field[parent].leftSon)
                    field[parent].totalLeft = 0;
                field[parent].totalRight = field[field[parent].rightSon].totalLeft + field[field[parent].rightSon].totalRight + 1;
                if(!field[parent].rightSon)
                    field[parent].totalRight = 0;

                evalDepth(parent);
                parent = fixTree(parent);

                parent = field[parent].parent;
            }
        }

        void freeNode(unsigned int index){
            if(index == 0)
                return;
            if(field[index].totalLeft > 0 && field[index].leftSon == 0) {
                return;
            }
            size--;
            field[index].leftSon = 0;
            field[index].totalLeft = 255;
            freeList.emplace_back(index);
        }

        unsigned int size;
        unsigned int root;
        unsigned int min;
        std::list<unsigned int> freeList;
        std::vector<Node> field;
    };

  bool empty() const {
      return tree.Size() == 0;
  }

  size_t size() const{
      return tree.Size();
  }

  struct Ref {
    unsigned int index;
  };

  Ref push_last(T x){
      Ref ptr;
      ptr.index = tree.insertMax(x);
      return ptr;
  }

  T pop_first(){
      if(tree.Size() == 0)
          throw std::out_of_range("Queue is empty! [pop_first()]");

      T val = tree.getMin();
      tree.extractMin();
      return val;
  }

  size_t position(const Ref& it) const {
      return tree.getPos(it.index);
  }

  void jump_ahead(const Ref& it, size_t positions){
      tree.jumpAhead(it.index, static_cast<unsigned int>(positions));
  }

private:
    AVL tree;
};

#ifndef __PROGTEST__

////////////////// Dark magic, ignore ////////////////////////

template < typename T >
auto quote(const T& t) { return t; }

std::string quote(const std::string& s) {
  std::string ret = "\"";
  for (char c : s) if (c != '\n') ret += c; else ret += "\\n";
  return ret + "\"";
}

#define STR_(a) #a
#define STR(a) STR_(a)

#define CHECK_(a, b, a_str, b_str) do { \
    auto _a = (a); \
    decltype(a) _b = (b); \
    if (_a != _b) { \
      std::cout << "Line " << __LINE__ << ": Assertion " \
        << a_str << " == " << b_str << " failed!" \
        << " (lhs: " << quote(_a) << ")" << std::endl; \
      fail++; \
    } else ok++; \
  } while (0)

#define CHECK(a, b) CHECK_(a, b, #a, #b)

#define CHECK_EX(expr, ex) do { \
    try { \
      (expr); \
      fail++; \
      std::cout << "Line " << __LINE__ << ": Expected " STR(expr) \
        " to throw " #ex " but no exception was raised." << std::endl; \
    } catch (const ex&) { ok++; \
    } catch (...) { \
      fail++; \
      std::cout << "Line " << __LINE__ << ": Expected " STR(expr) \
        " to throw " #ex " but got different exception." << std::endl; \
    } \
  } while (0)
 
////////////////// End of dark magic ////////////////////////


void test1(int& ok, int& fail) {
  Queue<int> Q;
  CHECK(Q.empty(), true);
  CHECK(Q.size(), 0);

  constexpr int RUN = 10, TOT = 105;

  for (int i = 0; i < TOT; i++) {
    Q.push_last(i % RUN);
    CHECK(Q.empty(), false);
    CHECK(Q.size(), i + 1);
  }

  for (int i = 0; i < TOT; i++) {
    CHECK(Q.pop_first(), i % RUN);
    CHECK(Q.size(), TOT - 1 - i);
  }

  CHECK(Q.empty(), true);
}

void test2(int& ok, int& fail) {
  Queue<int> Q;
  CHECK(Q.empty(), true);
  CHECK(Q.size(), 0);
  std::vector<decltype(Q.push_last(0))> refs;

  constexpr int RUN = 10, TOT = 105;

  for (int i = 0; i < TOT; i++) {
    refs.push_back(Q.push_last(i % RUN));
    CHECK(Q.size(), i + 1);
  }
  
  for (int i = 0; i < TOT; i++) CHECK(Q.position(refs[i]), i);

  Q.jump_ahead(refs[0], 15);
  Q.jump_ahead(refs[3], 0);
  
  CHECK(Q.size(), TOT);
  for (int i = 0; i < TOT; i++) CHECK(Q.position(refs[i]), i);

  Q.jump_ahead(refs[8], 100);
  Q.jump_ahead(refs[9], 100);
  Q.jump_ahead(refs[7], 1);

  static_assert(RUN == 10 && TOT >= 30);
  for (int i : { 9, 8, 0, 1, 2, 3, 4, 5, 7, 6 })
    CHECK(Q.pop_first(), i);

  for (int i = 0; i < TOT*2 / 3; i++) {
    CHECK(Q.pop_first(), i % RUN);
    CHECK(Q.size(), TOT - 11 - i);
  }

  CHECK(Q.empty(), false);
}

template < int TOT >
void test_speed(int& ok, int& fail) {
  Queue<int> Q;
  CHECK(Q.empty(), true);
  CHECK(Q.size(), 0);
  std::vector<decltype(Q.push_last(0))> refs;

  for (int i = 0; i < TOT; i++) {
    refs.push_back(Q.push_last(i));
    CHECK(Q.size(), i + 1);
  }
  
  for (int i = 0; i < TOT; i++) {
    CHECK(Q.position(refs[i]), i);
    Q.jump_ahead(refs[i], i);
  }

  for (int i = 0; i < TOT; i++) CHECK(Q.position(refs[i]), TOT - 1 - i);

  for (int i = 0; i < TOT; i++) {
    CHECK(Q.pop_first(), TOT - 1 - i);
    CHECK(Q.size(), TOT - 1 - i);
  }

  CHECK(Q.empty(), true);
}

int main() {
  int ok = 0, fail = 0;
  if (!fail) test1(ok, fail);
  if (!fail) test2(ok, fail);
  if (!fail) test_speed<1'000>(ok, fail);
 
  if (!fail) std::cout << "Passed all " << ok << " tests!" << std::endl;
  else std::cout << "Failed " << fail << " of " << (ok + fail) << " tests." << std::endl;
}

#endif


