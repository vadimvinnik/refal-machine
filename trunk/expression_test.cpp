#include <iostream>
#include "expression.h"

using namespace std;

void print_expression(PCExpression const& e, Expression::Direction direction) {
  for (auto p = e->begin(direction); p != e->end(direction); ++p) {
    cerr << p->toString();
  }

  cerr << endl;
}

int main() {
  auto a = PCExpression(new Literal("abcdefg"));
  auto b = PCExpression(new Symbol('w'));
  auto c = PCExpression(new Concatenation({a, b, a, b, a, b}));

  print_expression(a, Expression::Direction::LeftToRight);  
  print_expression(b, Expression::Direction::LeftToRight);  
  print_expression(c, Expression::Direction::LeftToRight);  

  return 0;
}

