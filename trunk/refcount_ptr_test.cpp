#include <iostream>
#include "refcount_ptr.h"

using std::cout;
using std::endl;

class Base {
public:
  Base() { cout << "Base .ctor" << endl; }
  ~Base() { cout << "Base .dtor" << endl; }
};

class Derived : public Base {
public:
  Derived() { cout << "Derived .ctor" << endl; }
  ~Derived() { cout << "Derived .dtor" << endl; }
};

int main() {
  auto p = refcount_ptr<Derived>::create();
  {
    cout << "Enter" << endl;
    auto r = p;
    cout << "Leave" << endl;
  }

  auto s = p;

  refcount_ptr<Base> k = refcount_ptr<Derived>::create();

  cout << "End" << endl;
  return 0;
}

