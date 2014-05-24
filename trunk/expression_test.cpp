#include <iostream>
#include "expression.h"

using namespace std;

void print_expression(PCExpression const& e, Direction direction) {
  for (auto p = e->begin(direction); p != e->end(direction); ++p) {
    cerr << p->toString();
  }

  cerr << endl;
}

int main() {
  auto a = PCExpression(new Literal("abcdefg"));
  auto b = PCExpression(new Symbol('w'));
  auto c = PCExpression(new Concatenation({a, b, a, b, a, b}));

  print_expression(a, LeftToRight);  
  print_expression(a, RightToLeft);  
  print_expression(b, LeftToRight);  
  print_expression(b, RightToLeft);  
  print_expression(c, LeftToRight);  
  print_expression(c, RightToLeft);  

  return 0;
}

