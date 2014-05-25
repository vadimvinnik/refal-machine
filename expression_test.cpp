#include <iostream>
#include "expression.h"

using namespace std;

void print_expression(PCExpression const& e, LookupDirection direction) {
  for_each(e->begin(direction), e->end(direction), [](Term const& t) { cerr << t.toString(); });
  cerr << endl;
}

int main() {
  auto a = PCExpression(new Literal("abcdefg"));
  auto b = PCExpression(new Symbol('w'));
  auto c = PCExpression(new Concatenation({a, b, a, b, a, b}));
  auto d = PCExpression(new Concatenation({a, b, c, a, b, c}));

  print_expression(a, LeftToRight);  
  print_expression(a, RightToLeft);  
  print_expression(b, LeftToRight);  
  print_expression(b, RightToLeft);  
  print_expression(c, LeftToRight);  
  print_expression(c, RightToLeft);  
  print_expression(d, LeftToRight);  
  print_expression(d, RightToLeft);  

  return 0;
}

