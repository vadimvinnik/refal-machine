#include <iostream>
#include "expression.h"

using namespace std;

void print_expression(PCExpression e) {
  for (auto p = e->front(); p != e->back(); ++p) {
    cerr << p->toString();
  }

  cerr << endl;
}

int main() {
  auto a = PCExpression(new Literal("abcdefg"));
  auto b = PCExpression(new Symbol('w'));
  auto c = PCExpression(new Concatenation({a, b, a, b, a, b}));

  print_expression(c);  

  return 0;
}

